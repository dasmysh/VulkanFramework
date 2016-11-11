/**
 * @file   FWApplication.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.27
 *
 * @brief  Contains the implementation of FWApplication.
 */

#include "FWApplication.h"
#include "app_constants.h"
#include <gfx/vk/GraphicsPipeline.h>
#include <app/VKWindow.h>
#include <gfx/vk/LogicalDevice.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <gfx/vk/Framebuffer.h>
#include "gfx/vk/HostBuffer.h"
#include "gfx/vk/DeviceBuffer.h"

namespace vkuapp {

    /**
     * Constructor.
     */
    FWApplication::FWApplication() :
        ApplicationBase{applicationName, applicationVersion, configFileName},
        vertices_{ { { -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f } },
                   { { 0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f } },
                   { { 0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f } },
                   { { -0.5f, 0.5f },{ 1.0f, 1.0f, 1.0f } } },
        indices_{ 0, 1, 2, 2, 3, 0 }
    {
        {
            auto stagingBuffer = vku::gfx::HostBuffer{ &GetWindow(0)->GetDevice(), vk::BufferUsageFlagBits::eTransferSrc };
            stagingBuffer.InitializeData(vertices_);

            auto& device = GetWindow(0)->GetDevice();
            vtxBuffer_ = std::make_unique<vku::gfx::DeviceBuffer>(&device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlags(), std::vector<uint32_t>{ { device.GetQueueInfo(0).familyIndex_, device.GetQueueInfo(1).familyIndex_ } });
            vtxBuffer_->InitializeBuffer(stagingBuffer.GetSize());

            stagingBuffer.CopyBufferSync(*vtxBuffer_, std::make_pair(device.GetQueueInfo(1).familyIndex_, 0));
        }

        {
            auto stagingBuffer = vku::gfx::HostBuffer{ &GetWindow(0)->GetDevice(), vk::BufferUsageFlagBits::eTransferSrc };
            stagingBuffer.InitializeData(indices_);

            auto& device = GetWindow(0)->GetDevice();
            idxBuffer_ = std::make_unique<vku::gfx::DeviceBuffer>(&device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlags(), std::vector<uint32_t>{ { device.GetQueueInfo(0).familyIndex_, device.GetQueueInfo(1).familyIndex_ } });
            idxBuffer_->InitializeBuffer(stagingBuffer.GetSize());

            stagingBuffer.CopyBufferSync(*idxBuffer_, std::make_pair(device.GetQueueInfo(1).familyIndex_, 0));
        }

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ vk::PipelineLayoutCreateFlags(), 0, nullptr, 0, nullptr };
        vkPipelineLayout_ = GetWindow(0)->GetDevice().GetDevice().createPipelineLayout(pipelineLayoutInfo);

        auto fbSize = GetWindow(0)->GetFramebuffers()[0].GetSize();
        Resize(fbSize, GetWindow(0));
    }

    FWApplication::~FWApplication()
    {
        // remove pipeline from command buffer.
        GetWindow(0)->UpdatePrimaryCommandBuffers([this](const vk::CommandBuffer& cmdBuffer) {});

        demoPipeline_.reset();

        if (vkPipelineLayout_) GetWindow(0)->GetDevice().GetDevice().destroyPipelineLayout(vkPipelineLayout_);
        vkPipelineLayout_ = vk::PipelineLayout();
    }

    void FWApplication::FrameMove(float time, float elapsed)
    {
        /*static auto fps = 0.0f;
        static auto accumulatedElapsed = 0.0f;
        static auto numAvg = 0.0f;

        accumulatedElapsed += elapsed;
        numAvg += 1.0f;
        if (accumulatedElapsed > 0.5f) {
            fps = numAvg / accumulatedElapsed;
            accumulatedElapsed = 0.0f;
            numAvg = 0.0f;
        }
        std::stringstream fpsString;
        fpsString << fps;
        fpsText_->SetText(fpsString.str());

        GetCameraView()->UpdateCamera();*/
    }

    void FWApplication::RenderScene(const vku::VKWindow* window)
    {
        // TODO: update pipeline? [11/3/2016 Sebastian Maisch]
        if (IsGUIMode()) {
        }
    }

    void FWApplication::RenderGUI()
    {
    }

    bool FWApplication::HandleKeyboard(int key, int scancode, int action, int mods, vku::VKWindow* sender)
    {
        if (ApplicationBase::HandleKeyboard(key, scancode, action, mods, sender)) return true;
        if (!IsRunning() || IsPaused()) return false;
        auto handled = false;
        return handled;
    }

    bool FWApplication::HandleMouseApp(int button, int action, int mods, float mouseWheelDelta, vku::VKWindow* sender)
    {
        auto handled = false;
        return handled;
    }

    void FWApplication::Resize(const glm::uvec2& screenSize, const vku::VKWindow* window)
    {
        // TODO: handle other windows... [11/9/2016 Sebastian Maisch]
        // TODO: maybe use lambdas to register for resize events...
        if (window != GetWindow(0)) return;

        demoPipeline_ = window->GetDevice().CreateGraphicsPipeline(std::vector<std::string>{"simple.vert", "simple.frag"}, screenSize, 1);
        demoPipeline_->ResetVertexInput<SimpleVertex>();
        demoPipeline_->CreatePipeline(true, window->GetRenderPass(), 0, vkPipelineLayout_);

        window->UpdatePrimaryCommandBuffers([this](const vk::CommandBuffer& cmdBuffer)
        {
            cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, demoPipeline_->GetPipeline());
            vk::DeviceSize offset = 0;
            cmdBuffer.bindVertexBuffers(0, 1, vtxBuffer_->GetBuffer(), &offset);
            cmdBuffer.bindIndexBuffer(*idxBuffer_->GetBuffer(), 0, vk::IndexType::eUint32);
            cmdBuffer.drawIndexed(static_cast<uint32_t>(indices_.size()), 1, 0, 0, 0);
        });

        // TODO: fpsText_->SetPosition(glm::vec2(static_cast<float>(screenSize.x) - 100.0f, 10.0f));
    }
}

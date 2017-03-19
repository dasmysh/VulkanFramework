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
#include "gfx/vk/buffers/DeviceBuffer.h"
#include "gfx/vk/QueuedDeviceTransfer.h"
#include <glm/gtc/matrix_transform.hpp>
#include "gfx/vk/buffers/HostBuffer.h"

namespace vkuapp {

    /**
     * Constructor.
     */
    FWApplication::FWApplication() :
        ApplicationBase{ applicationName, applicationVersion, configFileName },
        vertices_{ { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                   { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                   { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                   { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } } },
        indices_{ 0, 3, 1, 1, 3, 2 },
        memGroup_{ &GetWindow(0)->GetDevice(), vk::MemoryPropertyFlags() },
        uniformDataOffset_{ 0 }
    {
        auto& device = GetWindow(0)->GetDevice();
        vku::gfx::QueuedDeviceTransfer transfer{ &device, std::make_pair(1, 0) };

        auto numUBOBuffers = GetWindow(0)->GetFramebuffers().size();
        auto singleUBOSize = device.CalculateUniformBufferAlignment(sizeof(MVPMatrixUBO));

        MVPMatrixUBO initialUBO;
        initialUBO.model_ = glm::mat4();
        initialUBO.view_ = glm::mat4();
        initialUBO.proj_ = glm::mat4();

        {
            auto uboSize = singleUBOSize * numUBOBuffers;
            auto indexBufferOffset = vku::byteSizeOf(vertices_);
            uniformDataOffset_ = device.CalculateUniformBufferAlignment(indexBufferOffset + vku::byteSizeOf(indices_));
            auto completeBufferSize = uniformDataOffset_ + uboSize;

            completeBufferIdx_ = memGroup_.AddBufferToGroup(vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eUniformBuffer,
                completeBufferSize, std::vector<std::uint32_t>{ {0, 1} });
            memGroup_.AddDataToBufferInGroup(completeBufferIdx_, 0, vertices_);
            memGroup_.AddDataToBufferInGroup(completeBufferIdx_, indexBufferOffset, indices_);
            for (auto i = 0; i < numUBOBuffers; ++i) memGroup_.AddDataToBufferInGroup(completeBufferIdx_,
                uniformDataOffset_ + (i * singleUBOSize), sizeof(MVPMatrixUBO), &initialUBO);

            //////////////////////////////////////////////////////////////////////////
            // Multiple buffers section [3/18/2017 Sebastian Maisch]
            vertexBufferIdx_ = memGroup_.AddBufferToGroup(vk::BufferUsageFlagBits::eVertexBuffer, vertices_, std::vector<std::uint32_t>{ {0, 1} });
            indexBufferIdx_ = memGroup_.AddBufferToGroup(vk::BufferUsageFlagBits::eIndexBuffer, indices_, std::vector<std::uint32_t>{ {0, 1} });
            //////////////////////////////////////////////////////////////////////////
        }

        memGroup_.FinalizeGroup();
        memGroup_.TransferData(transfer);
        transfer.FinishTransfer();

        {
            vk::CommandBufferAllocateInfo cmdBufferallocInfo{ device.GetCommandPool(1),
                vk::CommandBufferLevel::ePrimary, static_cast<std::uint32_t>(numUBOBuffers) };
            vkTransferCommandBuffers_ = device.GetDevice().allocateCommandBuffers(cmdBufferallocInfo);
            for (auto i = 0U; i < numUBOBuffers; ++i) {
                vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };
                vkTransferCommandBuffers_[i].begin(beginInfo);
                auto uboOffset = uniformDataOffset_ + (i * singleUBOSize);
                memGroup_.FillUploadBufferCmdBuffer(completeBufferIdx_, vkTransferCommandBuffers_[i], uboOffset, sizeof(MVPMatrixUBO));
                vkTransferCommandBuffers_[i].end();
            }
        }

        /*{
            vku::gfx::QueuedDeviceTransfer transfer{ &device, std::make_pair(1, 0) };
            vtxBuffer_ = transfer.CreateDeviceBufferWithData(vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlags(),
                std::vector<std::uint32_t>{ {0, 1} }, vertices_);
            idxBuffer_ = transfer.CreateDeviceBufferWithData(vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlags(),
                std::vector<std::uint32_t>{ {0, 1} }, indices_);
            transfer.FinishTransfer();
        }*/

        {
            vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
            vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{ vk::DescriptorSetLayoutCreateFlags(), 1, &uboLayoutBinding };
            vkDescriptorSetLayout_ = device.GetDevice().createDescriptorSetLayout(layoutCreateInfo);
        }

        {
            vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ vk::PipelineLayoutCreateFlags(), 1, &vkDescriptorSetLayout_, 0, nullptr };
            vkPipelineLayout_ = device.GetDevice().createPipelineLayout(pipelineLayoutInfo);
        }

        {
            vk::DescriptorPoolSize descSetPoolSize{ vk::DescriptorType::eUniformBuffer, 2 };
            vk::DescriptorPoolCreateInfo descSetPoolInfo{ vk::DescriptorPoolCreateFlags(), static_cast<std::uint32_t>(numUBOBuffers), 1, &descSetPoolSize };
            vkUBODescriptorPool_ = device.GetDevice().createDescriptorPool(descSetPoolInfo);
        }

        {
            std::vector<vk::DescriptorSetLayout> descSetLayouts; descSetLayouts.resize(numUBOBuffers);
            for (auto i = 0U; i < numUBOBuffers; ++i) descSetLayouts[i] = vkDescriptorSetLayout_;
            vk::DescriptorSetAllocateInfo descSetAllocInfo{ vkUBODescriptorPool_, static_cast<std::uint32_t>(descSetLayouts.size()), descSetLayouts.data() };
            vkUBODescritorSets_ = device.GetDevice().allocateDescriptorSets(descSetAllocInfo);
        }

        {
            std::vector<vk::DescriptorBufferInfo> descBufferInfos; descBufferInfos.reserve(numUBOBuffers);
            std::vector<vk::WriteDescriptorSet> descSetWrites; descSetWrites.reserve(numUBOBuffers);
            for (auto i = 0U; i < vkUBODescritorSets_.size(); ++i) {
                auto bufferOffset = uniformDataOffset_ + (i * singleUBOSize);
                descBufferInfos.emplace_back(memGroup_.GetBuffer(completeBufferIdx_)->GetBuffer(), bufferOffset, singleUBOSize);
                descSetWrites.emplace_back(vkUBODescritorSets_[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descBufferInfos[i], nullptr);
            }
            device.GetDevice().updateDescriptorSets(descSetWrites, nullptr);
        }

        auto fbSize = GetWindow(0)->GetFramebuffers()[0].GetSize();
        Resize(fbSize, GetWindow(0));
    }

    FWApplication::~FWApplication()
    {
        // remove pipeline from command buffer.
        GetWindow(0)->UpdatePrimaryCommandBuffers([this](const vk::CommandBuffer& cmdBuffer, std::uint32_t cmdBufferIndex) {});

        demoPipeline_.reset();

        if (vkUBODescriptorPool_) GetWindow(0)->GetDevice().GetDevice().destroyDescriptorPool(vkUBODescriptorPool_);
        vkUBODescriptorPool_ = vk::DescriptorPool();

        if (vkPipelineLayout_) GetWindow(0)->GetDevice().GetDevice().destroyPipelineLayout(vkPipelineLayout_);
        vkPipelineLayout_ = vk::PipelineLayout();
        if (vkDescriptorSetLayout_) GetWindow(0)->GetDevice().GetDevice().destroyDescriptorSetLayout(vkDescriptorSetLayout_);
        vkDescriptorSetLayout_ = vk::DescriptorSetLayout();
    }

    void FWApplication::FrameMove(float time, float elapsed, const vku::VKWindow* window)
    {
        if (window != GetWindow(0)) return;

        auto& device = window->GetDevice();

        MVPMatrixUBO ubo;
        ubo.model_ = glm::rotate(glm::mat4(), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view_ = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        auto aspectRatio = static_cast<float>(GetWindow(0)->GetWidth()) / static_cast<float>(GetWindow(0)->GetHeight());
        ubo.proj_ = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);

        auto uboIndex = window->GetCurrentlyRenderedImageIndex();
        auto uboOffset = uniformDataOffset_ + (uboIndex * device.CalculateUniformBufferAlignment(sizeof(MVPMatrixUBO)));
        memGroup_.GetHostMemory()->CopyToHostMemory(memGroup_.GetHostBufferOffset(completeBufferIdx_) + uboOffset, sizeof(MVPMatrixUBO), &ubo);
        vk::Semaphore vkTransferSemaphore = window->GetDataAvailableSemaphore();
        vk::SubmitInfo submitInfo{ 0, nullptr, nullptr, 1, &vkTransferCommandBuffers_[uboIndex], 1, &vkTransferSemaphore };
        device.GetQueue(1, 0).submit(submitInfo, vk::Fence());

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

        window->UpdatePrimaryCommandBuffers([this](const vk::CommandBuffer& cmdBuffer, std::uint32_t cmdBufferIndex)
        {
            cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, demoPipeline_->GetPipeline());
            vk::DeviceSize offset = 0;

            cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout_, 0, vkUBODescritorSets_[cmdBufferIndex], nullptr);

            // cmdBuffer.bindVertexBuffers(0, 1, vtxBuffer_->GetBuffer(), &offset);
            // cmdBuffer.bindIndexBuffer(*idxBuffer_->GetBuffer(), 0, vk::IndexType::eUint32);

            // cmdBuffer.bindVertexBuffers(0, 1, buffers_.GetBuffer(vertexBufferIdx_)->GetBuffer(), &offset);
            // cmdBuffer.bindIndexBuffer(*buffers_.GetBuffer(indexBufferIdx_)->GetBuffer(), 0, vk::IndexType::eUint32);

            cmdBuffer.bindVertexBuffers(0, 1, memGroup_.GetBuffer(completeBufferIdx_)->GetBufferPtr(), &offset);
            cmdBuffer.bindIndexBuffer(memGroup_.GetBuffer(completeBufferIdx_)->GetBuffer(), vku::byteSizeOf(vertices_), vk::IndexType::eUint32);
            cmdBuffer.drawIndexed(static_cast<std::uint32_t>(indices_.size()), 1, 0, 0, 0);
        });

        // TODO: fpsText_->SetPosition(glm::vec2(static_cast<float>(screenSize.x) - 100.0f, 10.0f));
    }
}

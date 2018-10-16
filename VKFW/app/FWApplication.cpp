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
#include "gfx/vk/UniformBufferObject.h"
#include "gfx/Texture2D.h"
#include "gfx/meshes/AssImpScene.h"
#include "gfx/meshes/Mesh.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vkuapp {

    /**
     * Constructor.
     */
    FWApplication::FWApplication() :
        ApplicationBase{ applicationName, applicationVersion, configFileName },
        vertices_{ { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                   { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                   { { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                   { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },

                   { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                   { { 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                   { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                   { { -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } } },
        indices_{ 0, 1, 2, 2, 3, 0,
                  4, 5, 6, 6, 7, 4 },
        memGroup_{ &GetWindow(0)->GetDevice(), vk::MemoryPropertyFlags() },
        cameraUBO_{ vku::gfx::UniformBufferObject::Create<CameraMatrixUBO>(&GetWindow(0)->GetDevice(), GetWindow(0)->GetFramebuffers().size()) },
        worldUBO_{ vku::gfx::UniformBufferObject::Create<vku::gfx::WorldMatrixUBO>(&GetWindow(0)->GetDevice(), GetWindow(0)->GetFramebuffers().size()) }
    {
        auto& device = GetWindow(0)->GetDevice();
        vku::gfx::QueuedDeviceTransfer transfer{ &device, std::make_pair(0, 0) }; // as long as the last transfer of texture layouts is done on this queue, we have to use the graphics queue here.

        auto numUBOBuffers = GetWindow(0)->GetFramebuffers().size();

        CameraMatrixUBO initialCameraUBO;
        vku::gfx::WorldMatrixUBO initialWorldUBO;
        initialWorldUBO.model_ = glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        initialWorldUBO.normalMatrix_ = glm::mat4(glm::inverseTranspose(glm::mat3(initialWorldUBO.model_)));
        initialCameraUBO.view_ = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        auto aspectRatio = static_cast<float>(GetWindow(0)->GetWidth()) / static_cast<float>(GetWindow(0)->GetHeight());
        initialCameraUBO.proj_ = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);
        initialCameraUBO.proj_[1][1] *= -1.0f;

        {
            auto uboSize = cameraUBO_.GetCompleteSize() + worldUBO_.GetCompleteSize();
            auto indexBufferOffset = vku::byteSizeOf(vertices_);
            auto uniformDataOffset = device.CalculateUniformBufferAlignment(indexBufferOffset + vku::byteSizeOf(indices_));
            auto completeBufferSize = uniformDataOffset + uboSize;

            completeBufferIdx_ = memGroup_.AddBufferToGroup(vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eUniformBuffer,
                completeBufferSize, std::vector<std::uint32_t>{ {0, 1} });

            memGroup_.AddDataToBufferInGroup(completeBufferIdx_, 0, vertices_);
            memGroup_.AddDataToBufferInGroup(completeBufferIdx_, indexBufferOffset, indices_);

            cameraUBO_.AddUBOToBuffer(&memGroup_, completeBufferIdx_, uniformDataOffset, initialCameraUBO);
            worldUBO_.AddUBOToBuffer(&memGroup_, completeBufferIdx_, uniformDataOffset + cameraUBO_.GetCompleteSize(), initialWorldUBO);

            demoTexture_ = device.GetTextureManager()->GetResource("demo.jpg", true, memGroup_, std::vector<std::uint32_t>{ {0, 1} });
            vk::SamplerCreateInfo samplerCreateInfo{ vk::SamplerCreateFlags(), vk::Filter::eLinear, vk::Filter::eLinear,
                vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat };
            vkDemoSampler_ = device.GetDevice().createSamplerUnique(samplerCreateInfo);

            //////////////////////////////////////////////////////////////////////////
            // Multiple buffers section [3/18/2017 Sebastian Maisch]
            // vertexBufferIdx_ = memGroup_.AddBufferToGroup(vk::BufferUsageFlagBits::eVertexBuffer, vertices_, std::vector<std::uint32_t>{ {0, 1} });
            // indexBufferIdx_ = memGroup_.AddBufferToGroup(vk::BufferUsageFlagBits::eIndexBuffer, indices_, std::vector<std::uint32_t>{ {0, 1} });
            //////////////////////////////////////////////////////////////////////////
        }

        meshInfo_ = std::make_shared<vku::gfx::AssImpScene>("teapot/teapot.obj", &device);
        mesh_ = std::make_unique<vku::gfx::Mesh>(vku::gfx::Mesh::CreateWithInternalMemoryGroup<SimpleVertex, SimpleMaterial>(meshInfo_,
            numUBOBuffers, &device, vk::MemoryPropertyFlags(), std::vector<std::uint32_t>{ {0, 1} }));
        mesh_->UploadMeshData(transfer);

        memGroup_.FinalizeDeviceGroup();
        memGroup_.TransferData(transfer);
        transfer.FinishTransfer();

        mesh_->CreateDescriptorSets(numUBOBuffers);

        {
            vk::CommandBufferAllocateInfo cmdBufferallocInfo{ device.GetCommandPool(1),
                vk::CommandBufferLevel::ePrimary, static_cast<std::uint32_t>(numUBOBuffers) };
            vkTransferCommandBuffers_ = device.GetDevice().allocateCommandBuffersUnique(cmdBufferallocInfo);
            for (auto i = 0U; i < numUBOBuffers; ++i) {
                vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };
                vkTransferCommandBuffers_[i]->begin(beginInfo);

                cameraUBO_.FillUploadCmdBuffer<CameraMatrixUBO>(*vkTransferCommandBuffers_[i], i);
                worldUBO_.FillUploadCmdBuffer<vku::gfx::WorldMatrixUBO>(*vkTransferCommandBuffers_[i], i);

                mesh_->TransferWorldMatrices(*vkTransferCommandBuffers_[i], i);
                vkTransferCommandBuffers_[i]->end();
           }
        }


        {
            std::array<vk::DescriptorPoolSize, 2> descSetPoolSizes;
            descSetPoolSizes[0] = vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 1 };
            descSetPoolSizes[1] = vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBufferDynamic, 2 };
            vk::DescriptorPoolCreateInfo descSetPoolInfo{ vk::DescriptorPoolCreateFlags(), 3,
                static_cast<std::uint32_t>(descSetPoolSizes.size()), descSetPoolSizes.data() };
            vkUBODescriptorPool_ = device.GetDevice().createDescriptorPoolUnique(descSetPoolInfo);
        }

        cameraUBO_.CreateLayout(*vkUBODescriptorPool_, vk::ShaderStageFlagBits::eVertex, true, 0);
        worldUBO_.UseLayout(*vkUBODescriptorPool_, mesh_->GetWorldMatricesDescriptorLayout(), true, 0);
        vkImageSampleDescriptorSetLayout_ = mesh_->GetMaterialTexturesDescriptorLayout();

        {
            std::array<vk::DescriptorSetLayout, 4> pipelineDescSets;
            pipelineDescSets[0] = worldUBO_.GetDescriptorLayout();
            pipelineDescSets[1] = mesh_->GetMaterialBufferDescriptorLayout();
            pipelineDescSets[2] = vkImageSampleDescriptorSetLayout_;
            pipelineDescSets[3] = cameraUBO_.GetDescriptorLayout();

            vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ vk::PipelineLayoutCreateFlags(),
                static_cast<std::uint32_t>(pipelineDescSets.size()), pipelineDescSets.data() };
            vkPipelineLayout_ = device.GetDevice().createPipelineLayoutUnique(pipelineLayoutInfo);
        }

        {
            vk::DescriptorSetAllocateInfo descSetAllocInfo{ *vkUBODescriptorPool_, 1, &vkImageSampleDescriptorSetLayout_ };
            vkImageSamplerDescritorSet_ = device.GetDevice().allocateDescriptorSets(descSetAllocInfo)[0];
        }

        {
            std::vector<vk::WriteDescriptorSet> descSetWrites; descSetWrites.reserve(3);
            vk::DescriptorImageInfo descImageInfo{ *vkDemoSampler_, demoTexture_->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal };
            descSetWrites.emplace_back(vkImageSamplerDescritorSet_, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &descImageInfo);

            worldUBO_.FillDescriptorSetWrite(descSetWrites.emplace_back());
            cameraUBO_.FillDescriptorSetWrite(descSetWrites.emplace_back());

            device.GetDevice().updateDescriptorSets(descSetWrites, nullptr);
        }

        auto fbSize = GetWindow(0)->GetFramebuffers()[0].GetSize();
        Resize(fbSize, GetWindow(0));
    }

    FWApplication::~FWApplication()
    {
        // remove pipeline from command buffer.
        GetWindow(0)->UpdatePrimaryCommandBuffers([this](const vk::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex) {});

        demoPipeline_.reset();
        vkUBODescriptorPool_.reset();
        vkPipelineLayout_.reset();
    }

    void FWApplication::FrameMove(float time, float elapsed, const vku::VKWindow* window)
    {
        if (window != GetWindow(0)) return;

        auto& device = window->GetDevice();

        CameraMatrixUBO camera_ubo;
        vku::gfx::WorldMatrixUBO world_ubo;
        world_ubo.model_ = glm::rotate(glm::mat4(1.0f), 0.3f * time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        world_ubo.normalMatrix_ = glm::mat4(glm::inverseTranspose(glm::mat3(world_ubo.model_)));
        camera_ubo.view_ = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        auto aspectRatio = static_cast<float>(GetWindow(0)->GetWidth()) / static_cast<float>(GetWindow(0)->GetHeight());
        camera_ubo.proj_ = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);
        camera_ubo.proj_[1][1] *= -1.0f;

        auto uboIndex = window->GetCurrentlyRenderedImageIndex();
        cameraUBO_.UpdateInstanceData(uboIndex, camera_ubo);
        worldUBO_.UpdateInstanceData(uboIndex, world_ubo);

        glm::mat4 mesh_world = glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f)), -0.2f * time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mesh_->UpdateWorldMatrices(uboIndex, mesh_world);
        vk::Semaphore vkTransferSemaphore = window->GetDataAvailableSemaphore();
        vk::SubmitInfo submitInfo{ 0, nullptr, nullptr, 1, &(*vkTransferCommandBuffers_[uboIndex]), 1, &vkTransferSemaphore };
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


        // TODO: like this the shaders will be recompiled on each resize. [3/26/2017 Sebastian Maisch]
        // maybe set viewport as dynamic...
        demoPipeline_ = window->GetDevice().CreateGraphicsPipeline(std::vector<std::string>{"simple.vert", "simple.frag"}, screenSize, 1);
        demoPipeline_->ResetVertexInput<SimpleVertex>();
        demoPipeline_->CreatePipeline(true, window->GetRenderPass(), 0, *vkPipelineLayout_);

        window->UpdatePrimaryCommandBuffers([this](const vk::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex)
        {
            cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, demoPipeline_->GetPipeline());
            vk::DeviceSize offset = 0;

            // Material set
            cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *vkPipelineLayout_, 2, vkImageSamplerDescritorSet_, nullptr);

            worldUBO_.Bind(cmdBuffer, vk::PipelineBindPoint::eGraphics, *vkPipelineLayout_, 0, cmdBufferIndex);
            cameraUBO_.Bind(cmdBuffer, vk::PipelineBindPoint::eGraphics, *vkPipelineLayout_, 3, cmdBufferIndex);

            cmdBuffer.bindVertexBuffers(0, 1, memGroup_.GetBuffer(completeBufferIdx_)->GetBufferPtr(), &offset);
            cmdBuffer.bindIndexBuffer(memGroup_.GetBuffer(completeBufferIdx_)->GetBuffer(), vku::byteSizeOf(vertices_), vk::IndexType::eUint32);
            cmdBuffer.drawIndexed(static_cast<std::uint32_t>(indices_.size()), 1, 0, 0, 0);

            mesh_->Draw(cmdBuffer, cmdBufferIndex, *vkPipelineLayout_);
        });

        // TODO: fpsText_->SetPosition(glm::vec2(static_cast<float>(screenSize.x) - 100.0f, 10.0f));
    }
}

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
        uniformDataOffset_{ 0 }
    {
        auto& device = GetWindow(0)->GetDevice();
        vku::gfx::QueuedDeviceTransfer transfer{ &device, std::make_pair(0, 0) }; // as long as the last transfer of texture layouts is done on this queue, we have to use the graphics queue here.

        auto numUBOBuffers = GetWindow(0)->GetFramebuffers().size();
        auto singleCameraUBOSize = device.CalculateUniformBufferAlignment(sizeof(VPMatrixUBO));
        auto singleWorldUBOSize = device.CalculateUniformBufferAlignment(sizeof(WorldMatrixUBO));
        singleWorldCamUBOSize_ = singleCameraUBOSize + singleWorldUBOSize;

        VPMatrixUBO initialCameraUBO;
        WorldMatrixUBO initialWorldUBO;
        initialWorldUBO.model_ = glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        initialWorldUBO.normalMatrix_ = glm::mat4(glm::inverseTranspose(glm::mat3(initialWorldUBO.model_)));
        initialCameraUBO.view_ = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        auto aspectRatio = static_cast<float>(GetWindow(0)->GetWidth()) / static_cast<float>(GetWindow(0)->GetHeight());
        initialCameraUBO.proj_ = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);
        initialCameraUBO.proj_[1][1] *= -1.0f;

        {
            auto uboSize = singleWorldCamUBOSize_ * numUBOBuffers;
            auto indexBufferOffset = vku::byteSizeOf(vertices_);
            uniformDataOffset_ = device.CalculateUniformBufferAlignment(indexBufferOffset + vku::byteSizeOf(indices_));
            auto completeBufferSize = uniformDataOffset_ + uboSize;

            completeBufferIdx_ = memGroup_.AddBufferToGroup(vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eUniformBuffer,
                completeBufferSize, std::vector<std::uint32_t>{ {0, 1} });
            memGroup_.AddDataToBufferInGroup(completeBufferIdx_, 0, vertices_);
            memGroup_.AddDataToBufferInGroup(completeBufferIdx_, indexBufferOffset, indices_);
            for (auto i = 0; i < numUBOBuffers; ++i) {
                memGroup_.AddDataToBufferInGroup(completeBufferIdx_, uniformDataOffset_ + (i * singleWorldCamUBOSize_),
                    sizeof(VPMatrixUBO), &initialCameraUBO);
                memGroup_.AddDataToBufferInGroup(completeBufferIdx_, uniformDataOffset_ + (i * singleWorldCamUBOSize_) + singleCameraUBOSize,
                    sizeof(WorldMatrixUBO), &initialWorldUBO);
            }

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
                auto uboOffset = uniformDataOffset_ + (i * singleWorldCamUBOSize_);
                memGroup_.FillUploadBufferCmdBuffer(completeBufferIdx_, *vkTransferCommandBuffers_[i], uboOffset, sizeof(VPMatrixUBO));
                memGroup_.FillUploadBufferCmdBuffer(completeBufferIdx_, *vkTransferCommandBuffers_[i], uboOffset + singleCameraUBOSize, sizeof(WorldMatrixUBO));
                mesh_->TransferWorldMatrices(*vkTransferCommandBuffers_[i], i);
                vkTransferCommandBuffers_[i]->end();
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

        vkDescriptorSetLayouts_[0] = mesh_->GetWorldMatricesDescriptorLayout();
        vkDescriptorSetLayouts_[1] = mesh_->GetMaterialDescriptorLayout();
        {
            vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex };

            vk::DescriptorSetLayoutCreateInfo uboLayoutCreateInfo{ vk::DescriptorSetLayoutCreateFlags(), 1, &uboLayoutBinding };
            vkUDescriptorSetLayouts_[0] = device.GetDevice().createDescriptorSetLayoutUnique(uboLayoutCreateInfo);
            vkDescriptorSetLayouts_[2] = *vkUDescriptorSetLayouts_[0];
        }

        {
            vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ vk::PipelineLayoutCreateFlags(),
                static_cast<std::uint32_t>(vkDescriptorSetLayouts_.size()), vkDescriptorSetLayouts_.data() };
            vkPipelineLayout_ = device.GetDevice().createPipelineLayoutUnique(pipelineLayoutInfo);
        }

        {
            std::array<vk::DescriptorPoolSize, 2> descSetPoolSizes;
            descSetPoolSizes[0] = vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 1 };
            descSetPoolSizes[1] = vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBufferDynamic, 2 };
            vk::DescriptorPoolCreateInfo descSetPoolInfo{ vk::DescriptorPoolCreateFlags(), 3,
                static_cast<std::uint32_t>(descSetPoolSizes.size()), descSetPoolSizes.data() };
            vkUBODescriptorPool_ = device.GetDevice().createDescriptorPoolUnique(descSetPoolInfo);
        }

        {
            std::vector<vk::DescriptorSetLayout> descSetLayouts; descSetLayouts.resize(3);
            descSetLayouts[0] = vkDescriptorSetLayouts_[0];
            descSetLayouts[1] = vkDescriptorSetLayouts_[1];
            descSetLayouts[2] = vkDescriptorSetLayouts_[2];
            vk::DescriptorSetAllocateInfo descSetAllocInfo{ *vkUBODescriptorPool_, static_cast<std::uint32_t>(descSetLayouts.size()), descSetLayouts.data() };
            // [0]: world ubo
            // [1]: material
            // [2]: camera ubo
            vkUBOSamplerDescritorSets_ = device.GetDevice().allocateDescriptorSets(descSetAllocInfo);
        }

        {
            std::vector<vk::WriteDescriptorSet> descSetWrites; descSetWrites.reserve(3);
            vk::DescriptorImageInfo descImageInfo{ *vkDemoSampler_, demoTexture_->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal };
            descSetWrites.emplace_back(vkUBOSamplerDescritorSets_[1], 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &descImageInfo);

            std::vector<vk::DescriptorBufferInfo> descBufferInfos; descBufferInfos.reserve(2);
            // world ubo
            descBufferInfos.emplace_back(memGroup_.GetBuffer(completeBufferIdx_)->GetBuffer(), uniformDataOffset_ + singleCameraUBOSize, singleWorldUBOSize);
            descSetWrites.emplace_back(vkUBOSamplerDescritorSets_[0], 0, 0, 1, vk::DescriptorType::eUniformBufferDynamic, nullptr, &descBufferInfos.back());

            // camera ubo
            descBufferInfos.emplace_back(memGroup_.GetBuffer(completeBufferIdx_)->GetBuffer(), uniformDataOffset_, singleCameraUBOSize);
            descSetWrites.emplace_back(vkUBOSamplerDescritorSets_[2], 0, 0, 1, vk::DescriptorType::eUniformBufferDynamic, nullptr, &descBufferInfos.back());

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

        vkUBOSamplerDescritorSets_.clear();
        vkUBODescriptorPool_.reset();

        vkPipelineLayout_.reset();
        vkUDescriptorSetLayouts_[0].reset();
    }

    void FWApplication::FrameMove(float time, float elapsed, const vku::VKWindow* window)
    {
        if (window != GetWindow(0)) return;

        auto& device = window->GetDevice();

        VPMatrixUBO camera_ubo;
        WorldMatrixUBO world_ubo;
        world_ubo.model_ = glm::rotate(glm::mat4(1.0f), 0.3f * time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        world_ubo.normalMatrix_ = glm::mat4(glm::inverseTranspose(glm::mat3(world_ubo.model_)));
        camera_ubo.view_ = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        auto aspectRatio = static_cast<float>(GetWindow(0)->GetWidth()) / static_cast<float>(GetWindow(0)->GetHeight());
        camera_ubo.proj_ = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);
        camera_ubo.proj_[1][1] *= -1.0f;

        auto uboIndex = window->GetCurrentlyRenderedImageIndex();
        auto cameraUBOsingleSize = device.CalculateUniformBufferAlignment(sizeof(VPMatrixUBO));
        // auto combinedUBOSize = cameraUBOsingleSize + device.CalculateUniformBufferAlignment(sizeof(WorldMatrixUBO));
        auto uboOffset = uniformDataOffset_ + (uboIndex * singleWorldCamUBOSize_);
        memGroup_.GetHostMemory()->CopyToHostMemory(memGroup_.GetHostBufferOffset(completeBufferIdx_) + uboOffset, sizeof(VPMatrixUBO), &camera_ubo);
        memGroup_.GetHostMemory()->CopyToHostMemory(memGroup_.GetHostBufferOffset(completeBufferIdx_) + uboOffset + cameraUBOsingleSize, sizeof(WorldMatrixUBO), &world_ubo);


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

            // World Matrices UBO
            auto worldMatricesBufferOffset = static_cast<std::uint32_t>(cmdBufferIndex * singleWorldCamUBOSize_);
            cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *vkPipelineLayout_, 0, vkUBOSamplerDescritorSets_[0], worldMatricesBufferOffset);
            // Material set
            cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *vkPipelineLayout_, 1, vkUBOSamplerDescritorSets_[1], nullptr);
            // Camera Matrices UBO
            cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *vkPipelineLayout_, 2, vkUBOSamplerDescritorSets_[2], worldMatricesBufferOffset);

            // cmdBuffer.bindVertexBuffers(0, 1, vtxBuffer_->GetBuffer(), &offset);
            // cmdBuffer.bindIndexBuffer(*idxBuffer_->GetBuffer(), 0, vk::IndexType::eUint32);

            // cmdBuffer.bindVertexBuffers(0, 1, buffers_.GetBuffer(vertexBufferIdx_)->GetBuffer(), &offset);
            // cmdBuffer.bindIndexBuffer(*buffers_.GetBuffer(indexBufferIdx_)->GetBuffer(), 0, vk::IndexType::eUint32);

            cmdBuffer.bindVertexBuffers(0, 1, memGroup_.GetBuffer(completeBufferIdx_)->GetBufferPtr(), &offset);
            cmdBuffer.bindIndexBuffer(memGroup_.GetBuffer(completeBufferIdx_)->GetBuffer(), vku::byteSizeOf(vertices_), vk::IndexType::eUint32);
            cmdBuffer.drawIndexed(static_cast<std::uint32_t>(indices_.size()), 1, 0, 0, 0);

            mesh_->Draw(cmdBuffer, cmdBufferIndex, *vkPipelineLayout_);
        });

        // TODO: fpsText_->SetPosition(glm::vec2(static_cast<float>(screenSize.x) - 100.0f, 10.0f));
    }
}

/**
 * @file   RaytracingScene.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.04.27
 *
 * @brief  Implementation of a simple raytracing scene.
 */

#include "app/RaytracingScene.h"

#include <app/VKWindow.h>
#include <core/resources/ShaderManager.h>
#include <gfx/meshes/Mesh.h>
#include <gfx/vk/CommandBuffers.h>
#include <gfx/vk/Framebuffer.h>
#include <gfx/vk/LogicalDevice.h>
#include <gfx/vk/QueuedDeviceTransfer.h>
#include <gfx/vk/memory/DeviceMemory.h>
#include <gfx/camera/UserControlledCamera.h>
#include <glm/gtc/matrix_inverse.hpp>

#undef MemoryBarrier

namespace vkfw_app::scene::rt {

    RaytracingScene::RaytracingScene(vkfw_core::gfx::LogicalDevice* t_device,
                                     vkfw_core::gfx::UserControlledCamera* t_camera,
                                     std::size_t t_num_framebuffers)
        : Scene(t_device, t_camera, t_num_framebuffers),
          m_memGroup{GetDevice(), vk::MemoryPropertyFlags()},
          m_cameraUBO{
              vkfw_core::gfx::UniformBufferObject::Create<CameraMatrixUBO>(GetDevice(), GetNumberOfFramebuffers())},
          m_asGeometry{GetDevice()}
    {
        auto properties =
            GetDevice()
                ->GetPhysicalDevice()
                .getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPropertiesKHR>();
        m_raytracingProperties = properties.get<vk::PhysicalDeviceRayTracingPropertiesKHR>();

        InitializeScene();
        InitializeDescriptorSets();
    }

    void RaytracingScene::InitializeScene()
    {
        auto numUBOBuffers = GetNumberOfFramebuffers();

        CameraMatrixUBO initialCameraUBO{glm::inverse(GetCamera()->GetViewMatrix()),
                                         glm::inverse(GetCamera()->GetProjMatrix())};
        auto uboSize = m_cameraUBO.GetCompleteSize();

        // Setup vertices for a single triangle
        struct Vertex
        {
            float pos[3];
        };
        std::vector<Vertex> vertices = {{1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
        std::vector<Vertex> vertices2 = {{1.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}};

        // Setup indices
        std::vector<uint32_t> indicesRT = {0, 1, 2};
        auto indexBufferOffset = vkfw_core::byteSizeOf(vertices);
        auto vertexBuffer2Offset = indexBufferOffset + vkfw_core::byteSizeOf(indicesRT);
        auto indexBuffer2Offset = vertexBuffer2Offset + vkfw_core::byteSizeOf(vertices2);
        auto uniformDataOffset =
            GetDevice()->CalculateUniformBufferAlignment(indexBuffer2Offset + vkfw_core::byteSizeOf(indicesRT));
        auto completeBufferSize = uniformDataOffset + uboSize;
        auto completeBufferIdx = m_memGroup.AddBufferToGroup(
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer
                | vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            completeBufferSize, std::vector<std::uint32_t>{{0, 1}});

        m_memGroup.AddDataToBufferInGroup(completeBufferIdx, 0, vertices);
        m_memGroup.AddDataToBufferInGroup(completeBufferIdx, indexBufferOffset, indicesRT);
        m_memGroup.AddDataToBufferInGroup(completeBufferIdx, vertexBuffer2Offset, vertices2);
        m_memGroup.AddDataToBufferInGroup(completeBufferIdx, indexBuffer2Offset, indicesRT);

        m_cameraUBO.AddUBOToBuffer(&m_memGroup, completeBufferIdx, uniformDataOffset, initialCameraUBO);

        vkfw_core::gfx::QueuedDeviceTransfer transfer{GetDevice(), std::make_pair(0, 0)};
        m_memGroup.FinalizeDeviceGroup();
        m_memGroup.TransferData(transfer);
        transfer.FinishTransfer();

        {
            m_transferCmdPool = GetDevice()->CreateCommandPoolForQueue(1);
            vk::CommandBufferAllocateInfo cmdBufferallocInfo{*m_transferCmdPool, vk::CommandBufferLevel::ePrimary,
                                                             static_cast<std::uint32_t>(numUBOBuffers)};
            m_vkTransferCommandBuffers = GetDevice()->GetDevice().allocateCommandBuffersUnique(cmdBufferallocInfo);
            for (auto i = 0U; i < numUBOBuffers; ++i) {
                vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eSimultaneousUse};
                m_vkTransferCommandBuffers[i]->begin(beginInfo);

                m_cameraUBO.FillUploadCmdBuffer<CameraMatrixUBO>(*m_vkTransferCommandBuffers[i], i);

                m_vkTransferCommandBuffers[i]->end();
            }
        }

        vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress =
            m_memGroup.GetBuffer(completeBufferIdx)->GetDeviceAddressConst();
        vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress{vertexBufferDeviceAddress.deviceAddress
                                                                 + indexBufferOffset};

        vk::DeviceOrHostAddressConstKHR vertexBuffer2DeviceAddress{vertexBufferDeviceAddress.deviceAddress
                                                                   + vertexBuffer2Offset};
        vk::DeviceOrHostAddressConstKHR indexBuffer2DeviceAddress{vertexBufferDeviceAddress.deviceAddress
                                                                  + indexBuffer2Offset};

        m_asGeometry.AddTriangleGeometry(1, vertices.size(), sizeof(Vertex), vertexBufferDeviceAddress,
                                         indexBufferDeviceAddress);

        m_asGeometry.AddTriangleGeometry(1, vertices2.size(), sizeof(Vertex), vertexBuffer2DeviceAddress,
                                         indexBuffer2DeviceAddress);

        m_asGeometry.InitializeAccelerationStructure();
    }

    void RaytracingScene::InitializeDescriptorSets()
    {
        std::vector<vk::DescriptorPoolSize> poolSizes = {{vk::DescriptorType::eAccelerationStructureKHR, 1},
                                                         {vk::DescriptorType::eStorageImage, 1},
                                                         {vk::DescriptorType::eUniformBufferDynamic, 1}};
        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{
            vk::DescriptorPoolCreateFlags{}, 2, static_cast<std::uint32_t>(poolSizes.size()), poolSizes.data()};
        m_vkDescriptorPool = GetDevice()->GetDevice().createDescriptorPoolUnique(descriptorPoolCreateInfo);

        vk::DescriptorSetLayoutBinding asLayoutBinding;
        m_asGeometry.FillDescriptorLayoutBinding(asLayoutBinding, vk::ShaderStageFlagBits::eRaygenKHR, 0);
        vk::DescriptorSetLayoutBinding resultImageLayoutBinding{1, vk::DescriptorType::eStorageImage, 1,
                                                                vk::ShaderStageFlagBits::eRaygenKHR};
        std::vector<vk::DescriptorSetLayoutBinding> bindings{asLayoutBinding, resultImageLayoutBinding};

        vk::DescriptorSetLayoutCreateInfo layoutInfo{vk::DescriptorSetLayoutCreateFlags{},
                                                     static_cast<std::uint32_t>(bindings.size()), bindings.data()};
        m_vkDescriptorSetLayout = GetDevice()->GetDevice().createDescriptorSetLayoutUnique(layoutInfo);

        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{*m_vkDescriptorPool, 1, &*m_vkDescriptorSetLayout};
        m_vkDescriptorSet = std::move(GetDevice()->GetDevice().allocateDescriptorSets(descriptorSetAllocateInfo)[0]);
        m_asGeometry.UseDescriptorSet(m_vkDescriptorSet, m_vkDescriptorSetLayout.get(), 0);

        m_cameraUBO.CreateLayout(*m_vkDescriptorPool, vk::ShaderStageFlagBits::eRaygenKHR, true, 0);

        {
            std::array<vk::DescriptorSetLayout, 2> pipelineDescSets;
            pipelineDescSets[0] = *m_vkDescriptorSetLayout;
            pipelineDescSets[1] = m_cameraUBO.GetDescriptorLayout();

            vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{vk::PipelineLayoutCreateFlags{},
                                                                  static_cast<std::uint32_t>(pipelineDescSets.size()),
                                                                  pipelineDescSets.data()};
            m_vkPipelineLayout = GetDevice()->GetDevice().createPipelineLayoutUnique(pipelineLayoutCreateInfo);
        }
    }

    void RaytracingScene::CreatePipeline(const glm::uvec2& screenSize, vkfw_core::VKWindow* window)
    {
        InitializeStorageImage(screenSize, window);
        FillDescriptorSets();

        m_shaders.resize(3);
        m_shaders[indexRaygen] = GetDevice()->GetShaderManager()->GetResource("shader/rt/raygen.rgen");
        m_shaders[indexMiss] = GetDevice()->GetShaderManager()->GetResource("shader/rt/miss.rmiss");
        m_shaders[indexClosestHit] = GetDevice()->GetShaderManager()->GetResource("shader/rt/closesthit.rchit");

        std::array<vk::PipelineShaderStageCreateInfo, 3> shaderStages;
        m_shaders[indexRaygen]->FillShaderStageInfo(shaderStages[indexRaygen]);
        m_shaders[indexMiss]->FillShaderStageInfo(shaderStages[indexMiss]);
        m_shaders[indexClosestHit]->FillShaderStageInfo(shaderStages[indexClosestHit]);

        m_shaderGroups.resize(shaderGroupCount);
        m_shaderGroups[indexRaygen].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral);
        m_shaderGroups[indexRaygen].setGeneralShader(indexRaygen);
        m_shaderGroups[indexRaygen].setClosestHitShader(VK_SHADER_UNUSED_KHR);
        m_shaderGroups[indexRaygen].setAnyHitShader(VK_SHADER_UNUSED_KHR);
        m_shaderGroups[indexRaygen].setIntersectionShader(VK_SHADER_UNUSED_KHR);
        m_shaderGroups[indexMiss].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral);
        m_shaderGroups[indexMiss].setGeneralShader(indexMiss);
        m_shaderGroups[indexMiss].setClosestHitShader(VK_SHADER_UNUSED_KHR);
        m_shaderGroups[indexMiss].setAnyHitShader(VK_SHADER_UNUSED_KHR);
        m_shaderGroups[indexMiss].setIntersectionShader(VK_SHADER_UNUSED_KHR);
        m_shaderGroups[indexClosestHit].setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
        m_shaderGroups[indexClosestHit].setGeneralShader(VK_SHADER_UNUSED_KHR);
        m_shaderGroups[indexClosestHit].setClosestHitShader(indexClosestHit);
        m_shaderGroups[indexClosestHit].setAnyHitShader(VK_SHADER_UNUSED_KHR);
        m_shaderGroups[indexClosestHit].setIntersectionShader(VK_SHADER_UNUSED_KHR);

        vk::PipelineLibraryCreateInfoKHR pipelineLibraryInfo{ 0, nullptr };
        vk::RayTracingPipelineCreateInfoKHR pipelineInfo{
            vk::PipelineCreateFlags{}, static_cast<std::uint32_t>(shaderStages.size()),
            shaderStages.data(),       static_cast<std::uint32_t>(m_shaderGroups.size()),
            m_shaderGroups.data(),     1,
            pipelineLibraryInfo,       nullptr,
            *m_vkPipelineLayout};

        if (auto result =
                GetDevice()->GetDevice().createRayTracingPipelinesKHRUnique(vk::PipelineCache{}, pipelineInfo);
            result.result != vk::Result::eSuccess) {
            spdlog::error("Could not create ray tracing pipeline.");
            assert(false);
        } else {
            m_vkPipeline = std::move(result.value[0]);
        }

        InitializeShaderBindingTable();
    }

    void RaytracingScene::InitializeStorageImage(const glm::uvec2& screenSize, const vkfw_core::VKWindow* window)
    {
        auto& bbDesc = window->GetFramebuffers()[0].GetDescriptor().m_tex[0];
        vkfw_core::gfx::TextureDescriptor storageTexDesc{bbDesc.m_bytesPP, bbDesc.m_format, bbDesc.m_samples};
        storageTexDesc.m_imageTiling = vk::ImageTiling::eOptimal;
        storageTexDesc.m_imageUsage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage;
        storageTexDesc.m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;

        m_storageImage = std::make_unique<vkfw_core::gfx::DeviceTexture>(GetDevice(), storageTexDesc);
        m_storageImage->InitializeImage(glm::u32vec4{screenSize, 1, 1}, 1);

        {
            vk::FenceCreateInfo fenceInfo;
            auto fence = GetDevice()->GetDevice().createFenceUnique(fenceInfo);
            auto cmdBuffer =
                m_storageImage->TransitionLayout(vk::ImageLayout::eGeneral, std::make_pair(0u, 0u), {}, {}, *fence);
            if (auto r = GetDevice()->GetDevice().waitForFences({*fence}, VK_TRUE, vkfw_core::defaultFenceTimeout);
                r != vk::Result::eSuccess) {
                spdlog::error("Could not wait for fence while transitioning layout: {}.", r);
                throw std::runtime_error("Could not wait for fence while transitioning layout.");
            }
        }
    }

    void RaytracingScene::FillDescriptorSets()
    {
        vk::WriteDescriptorSet accStructureWrite;
        m_asGeometry.FillDescriptorSetWrite(accStructureWrite);

        vk::DescriptorImageInfo storageImageDesc{vk::Sampler{}, m_storageImage->GetImageView(),
                                                 vk::ImageLayout::eGeneral};

        vk::WriteDescriptorSet resultImageWrite{m_vkDescriptorSet, 1, 0, 1, vk::DescriptorType::eStorageImage,
                                                &storageImageDesc};
        vk::WriteDescriptorSet uboWrite;
        m_cameraUBO.FillDescriptorSetWrite(uboWrite);

        GetDevice()->GetDevice().updateDescriptorSets(
            vk::ArrayProxy<const vk::WriteDescriptorSet>{accStructureWrite, resultImageWrite, uboWrite}, nullptr);
    }

    void vkfw_app::scene::rt::RaytracingScene::InitializeShaderBindingTable()
    {
        const std::uint32_t sbtSize = m_raytracingProperties.shaderGroupBaseAlignment * shaderGroupCount;

        std::vector<std::uint8_t> shaderHandleStorage;
        shaderHandleStorage.resize(sbtSize, 0);

        m_shaderBindingTable = std::make_unique<vkfw_core::gfx::HostBuffer>(
            GetDevice(), vk::BufferUsageFlagBits::eRayTracingKHR, vk::MemoryPropertyFlagBits::eHostVisible);

        GetDevice()->GetDevice().getRayTracingShaderGroupHandlesKHR(*m_vkPipeline, 0, shaderGroupCount,
                                                                    vk::ArrayProxy<std::uint8_t>(shaderHandleStorage));

        std::vector<std::uint8_t> shaderBindingTable;
        shaderBindingTable.resize(sbtSize, 0);
        auto* data = shaderBindingTable.data();
        // This part is required, as the alignment and handle size may differ
        for (uint32_t i = 0; i < shaderGroupCount; i++) {
            memcpy(data, shaderHandleStorage.data() + i * m_raytracingProperties.shaderGroupHandleSize,
                   m_raytracingProperties.shaderGroupHandleSize);
            data += m_raytracingProperties.shaderGroupBaseAlignment;
        }

        m_shaderBindingTable->InitializeData(shaderBindingTable);
    }

    void RaytracingScene::UpdateCommandBuffer(const vk::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex,
                                              vkfw_core::VKWindow* window)
    {
        // TODO: do we need to update the storage image? [11/18/2020 Sebastian Maisch]

        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *m_vkPipeline);
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *m_vkPipelineLayout, 0, m_vkDescriptorSet,
                                     nullptr);
        m_cameraUBO.Bind(cmdBuffer, vk::PipelineBindPoint::eRayTracingKHR, *m_vkPipelineLayout, 1, cmdBufferIndex);

        vk::DeviceSize shaderBindingTableSize = m_raytracingProperties.shaderGroupBaseAlignment * m_shaderGroups.size();

        vk::StridedBufferRegionKHR raygenShaderSBTEntry{
            m_shaderBindingTable->GetBuffer(),
            static_cast<vk::DeviceSize>(m_raytracingProperties.shaderGroupBaseAlignment * indexRaygen),
            m_raytracingProperties.shaderGroupBaseAlignment, shaderBindingTableSize};
        vk::StridedBufferRegionKHR missShaderSBTEntry{
            m_shaderBindingTable->GetBuffer(),
            static_cast<vk::DeviceSize>(m_raytracingProperties.shaderGroupBaseAlignment * indexMiss),
            m_raytracingProperties.shaderGroupBaseAlignment, shaderBindingTableSize};
        vk::StridedBufferRegionKHR hitShaderSBTEntry{
            m_shaderBindingTable->GetBuffer(),
            static_cast<vk::DeviceSize>(m_raytracingProperties.shaderGroupBaseAlignment * indexClosestHit),
            m_raytracingProperties.shaderGroupBaseAlignment, shaderBindingTableSize};

        vk::StridedBufferRegionKHR callableShaderSTBEntry{};

        cmdBuffer.traceRaysKHR(raygenShaderSBTEntry, missShaderSBTEntry, hitShaderSBTEntry, callableShaderSTBEntry,
                               m_storageImage->GetPixelSize().x, m_storageImage->GetPixelSize().y,
                               m_storageImage->GetPixelSize().z);

        // Prepare current swapchain image as transfer destination
        window->GetFramebuffers()[cmdBufferIndex].GetTexture(0).TransitionLayout(vk::ImageLayout::eTransferDstOptimal,
                                                                                 cmdBuffer);
        m_storageImage->TransitionLayout(vk::ImageLayout::eTransferSrcOptimal, cmdBuffer);

        m_storageImage->CopyImageAsync(0, glm::u32vec4{0}, window->GetFramebuffers()[cmdBufferIndex].GetTexture(0), 0,
                                       glm::u32vec4{0}, m_storageImage->GetSize(), cmdBuffer);

        window->GetFramebuffers()[cmdBufferIndex].GetTexture(0).TransitionLayout(vk::ImageLayout::eColorAttachmentOptimal,
                                                                                 cmdBuffer);
        m_storageImage->TransitionLayout(vk::ImageLayout::eGeneral, cmdBuffer);
    }

    void RaytracingScene::FrameMove(float, float, const vkfw_core::VKWindow* window)
    {
        CameraMatrixUBO camera_ubo{glm::inverse(GetCamera()->GetViewMatrix()),
                                   glm::inverse(GetCamera()->GetProjMatrix())};

        auto uboIndex = window->GetCurrentlyRenderedImageIndex();
        m_cameraUBO.UpdateInstanceData(uboIndex, camera_ubo);

        vk::Semaphore vkTransferSemaphore = window->GetDataAvailableSemaphore();
        vk::SubmitInfo submitInfo{
            0, nullptr, nullptr, 1, &(*m_vkTransferCommandBuffers[uboIndex]), 1, &vkTransferSemaphore};
        GetDevice()->GetQueue(1, 0).submit(submitInfo, vk::Fence());
    }

    void RaytracingScene::RenderScene(const vkfw_core::VKWindow*) {}

}

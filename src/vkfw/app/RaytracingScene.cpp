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
#include <gfx/meshes/AssImpScene.h>
#include <gfx/vk/wrappers/CommandBuffer.h>
#include <gfx/vk/Framebuffer.h>
#include <gfx/vk/LogicalDevice.h>
#include <gfx/vk/QueuedDeviceTransfer.h>
#include <gfx/vk/memory/DeviceMemory.h>
#include <gfx/camera/UserControlledCamera.h>
// #include <gfx/VertexFormats.h>
#include <glm/gtc/matrix_inverse.hpp>
#include "imgui.h"

#undef MemoryBarrier

namespace vkfw_app::scene::rt {

    RaytracingScene::RaytracingScene(vkfw_core::gfx::LogicalDevice* t_device,
                                     vkfw_core::gfx::UserControlledCamera* t_camera,
                                     std::size_t t_num_framebuffers)
        : Scene(t_device, t_camera, t_num_framebuffers)
        , m_memGroup{GetDevice(), "RTSceneMemoryGroup", vk::MemoryPropertyFlags()}
        , m_cameraUBO{vkfw_core::gfx::UniformBufferObject::Create<CameraPropertiesBuffer>(GetDevice(), GetNumberOfFramebuffers())}
        , m_asGeometry{GetDevice(), "RTSceneASGeometry", std::vector<std::uint32_t>{{0, 1}}}
        , m_descriptorSetLayout{"RTSceneDescriptorSetLayout"}
        , m_pipelineLayout{GetDevice()->GetHandle(), "RTScenePipelineLayout", vk::UniquePipelineLayout{}}
        , m_descriptorSet{GetDevice()->GetHandle(), "RTSceneDescriptorSet", vk::DescriptorSet{}}
        , m_pipeline{GetDevice(), "RTScenePipeline", {}}
    {
        m_triangleMaterial.m_materialName = "RT_DemoScene_TriangleMaterial";
        m_triangleMaterial.m_diffuse = glm::vec3{0.988f, 0.059f, 0.753};
        InitializeScene();
        InitializeDescriptorSets();
    }

    void RaytracingScene::InitializeScene()
    {
        auto numUBOBuffers = GetNumberOfFramebuffers();

        m_cameraProperties.viewInverse = glm::inverse(GetCamera()->GetViewMatrix());
        m_cameraProperties.projInverse = glm::inverse(GetCamera()->GetProjMatrix());
        m_cameraProperties.frameId = 0;
        m_cameraProperties.cosineSampled = 0;
        m_cameraProperties.cameraMovedThisFrame = 1;
        m_cameraProperties.maxRange = 100.0f;
        auto uboSize = m_cameraUBO.GetCompleteSize();

        // Setup vertices for a single triangle
        std::vector<RayTracingVertex> vertices = {{glm::vec3{1.0f, 1.0f, 0.0f}, glm::vec3{0.0f, 0.0f, -1.0f}, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}, glm::vec2{0.0f}},
                                                  {glm::vec3{-1.0f, 1.0f, 0.0f}, glm::vec3{0.0f, 0.0f, -1.0f}, glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}, glm::vec2{0.0f}},
                                                  {glm::vec3{0.0f, -1.0f, 0.0f}, glm::vec3{0.0f, 0.0f, -1.0f}, glm::vec4{0.0f, 0.0f, 1.0f, 1.0f}, glm::vec2{0.0f}}};
        // Setup indices
        std::vector<uint32_t> indicesRT = {0, 1, 2};

        m_teapotMeshInfo = std::make_shared<vkfw_core::gfx::AssImpScene>("teapot/teapot.obj", GetDevice());
        m_sponzaMeshInfo = std::make_shared<vkfw_core::gfx::AssImpScene>("sponza/sponza.obj", GetDevice());

        glm::mat4 worldMatrixTeapot = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(0.015f));
        glm::mat4 worldMatrixSponza = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(0.015f));

        auto indexBufferOffset = GetDevice()->CalculateStorageBufferAlignment(vkfw_core::byteSizeOf(vertices));
        // this is not documented but it seems this memory needs the same alignment as uniform buffers.
        // auto transformBufferMeshOffset = GetDevice()->CalculateUniformBufferAlignment(indexBufferMeshOffset + vkfw_core::byteSizeOf(indicesMesh));
        auto uniformDataOffset =
            GetDevice()->CalculateUniformBufferAlignment(indexBufferOffset + vkfw_core::byteSizeOf(indicesRT));
        auto completeBufferSize = uniformDataOffset + uboSize;
        auto completeBufferIdx = m_memGroup.AddBufferToGroup("RTSceneCompleteBuffer",
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer
                | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eUniformBuffer
                | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            completeBufferSize, std::vector<std::uint32_t>{{0, 1}});

        m_memGroup.AddDataToBufferInGroup(completeBufferIdx, 0, vertices);
        m_memGroup.AddDataToBufferInGroup(completeBufferIdx, indexBufferOffset, indicesRT);

        m_cameraUBO.AddUBOToBuffer(&m_memGroup, completeBufferIdx, uniformDataOffset, m_cameraProperties);

        vkfw_core::gfx::QueuedDeviceTransfer transfer{GetDevice(), GetDevice()->GetQueue(0, 0)};
        m_memGroup.FinalizeDeviceGroup();
        m_memGroup.TransferData(transfer);
        transfer.FinishTransfer();

        {
            m_transferCmdPool = GetDevice()->CreateCommandPoolForQueue("RTSceneTransferCommandPool", 1);
            vk::CommandBufferAllocateInfo cmdBufferallocInfo{m_transferCmdPool.GetHandle(), vk::CommandBufferLevel::ePrimary,
                                                             static_cast<std::uint32_t>(numUBOBuffers)};
            m_transferCommandBuffers = vkfw_core::gfx::CommandBuffer::Initialize(GetDevice()->GetHandle(), "RTSceneTransferCommandBuffer", GetDevice()->GetHandle().allocateCommandBuffersUnique(cmdBufferallocInfo));
            for (auto i = 0U; i < numUBOBuffers; ++i) {
                vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eSimultaneousUse};
                m_transferCommandBuffers[i].Begin(beginInfo);

                m_cameraUBO.FillUploadCmdBuffer<CameraPropertiesBuffer>(m_transferCommandBuffers[i], i);

                m_transferCommandBuffers[i].End();
            }
        }

        m_asGeometry.AddTriangleGeometry(glm::mat3x4{1.0f}, m_triangleMaterial, 1, vertices.size(), sizeof(RayTracingVertex),
                                         m_memGroup.GetBuffer(completeBufferIdx));

        m_asGeometry.AddMeshGeometry(*m_teapotMeshInfo.get(), worldMatrixTeapot);
        m_asGeometry.AddMeshGeometry(*m_sponzaMeshInfo.get(), worldMatrixSponza);
        m_asGeometry.FinalizeGeometry<RayTracingVertex>();

        // m_asGeometry.AddMeshGeometry(*m_meshInfo.get(), worldMatrixMesh);
        // m_asGeometry.FinalizeMeshGeometry<RayTracingVertex>();

        m_asGeometry.BuildAccelerationStructure();
    }

    void RaytracingScene::InitializeDescriptorSets()
    {
        using UniformBufferObject = vkfw_core::gfx::UniformBufferObject;
        using Texture = vkfw_core::gfx::Texture;
        using Bindings = RTBindings;

        m_asGeometry.AddDescriptorLayoutBindingAS(m_descriptorSetLayout, vk::ShaderStageFlagBits::eRaygenKHR, static_cast<uint32_t>(Bindings::AccelerationStructure));
        m_asGeometry.AddDescriptorLayoutBindingBuffers(m_descriptorSetLayout, vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR, static_cast<uint32_t>(Bindings::Vertices), static_cast<uint32_t>(Bindings::Indices),
                                                       static_cast<uint32_t>(Bindings::InstanceInfos), static_cast<uint32_t>(Bindings::MaterialInfos), static_cast<uint32_t>(Bindings::DiffuseTextures),
                                                       static_cast<uint32_t>(Bindings::BumpTextures));
        Texture::AddDescriptorLayoutBinding(m_descriptorSetLayout, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenKHR, static_cast<uint32_t>(Bindings::ResultImage));
        UniformBufferObject::AddDescriptorLayoutBinding(m_descriptorSetLayout, vk::ShaderStageFlagBits::eRaygenKHR, true, static_cast<uint32_t>(Bindings::CameraProperties));

        auto descLayout = m_descriptorSetLayout.CreateDescriptorLayout(GetDevice());

        m_descriptorPool = m_descriptorSetLayout.CreateDescriptorPool(GetDevice(), "RTSceneDescriptorPool");

        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{m_descriptorPool.GetHandle(), 1, &descLayout};
        m_descriptorSet.SetHandle(GetDevice()->GetHandle(), GetDevice()->GetHandle().allocateDescriptorSets(descriptorSetAllocateInfo)[0]);

        {
            std::vector<vk::DescriptorSetLayout> pipelineDescSets;
            pipelineDescSets.emplace_back(descLayout);

            vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{vk::PipelineLayoutCreateFlags{},
                                                                  static_cast<std::uint32_t>(pipelineDescSets.size()),
                                                                  pipelineDescSets.data()};
            m_pipelineLayout.SetHandle(GetDevice()->GetHandle(), GetDevice()->GetHandle().createPipelineLayoutUnique(pipelineLayoutCreateInfo));
        }
    }

    void RaytracingScene::CreatePipeline(const glm::uvec2& screenSize, vkfw_core::VKWindow* window)
    {
        if (m_pipeline) { return; }
        InitializeStorageImage(screenSize, window);
        FillDescriptorSets();

        std::vector<std::shared_ptr<vkfw_core::gfx::Shader>> shaders{GetDevice()->GetShaderManager()->GetResource("shader/rt/ao.rgen"), GetDevice()->GetShaderManager()->GetResource("shader/rt/miss.rmiss"),
                                                                     GetDevice()->GetShaderManager()->GetResource("shader/rt/closesthit.rchit"), GetDevice()->GetShaderManager()->GetResource("shader/rt/skipAlpha.rahit")};
        m_pipeline.ResetShaders(std::move(shaders));
        m_pipeline.CreatePipeline(1, m_pipelineLayout);
    }

    void RaytracingScene::InitializeStorageImage(const glm::uvec2& screenSize, const vkfw_core::VKWindow* window)
    {
        auto& bbDesc = window->GetFramebuffers()[0].GetDescriptor().m_tex[0];
        vkfw_core::gfx::TextureDescriptor storageTexDesc{bbDesc.m_bytesPP, bbDesc.m_format, bbDesc.m_samples};
        storageTexDesc.m_imageTiling = vk::ImageTiling::eOptimal;
        storageTexDesc.m_imageUsage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage;
        storageTexDesc.m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;

        m_storageImage = std::make_unique<vkfw_core::gfx::DeviceTexture>(GetDevice(), "RTSceneStorageImage", storageTexDesc);
        m_storageImage->InitializeImage(glm::u32vec4{screenSize, 1, 1}, 1);

        {
            vk::FenceCreateInfo fenceInfo;
            vkfw_core::gfx::Fence fence{GetDevice()->GetHandle(), "RTSceneImageTransitionFence", GetDevice()->GetHandle().createFenceUnique(fenceInfo)};
            auto cmdBuffer =
                m_storageImage->TransitionLayout(vk::ImageLayout::eGeneral, GetDevice()->GetQueue(0, 0), {}, {}, fence);
            if (auto r = GetDevice()->GetHandle().waitForFences({fence.GetHandle()}, VK_TRUE, vkfw_core::defaultFenceTimeout);
                r != vk::Result::eSuccess) {
                spdlog::error("Could not wait for fence while transitioning layout: {}.", r);
                throw std::runtime_error("Could not wait for fence while transitioning layout.");
            }
        }
    }

    void RaytracingScene::FillDescriptorSets()
    {
        using Bindings = RTBindings;

        std::vector<vk::WriteDescriptorSet> descSetWrites;
        descSetWrites.resize(static_cast<std::size_t>(Bindings::BindingsSize));
        vk::WriteDescriptorSetAccelerationStructureKHR descSetAccStructure;
        vk::DescriptorBufferInfo camrraBufferInfo;
        vk::DescriptorImageInfo storageImageDesc;
        std::vector<vk::DescriptorBufferInfo> vboBufferInfos;
        std::vector<vk::DescriptorBufferInfo> iboBufferInfos;
        vk::DescriptorBufferInfo instanceBufferInfo;
        vk::DescriptorBufferInfo materialBufferInfo;
        std::vector<vk::DescriptorImageInfo> diffuseTextureInfos;
        std::vector<vk::DescriptorImageInfo> bumpTextureInfos;

        m_asGeometry.FillDescriptorAccelerationStructureInfo(descSetAccStructure);
        descSetWrites[static_cast<std::size_t>(Bindings::AccelerationStructure)] = m_descriptorSetLayout.MakeWrite(m_descriptorSet, static_cast<uint32_t>(Bindings::AccelerationStructure), &descSetAccStructure);

        m_storageImage->FillDescriptorImageInfo(storageImageDesc, vkfw_core::gfx::Sampler{});
        descSetWrites[static_cast<std::size_t>(Bindings::ResultImage)] = m_descriptorSetLayout.MakeWrite(m_descriptorSet, static_cast<uint32_t>(Bindings::ResultImage), &storageImageDesc);

        m_cameraUBO.FillDescriptorBufferInfo(camrraBufferInfo);
        descSetWrites[static_cast<std::size_t>(Bindings::CameraProperties)] = m_descriptorSetLayout.MakeWrite(m_descriptorSet, static_cast<uint32_t>(Bindings::CameraProperties), &camrraBufferInfo);

        m_asGeometry.FillDescriptorBuffersInfo(vboBufferInfos, iboBufferInfos, instanceBufferInfo, materialBufferInfo, diffuseTextureInfos, bumpTextureInfos);
        descSetWrites[static_cast<std::size_t>(Bindings::Vertices)] = m_descriptorSetLayout.MakeWriteArray(m_descriptorSet, static_cast<uint32_t>(Bindings::Vertices), vboBufferInfos.data());
        descSetWrites[static_cast<std::size_t>(Bindings::Indices)] = m_descriptorSetLayout.MakeWriteArray(m_descriptorSet, static_cast<uint32_t>(Bindings::Indices), iboBufferInfos.data());
        descSetWrites[static_cast<std::size_t>(Bindings::InstanceInfos)] = m_descriptorSetLayout.MakeWrite(m_descriptorSet, static_cast<uint32_t>(Bindings::InstanceInfos), &instanceBufferInfo);
        descSetWrites[static_cast<std::size_t>(Bindings::MaterialInfos)] = m_descriptorSetLayout.MakeWrite(m_descriptorSet, static_cast<uint32_t>(Bindings::MaterialInfos), &materialBufferInfo);
        descSetWrites[static_cast<std::size_t>(Bindings::DiffuseTextures)] = m_descriptorSetLayout.MakeWriteArray(m_descriptorSet, static_cast<uint32_t>(Bindings::DiffuseTextures), diffuseTextureInfos.data());
        descSetWrites[static_cast<std::size_t>(Bindings::BumpTextures)] = m_descriptorSetLayout.MakeWriteArray(m_descriptorSet, static_cast<uint32_t>(Bindings::BumpTextures), bumpTextureInfos.data());

        GetDevice()->GetHandle().updateDescriptorSets(descSetWrites, nullptr);
    }

    void RaytracingScene::UpdateCommandBuffer(const vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex,
                                              vkfw_core::VKWindow* window)
    {
        auto& sbtDeviceAddressRegions = m_pipeline.GetSBTDeviceAddresses();

        cmdBuffer.GetHandle().bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline.GetHandle());
        cmdBuffer.GetHandle().bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_pipelineLayout.GetHandle(), 0, m_descriptorSet.GetHandle(),
                                     static_cast<std::uint32_t>(cmdBufferIndex * m_cameraUBO.GetInstanceSize()));

        cmdBuffer.GetHandle().traceRaysKHR(sbtDeviceAddressRegions[0], sbtDeviceAddressRegions[1], sbtDeviceAddressRegions[2], sbtDeviceAddressRegions[3], m_storageImage->GetPixelSize().x, m_storageImage->GetPixelSize().y,
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
        m_cameraProperties.viewInverse = glm::inverse(GetCamera()->GetViewMatrix());
        m_cameraProperties.projInverse = glm::inverse(GetCamera()->GetProjMatrix());
        m_cameraProperties.frameId += 1;

        auto uboIndex = window->GetCurrentlyRenderedImageIndex();
        m_cameraUBO.UpdateInstanceData(uboIndex, m_cameraProperties);

        const auto& transferQueue = GetDevice()->GetQueue(1, 0);
        {
            QUEUE_REGION(transferQueue, "FrameMove");
            auto& transferSemaphore = window->GetDataAvailableSemaphore();
            vk::SubmitInfo submitInfo{0, nullptr, nullptr, 1, m_transferCommandBuffers[uboIndex].GetHandlePtr(), 1, transferSemaphore.GetHandlePtr()};
            transferQueue.Submit(submitInfo, vkfw_core::gfx::Fence{});

        }
    }

    void RaytracingScene::RenderScene(const vkfw_core::VKWindow*) {}

    bool RaytracingScene::RenderGUI(const vkfw_core::VKWindow*)
    {
        ImGui::SetNextWindowPos(ImVec2(5, 100), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(220, 190), ImGuiCond_Always);
        if (ImGui::Begin("Scene Control")) {
            bool cosSample = m_cameraProperties.cosineSampled == 1;
            ImGui::Checkbox("Samples Cosine Weigthed", &cosSample);
            m_cameraProperties.cosineSampled = cosSample ? 1 : 0;
            ImGui::SliderFloat("Max. Range", &m_cameraProperties.maxRange, 0.1f, 10000.0f);
        }
        ImGui::End();

        return false;
    }

}
;
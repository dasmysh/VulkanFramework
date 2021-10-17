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
        , m_rtResourcesDescriptorSetLayout{"RTSceneResourcesDescriptorSetLayout"}
        , m_convergenceImageDescriptorSetLayout{"RTSceneConvergenceDescriptorSet"}
        , m_pipelineLayout{GetDevice()->GetHandle(), "RTScenePipelineLayout", vk::UniquePipelineLayout{}}
        , m_rtResourcesDescriptorSet{GetDevice()->GetHandle(), "RTSceneResourcesDescriptorSet", vk::DescriptorSet{}}
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
            m_transferCommandBuffers =
                vkfw_core::gfx::CommandBuffer::Initialize(GetDevice()->GetHandle(), "RTSceneTransferCommandBuffer", m_transferCmdPool.GetQueueFamily(), GetDevice()->GetHandle().allocateCommandBuffersUnique(cmdBufferallocInfo));
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

        {
            vk::FenceCreateInfo fenceInfo;
            vkfw_core::gfx::Fence fence{GetDevice()->GetHandle(), "TransferImageLayoutsInitialFence", GetDevice()->GetHandle().createFenceUnique(fenceInfo)};
            auto cmdBuffer = vkfw_core::gfx::CommandBuffer::beginSingleTimeSubmit(GetDevice(), "TransferImageLayoutsInitialCommandBuffer", "TransferImageLayoutsInitial", GetDevice()->GetCommandPool(0));
            vkfw_core::gfx::PipelineBarrier barrier{GetDevice(), vk::PipelineStageFlagBits::eRayTracingShaderKHR};
            m_asGeometry.CreateResourceUseBarriers(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eRayTracingShaderKHR, vk::ImageLayout::eShaderReadOnlyOptimal, barrier);
            for (auto& convImage : m_rayTracingConvergenceImages) {
                auto convergenceImageAccessor = convImage.GetAccess();
                convergenceImageAccessor.SetAccess(vk::AccessFlagBits::eShaderWrite, vk::PipelineStageFlagBits::eRayTracingShaderKHR, vk::ImageLayout::eGeneral, barrier);
            }
            barrier.Record(cmdBuffer);
            vkfw_core::gfx::CommandBuffer::endSingleTimeSubmit(GetDevice()->GetQueue(0, 0), cmdBuffer, {}, {}, fence);
            if (auto r = GetDevice()->GetHandle().waitForFences({fence.GetHandle()}, VK_TRUE, vkfw_core::defaultFenceTimeout); r != vk::Result::eSuccess) {
                spdlog::error("Could not wait for fence while transitioning layout: {}.", r);
                throw std::runtime_error("Could not wait for fence while transitioning layout.");
            }
        }
    }

    void RaytracingScene::InitializeDescriptorSets()
    {
        using UniformBufferObject = vkfw_core::gfx::UniformBufferObject;
        using Texture = vkfw_core::gfx::Texture;
        using ResBindings = ResSetBindings;
        using ConvBindings = ConvSetBindings;

        m_asGeometry.AddDescriptorLayoutBindingAS(m_rtResourcesDescriptorSetLayout, vk::ShaderStageFlagBits::eRaygenKHR, static_cast<uint32_t>(ResBindings::AccelerationStructure));
        m_asGeometry.AddDescriptorLayoutBindingBuffers(m_rtResourcesDescriptorSetLayout, vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR, static_cast<uint32_t>(ResBindings::Vertices),
                                                       static_cast<uint32_t>(ResBindings::Indices), static_cast<uint32_t>(ResBindings::InstanceInfos), static_cast<uint32_t>(ResBindings::MaterialInfos),
                                                       static_cast<uint32_t>(ResBindings::DiffuseTextures), static_cast<uint32_t>(ResBindings::BumpTextures));
        UniformBufferObject::AddDescriptorLayoutBinding(m_rtResourcesDescriptorSetLayout, vk::ShaderStageFlagBits::eRaygenKHR, true, static_cast<uint32_t>(ResBindings::CameraProperties));

        Texture::AddDescriptorLayoutBinding(m_convergenceImageDescriptorSetLayout, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenKHR, static_cast<uint32_t>(ConvBindings::ResultImage));

        auto rtResourcesDescSetLayout = m_rtResourcesDescriptorSetLayout.CreateDescriptorLayout(GetDevice());
        auto convergenceDescSetLayout = m_convergenceImageDescriptorSetLayout.CreateDescriptorLayout(GetDevice());

        std::vector<vk::DescriptorPoolSize> descSetPoolSizes;
        std::size_t descSetCount = 0;
        std::vector<vk::DescriptorSetLayout> descSetCreateLayouts;

        descSetCreateLayouts.emplace_back(rtResourcesDescSetLayout);
        m_rtResourcesDescriptorSetLayout.AddDescriptorPoolSizes(descSetPoolSizes, 1);
        descSetCount += 1;
        for (std::size_t i = 0; i < GetNumberOfFramebuffers(); ++i) {
            descSetCreateLayouts.emplace_back(convergenceDescSetLayout);
            m_convergenceImageDescriptorSetLayout.AddDescriptorPoolSizes(descSetPoolSizes, 1);
            descSetCount += 1;
        }

        m_descriptorPool = vkfw_core::gfx::DescriptorSetLayout::CreateDescriptorPool(GetDevice(), "RTSceneDescriptorPool", descSetPoolSizes, descSetCount);

        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{m_descriptorPool.GetHandle(), descSetCreateLayouts};
        auto descSetAllocateResults = GetDevice()->GetHandle().allocateDescriptorSets(descriptorSetAllocateInfo);
        m_rtResourcesDescriptorSet.SetHandle(GetDevice()->GetHandle(), std::move(descSetAllocateResults[0]));
        m_convergenceImageDescriptorSets.reserve(GetNumberOfFramebuffers());
        for (std::size_t i = 0; i < GetNumberOfFramebuffers(); ++i) { m_convergenceImageDescriptorSets.emplace_back(GetDevice()->GetHandle(), fmt::format("RTSceneConvergenceDescriptorSet-{}", i), std::move(descSetAllocateResults[1 + i])); }

        {
            std::array<vk::DescriptorSetLayout, 2> descSetLayouts;
            descSetLayouts[static_cast<std::size_t>(BindingSets::RTResourcesSet)] = rtResourcesDescSetLayout;
            descSetLayouts[static_cast<std::size_t>(BindingSets::ConvergenceSet)] = convergenceDescSetLayout;

            vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{vk::PipelineLayoutCreateFlags{}, descSetLayouts};
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

        {
            // vk::FenceCreateInfo fenceInfo;
            // vkfw_core::gfx::Fence fence{GetDevice()->GetHandle(), "RTSceneImageTransitionFence", GetDevice()->GetHandle().createFenceUnique(fenceInfo)};
            // auto transitionCmdBuffer = vkfw_core::gfx::CommandBuffer::beginSingleTimeSubmit(GetDevice(), "TransitionLayoutCmdBuffer:RTSceneConvergenceImages", "TransitionLayout", GetDevice()->GetQueue(0, 0).GetCommandPool());

            // vkfw_core::gfx::PipelineBarrier barrier{GetDevice(), dstPipelineStage};
            for (std::size_t i = 0; i < window->GetFramebuffers().size(); ++i) {
                auto& image = m_rayTracingConvergenceImages.emplace_back(GetDevice(), fmt::format("RTSceneConvergenceImage-{}", i), storageTexDesc, vk::ImageLayout::eUndefined);
                image.InitializeImage(glm::u32vec4{screenSize, 1, 1}, 1);
                // auto accessor = image.GetAccess();
                // [[maybe_unused]] auto dummy = accessor.Get(access, pipelineStages, vk::ImageLayout::eGeneral, barrier);
                // image.TransitionLayout(vk::ImageLayout::eGeneral, transitionCmdBuffer);
            }
            // barrier.Record(transitionCmdBuffer);

            // vkfw_core::gfx::CommandBuffer::endSingleTimeSubmit(GetDevice()->GetQueue(0, 0), transitionCmdBuffer, {}, {}, fence);

            // if (auto r = GetDevice()->GetHandle().waitForFences({fence.GetHandle()}, VK_TRUE, vkfw_core::defaultFenceTimeout); r != vk::Result::eSuccess) {
            //     spdlog::error("Could not wait for fence while transitioning layout: {}.", r);
            //     throw std::runtime_error("Could not wait for fence while transitioning layout.");
            // }
        }
    }

    void RaytracingScene::FillDescriptorSets()
    {
        using ResBindings = ResSetBindings;
        using ConvBindings = ConvSetBindings;

        std::vector<vk::WriteDescriptorSet> descSetWrites;
        descSetWrites.resize(static_cast<std::size_t>(ResBindings::BindingsSize) + m_convergenceImageDescriptorSets.size());
        vk::WriteDescriptorSetAccelerationStructureKHR descSetAccStructure;
        vk::DescriptorBufferInfo camrraBufferInfo;
        std::vector<vk::DescriptorImageInfo> storageImageDescs;
        std::vector<vk::DescriptorBufferInfo> vboBufferInfos;
        std::vector<vk::DescriptorBufferInfo> iboBufferInfos;
        vk::DescriptorBufferInfo instanceBufferInfo;
        vk::DescriptorBufferInfo materialBufferInfo;
        std::vector<vk::DescriptorImageInfo> diffuseTextureInfos;
        std::vector<vk::DescriptorImageInfo> bumpTextureInfos;

        m_asGeometry.FillDescriptorAccelerationStructureInfo(descSetAccStructure);
        descSetWrites[static_cast<std::size_t>(ResBindings::AccelerationStructure)] = m_rtResourcesDescriptorSetLayout.MakeWrite(m_rtResourcesDescriptorSet, static_cast<uint32_t>(ResBindings::AccelerationStructure), &descSetAccStructure);

        m_cameraUBO.FillDescriptorBufferInfo(camrraBufferInfo);
        descSetWrites[static_cast<std::size_t>(ResBindings::CameraProperties)] = m_rtResourcesDescriptorSetLayout.MakeWrite(m_rtResourcesDescriptorSet, static_cast<uint32_t>(ResBindings::CameraProperties), &camrraBufferInfo);

        m_asGeometry.FillDescriptorBuffersInfo(vboBufferInfos, iboBufferInfos, instanceBufferInfo, materialBufferInfo, diffuseTextureInfos, bumpTextureInfos);
        descSetWrites[static_cast<std::size_t>(ResBindings::Vertices)] = m_rtResourcesDescriptorSetLayout.MakeWriteArray(m_rtResourcesDescriptorSet, static_cast<uint32_t>(ResBindings::Vertices), vboBufferInfos.data());
        descSetWrites[static_cast<std::size_t>(ResBindings::Indices)] = m_rtResourcesDescriptorSetLayout.MakeWriteArray(m_rtResourcesDescriptorSet, static_cast<uint32_t>(ResBindings::Indices), iboBufferInfos.data());
        descSetWrites[static_cast<std::size_t>(ResBindings::InstanceInfos)] = m_rtResourcesDescriptorSetLayout.MakeWrite(m_rtResourcesDescriptorSet, static_cast<uint32_t>(ResBindings::InstanceInfos), &instanceBufferInfo);
        descSetWrites[static_cast<std::size_t>(ResBindings::MaterialInfos)] = m_rtResourcesDescriptorSetLayout.MakeWrite(m_rtResourcesDescriptorSet, static_cast<uint32_t>(ResBindings::MaterialInfos), &materialBufferInfo);
        descSetWrites[static_cast<std::size_t>(ResBindings::DiffuseTextures)] = m_rtResourcesDescriptorSetLayout.MakeWriteArray(m_rtResourcesDescriptorSet, static_cast<uint32_t>(ResBindings::DiffuseTextures), diffuseTextureInfos.data());
        descSetWrites[static_cast<std::size_t>(ResBindings::BumpTextures)] = m_rtResourcesDescriptorSetLayout.MakeWriteArray(m_rtResourcesDescriptorSet, static_cast<uint32_t>(ResBindings::BumpTextures), bumpTextureInfos.data());

        storageImageDescs.resize(m_convergenceImageDescriptorSets.size());
        for (std::size_t i = 0; i < m_convergenceImageDescriptorSets.size(); ++i) {
            m_rayTracingConvergenceImages[i].FillDescriptorImageInfo(storageImageDescs[i], vkfw_core::gfx::Sampler{}, vk::ImageLayout::eGeneral);
            descSetWrites[static_cast<std::size_t>(ResBindings::BindingsSize) + i] = m_convergenceImageDescriptorSetLayout.MakeWrite(m_convergenceImageDescriptorSets[i], static_cast<uint32_t>(ConvBindings::ResultImage), &storageImageDescs[i]);
        }

        GetDevice()->GetHandle().updateDescriptorSets(descSetWrites, nullptr);
    }

    void RaytracingScene::UpdateCommandBuffer(const vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex,
                                              vkfw_core::VKWindow* window)
    {
        auto& sbtDeviceAddressRegions = m_pipeline.GetSBTDeviceAddresses();

        {
            vkfw_core::gfx::PipelineBarrier barrier{GetDevice(), vk::PipelineStageFlagBits::eRayTracingShaderKHR};
            m_asGeometry.CreateResourceUseBarriers(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eRayTracingShaderKHR, vk::ImageLayout::eShaderReadOnlyOptimal, barrier);
            auto convergenceImageAccessor = m_rayTracingConvergenceImages[cmdBufferIndex].GetAccess();
            convergenceImageAccessor.SetAccess(vk::AccessFlagBits::eShaderWrite, vk::PipelineStageFlagBits::eRayTracingShaderKHR, vk::ImageLayout::eGeneral, barrier);
            barrier.Record(cmdBuffer);
        }

        cmdBuffer.GetHandle().bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline.GetHandle());
        cmdBuffer.GetHandle().bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_pipelineLayout.GetHandle(), 0, m_rtResourcesDescriptorSet.GetHandle(), static_cast<std::uint32_t>(cmdBufferIndex * m_cameraUBO.GetInstanceSize()));

        // TODO: make sure the image has the correct layout here (same as on fill)...
        cmdBuffer.GetHandle().bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_pipelineLayout.GetHandle(), 1, m_convergenceImageDescriptorSets[cmdBufferIndex].GetHandle(), {});

        cmdBuffer.GetHandle().traceRaysKHR(sbtDeviceAddressRegions[0], sbtDeviceAddressRegions[1], sbtDeviceAddressRegions[2], sbtDeviceAddressRegions[3], m_rayTracingConvergenceImages[cmdBufferIndex].GetPixelSize().x,
                                           m_rayTracingConvergenceImages[cmdBufferIndex].GetPixelSize().y, m_rayTracingConvergenceImages[cmdBufferIndex].GetPixelSize().z);

        // Prepare current swapchain image as transfer destination
        // window->GetFramebuffers()[cmdBufferIndex].GetTexture(0).TransitionLayout(vk::ImageLayout::eTransferDstOptimal,
        //                                                                          cmdBuffer);

        // srcAccess, dstAccess srcStage, dstStage;
        // vk::ImageSubresourceRange subresourceRange{GetValidAspects(), 0, m_mipLevels, 0, m_size.w};
        // vk::ImageMemoryBarrier transitionBarrier{vk::AccessFlags(), vk::AccessFlags(), m_desc.m_imageLayout, newLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, m_image, subresourceRange};

        // transitionBarrier.srcAccessMask = GetAccessFlagsForLayout(m_desc.m_imageLayout);
        // transitionBarrier.dstAccessMask = GetAccessFlagsForLayout(newLayout);

        // cmdBuffer.GetHandle().pipelineBarrier(GetStageFlagsForLayout(m_desc.m_imageLayout), GetStageFlagsForLayout(newLayout), vk::DependencyFlags(), nullptr, nullptr, transitionBarrier);
        // m_desc.m_imageLayout = newLayout;

        // m_rayTracingConvergenceImages[cmdBufferIndex].TransitionLayout(vk::ImageLayout::eTransferSrcOptimal, cmdBuffer);

        m_rayTracingConvergenceImages[cmdBufferIndex].CopyImageAsync(0, glm::u32vec4{0}, window->GetFramebuffers()[cmdBufferIndex].GetTexture(0), 0, glm::u32vec4{0}, m_rayTracingConvergenceImages[cmdBufferIndex].GetSize(), cmdBuffer);

        {
            vkfw_core::gfx::PipelineBarrier barrier{GetDevice(), vk::PipelineStageFlagBits::eColorAttachmentOutput};
            auto access = window->GetFramebuffers()[cmdBufferIndex].GetTexture(0).GetAccess();
            access.SetAccess(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::ImageLayout::eColorAttachmentOptimal, barrier);
            barrier.Record(cmdBuffer);
        }

        // here we somehow forget to set the correct image layout in dst image???

        // window->GetFramebuffers()[cmdBufferIndex].GetTexture(0).TransitionLayout(vk::ImageLayout::eColorAttachmentOptimal,
        //                                                                          cmdBuffer);
        // m_rayTracingConvergenceImages[cmdBufferIndex].TransitionLayout(vk::ImageLayout::eGeneral, cmdBuffer);
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
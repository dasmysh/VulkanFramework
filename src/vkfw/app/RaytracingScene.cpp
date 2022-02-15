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
#include "gfx/AOIntegrator.h"
#include "gfx/PathIntegrator.h"

#undef MemoryBarrier

namespace vkfw_app::scene::rt {

    RaytracingScene::RaytracingScene(vkfw_core::gfx::LogicalDevice* t_device,
                                     vkfw_core::gfx::UserControlledCamera* t_camera,
                                     std::size_t t_num_framebuffers)
        : Scene(t_device, t_camera, t_num_framebuffers)
        , m_memGroup{GetDevice(), "RTSceneMemoryGroup", vk::MemoryPropertyFlags()}
        , m_cameraUBO{vkfw_core::gfx::UniformBufferObject::Create<CameraPropertiesBuffer>(GetDevice(), GetNumberOfFramebuffers())}
        , m_asGeometry{GetDevice(), "RTSceneASGeometry", std::vector<std::uint32_t>{{0, 1}}}
        , m_sampler{GetDevice()->GetHandle(), "RTSceneSampler", vk::UniqueSampler{}}
        , m_rtResourcesDescriptorSetLayout{"RTSceneResourcesDescriptorSetLayout"}
        , m_convergenceImageDescriptorSetLayout{"RTSceneConvergenceDescriptorSet"}
        , m_rtPipelineLayout{GetDevice()->GetHandle(), "RTScenePipelineLayout", vk::UniquePipelineLayout{}}
        , m_rtResourcesDescriptorSet{GetDevice(), "RTSceneResourcesDescriptorSet", vk::DescriptorSet{}}
        , m_accumulatedResultSampler{GetDevice()->GetHandle(), "AccumulatedResultSampler", vk::UniqueSampler{}}
        , m_accumulatedResultImageDescriptorSetLayout{"AccumulatedResultDescriptorSet"}
        , m_compositingPipelineLayout{GetDevice()->GetHandle(), "RTCompositingPipelineLayout", vk::UniquePipelineLayout{}}
        , m_compositingFullscreenQuad{"shader/rt/ao/ao_composite.frag", 1}
    {
        vk::SamplerCreateInfo samplerCreateInfo{vk::SamplerCreateFlags(),       vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
                                                vk::SamplerAddressMode::eRepeat};
        m_sampler.SetHandle(GetDevice()->GetHandle(), GetDevice()->GetHandle().createSamplerUnique(samplerCreateInfo));

        m_triangleMaterial.m_materialName = "RT_DemoScene_TriangleMaterial";
        m_triangleMaterial.m_diffuse = glm::vec3{0.988f, 0.059f, 0.753};
        // m_triangleMaterial. m_diffuse = glm::vec3{0.988f, 0.059f, 0.753};
        InitializeScene();
        InitializeDescriptorSets();
    }

    RaytracingScene::~RaytracingScene() = default;

    void RaytracingScene::InitializeScene()
    {
        auto numUBOBuffers = GetNumberOfFramebuffers();

        m_cameraProperties.viewInverse = glm::inverse(GetCamera()->GetViewMatrix());
        m_cameraProperties.projInverse = glm::inverse(GetCamera()->GetProjMatrix());
        m_cameraProperties.frameId = 0;
        m_cameraProperties.cosineSampled = 0;
        m_cameraProperties.cameraMovedThisFrame = 1;
        m_cameraProperties.maxRange = 10.0f;
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

        vkfw_core::gfx::QueuedDeviceTransfer transfer{GetDevice(), GetDevice()->GetQueue(TRANSFER_QUEUE, 0)};
        m_memGroup.FinalizeDeviceGroup();
        m_memGroup.TransferData(transfer);
        transfer.FinishTransfer();

        {
            m_transferCmdPool = GetDevice()->CreateCommandPoolForQueue("RTSceneTransferCommandPool", TRANSFER_QUEUE);
            vk::CommandBufferAllocateInfo cmdBufferallocInfo{m_transferCmdPool.GetHandle(), vk::CommandBufferLevel::ePrimary,
                                                             static_cast<std::uint32_t>(numUBOBuffers)};
            m_transferCommandBuffers =
                vkfw_core::gfx::CommandBuffer::Initialize(GetDevice(), "RTSceneTransferCommandBuffer", m_transferCmdPool.GetQueueFamily(), GetDevice()->GetHandle().allocateCommandBuffersUnique(cmdBufferallocInfo));
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

        vkfw_core::gfx::rt::AccelerationStructureGeometry::AccelerationStructureBufferInfo bufferInfo;
        m_asGeometry.FinalizeGeometry<RayTracingVertex>(bufferInfo);
        m_asGeometry.FinalizeMaterial<vkfw_core::gfx::PhongMaterialInfo>(bufferInfo);
        m_asGeometry.FinalizeMaterial<vkfw_core::gfx::PhongBumpMaterialInfo>(bufferInfo);
        m_asGeometry.FinalizeBuffer(bufferInfo);

        // m_asGeometry.AddMeshGeometry(*m_meshInfo.get(), worldMatrixMesh);
        // m_asGeometry.FinalizeMeshGeometry<RayTracingVertex>();

        m_asGeometry.BuildAccelerationStructure();

        {
            vk::SamplerCreateInfo samplerCreateInfo{vk::SamplerCreateFlags(),       vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
                                                    vk::SamplerAddressMode::eRepeat};
            m_accumulatedResultSampler.SetHandle(GetDevice()->GetHandle(), GetDevice()->GetHandle().createSamplerUnique(samplerCreateInfo));
        }

        {
            // This barrier is needed to get all images into the same layout they will be at the beginning of each command buffer submit.
            // if we would fill the command buffer each frame (and therefore create barriers containing the actual image layouts) this would not be neccessary.
            auto cmdBuffer = vkfw_core::gfx::CommandBuffer::beginSingleTimeSubmit(GetDevice(), "TransferImageLayoutsInitialCommandBuffer", "TransferImageLayoutsInitial", GetDevice()->GetCommandPool(GRAPHICS_QUEUE));
            vkfw_core::gfx::PipelineBarrier barrier{GetDevice()};
            m_asGeometry.CreateResourceUseBarriers(vk::AccessFlagBits2KHR::eShaderRead, vk::PipelineStageFlagBits2KHR::eRayTracingShader, vk::ImageLayout::eShaderReadOnlyOptimal, barrier);
            barrier.Record(cmdBuffer);
            auto fence = vkfw_core::gfx::CommandBuffer::endSingleTimeSubmit(GetDevice()->GetQueue(GRAPHICS_QUEUE, 0), cmdBuffer, {}, {});
            if (auto r = GetDevice()->GetHandle().waitForFences({fence->GetHandle()}, VK_TRUE, vkfw_core::defaultFenceTimeout); r != vk::Result::eSuccess) {
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
                                                       static_cast<uint32_t>(ResBindings::Textures));
        UniformBufferObject::AddDescriptorLayoutBinding(m_rtResourcesDescriptorSetLayout, vk::ShaderStageFlagBits::eRaygenKHR, true, static_cast<uint32_t>(ResBindings::CameraProperties));

        Texture::AddDescriptorLayoutBinding(m_convergenceImageDescriptorSetLayout, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenKHR, static_cast<uint32_t>(ConvBindings::ResultImage));
        Texture::AddDescriptorLayoutBinding(m_accumulatedResultImageDescriptorSetLayout, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, static_cast<uint32_t>(CompositeConvSetBindings::AccumulatedImage));

        auto rtResourcesDescSetLayout = m_rtResourcesDescriptorSetLayout.CreateDescriptorLayout(GetDevice());
        auto convergenceDescSetLayout = m_convergenceImageDescriptorSetLayout.CreateDescriptorLayout(GetDevice());
        auto accumulatedResultDescSetLayout = m_accumulatedResultImageDescriptorSetLayout.CreateDescriptorLayout(GetDevice());

        std::vector<vk::DescriptorPoolSize> descSetPoolSizes;
        std::size_t descSetCount = 0;
        std::vector<vk::DescriptorSetLayout> descSetCreateLayouts;

        descSetCreateLayouts.emplace_back(rtResourcesDescSetLayout);
        m_rtResourcesDescriptorSetLayout.AddDescriptorPoolSizes(descSetPoolSizes, 1);
        descSetCount += 1;
        for (std::size_t i = 0; i < GetNumberOfFramebuffers(); ++i) {
            descSetCreateLayouts.emplace_back(convergenceDescSetLayout);
            descSetCreateLayouts.emplace_back(accumulatedResultDescSetLayout);
            m_convergenceImageDescriptorSetLayout.AddDescriptorPoolSizes(descSetPoolSizes, 1);
            m_accumulatedResultImageDescriptorSetLayout.AddDescriptorPoolSizes(descSetPoolSizes, 1);
            descSetCount += 2;
        }

        m_descriptorPool = vkfw_core::gfx::DescriptorSetLayout::CreateDescriptorPool(GetDevice(), "RTSceneDescriptorPool", descSetPoolSizes, descSetCount);

        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{m_descriptorPool.GetHandle(), descSetCreateLayouts};
        auto descSetAllocateResults = GetDevice()->GetHandle().allocateDescriptorSets(descriptorSetAllocateInfo);
        m_rtResourcesDescriptorSet.SetHandle(GetDevice()->GetHandle(), std::move(descSetAllocateResults[0]));
        m_convergenceImageDescriptorSets.reserve(GetNumberOfFramebuffers());
        for (std::size_t i = 0; i < GetNumberOfFramebuffers(); ++i) {
            m_convergenceImageDescriptorSets.emplace_back(GetDevice(), fmt::format("RTSceneConvergenceDescriptorSet-{}", i), std::move(descSetAllocateResults[1 + 2 * i]));
            m_accumulatedResultImageDescriptorSets.emplace_back(GetDevice(), fmt::format("AccumulatedResultDescriptorSet-{}", i), std::move(descSetAllocateResults[1 + 2 * i + 1]));
        }

        {
            std::array<vk::DescriptorSetLayout, 2> descSetLayouts;
            descSetLayouts[static_cast<std::size_t>(BindingSets::RTResourcesSet)] = rtResourcesDescSetLayout;
            descSetLayouts[static_cast<std::size_t>(BindingSets::ConvergenceSet)] = convergenceDescSetLayout;

            vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{vk::PipelineLayoutCreateFlags{}, descSetLayouts};
            m_rtPipelineLayout.SetHandle(GetDevice()->GetHandle(), GetDevice()->GetHandle().createPipelineLayoutUnique(pipelineLayoutCreateInfo));
        }

        {
            std::array<vk::DescriptorSetLayout, 1/* static_cast<std::size_t>(CompositeBindingSets::ConvergenceSet)*/> descSetLayouts;
            descSetLayouts[static_cast<std::size_t>(CompositeBindingSets::ConvergenceSet)] = accumulatedResultDescSetLayout;
            vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{vk::PipelineLayoutCreateFlags{}, descSetLayouts};
            m_compositingPipelineLayout.SetHandle(GetDevice()->GetHandle(), GetDevice()->GetHandle().createPipelineLayoutUnique(pipelineLayoutCreateInfo));
        }
    }

    void RaytracingScene::CreatePipeline(const glm::uvec2& screenSize, vkfw_core::VKWindow* window)
    {
        InitializeStorageImage(screenSize, window);
        FillDescriptorSets();

        m_integrators.clear();
        m_integrators.emplace_back(std::make_unique<gfx::rt::AOIntegrator>(GetDevice(), m_rtPipelineLayout, m_cameraUBO, m_rtResourcesDescriptorSet, m_convergenceImageDescriptorSets));

        m_compositingFullscreenQuad.CreatePipeline(GetDevice(), screenSize, window->GetRenderPass(), 0, m_compositingPipelineLayout);
    }

    void RaytracingScene::InitializeStorageImage(const glm::uvec2& screenSize, const vkfw_core::VKWindow* window)
    {
        vkfw_core::gfx::TextureDescriptor storageTexDesc{16, vk::Format::eR32G32B32A32Sfloat, vk::SampleCountFlagBits::e1};
        storageTexDesc.m_imageTiling = vk::ImageTiling::eOptimal;
        storageTexDesc.m_imageUsage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;
        storageTexDesc.m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;

        {
            auto cmdBuffer = vkfw_core::gfx::CommandBuffer::beginSingleTimeSubmit(GetDevice(), "TransferConvImageLayoutsInitialCommandBuffer", "TransferConvImageLayoutsInitial", GetDevice()->GetCommandPool(GRAPHICS_QUEUE));
            vkfw_core::gfx::PipelineBarrier barrier{GetDevice()};

            for (std::size_t i = 0; i < window->GetFramebuffers().size(); ++i) {
                auto& image = m_rayTracingConvergenceImages.emplace_back(GetDevice(), fmt::format("RTSceneConvergenceImage-{}", i), storageTexDesc, vk::ImageLayout::eUndefined);
                image.InitializeImage(glm::u32vec4{screenSize, 1, 1}, 1);
            }
            for (auto& image : m_rayTracingConvergenceImages) {
                image.AccessBarrier(vk::AccessFlagBits2KHR::eShaderRead, vk::PipelineStageFlagBits2KHR::eFragmentShader, vk::ImageLayout::eShaderReadOnlyOptimal, barrier);
            }

            barrier.Record(cmdBuffer);
            auto fence = vkfw_core::gfx::CommandBuffer::endSingleTimeSubmit(GetDevice()->GetQueue(GRAPHICS_QUEUE, 0), cmdBuffer, {}, {});
            if (auto r = GetDevice()->GetHandle().waitForFences({fence->GetHandle()}, VK_TRUE, vkfw_core::defaultFenceTimeout); r != vk::Result::eSuccess) {
                spdlog::error("Could not wait for fence while transitioning layout: {}.", r);
                throw std::runtime_error("Could not wait for fence while transitioning layout.");
            }
        }
    }

    void RaytracingScene::FillDescriptorSets()
    {
        using ResBindings = ResSetBindings;
        using ConvBindings = ConvSetBindings;

        std::array<const vkfw_core::gfx::rt::AccelerationStructureGeometry*, 1> accelerationStructure;
        std::array<vkfw_core::gfx::BufferRange, 1> cameraBufferRange;
        std::vector<vkfw_core::gfx::BufferRange> vboBufferRanges;
        std::vector<vkfw_core::gfx::BufferRange> iboBufferRanges;
        std::array<vkfw_core::gfx::BufferRange, 1> instanceBufferRange;
        std::array<vkfw_core::gfx::BufferRange, 2> materialBufferRange;
        std::vector<vkfw_core::gfx::Texture*> textures;
        // std::vector<vkfw_core::gfx::Texture*> bumpMaps;

        m_rtResourcesDescriptorSet.InitializeWrites(GetDevice(), m_rtResourcesDescriptorSetLayout);

        accelerationStructure[0] = &m_asGeometry;
        m_rtResourcesDescriptorSet.WriteAccelerationStructureDescriptor(static_cast<uint32_t>(ResBindings::AccelerationStructure), 0, accelerationStructure);

        m_cameraUBO.FillBufferRange(cameraBufferRange[0]);
        m_rtResourcesDescriptorSet.WriteBufferDescriptor(static_cast<uint32_t>(ResBindings::CameraProperties), 0, cameraBufferRange, vk::AccessFlagBits2KHR::eShaderRead);

        m_asGeometry.FillGeometryInfo(vboBufferRanges, iboBufferRanges, instanceBufferRange[0]);
        m_asGeometry.FillMaterialInfo<vkfw_core::gfx::PhongMaterialInfo>(materialBufferRange[0]);
        m_asGeometry.FillMaterialInfo<vkfw_core::gfx::PhongBumpMaterialInfo>(materialBufferRange[1]);
        m_asGeometry.FillTextureInfo(textures);
        m_rtResourcesDescriptorSet.WriteBufferDescriptor(static_cast<uint32_t>(ResBindings::Vertices), 0, vboBufferRanges, vk::AccessFlagBits2KHR::eShaderRead);
        m_rtResourcesDescriptorSet.WriteBufferDescriptor(static_cast<uint32_t>(ResBindings::Indices), 0, iboBufferRanges, vk::AccessFlagBits2KHR::eShaderRead);
        m_rtResourcesDescriptorSet.WriteBufferDescriptor(static_cast<uint32_t>(ResBindings::InstanceInfos), 0, instanceBufferRange, vk::AccessFlagBits2KHR::eShaderRead);
        m_rtResourcesDescriptorSet.WriteBufferDescriptor(static_cast<uint32_t>(ResBindings::MaterialInfos), 0, materialBufferRange, vk::AccessFlagBits2KHR::eShaderRead);
        m_rtResourcesDescriptorSet.WriteImageDescriptor(static_cast<uint32_t>(ResBindings::Textures), 0, textures, m_sampler, vk::AccessFlagBits2KHR::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal);
        // m_rtResourcesDescriptorSet.WriteImageDescriptor(static_cast<uint32_t>(ResBindings::BumpTextures), 0, bumpMaps, m_sampler, vk::AccessFlagBits2KHR::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal);

        m_rtResourcesDescriptorSet.FinalizeWrite(GetDevice());

        for (std::size_t i = 0; i < m_convergenceImageDescriptorSets.size(); ++i) {
            m_convergenceImageDescriptorSets[i].InitializeWrites(GetDevice(), m_convergenceImageDescriptorSetLayout);
            std::array<vkfw_core::gfx::Texture*, 1> convergenceImage = {&m_rayTracingConvergenceImages[i]};
            m_convergenceImageDescriptorSets[i].WriteImageDescriptor(static_cast<uint32_t>(ConvBindings::ResultImage), 0, convergenceImage, vkfw_core::gfx::Sampler{},
                                                                     vk::AccessFlagBits2KHR::eShaderRead | vk::AccessFlagBits2KHR::eShaderWrite,
                                                                     vk::ImageLayout::eGeneral);
            m_convergenceImageDescriptorSets[i].FinalizeWrite(GetDevice());

            m_accumulatedResultImageDescriptorSets[i].InitializeWrites(GetDevice(), m_accumulatedResultImageDescriptorSetLayout);
            std::array<vkfw_core::gfx::Texture*, 1> accumulatedResultImage = {&m_rayTracingConvergenceImages[i]};
            m_accumulatedResultImageDescriptorSets[i].WriteImageDescriptor(static_cast<uint32_t>(CompositeConvSetBindings::AccumulatedImage), 0, accumulatedResultImage, m_accumulatedResultSampler,
                                                                     vk::AccessFlagBits2KHR::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal);
            m_accumulatedResultImageDescriptorSets[i].FinalizeWrite(GetDevice());
        }
    }

    void RaytracingScene::RenderScene(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, vkfw_core::VKWindow* window)
    {
        m_integrators[m_integrator]->TraceRays(cmdBuffer, cmdBufferIndex, m_rayTracingConvergenceImages[cmdBufferIndex].GetPixelSize());

        m_accumulatedResultImageDescriptorSets[cmdBufferIndex].BindBarrier(cmdBuffer);
        window->BeginSwapchainRenderPass(cmdBufferIndex, {}, {});
        m_accumulatedResultImageDescriptorSets[cmdBufferIndex].Bind(cmdBuffer, vk::PipelineBindPoint::eGraphics, m_compositingPipelineLayout, 0);
        m_compositingFullscreenQuad.Render(cmdBuffer);
        window->EndSwapchainRenderPass(cmdBufferIndex);
    }

    void RaytracingScene::FrameMove(float, float, bool cameraChanged, const vkfw_core::VKWindow* window)
    {
        static bool firstFrame = true;
        m_cameraProperties.viewInverse = glm::inverse(GetCamera()->GetViewMatrix());
        m_cameraProperties.projInverse = glm::inverse(GetCamera()->GetProjMatrix());

        auto uboIndex = window->GetCurrentlyRenderedImageIndex();

        if (m_lastMoveFrame == uboIndex) {
            m_lastMoveFrame = static_cast<std::size_t>(-1);
            m_cameraProperties.cameraMovedThisFrame = 0;
        }

        if (window->GetCurrentlyRenderedImageIndex() == 0) { m_cameraProperties.frameId += 1; }

        if (cameraChanged || m_guiChanged) {
            m_cameraProperties.cameraMovedThisFrame = 1;
            m_guiChanged = false;
            m_lastMoveFrame = uboIndex;
        }

        m_cameraUBO.UpdateInstanceData(uboIndex, m_cameraProperties);

        const auto& transferQueue = GetDevice()->GetQueue(TRANSFER_QUEUE, 0);
        {
            QUEUE_REGION(transferQueue, "FrameMove");
            std::array<vk::SemaphoreSubmitInfoKHR, 1> signalSemaphore = {vk::SemaphoreSubmitInfoKHR{window->GetDataAvailableSemaphore().GetHandle(), 0, vk::PipelineStageFlagBits2KHR::eTopOfPipe}};
            // dont wait on first frame
            if (firstFrame) {
                m_transferCommandBuffers[uboIndex].SubmitToQueue(transferQueue, std::span<vk::SemaphoreSubmitInfoKHR>{}, signalSemaphore);
                firstFrame = false;
            } else {
                std::array<vk::SemaphoreSubmitInfoKHR, 1> waitSemaphore = {vk::SemaphoreSubmitInfoKHR{window->GetRenderingFinishedSemaphore().GetHandle(), 0, vk::PipelineStageFlagBits2KHR::eTransfer}};
                m_transferCommandBuffers[uboIndex].SubmitToQueue(transferQueue, waitSemaphore, signalSemaphore);
            }

        }
    }

    void RaytracingScene::RenderScene(const vkfw_core::VKWindow*) {}

    bool RaytracingScene::RenderGUI([[maybe_unused]] const vkfw_core::VKWindow* window)
    {
        ImGui::SetNextWindowPos(ImVec2(5, 100), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(220, 190), ImGuiCond_Always);
        if (ImGui::Begin("Scene Control")) {
            for (const auto& integrator : m_integrators) {
                if (ImGui::RadioButton(integrator->GetName().data(), &m_integrator, 0)) { m_guiChanged = true; }
            }

            bool cosSample = m_cameraProperties.cosineSampled == 1;
            if (ImGui::Checkbox("Samples Cosine Weigthed", &cosSample)) {
                // add camera changed here.
                m_guiChanged = true;
            }
            m_cameraProperties.cosineSampled = cosSample ? 1 : 0;
            if (ImGui::SliderFloat("Max. Range", &m_cameraProperties.maxRange, 0.1f, 10000.0f)) {
                // add camera changed here.
                m_guiChanged = true;
            }
        }
        ImGui::End();

        return false;
    }

}
;
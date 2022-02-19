/**
 * @file   SimpleScene.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.04.27
 *
 * @brief  Implementation of a simple scene.
 */

#include "app/SimpleScene.h"

#include <gfx/Texture2D.h>
#include <gfx/camera/UserControlledCamera.h>
#include <gfx/meshes/Mesh.h>
#include <gfx/meshes/AssImpScene.h>
#include <gfx/vk/QueuedDeviceTransfer.h>
#include <gfx/vk/pipeline/GraphicsPipeline.h>
#include <app/VKWindow.h>
#include <gfx/renderer/RenderList.h>

#include <glm/gtc/matrix_inverse.hpp>

namespace vkfw_app::scene::simple {

    SimpleScene::SimpleScene(vkfw_core::gfx::LogicalDevice* t_device, vkfw_core::gfx::UserControlledCamera* t_camera,
                             std::size_t t_num_framebuffers)
        : Scene(t_device, t_camera, t_num_framebuffers)
        , m_cameraMatrixDescriptorSetLayout{"SimpleSceneCameraDescriptorSetLayout"}
        , m_worldMatrixDescriptorSetLayout{"SimpleSceneWorldMatrixDescriptorSetLayout"}
        , m_imageSamplerDescriptorSetLayout{"SimpleSceneImageSamplerDescriptorSetLayout"}
        , m_pipelineLayout{GetDevice()->GetHandle(), "SimpleScenePipelineLayout", vk::UniquePipelineLayout{}}
        , m_cameraMatrixDescriptorSet{GetDevice(), "SimpleSceneCameraDescriptorSet", vk::DescriptorSet{}}
        , m_worldMatrixDescriptorSet{GetDevice(), "SimpleSceneWorldMatrixDescriptorSet", vk::DescriptorSet{}}
        , m_imageSamplerDescriptorSet{GetDevice(), "SimpleSceneImageSamplerDescriptorSet", vk::DescriptorSet{}}
        , m_vertices{{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                                         {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                                         {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                                         {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

                                         {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                                         {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                                         {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                                         {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}}
        , m_indices{0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4}
        , m_memGroup{GetDevice(), "SimpleSceneMemoryGroup", vk::MemoryPropertyFlags()}
        , m_vertexInputResources{GetDevice(), 0, {}, vkfw_core::gfx::BufferDescription{}, vk::IndexType::eUint32}
        , m_cameraUBO{vkfw_core::gfx::UniformBufferObject::Create<mesh_sample::CameraUniformBufferObject>(GetDevice(), GetNumberOfFramebuffers())}
        , m_worldUBO{vkfw_core::gfx::UniformBufferObject::Create<mesh::WorldUniformBufferObject>(GetDevice(), GetNumberOfFramebuffers())}
        , m_demoSampler{GetDevice()->GetHandle(), "SimpleSceneDemoSampler", vk::UniqueSampler{}}
    {
        InitializeScene();
        InitializeDescriptorSets();
    }

    SimpleScene::~SimpleScene() = default;

    void SimpleScene::CreatePipeline(const glm::uvec2& screenSize, vkfw_core::VKWindow* window)
    {
        // TODO: like this the shaders will be recompiled on each resize. [3/26/2017 Sebastian Maisch]
        // maybe set viewport as dynamic...
        m_demoPipeline = window->GetDevice().CreateGraphicsPipeline(
            std::vector<std::string>{"shader/mesh/mesh.vert", "shader/mesh/mesh.frag"}, screenSize, 1);
        m_demoPipeline->ResetVertexInput<mesh_sample::SimpleVertex>();
        m_demoPipeline->CreatePipeline(true, window->GetRenderPass(), 0, m_pipelineLayout);

        m_demoTransparentPipeline = window->GetDevice().CreateGraphicsPipeline(
            std::vector<std::string>{"shader/simple_transparent.vert", "shader/simple_transparent.frag"}, screenSize, 1);
        m_demoTransparentPipeline->ResetVertexInput<mesh_sample::SimpleVertex>();
        m_demoTransparentPipeline->GetRasterizer().cullMode = vk::CullModeFlagBits::eNone;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).blendEnable = VK_TRUE;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).colorBlendOp = vk::BlendOp::eAdd;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).alphaBlendOp = vk::BlendOp::eAdd;

        m_demoTransparentPipeline->CreatePipeline(true, window->GetRenderPass(), 0, m_pipelineLayout);
    }

    void SimpleScene::RenderScene(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, vkfw_core::VKWindow* window)
    {
        using UBOBinding = vkfw_core::gfx::RenderElement::UBOBinding;
        using DescSetBinding = vkfw_core::gfx::RenderElement::DescSetBinding;

        vkfw_core::gfx::RenderList renderList{GetCamera(), UBOBinding{&m_cameraMatrixDescriptorSet, 2, static_cast<std::uint32_t>(cmdBufferIndex * m_cameraUBO.GetInstanceSize())}};
        renderList.SetCurrentPipeline(m_pipelineLayout, *m_demoPipeline, *m_demoTransparentPipeline);

        auto planesWorldAABB = m_planesAABB.NewFromTransform(m_planesWorldMatrix);
        auto& re = renderList.AddTransparentElement(static_cast<std::uint32_t>(m_indices.size()), 1, 0, 0, 0, GetCamera()->GetViewMatrix(), planesWorldAABB);
        re.BindVertexInput(&m_vertexInputResources);
        re.BindWorldMatricesUBO(UBOBinding{&m_worldMatrixDescriptorSet, 0, static_cast<std::uint32_t>(cmdBufferIndex * m_worldUBO.GetInstanceSize())});
        re.BindDescriptorSet(DescSetBinding{&m_imageSamplerDescriptorSet, 1});

        m_mesh->GetDrawElements(m_meshWorldMatrix, *GetCamera(), cmdBufferIndex, renderList);

        std::vector<vkfw_core::gfx::DescriptorSet*> descriptorSets;
        std::vector<vkfw_core::gfx::VertexInputResources*> vertexInputs;
        renderList.AccessBarriers(descriptorSets, vertexInputs);

        window->BeginSwapchainRenderPass(cmdBufferIndex, descriptorSets, vertexInputs);

        renderList.Render(cmdBuffer);

        window->EndSwapchainRenderPass(cmdBufferIndex);
    }

    void SimpleScene::FrameMove(float time, float, bool, const vkfw_core::VKWindow* window)
    {
        mesh_sample::CameraUniformBufferObject camera_ubo;
        mesh::WorldUniformBufferObject world_ubo;
        world_ubo.model = glm::rotate(glm::mat4(1.0f), 0.3f * time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        world_ubo.normalMatrix = glm::mat4(glm::inverseTranspose(glm::mat3(world_ubo.model)));
        m_planesWorldMatrix = world_ubo.model;
        camera_ubo.view = GetCamera()->GetViewMatrix();
        camera_ubo.proj = GetCamera()->GetProjMatrix();

        auto uboIndex = window->GetCurrentlyRenderedImageIndex();
        m_cameraUBO.UpdateInstanceData(uboIndex, camera_ubo);
        m_worldUBO.UpdateInstanceData(uboIndex, world_ubo);

        m_meshWorldMatrix = glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f)),
                                       -0.2f * time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        m_mesh->UpdateWorldMatrices(uboIndex, m_meshWorldMatrix);

        const auto& transferQueue = GetDevice()->GetQueue(TRANSFER_QUEUE, 0);
        {
            QUEUE_REGION(transferQueue, "FrameMove");
            std::array<vk::SemaphoreSubmitInfoKHR, 1> transferSemaphore = {vk::SemaphoreSubmitInfoKHR{window->GetDataAvailableSemaphore().GetHandle(), 0, vk::PipelineStageFlagBits2KHR::eTopOfPipe}};
            m_transferCommandBuffers[uboIndex].SubmitToQueue(transferQueue, {}, transferSemaphore);
        }
    }

    void SimpleScene::RenderScene(const vkfw_core::VKWindow*) {}

    void SimpleScene::InitializeScene()
    {
        // as long as the last transfer of texture layouts is done on this queue, we have to use the graphics queue here.
        vkfw_core::gfx::QueuedDeviceTransfer transfer{GetDevice(), GetDevice()->GetQueue(TRANSFER_QUEUE, 0)};

        auto numUBOBuffers = GetNumberOfFramebuffers();

        mesh_sample::CameraUniformBufferObject initialCameraUBO{GetCamera()->GetViewMatrix(), GetCamera()->GetProjMatrix()};
        mesh::WorldUniformBufferObject initialWorldUBO;
        initialWorldUBO.model =
            glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        initialWorldUBO.normalMatrix = glm::mat4(glm::inverseTranspose(glm::mat3(initialWorldUBO.model)));
        std::size_t staticBufferSize = vkfw_core::byteSizeOf(m_vertices) + vkfw_core::byteSizeOf(m_indices);

        std::vector<glm::vec3> planesPoints;
        for (const auto& v : m_vertices) planesPoints.push_back(v.inPosition);
        m_planesAABB.FromPoints(planesPoints);

        {
            auto uboSize = m_cameraUBO.GetCompleteSize() + m_worldUBO.GetCompleteSize();
            auto indexBufferOffset = vkfw_core::byteSizeOf(m_vertices);
            auto uniformDataOffset =
                GetDevice()->CalculateUniformBufferAlignment(indexBufferOffset + vkfw_core::byteSizeOf(m_indices));
            auto completeBufferSize = uniformDataOffset + uboSize;

            m_completeBufferIdx = m_memGroup.AddBufferToGroup("SimpleSceneCompleteBuffer", vk::BufferUsageFlagBits::eVertexBuffer
                                                                  | vk::BufferUsageFlagBits::eIndexBuffer
                                                                  | vk::BufferUsageFlagBits::eUniformBuffer,
                                                              completeBufferSize, std::vector<std::uint32_t>{{0, 1}});

            m_memGroup.AddDataToBufferInGroup(m_completeBufferIdx, 0, m_vertices);
            m_memGroup.AddDataToBufferInGroup(m_completeBufferIdx, indexBufferOffset, m_indices);

            m_cameraUBO.AddUBOToBuffer(&m_memGroup, m_completeBufferIdx, uniformDataOffset, initialCameraUBO);
            m_worldUBO.AddUBOToBuffer(&m_memGroup, m_completeBufferIdx,
                                      uniformDataOffset + m_cameraUBO.GetCompleteSize(), initialWorldUBO);

            m_demoTexture = GetDevice()->GetTextureManager()->GetResource("demo.jpg", true, true, m_memGroup,
                                                                          std::vector<std::uint32_t>{{0, 1}});
            vk::SamplerCreateInfo samplerCreateInfo{vk::SamplerCreateFlags(),
                                                    vk::Filter::eLinear,
                                                    vk::Filter::eLinear,
                                                    vk::SamplerMipmapMode::eNearest,
                                                    vk::SamplerAddressMode::eRepeat,
                                                    vk::SamplerAddressMode::eRepeat,
                                                    vk::SamplerAddressMode::eRepeat};
            m_demoSampler.SetHandle(GetDevice()->GetHandle(), GetDevice()->GetHandle().createSamplerUnique(samplerCreateInfo));

            //////////////////////////////////////////////////////////////////////////
            // Multiple buffers section [3/18/2017 Sebastian Maisch]
            // vertexBufferIdx_ = memGroup_.AddBufferToGroup(vk::BufferUsageFlagBits::eVertexBuffer, vertices_, std::vector<std::uint32_t>{ {0, 1} });
            // indexBufferIdx_ = memGroup_.AddBufferToGroup(vk::BufferUsageFlagBits::eIndexBuffer, indices_, std::vector<std::uint32_t>{ {0, 1} });
            //////////////////////////////////////////////////////////////////////////
        }

        m_meshInfo = std::make_shared<vkfw_core::gfx::AssImpScene>("teapot/teapot.obj", GetDevice());
        m_mesh = std::make_unique<vkfw_core::gfx::Mesh>(
            vkfw_core::gfx::Mesh::CreateWithInternalMemoryGroup<mesh_sample::SimpleVertex, SimpleMaterial>("SimpleSceneMesh", m_meshInfo, numUBOBuffers, GetDevice(), vk::MemoryPropertyFlags(), std::vector<std::uint32_t>{{0, 1}}));
        m_mesh->UploadMeshData(transfer);

        m_memGroup.FinalizeDeviceGroup();
        m_memGroup.TransferData(transfer);
        transfer.FinishTransfer();

        std::array<vkfw_core::gfx::BufferDescription, 1> vertexBuffer;
        vertexBuffer[0].m_buffer = m_memGroup.GetBuffer(m_completeBufferIdx);
        vertexBuffer[0].m_offset = 0;
        m_vertexInputResources = vkfw_core::gfx::VertexInputResources{GetDevice(), 0, vertexBuffer, vkfw_core::gfx::BufferDescription{m_memGroup.GetBuffer(m_completeBufferIdx), vkfw_core::byteSizeOf(m_vertices)}, vk::IndexType::eUint32};

        {
            m_transferCmdPool = GetDevice()->CreateCommandPoolForQueue("SimpleSceneTransferCmdPool", TRANSFER_QUEUE);
            vk::CommandBufferAllocateInfo cmdBufferallocInfo{m_transferCmdPool.GetHandle(), vk::CommandBufferLevel::ePrimary, static_cast<std::uint32_t>(numUBOBuffers)};
            m_transferCommandBuffers =
                vkfw_core::gfx::CommandBuffer::Initialize(GetDevice(), "SimpleSceneTransferCommandBuffer", m_transferCmdPool.GetQueueFamily(), GetDevice()->GetHandle().allocateCommandBuffersUnique(cmdBufferallocInfo));
            for (auto i = 0U; i < numUBOBuffers; ++i) {
                vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eSimultaneousUse};
                m_transferCommandBuffers[i].Begin(beginInfo);

                m_cameraUBO.FillUploadCmdBuffer<mesh_sample::CameraUniformBufferObject>(m_transferCommandBuffers[i], i);
                m_worldUBO.FillUploadCmdBuffer<mesh::WorldUniformBufferObject>(m_transferCommandBuffers[i], i);

                m_mesh->TransferWorldMatrices(m_transferCommandBuffers[i], i);
                m_transferCommandBuffers[i].End();
            }
        }

        {
            auto cmdBuffer = vkfw_core::gfx::CommandBuffer::beginSingleTimeSubmit(GetDevice(), "TransferImageLayoutsInitialCommandBuffer", "TransferImageLayoutsInitial", GetDevice()->GetCommandPool(GRAPHICS_QUEUE));
            vkfw_core::gfx::PipelineBarrier barrier{GetDevice()};
            m_demoTexture->GetTexture().AccessBarrier(vk::AccessFlagBits2KHR::eShaderRead, vk::PipelineStageFlagBits2KHR::eFragmentShader, vk::ImageLayout::eShaderReadOnlyOptimal, barrier);
            m_memGroup.GetBuffer(m_completeBufferIdx)->AccessBarrierRange(false, 0, staticBufferSize, vk::AccessFlagBits2KHR::eShaderRead, vk::PipelineStageFlagBits2KHR::eFragmentShader, barrier);
            // m_memGroup.GetBuffer(m_completeBufferIdx)->AccessBarrier(vk::AccessFlagBits2KHR::eShaderRead, vk::PipelineStageFlagBits2KHR::eFragmentShader, barrier);
            m_mesh->CreateBufferUseBarriers(vk::AccessFlagBits2KHR::eShaderRead, vk::PipelineStageFlagBits2KHR::eFragmentShader, barrier);
            barrier.Record(cmdBuffer);
            auto fence = vkfw_core::gfx::CommandBuffer::endSingleTimeSubmit(GetDevice()->GetQueue(GRAPHICS_QUEUE, 0), cmdBuffer, {}, {});
            if (auto r = GetDevice()->GetHandle().waitForFences({fence->GetHandle()}, VK_TRUE, vkfw_core::defaultFenceTimeout); r != vk::Result::eSuccess) {
                spdlog::error("Could not wait for fence while transitioning layout: {}.", r);
                throw std::runtime_error("Could not wait for fence while transitioning layout.");
            }
        }
    }

    void SimpleScene::InitializeDescriptorSets()
    {
        using UniformBufferObject = vkfw_core::gfx::UniformBufferObject;
        using Texture = vkfw_core::gfx::Texture;
        UniformBufferObject::AddDescriptorLayoutBinding(m_cameraMatrixDescriptorSetLayout, vk::ShaderStageFlagBits::eVertex, true, static_cast<std::uint32_t>(mesh_sample::MeshBindings::CameraProperties));
        UniformBufferObject::AddDescriptorLayoutBinding(m_worldMatrixDescriptorSetLayout,
                                                        vk::ShaderStageFlagBits::eVertex, true, 0);
        Texture::AddDescriptorLayoutBinding(m_imageSamplerDescriptorSetLayout,
                                            vk::DescriptorType::eCombinedImageSampler,
                                            vk::ShaderStageFlagBits::eFragment, 0);


        {
            std::vector<vk::DescriptorPoolSize> descSetPoolSizes;
            std::size_t descSetCount = 0;
            m_mesh->AddDescriptorPoolSizes(descSetPoolSizes, descSetCount);
            m_cameraMatrixDescriptorSetLayout.AddDescriptorPoolSizes(descSetPoolSizes, 1);
            descSetCount += 1;
            m_worldMatrixDescriptorSetLayout.AddDescriptorPoolSizes(descSetPoolSizes, 1);
            descSetCount += 1;
            m_imageSamplerDescriptorSetLayout.AddDescriptorPoolSizes(descSetPoolSizes, 1);
            descSetCount += 1;

            m_descriptorPool = vkfw_core::gfx::DescriptorSetLayout::CreateDescriptorPool(GetDevice(), "SimpleSceneDescriptorPool", descSetPoolSizes, descSetCount);
        }

        m_mesh->CreateDescriptorSets(m_descriptorPool);

        auto cameraDescSetLayout = m_cameraMatrixDescriptorSetLayout.CreateDescriptorLayout(GetDevice());
        auto worldDescSetLayout = m_worldMatrixDescriptorSetLayout.CreateDescriptorLayout(GetDevice());
        auto materialDescSetLayout = m_mesh->GetMaterialDescriptorLayout().GetHandle();

        std::vector<vk::DescriptorSetLayout> descSetsLayouts = {cameraDescSetLayout, worldDescSetLayout,
                                                                materialDescSetLayout};
        vk::DescriptorSetAllocateInfo descSetsAllocInfo{
            m_descriptorPool.GetHandle(), static_cast<std::uint32_t>(descSetsLayouts.size()), descSetsLayouts.data()};
        auto descSets = GetDevice()->GetHandle().allocateDescriptorSets(descSetsAllocInfo);

        m_cameraMatrixDescriptorSet.SetHandle(GetDevice()->GetHandle(), descSets[0]);
        m_worldMatrixDescriptorSet.SetHandle(GetDevice()->GetHandle(), descSets[1]);
        m_imageSamplerDescriptorSet.SetHandle(GetDevice()->GetHandle(), descSets[2]);

        {
            std::array<vk::DescriptorSetLayout, 3> pipelineDescSets;
            pipelineDescSets[0] = worldDescSetLayout;
            pipelineDescSets[1] = materialDescSetLayout;
            pipelineDescSets[2] = cameraDescSetLayout;

            vk::PipelineLayoutCreateInfo pipelineLayoutInfo{vk::PipelineLayoutCreateFlags(),
                                                            static_cast<std::uint32_t>(pipelineDescSets.size()),
                                                            pipelineDescSets.data()};
            m_pipelineLayout.SetHandle(GetDevice()->GetHandle(), GetDevice()->GetHandle().createPipelineLayoutUnique(pipelineLayoutInfo));
        }

        {
            std::array<vkfw_core::gfx::BufferRange, 1> worldUBOBufferRange, cameraUBOBufferRange;
            m_worldUBO.FillBufferRange(worldUBOBufferRange[0]);
            m_cameraUBO.FillBufferRange(cameraUBOBufferRange[0]);

            m_worldMatrixDescriptorSet.InitializeWrites(GetDevice(), m_worldMatrixDescriptorSetLayout);
            m_worldMatrixDescriptorSet.WriteBufferDescriptor(0, 0, worldUBOBufferRange, vk::AccessFlagBits2KHR::eShaderRead);
            m_worldMatrixDescriptorSet.FinalizeWrite(GetDevice());

            std::array<vkfw_core::gfx::Texture*, 1> demoTextureArray = {&m_demoTexture->GetTexture()};
            m_imageSamplerDescriptorSet.InitializeWrites(GetDevice(), m_imageSamplerDescriptorSetLayout);
            m_imageSamplerDescriptorSet.WriteImageDescriptor(0, 0, demoTextureArray, m_demoSampler, vk::AccessFlagBits2KHR::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal);
            m_imageSamplerDescriptorSet.FinalizeWrite(GetDevice());

            m_cameraMatrixDescriptorSet.InitializeWrites(GetDevice(), m_worldMatrixDescriptorSetLayout);
            m_cameraMatrixDescriptorSet.WriteBufferDescriptor(static_cast<std::uint32_t>(mesh_sample::MeshBindings::CameraProperties), 0, cameraUBOBufferRange, vk::AccessFlagBits2KHR::eShaderRead);
            m_cameraMatrixDescriptorSet.FinalizeWrite(GetDevice());
        }
    }

}

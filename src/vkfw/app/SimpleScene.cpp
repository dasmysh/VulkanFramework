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
#include <gfx/vk/GraphicsPipeline.h>
#include <app/VKWindow.h>
#include <gfx/renderer/RenderList.h>

#include <glm/gtc/matrix_inverse.hpp>

namespace vkfw_app::scene::simple {

    SimpleScene::SimpleScene(vkfw_core::gfx::LogicalDevice* t_device, vkfw_core::gfx::UserControlledCamera* t_camera,
                             std::size_t t_num_framebuffers)
        :
        Scene(t_device, t_camera, t_num_framebuffers),
        m_vertices{{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                                         {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                                         {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                                         {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

                                         {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                                         {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                                         {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                                         {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}},
        m_indices{0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4},
        m_memGroup{GetDevice(), vk::MemoryPropertyFlags()},
        m_cameraUBO{
            vkfw_core::gfx::UniformBufferObject::Create<CameraMatrixUBO>(GetDevice(), GetNumberOfFramebuffers())},
        m_worldUBO{vkfw_core::gfx::UniformBufferObject::Create<vkfw_core::gfx::WorldMatrixUBO>(
            GetDevice(), GetNumberOfFramebuffers())}
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
            std::vector<std::string>{"simple.vert", "simple.frag"}, screenSize, 1);
        m_demoPipeline->ResetVertexInput<SimpleVertex>();
        m_demoPipeline->CreatePipeline(true, window->GetRenderPass(), 0, *m_vkPipelineLayout);

        m_demoTransparentPipeline = window->GetDevice().CreateGraphicsPipeline(
            std::vector<std::string>{"simple_transparent.vert", "simple_transparent.frag"}, screenSize, 1);
        m_demoTransparentPipeline->ResetVertexInput<SimpleVertex>();
        m_demoTransparentPipeline->GetRasterizer().cullMode = vk::CullModeFlagBits::eNone;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).blendEnable = VK_TRUE;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).colorBlendOp = vk::BlendOp::eAdd;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        m_demoTransparentPipeline->GetColorBlendAttachment(0).alphaBlendOp = vk::BlendOp::eAdd;

        m_demoTransparentPipeline->CreatePipeline(true, window->GetRenderPass(), 0, *m_vkPipelineLayout);
    }

    void SimpleScene::UpdateCommandBuffer(const vk::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex,
                                          vkfw_core::VKWindow*)
    {
        using BufferReference = vkfw_core::gfx::RenderElement::BufferReference;
        using UBOBinding = vkfw_core::gfx::RenderElement::UBOBinding;
        using DescSetBinding = vkfw_core::gfx::RenderElement::DescSetBinding;
        vk::DeviceSize offset = 0;

        vkfw_core::gfx::RenderList renderList{GetCamera(), UBOBinding{&m_cameraUBO, 3, cmdBufferIndex}};
        renderList.SetCurrentPipeline(*m_vkPipelineLayout, m_demoPipeline->GetPipeline(),
                                      m_demoTransparentPipeline->GetPipeline());

        auto planesWorldAABB = m_planesAABB.NewFromTransform(m_planesWorldMatrix);
        auto& re = renderList.AddTransparentElement(static_cast<std::uint32_t>(m_indices.size()), 1, 0, 0, 0,
                                                    GetCamera()->GetViewMatrix(), planesWorldAABB);
        re.BindVertexBuffer(BufferReference{m_memGroup.GetBuffer(m_completeBufferIdx), offset});
        re.BindIndexBuffer(BufferReference{m_memGroup.GetBuffer(m_completeBufferIdx), vkfw_core::byteSizeOf(m_vertices)});
        re.BindWorldMatricesUBO(UBOBinding{&m_worldUBO, 0, cmdBufferIndex});
        re.BindDescriptorSet(DescSetBinding{m_vkImageSamplerDescritorSet, 2});

        m_mesh->GetDrawElements(m_meshWorldMatrix, *GetCamera(), cmdBufferIndex, renderList);

        renderList.Render(cmdBuffer);
    }

    void SimpleScene::FrameMove(float time, float, const vkfw_core::VKWindow* window)
    {
        CameraMatrixUBO camera_ubo;
        vkfw_core::gfx::WorldMatrixUBO world_ubo;
        world_ubo.m_model = glm::rotate(glm::mat4(1.0f), 0.3f * time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        world_ubo.m_normalMatrix = glm::mat4(glm::inverseTranspose(glm::mat3(world_ubo.m_model)));
        m_planesWorldMatrix = world_ubo.m_model;
        camera_ubo.m_view = GetCamera()->GetViewMatrix();
        camera_ubo.m_proj = GetCamera()->GetProjMatrix();

        auto uboIndex = window->GetCurrentlyRenderedImageIndex();
        m_cameraUBO.UpdateInstanceData(uboIndex, camera_ubo);
        m_worldUBO.UpdateInstanceData(uboIndex, world_ubo);

        m_meshWorldMatrix = glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f)),
                                       -0.2f * time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        m_mesh->UpdateWorldMatrices(uboIndex, m_meshWorldMatrix);
        vk::Semaphore vkTransferSemaphore = window->GetDataAvailableSemaphore();
        vk::SubmitInfo submitInfo{
            0, nullptr, nullptr, 1, &(*m_vkTransferCommandBuffers[uboIndex]), 1, &vkTransferSemaphore};
        GetDevice()->GetQueue(1, 0).submit(submitInfo, vk::Fence());
    }

    void SimpleScene::RenderScene(const vkfw_core::VKWindow*) {}

    void SimpleScene::InitializeScene()
    {
        // as long as the last transfer of texture layouts is done on this queue, we have to use the graphics queue here.
        vkfw_core::gfx::QueuedDeviceTransfer transfer{GetDevice(), std::make_pair(0, 0)};

        auto numUBOBuffers = GetNumberOfFramebuffers();

        CameraMatrixUBO initialCameraUBO{GetCamera()->GetViewMatrix(), GetCamera()->GetProjMatrix()};
        vkfw_core::gfx::WorldMatrixUBO initialWorldUBO;
        initialWorldUBO.m_model =
            glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        initialWorldUBO.m_normalMatrix = glm::mat4(glm::inverseTranspose(glm::mat3(initialWorldUBO.m_model)));

        std::vector<glm::vec3> planesPoints;
        for (const auto& v : m_vertices) planesPoints.push_back(v.m_position);
        m_planesAABB.FromPoints(planesPoints);

        {
            auto uboSize = m_cameraUBO.GetCompleteSize() + m_worldUBO.GetCompleteSize();
            auto indexBufferOffset = vkfw_core::byteSizeOf(m_vertices);
            auto uniformDataOffset =
                GetDevice()->CalculateUniformBufferAlignment(indexBufferOffset + vkfw_core::byteSizeOf(m_indices));
            auto completeBufferSize = uniformDataOffset + uboSize;

            m_completeBufferIdx = m_memGroup.AddBufferToGroup(vk::BufferUsageFlagBits::eVertexBuffer
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
            m_vkDemoSampler = GetDevice()->GetDevice().createSamplerUnique(samplerCreateInfo);

            //////////////////////////////////////////////////////////////////////////
            // Multiple buffers section [3/18/2017 Sebastian Maisch]
            // vertexBufferIdx_ = memGroup_.AddBufferToGroup(vk::BufferUsageFlagBits::eVertexBuffer, vertices_, std::vector<std::uint32_t>{ {0, 1} });
            // indexBufferIdx_ = memGroup_.AddBufferToGroup(vk::BufferUsageFlagBits::eIndexBuffer, indices_, std::vector<std::uint32_t>{ {0, 1} });
            //////////////////////////////////////////////////////////////////////////
        }

        m_meshInfo = std::make_shared<vkfw_core::gfx::AssImpScene>("teapot/teapot.obj", GetDevice());
        m_mesh = std::make_unique<vkfw_core::gfx::Mesh>(
            vkfw_core::gfx::Mesh::CreateWithInternalMemoryGroup<SimpleVertex, SimpleMaterial>(
                m_meshInfo, numUBOBuffers, GetDevice(), vk::MemoryPropertyFlags(), std::vector<std::uint32_t>{{0, 1}}));
        m_mesh->UploadMeshData(transfer);

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
                m_worldUBO.FillUploadCmdBuffer<vkfw_core::gfx::WorldMatrixUBO>(*m_vkTransferCommandBuffers[i], i);

                m_mesh->TransferWorldMatrices(*m_vkTransferCommandBuffers[i], i);
                m_vkTransferCommandBuffers[i]->end();
            }
        }
    }

    void SimpleScene::InitializeDescriptorSets()
    {
        auto numUBOBuffers = GetNumberOfFramebuffers();
        m_mesh->CreateDescriptorSets(numUBOBuffers);

        {
            std::array<vk::DescriptorPoolSize, 2> descSetPoolSizes;
            descSetPoolSizes[0] = vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1};
            descSetPoolSizes[1] = vk::DescriptorPoolSize{vk::DescriptorType::eUniformBufferDynamic, 2};
            vk::DescriptorPoolCreateInfo descSetPoolInfo{vk::DescriptorPoolCreateFlags(), 3,
                                                         static_cast<std::uint32_t>(descSetPoolSizes.size()),
                                                         descSetPoolSizes.data()};
            m_vkUBODescriptorPool = GetDevice()->GetDevice().createDescriptorPoolUnique(descSetPoolInfo);
        }

        m_cameraUBO.CreateLayout(*m_vkUBODescriptorPool, vk::ShaderStageFlagBits::eVertex, true, 0);
        m_worldUBO.UseLayout(*m_vkUBODescriptorPool, m_mesh->GetWorldMatricesDescriptorLayout(), true, 0);
        m_vkImageSampleDescriptorSetLayout = m_mesh->GetMaterialTexturesDescriptorLayout();

        {
            std::array<vk::DescriptorSetLayout, 4> pipelineDescSets;
            pipelineDescSets[0] = m_worldUBO.GetDescriptorLayout();
            pipelineDescSets[1] = m_mesh->GetMaterialBufferDescriptorLayout();
            pipelineDescSets[2] = m_vkImageSampleDescriptorSetLayout;
            pipelineDescSets[3] = m_cameraUBO.GetDescriptorLayout();

            vk::PipelineLayoutCreateInfo pipelineLayoutInfo{vk::PipelineLayoutCreateFlags(),
                                                            static_cast<std::uint32_t>(pipelineDescSets.size()),
                                                            pipelineDescSets.data()};
            m_vkPipelineLayout = GetDevice()->GetDevice().createPipelineLayoutUnique(pipelineLayoutInfo);
        }

        {
            vk::DescriptorSetAllocateInfo descSetAllocInfo{*m_vkUBODescriptorPool, 1,
                                                           &m_vkImageSampleDescriptorSetLayout};
            m_vkImageSamplerDescritorSet = GetDevice()->GetDevice().allocateDescriptorSets(descSetAllocInfo)[0];
        }

        {
            std::vector<vk::WriteDescriptorSet> descSetWrites;
            descSetWrites.reserve(3);
            vk::DescriptorImageInfo descImageInfo{*m_vkDemoSampler, m_demoTexture->GetTexture().GetImageView(),
                                                  vk::ImageLayout::eShaderReadOnlyOptimal};
            descSetWrites.emplace_back(m_vkImageSamplerDescritorSet, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                       &descImageInfo);

            m_worldUBO.FillDescriptorSetWrite(descSetWrites.emplace_back());
            m_cameraUBO.FillDescriptorSetWrite(descSetWrites.emplace_back());

            GetDevice()->GetDevice().updateDescriptorSets(descSetWrites, nullptr);
        }
    }

}

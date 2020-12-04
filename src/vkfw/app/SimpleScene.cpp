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
            std::vector<std::string>{"shader/mesh/mesh.vert", "shader/mesh/mesh.frag"}, screenSize, 1);
        m_demoPipeline->ResetVertexInput<SimpleVertex>();
        m_demoPipeline->CreatePipeline(true, window->GetRenderPass(), 0, *m_vkPipelineLayout);

        m_demoTransparentPipeline = window->GetDevice().CreateGraphicsPipeline(
            std::vector<std::string>{"shader/simple_transparent.vert", "shader/simple_transparent.frag"}, screenSize, 1);
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

        vkfw_core::gfx::RenderList renderList{
            GetCamera(), UBOBinding{m_vkCameraMatrixDescriptorSet, 2,
                                    static_cast<std::uint32_t>(cmdBufferIndex * m_cameraUBO.GetInstanceSize())}};
        renderList.SetCurrentPipeline(*m_vkPipelineLayout, m_demoPipeline->GetPipeline(),
                                      m_demoTransparentPipeline->GetPipeline());

        auto planesWorldAABB = m_planesAABB.NewFromTransform(m_planesWorldMatrix);
        auto& re = renderList.AddTransparentElement(static_cast<std::uint32_t>(m_indices.size()), 1, 0, 0, 0,
                                                    GetCamera()->GetViewMatrix(), planesWorldAABB);
        re.BindVertexBuffer(BufferReference{m_memGroup.GetBuffer(m_completeBufferIdx), offset});
        re.BindIndexBuffer(BufferReference{m_memGroup.GetBuffer(m_completeBufferIdx), vkfw_core::byteSizeOf(m_vertices)});
        re.BindWorldMatricesUBO(UBOBinding{m_vkWorldMatrixDescriptorSet, 0,
                                           static_cast<std::uint32_t>(cmdBufferIndex * m_worldUBO.GetInstanceSize())});
        re.BindDescriptorSet(DescSetBinding{m_vkImageSamplerDescriptorSet, 1});

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
        using UniformBufferObject = vkfw_core::gfx::UniformBufferObject;
        using Texture = vkfw_core::gfx::Texture;
        UniformBufferObject::AddDescriptorLayoutBinding(m_cameraMatrixDescriptorSetLayout,
                                                        vk::ShaderStageFlagBits::eVertex, true, 0);
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

            m_vkDescriptorPool =
                vkfw_core::gfx::DescriptorSetLayout::CreateDescriptorPool(GetDevice(), descSetPoolSizes, descSetCount);
        }

        m_mesh->CreateDescriptorSets(m_vkDescriptorPool.get());

        auto cameraDescSetLayout = m_cameraMatrixDescriptorSetLayout.CreateDescriptorLayout(GetDevice());
        auto worldDescSetLayout = m_worldMatrixDescriptorSetLayout.CreateDescriptorLayout(GetDevice());
        auto materialDescSetLayout = m_mesh->GetMaterialDescriptorLayout().GetDescriptorLayout();

        std::vector<vk::DescriptorSetLayout> descSetsLayouts = {cameraDescSetLayout, worldDescSetLayout,
                                                                materialDescSetLayout};
        vk::DescriptorSetAllocateInfo descSetsAllocInfo{
            m_vkDescriptorPool.get(), static_cast<std::uint32_t>(descSetsLayouts.size()), descSetsLayouts.data()};
        auto descSets = GetDevice()->GetDevice().allocateDescriptorSets(descSetsAllocInfo);

        m_vkCameraMatrixDescriptorSet = descSets[0];
        m_vkWorldMatrixDescriptorSet = descSets[1];
        m_vkImageSamplerDescriptorSet = descSets[2];

        {
            std::array<vk::DescriptorSetLayout, 3> pipelineDescSets;
            pipelineDescSets[0] = worldDescSetLayout;
            pipelineDescSets[1] = materialDescSetLayout;
            pipelineDescSets[2] = cameraDescSetLayout;

            vk::PipelineLayoutCreateInfo pipelineLayoutInfo{vk::PipelineLayoutCreateFlags(),
                                                            static_cast<std::uint32_t>(pipelineDescSets.size()),
                                                            pipelineDescSets.data()};
            m_vkPipelineLayout = GetDevice()->GetDevice().createPipelineLayoutUnique(pipelineLayoutInfo);
        }

        {
            std::vector<vk::WriteDescriptorSet> descSetWrites;
            descSetWrites.resize(3);
            std::array<vk::DescriptorBufferInfo, 2> descBufferInfos;
            vk::DescriptorImageInfo descImageInfo;

            m_worldUBO.FillDescriptorBufferInfo(descBufferInfos[0]);
            descSetWrites[0] = m_worldMatrixDescriptorSetLayout.MakeWrite(m_vkWorldMatrixDescriptorSet, 0, &descBufferInfos[0]);

            m_demoTexture->GetTexture().FillDescriptorImageInfo(descImageInfo, m_vkDemoSampler.get());
            descSetWrites[1] = m_imageSamplerDescriptorSetLayout.MakeWrite(m_vkImageSamplerDescriptorSet, 0, &descImageInfo);

            m_cameraUBO.FillDescriptorBufferInfo(descBufferInfos[1]);
            descSetWrites[2] = m_worldMatrixDescriptorSetLayout.MakeWrite(m_vkCameraMatrixDescriptorSet, 0, &descBufferInfos[1]);

            GetDevice()->GetDevice().updateDescriptorSets(descSetWrites, nullptr);
        }
    }

}

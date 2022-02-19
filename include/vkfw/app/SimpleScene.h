/**
 * @file   SimpleScene.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.04.27
 *
 * @brief  Scene object for a simple scene with a teapot mesh and two transparent planes.
 */

#pragma once

#include "app/Scene.h"
#include "mesh/mesh_sample_host_interface.h"
#include "gfx/VertexFormats.h"

#include <gfx/vk/UniformBufferObject.h>
#include <gfx/vk/memory/MemoryGroup.h>
#include <gfx/vk/pipeline/DescriptorSetLayout.h>
#include <gfx/vk/wrappers/CommandPool.h>
#include <gfx/vk/wrappers/Sampler.h>
#include <gfx/vk/wrappers/DescriptorSet.h>
#include <gfx/vk/wrappers/DescriptorPool.h>
#include <gfx/vk/wrappers/PipelineLayout.h>
#include <gfx/vk/wrappers/VertexInputResources.h>
#include <core/math/primitives.h>

#include <glm/mat4x4.hpp>

#include <memory>
#include <vector>

namespace vkfw_core::gfx {
    class DescriptorSetLayout;
    class GraphicsPipeline;
    class AssImpScene;
    class Mesh;
}

namespace vkfw_app::scene::simple {

    class SimpleScene : public Scene
    {
    public:
        SimpleScene(vkfw_core::gfx::LogicalDevice* t_device, vkfw_core::gfx::UserControlledCamera* t_camera,
                    std::size_t num_framebuffers);
        ~SimpleScene();

        void CreatePipeline(const glm::uvec2& screenSize, vkfw_core::VKWindow* window) override;
        void RenderScene(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, vkfw_core::VKWindow* window) override;
        void FrameMove(float time, float elapsed, bool cameraChanged, const vkfw_core::VKWindow* window) override;
        void RenderScene(const vkfw_core::VKWindow* window) override;

 private:
        void InitializeScene();
        void InitializeDescriptorSets();

        /** Holds the descriptor set layouts for the demo pipeline. */
        vkfw_core::gfx::DescriptorSetLayout m_cameraMatrixDescriptorSetLayout;
        vkfw_core::gfx::DescriptorSetLayout m_worldMatrixDescriptorSetLayout;
        vkfw_core::gfx::DescriptorSetLayout m_imageSamplerDescriptorSetLayout;
        /** Holds the pipeline layout for demo rendering. */
        vkfw_core::gfx::PipelineLayout m_pipelineLayout;
        /** Holds the descriptor pool for the UBO binding. */
        vkfw_core::gfx::DescriptorPool m_descriptorPool;
        /** Holds the descriptor sets. */
        vkfw_core::gfx::DescriptorSet m_cameraMatrixDescriptorSet;
        vkfw_core::gfx::DescriptorSet m_worldMatrixDescriptorSet;
        vkfw_core::gfx::DescriptorSet m_imageSamplerDescriptorSet;
        /** Holds the graphics pipeline for demo rendering. */
        std::unique_ptr<vkfw_core::gfx::GraphicsPipeline> m_demoPipeline;
        /** Holds the graphics pipeline for transparent demo rendering. */
        std::unique_ptr<vkfw_core::gfx::GraphicsPipeline> m_demoTransparentPipeline;
        /** Holds vertex information. */
        std::vector<mesh_sample::SimpleVertex> m_vertices;
        /** Holds index information. */
        std::vector<std::uint32_t> m_indices;
        /** Holds the vertex and index buffer. */
        vkfw_core::gfx::MemoryGroup m_memGroup;
        /** Holds the memory group index of the complete buffer. */
        unsigned int m_completeBufferIdx = vkfw_core::gfx::MemoryGroup::INVALID_INDEX;
        /** Hold the vertex input information. */
        vkfw_core::gfx::VertexInputResources m_vertexInputResources;

        /** The world matrix of the two rotating planes. */
        glm::mat4 m_planesWorldMatrix;
        vkfw_core::math::AABB3<float> m_planesAABB;

        /** The uniform buffer object for the camera matrices. */
        vkfw_core::gfx::UniformBufferObject m_cameraUBO;
        /** The uniform buffer object for the world matrices. */
        vkfw_core::gfx::UniformBufferObject m_worldUBO;

        /** The command pool for the transfer cmd buffers. */
        vkfw_core::gfx::CommandPool m_transferCmdPool; // TODO: at some point we may need one per buffer? [10/19/2018 Sebastian Maisch]
        /** Holds the command buffers for transferring the uniform buffers. */
        std::vector<vkfw_core::gfx::CommandBuffer> m_transferCommandBuffers;

        /** Holds the texture used. */
        std::shared_ptr<vkfw_core::gfx::Texture2D> m_demoTexture;
        /** Holds the texture sampler. */
        vkfw_core::gfx::Sampler m_demoSampler;

        /** Holds the AssImp demo model. */
        std::shared_ptr<vkfw_core::gfx::AssImpScene> m_meshInfo;
        /** Holds the mesh to be rendered. */
        std::unique_ptr<vkfw_core::gfx::Mesh> m_mesh;
        /** The world matrix of the mesh. */
        glm::mat4 m_meshWorldMatrix;
    };
}

/**
 * @file   RaytracingScene.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.04.27
 *
 * @brief  A scene using raytracing for rendering.
 */

#pragma once

#include "app/Scene.h"

#include <gfx/vk/UniformBufferObject.h>
#include <gfx/vk/rt/AccelerationStructureGeometry.h>
#include <gfx/vk/pipeline/DescriptorSetLayout.h>

#include <glm/mat4x4.hpp>

namespace vkfw_core::gfx {
    class Shader;
    class DeviceTexture;
    class DeviceBuffer;
    class AssImpScene;
    class SceneMeshNode;
    class SubMesh;
}

namespace vkfw_app::scene::rt {

    struct CameraMatrixUBO
    {
        glm::mat4 m_viewInverse;
        glm::mat4 m_projInverse;
    };

    class RaytracingScene : public Scene
    {
    public:
        RaytracingScene(vkfw_core::gfx::LogicalDevice* t_device, vkfw_core::gfx::UserControlledCamera* t_camera,
                    std::size_t num_framebuffers);

        void CreatePipeline(const glm::uvec2& screenSize, vkfw_core::VKWindow* window) override;
        void UpdateCommandBuffer(const vk::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex,
                                 vkfw_core::VKWindow* window) override;
        void FrameMove(float time, float elapsed, const vkfw_core::VKWindow* window) override;
        void RenderScene(const vkfw_core::VKWindow* window) override;

    private:
        constexpr static std::uint32_t indexRaygen = 0;
        constexpr static std::uint32_t indexMiss = 1;
        constexpr static std::uint32_t indexClosestHit = 2;
        constexpr static std::uint32_t shaderGroupCount = 3;

        void InitializeScene();
        void InitializeDescriptorSets();

        void InitializeStorageImage(const glm::uvec2& screenSize, const vkfw_core::VKWindow* window);
        void FillDescriptorSets();

        void InitializeShaderBindingTable();

        /** Holds the memory for the world and camera UBOs. */
        vkfw_core::gfx::MemoryGroup m_memGroup;
        /** The uniform buffer object for the camera matrices. */
        vkfw_core::gfx::UniformBufferObject m_cameraUBO;
        /** The acceleration structure. */
        vkfw_core::gfx::rt::AccelerationStructureGeometry m_asGeometry;

        /** The command pool for the transfer cmd buffers. */
        vk::UniqueCommandPool m_transferCmdPool;
        /** Holds the command buffers for transferring the uniform buffers. */
        std::vector<vk::UniqueCommandBuffer> m_vkTransferCommandBuffers;

        std::unique_ptr<vkfw_core::gfx::HostBuffer> m_shaderBindingTable;

        /** The texture to store raytracing results. */
        std::unique_ptr<vkfw_core::gfx::DeviceTexture> m_storageImage;

        /** Holds the descriptor set layouts for the raytracing pipeline. */
        vkfw_core::gfx::DescriptorSetLayout m_descriptorSetLayout;
        /** Holds the pipeline layout for raytracing. */
        vk::UniquePipelineLayout m_vkPipelineLayout;
        /** The descriptor pool. */
        vk::UniqueDescriptorPool m_vkDescriptorPool;
        /** The descriptor set. */
        vk::DescriptorSet m_vkDescriptorSet;

        /** Holds the shaders used for raytracing. */
        std::vector<std::shared_ptr<vkfw_core::gfx::Shader>> m_shaders;
        /** Holds the shader groups used for raytracing. */
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_shaderGroups;
        /** The raytracing pipeline. */
        vk::UniquePipeline m_vkPipeline;

        /** Holds the AssImp demo model. */
        std::shared_ptr<vkfw_core::gfx::AssImpScene> m_meshInfo;
    };
}

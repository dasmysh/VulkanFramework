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

#include <glm/mat4x4.hpp>

namespace vkfw_core::gfx {
    class Shader;
    // class DeviceMemory;
    class DeviceTexture;
    class DeviceBuffer;
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
        void InitializeRayTracingPipeline();

        struct AccelerationStructure
        {
            vk::UniqueAccelerationStructureKHR as;
            vk::UniqueDeviceMemory memory;
            vk::DeviceAddress handle = 0;
        };

        constexpr static std::uint32_t indexRaygen = 0;
        constexpr static std::uint32_t indexMiss = 1;
        constexpr static std::uint32_t indexClosestHit = 2;
        constexpr static std::uint32_t numShaderGroups = 3;

        void InitializeScene();
        void CreateBottomLevelAccelerationStructure();
        void CreateTopLevelAccelerationStructure();
        void InitializeStorageImage(const glm::uvec2& screenSize, const vkfw_core::VKWindow* window);

        void InitializeShaderBindingTable();
        void InitializeDescriptorSets();
        AccelerationStructure CreateAS(const vk::AccelerationStructureCreateInfoKHR& info);
        std::unique_ptr<vkfw_core::gfx::DeviceBuffer> CreateASScratchBuffer(AccelerationStructure& as);

        /** The raytracing properties for the selected device. */
        vk::PhysicalDeviceRayTracingPropertiesKHR m_raytracingProperties;
        /** The raytracing features of the selected device. */
        vk::PhysicalDeviceRayTracingFeaturesKHR m_raytracingFeatures;
        /** Holds the memory for the world and camera UBOs. */
        vkfw_core::gfx::MemoryGroup m_memGroup;
        /** The uniform buffer object for the camera matrices. */
        vkfw_core::gfx::UniformBufferObject m_cameraUBO;
        /** The uniform buffer object for the world matrices. */
        vkfw_core::gfx::UniformBufferObject m_worldUBO;
        /** The bottom level acceleration structure for the scene. */
        AccelerationStructure m_BLAS;
        /** The top level acceleration structure for the scene. */
        AccelerationStructure m_TLAS;

        std::unique_ptr<vkfw_core::gfx::HostBuffer> m_shaderBindingTable;

        /** The texture to store raytracing results. */
        std::unique_ptr<vkfw_core::gfx::DeviceTexture> m_storageImage;
        /** The image to store raytracing results. */
        vk::UniqueImage m_vkStorageImage;
        /** Memory for the storage image. */
        // std::unique_ptr<vkfw_core::gfx::DeviceMemory> m_storageImageMemory;
        /** The image view to store raytracing results. */
        vk::UniqueImageView m_vkStorageImageView;

        /** Holds the descriptor set layouts for the raytracing pipeline. */
        vk::UniqueDescriptorSetLayout m_vkDescriptorSetLayout;
        /** Holds the pipeline layout for raytracing. */
        vk::UniquePipelineLayout m_vkPipelineLayout;
        /** The descriptor pool. */
        vk::UniqueDescriptorPool m_vkDescriptorPool;
        /** The descriptor set. */
        vk::UniqueDescriptorSet m_vkDescriptorSet;

        /** Holds the shaders used for raytracing. */
        std::vector<std::shared_ptr<vkfw_core::gfx::Shader>> m_shaders;
        /** Holds the shader groups used for raytracing. */
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_shaderGroups;
        /** The raytracing pipeline. */
        vk::UniquePipeline m_vkPipeline;
    };
}

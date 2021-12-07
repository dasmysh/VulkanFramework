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
#include <gfx/vk/wrappers/PipelineLayout.h>
#include <gfx/renderer/FullscreenQuad.h>
#include "rt/rt_sample_host_interface.h"
#include "rt/ao/ao_composite_shader_interface.h"

#include <glm/mat4x4.hpp>

namespace vkfw_core::gfx {
    class Shader;
    class DeviceTexture;
    class DeviceBuffer;
    class AssImpScene;
    class SceneMeshNode;
    class SubMesh;
}

namespace vkfw_app::gfx::rt {
    class RTIntegrator;
}

namespace vkfw_app::scene::rt {

    class RaytracingScene : public Scene
    {
    public:
        RaytracingScene(vkfw_core::gfx::LogicalDevice* t_device, vkfw_core::gfx::UserControlledCamera* t_camera,
                    std::size_t num_framebuffers);
        ~RaytracingScene();

        void CreatePipeline(const glm::uvec2& screenSize, vkfw_core::VKWindow* window) override;
        void RenderScene(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, vkfw_core::VKWindow* window) override;
        void FrameMove(float time, float elapsed, bool cameraChanged, const vkfw_core::VKWindow* window) override;
        void RenderScene(const vkfw_core::VKWindow* window) override;
        bool RenderGUI(const vkfw_core::VKWindow* window) override;

    private:
        constexpr static std::uint32_t indexRaygen = 0;
        constexpr static std::uint32_t indexMiss = 1;
        constexpr static std::uint32_t indexClosestHit = 2;
        constexpr static std::uint32_t shaderGroupCount = 3;

        void InitializeScene();
        void InitializeDescriptorSets();

        void InitializeStorageImage(const glm::uvec2& screenSize, const vkfw_core::VKWindow* window);
        void FillDescriptorSets();

        /** Holds the memory for the world and camera UBOs. */
        vkfw_core::gfx::MemoryGroup m_memGroup;
        /** The uniform buffer object for the camera matrices. */
        vkfw_core::gfx::UniformBufferObject m_cameraUBO;
        /** The acceleration structure. */
        vkfw_core::gfx::rt::AccelerationStructureGeometry m_asGeometry;

        /** The command pool for the transfer cmd buffers. */
        vkfw_core::gfx::CommandPool m_transferCmdPool;
        /** Holds the command buffers for transferring the uniform buffers. */
        std::vector<vkfw_core::gfx::CommandBuffer> m_transferCommandBuffers;

        /** The texture to store raytracing results. */
        std::vector<vkfw_core::gfx::DeviceTexture> m_rayTracingConvergenceImages;
        /** The sampler for material textures */
        vkfw_core::gfx::Sampler m_sampler;

        /** Holds the descriptor set layouts for the raytracing pipeline and geometry / material resources. */
        vkfw_core::gfx::DescriptorSetLayout m_rtResourcesDescriptorSetLayout;
        /** Holds the descriptor set layouts for the convergence image. */
        vkfw_core::gfx::DescriptorSetLayout m_convergenceImageDescriptorSetLayout;
        /** Holds the pipeline layout for raytracing. */
        vkfw_core::gfx::PipelineLayout m_rtPipelineLayout;
        /** The descriptor pool. */
        vkfw_core::gfx::DescriptorPool m_descriptorPool;
        /** The descriptor set for the ray tracing resources. */
        vkfw_core::gfx::DescriptorSet m_rtResourcesDescriptorSet;
        /** The descriptor set for the convergence image. */
        std::vector<vkfw_core::gfx::DescriptorSet> m_convergenceImageDescriptorSets;

        std::vector<std::unique_ptr<gfx::rt::RTIntegrator>> m_integrators;
        int m_integrator = 0;

        /** Holds the texture sampler for the accumulated result. */
        vkfw_core::gfx::Sampler m_accumulatedResultSampler;
        /** Holds the descriptor set layout for the accumulated result image. */
        vkfw_core::gfx::DescriptorSetLayout m_accumulatedResultImageDescriptorSetLayout;
        /** The descriptor set for the accumulated image. */
        std::vector<vkfw_core::gfx::DescriptorSet> m_accumulatedResultImageDescriptorSets;
        /** Holds the pipeline layout for compositing. */
        vkfw_core::gfx::PipelineLayout m_compositingPipelineLayout;
        /** The fullscreen quad for compositing. */
        vkfw_core::gfx::FullscreenQuad m_compositingFullscreenQuad;

        vkfw_core::gfx::MaterialInfo m_triangleMaterial;
        /** Holds the AssImp demo models. */
        std::shared_ptr<vkfw_core::gfx::AssImpScene> m_teapotMeshInfo;
        std::shared_ptr<vkfw_core::gfx::AssImpScene> m_sponzaMeshInfo;

        CameraPropertiesBuffer m_cameraProperties;
        std::size_t m_lastMoveFrame = static_cast<std::size_t>(-1);
        bool m_guiChanged = true;
    };
}

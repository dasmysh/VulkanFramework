/**v
 * @file   RTIntegrator.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.21.05
 *
 * @brief  Base class for all integrators.
 */

#pragma once

#include <gfx/vk/pipeline/RayTracingPipeline.h>
#include <gfx/vk/UniformBufferObject.h>
#include <glm/fwd.hpp>
#include "rt/rt_sample_host_interface.h"

namespace vkfw_core::gfx {
    class LogicalDevice;
    class MemoryGroup;
    class PipelineLayout;
    class CommandBuffer;
    class DescriptorSet;
    class UserControlledCamera;
}

namespace vkfw_app::gfx::rt {

    class RTIntegrator
    {
    public:
        RTIntegrator(std::string_view integratorName, std::string_view pipelineName, vkfw_core::gfx::LogicalDevice* device, vkfw_core::gfx::UserControlledCamera* camera, std::uint32_t maxRecursionDepth, std::size_t framebufferCount);
        virtual ~RTIntegrator();

        [[nodiscard]] std::string_view GetName() const { return m_integratorName; }
        [[nodiscard]] const std::vector<std::uint32_t>& GetMaterialSBTMapping() const { return m_materialSBTMapping; }

        [[nodiscard]] virtual std::size_t CalculateUniformBuffersSize(std::size_t offset);
        virtual std::size_t AddUBOsToMemoryGroup(vkfw_core::gfx::MemoryGroup& memGroup, unsigned int completeBufferIndex, std::size_t offset);
        virtual void AddDescriptorLayoutBinding(vkfw_core::gfx::DescriptorSetLayout& rtResourcesDescriptorSetLayout);

        virtual void FillUBOUploadCmdBuffer(vkfw_core::gfx::CommandBuffer& transferCommandBuffer, std::size_t imageIndex) const;

        virtual void FillDescriptorSets(vkfw_core::gfx::DescriptorSet& rtResourcesDescriptorSet, std::vector<vkfw_core::gfx::DescriptorSet>& convergenceImageDescriptorSets);

        void InitializePipeline(const vkfw_core::gfx::PipelineLayout& pipelineLayout);

        void FrameMove(std::uint32_t uboIndex, bool sceneChanged);
        virtual bool RenderGUI() = 0;

        virtual void TraceRays(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, const glm::u32vec4& rtGroups) = 0;

    protected:
        [[nodiscard]] vkfw_core::gfx::LogicalDevice* GetDevice() const { return m_device; }

        [[nodiscard]] const vkfw_core::gfx::PipelineLayout& GetPipelineLayout() const { return *m_rtPipelineLayout; }
        [[nodiscard]] vkfw_core::gfx::RayTracingPipeline& GetPipeline() { return m_rtPipeline; }
        [[nodiscard]] vkfw_core::gfx::DescriptorSet& GetResourcesDescriptorSet() { return *m_rtResourcesDescriptorSet; }
        [[nodiscard]] vkfw_core::gfx::DescriptorSet& GetImageDescriptorSet(std::size_t imageIndex) { return (*m_convergenceImageDescriptorSets)[imageIndex]; }
        [[nodiscard]] const vkfw_core::gfx::UniformBufferObject& GetCameraUBO() const { return m_cameraUBO; }
        [[nodiscard]] const vkfw_core::gfx::UniformBufferObject& GetFrameUBO() const { return m_frameUBO; }

        [[nodiscard]] std::vector<std::uint32_t>& materialSBTMapping() { return m_materialSBTMapping; }
        [[nodiscard]] virtual std::vector<vkfw_core::gfx::RayTracingPipeline::RTShaderInfo> GetShaders() const = 0;

    private:
        /** Name of the integrator. */
        std::string_view m_integratorName;
        /** Maximum recursion depth of the shaders (i.e. number of trace rays calls from hit shaders) */
        std::uint32_t m_maxRecursionDepth;

        /** The logical device. */
        vkfw_core::gfx::LogicalDevice* m_device;
        /** The camera to render the scene into. */
        vkfw_core::gfx::UserControlledCamera* m_camera;

        /** The raytracing pipeline. */
        vkfw_core::gfx::RayTracingPipeline m_rtPipeline;

        /** The uniform buffer object for the camera matrices. */
        vkfw_core::gfx::UniformBufferObject m_cameraUBO;
        /** The uniform buffer object for the frame parameters. */
        vkfw_core::gfx::UniformBufferObject m_frameUBO;

        /** Camera parameters for shader upload. */
        scene::rt::CameraParameters m_cameraProperties;
        /** Frame parameters for shader upload. */
        scene::rt::FrameParameters m_frameProperties;
        /** Indicator in which image of the last frame a move occured (if any). */
        std::size_t m_lastMoveFrame = static_cast<std::size_t>(-1);

        /** The descriptor set for the ray tracing resources. */
        vkfw_core::gfx::DescriptorSet* m_rtResourcesDescriptorSet = nullptr;
        /** The descriptor set for the convergence image. */
        std::vector<vkfw_core::gfx::DescriptorSet>* m_convergenceImageDescriptorSets = nullptr;
        /** Holds the pipeline layout for raytracing. */
        const vkfw_core::gfx::PipelineLayout* m_rtPipelineLayout = nullptr;
        /** The mapping between materials and shader binding table entries. */
        std::vector<std::uint32_t> m_materialSBTMapping;
    };
}

/**
 * @file   RTIntegrator.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.21.05
 *
 * @brief  Base class for all integrators.
 */

#pragma once

#include <gfx/vk/pipeline/RayTracingPipeline.h>
#include <glm/fwd.hpp>

namespace vkfw_core::gfx {
    class LogicalDevice;
    class PipelineLayout;
    class CommandBuffer;
    class DescriptorSet;
    class UniformBufferObject;
}

namespace vkfw_app::gfx::rt {

    class RTIntegrator
    {
    public:
        RTIntegrator(std::string_view integratorName, std::string_view pipelineName, vkfw_core::gfx::LogicalDevice* device, const vkfw_core::gfx::PipelineLayout& rtPipelineLayout, const vkfw_core::gfx::UniformBufferObject& cameraUBO,
                     vkfw_core::gfx::DescriptorSet& rtResourcesDescriptorSet, std::vector<vkfw_core::gfx::DescriptorSet>& convergenceImageDescriptorSets);
        virtual ~RTIntegrator();

        std::string_view GetName() const { return m_integratorName; }
        virtual void TraceRays(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, const glm::u32vec4& rtGroups) = 0;

    protected:
        const vkfw_core::gfx::PipelineLayout& GetPipelineLayout() const { return m_rtPipelineLayout; }
        vkfw_core::gfx::RayTracingPipeline& GetPipeline() { return m_rtPipeline; }
        vkfw_core::gfx::DescriptorSet& GetResourcesDescriptorSet() { return m_rtResourcesDescriptorSet; }
        vkfw_core::gfx::DescriptorSet& GetImageDescriptorSet(std::size_t imageIndex) { return m_convergenceImageDescriptorSets[imageIndex]; }
        const vkfw_core::gfx::UniformBufferObject& GetCameraUBO() const { return m_cameraUBO; }

    private:
        std::string_view m_integratorName;
        /** The uniform buffer object for the camera matrices. */
        const vkfw_core::gfx::UniformBufferObject& m_cameraUBO;
        /** The descriptor set for the ray tracing resources. */
        vkfw_core::gfx::DescriptorSet& m_rtResourcesDescriptorSet;
        /** The descriptor set for the convergence image. */
        std::vector<vkfw_core::gfx::DescriptorSet>& m_convergenceImageDescriptorSets;
        /** Holds the pipeline layout for raytracing. */
        const vkfw_core::gfx::PipelineLayout& m_rtPipelineLayout;
        /** The raytracing pipeline. */
        vkfw_core::gfx::RayTracingPipeline m_rtPipeline;
    };
}

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
        RTIntegrator(std::string_view integratorName, std::string_view pipelineName, vkfw_core::gfx::LogicalDevice* device, std::uint32_t maxRecursionDepth);
        virtual ~RTIntegrator();

        std::string_view GetName() const { return m_integratorName; }
        const std::vector<std::uint32_t>& GetMaterialSBTMapping() const { return m_materialSBTMapping; }

        void InitializePipeline(const vkfw_core::gfx::PipelineLayout& pipelineLayout);
        void InitializeMisc(const vkfw_core::gfx::UniformBufferObject& cameraUBO, vkfw_core::gfx::DescriptorSet& rtResourcesDescriptorSet, std::vector<vkfw_core::gfx::DescriptorSet>& convergenceImageDescriptorSets);

        virtual void TraceRays(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, const glm::u32vec4& rtGroups) = 0;

    protected:
        vkfw_core::gfx::LogicalDevice* GetDevice() const { return m_device; }

        const vkfw_core::gfx::PipelineLayout& GetPipelineLayout() const { return *m_rtPipelineLayout; }
        vkfw_core::gfx::RayTracingPipeline& GetPipeline() { return m_rtPipeline; }
        vkfw_core::gfx::DescriptorSet& GetResourcesDescriptorSet() { return *m_rtResourcesDescriptorSet; }
        vkfw_core::gfx::DescriptorSet& GetImageDescriptorSet(std::size_t imageIndex) { return (*m_convergenceImageDescriptorSets)[imageIndex]; }
        const vkfw_core::gfx::UniformBufferObject& GetCameraUBO() const { return *m_cameraUBO; }

        std::vector<std::uint32_t>& materialSBTMapping() { return m_materialSBTMapping; }
        virtual std::vector<vkfw_core::gfx::RayTracingPipeline::RTShaderInfo> GetShaders() const = 0;

    private:
        std::string_view m_integratorName;
        std::uint32_t m_maxRecursionDepth;
        vkfw_core::gfx::LogicalDevice* m_device;
        /** The raytracing pipeline. */
        vkfw_core::gfx::RayTracingPipeline m_rtPipeline;

        /** The uniform buffer object for the camera matrices. */
        const vkfw_core::gfx::UniformBufferObject* m_cameraUBO = nullptr;
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

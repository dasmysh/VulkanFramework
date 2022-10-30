/**
 * @file   PathIntegrator.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.21.05
 *
 * @brief  Class for the ambient path tracing integrator with shadow rays in closest hit shaders.
 */

#pragma once

#include "gfx/RTIntegrator.h"
#include "rt/path/path_host_interface.h"

namespace vkfw_app::gfx::rt {

    class PathRecursiveIntegrator : public RTIntegrator
    {
    public:
        PathRecursiveIntegrator(vkfw_core::gfx::LogicalDevice* device, vkfw_core::gfx::UserControlledCamera* camera, std::size_t framebufferCount);
        ~PathRecursiveIntegrator() override;

        [[nodiscard]] std::size_t CalculateUniformBuffersSize(std::size_t offset) override;
        std::size_t AddUBOsToMemoryGroup(vkfw_core::gfx::MemoryGroup& memGroup, unsigned int completeBufferIndex, std::size_t offset) override;
        void AddDescriptorLayoutBinding(vkfw_core::gfx::DescriptorSetLayout& rtResourcesDescriptorSetLayout) override;

        void FillUBOUploadCmdBuffer(vkfw_core::gfx::CommandBuffer& transferCommandBuffer, std::size_t imageIndex) const override;

        void FillDescriptorSets(vkfw_core::gfx::DescriptorSet& rtResourcesDescriptorSet, std::vector<vkfw_core::gfx::DescriptorSet>& convergenceImageDescriptorSets) override;

        bool RenderGUI() override;

        void TraceRays(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, const glm::u32vec4& rtGroups) override;

    private:
        std::vector<vkfw_core::gfx::RayTracingPipeline::RTShaderInfo> GetShaders() const override;

        /** The uniform buffer object for the algorithm parameters. */
        vkfw_core::gfx::UniformBufferObject m_algorithmUBO;
        /** Algorithm parameters for shader upload. */
        scene::rt::path::AlgorithmParameters m_algorithmProperties;
    };
}
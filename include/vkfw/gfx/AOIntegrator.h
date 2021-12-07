/**
 * @file   AOIntegrator.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.21.05
 *
 * @brief  Class for the ambient occlusion integrator.
 */

#pragma once

#include "gfx/RTIntegrator.h"

namespace vkfw_app::gfx::rt {

    class AOIntegrator : public RTIntegrator
    {
    public:
        AOIntegrator(vkfw_core::gfx::LogicalDevice* device, const vkfw_core::gfx::PipelineLayout& rtPipelineLayout, const vkfw_core::gfx::UniformBufferObject& cameraUBO,
                     vkfw_core::gfx::DescriptorSet& rtResourcesDescriptorSet, std::vector<vkfw_core::gfx::DescriptorSet>& convergenceImageDescriptorSets);
        ~AOIntegrator() override;

        void TraceRays(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, const glm::u32vec4& rtGroups) override;
    };
}

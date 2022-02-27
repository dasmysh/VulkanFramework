/**
 * @file   PathIntegrator.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.21.05
 *
 * @brief  Class for the ambient path tracing integrator.
 */

#pragma once

#include "gfx/RTIntegrator.h"

namespace vkfw_app::gfx::rt {

    class PathIntegrator : public RTIntegrator
    {
    public:
        PathIntegrator(vkfw_core::gfx::LogicalDevice* device, vkfw_core::gfx::UserControlledCamera* camera, std::size_t framebufferCount);
        ~PathIntegrator() override;

        void TraceRays(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, const glm::u32vec4& rtGroups) override;

    private:
        std::vector<vkfw_core::gfx::RayTracingPipeline::RTShaderInfo> GetShaders() const override;
    };
}

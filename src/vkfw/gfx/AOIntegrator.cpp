/**
 * @file   AOIntegrator.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.12.05
 *
 * @brief  Implementation the ambient occlusion integrator.
 */

#include "gfx/AOIntegrator.h"
#include "main.h"
#include <gfx/vk/LogicalDevice.h>
#include <core/resources/ShaderManager.h>
#include <gfx/vk/wrappers/CommandBuffer.h>
#include <gfx/vk/wrappers/DescriptorSet.h>
#include <gfx/vk/UniformBufferObject.h>

namespace vkfw_app::gfx::rt {

    AOIntegrator::AOIntegrator(vkfw_core::gfx::LogicalDevice* device, const vkfw_core::gfx::PipelineLayout& rtPipelineLayout, const vkfw_core::gfx::UniformBufferObject& cameraUBO,
                               vkfw_core::gfx::DescriptorSet& rtResourcesDescriptorSet, std::vector<vkfw_core::gfx::DescriptorSet>& convergenceImageDescriptorSets)
        : RTIntegrator{"Ambient Occlusion Integrator", "AOPipeline", device, rtPipelineLayout, cameraUBO, rtResourcesDescriptorSet, convergenceImageDescriptorSets}
    {
        std::vector<std::shared_ptr<vkfw_core::gfx::Shader>> shaders{device->GetShaderManager()->GetResource("shader/rt/ao/ao.rgen"), device->GetShaderManager()->GetResource("shader/rt/ao/miss.rmiss"),
                                                                     device->GetShaderManager()->GetResource("shader/rt/ao/closesthit.rchit"), device->GetShaderManager()->GetResource("shader/rt/skipAlpha.rahit")};
        GetPipeline().ResetShaders(std::move(shaders));
        GetPipeline().CreatePipeline(1, GetPipelineLayout());
    }

    AOIntegrator::~AOIntegrator() = default;

    void AOIntegrator::TraceRays(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, const glm::u32vec4& rtGroups)
    {
        auto& sbtDeviceAddressRegions = GetPipeline().GetSBTDeviceAddresses();

        GetPipeline().BindPipeline(cmdBuffer);
        GetResourcesDescriptorSet().Bind(cmdBuffer, vk::PipelineBindPoint::eRayTracingKHR, GetPipelineLayout(), 0, static_cast<std::uint32_t>(cmdBufferIndex * GetCameraUBO().GetInstanceSize()));
        GetImageDescriptorSet(cmdBufferIndex).Bind(cmdBuffer, vk::PipelineBindPoint::eRayTracingKHR, GetPipelineLayout(), 1);

        cmdBuffer.GetHandle().traceRaysKHR(sbtDeviceAddressRegions[0], sbtDeviceAddressRegions[1], sbtDeviceAddressRegions[2], sbtDeviceAddressRegions[3], rtGroups.x, rtGroups.y, rtGroups.z);
    }
}

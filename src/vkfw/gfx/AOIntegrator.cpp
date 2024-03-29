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
#include "materials/material_sample_host_interface.h"

namespace vkfw_app::gfx::rt {

    AOIntegrator::AOIntegrator(vkfw_core::gfx::LogicalDevice* device) : RTIntegrator{"Ambient Occlusion Integrator", "AOPipeline", device, 1}
    {
        materialSBTMapping().resize(static_cast<std::size_t>(materials::MaterialIdentifierApp::TotalMaterialCount), 0);
        materialSBTMapping()[static_cast<std::size_t>(materials::MaterialIdentifierApp::MirrorMaterialType)] = 1;
    }

    AOIntegrator::~AOIntegrator() = default;

    std::vector<vkfw_core::gfx::RayTracingPipeline::RTShaderInfo> AOIntegrator::GetShaders() const
    {
        std::vector<vkfw_core::gfx::RayTracingPipeline::RTShaderInfo> shaders;
        shaders.emplace_back(GetDevice()->GetShaderManager()->GetResource("shader/rt/ao/ao.rgen"), 0);
        shaders.emplace_back(GetDevice()->GetShaderManager()->GetResource("shader/rt/ao/miss.rmiss"), 0);

        shaders.emplace_back(GetDevice()->GetShaderManager()->GetResource("shader/rt/ao/closesthit.rchit"), 0);
        shaders.emplace_back(GetDevice()->GetShaderManager()->GetResource("shader/rt/skipAlpha.rahit"), 0);

        shaders.emplace_back(GetDevice()->GetShaderManager()->GetResource("shader/rt/ao/closesthit_mirror.rchit"), 1);
        return shaders;
    }

    void AOIntegrator::TraceRays(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, const glm::u32vec4& rtGroups)
    {
        auto& sbtDeviceAddressRegions = GetPipeline().GetSBTDeviceAddresses();

        GetPipeline().BindPipeline(cmdBuffer);
        GetResourcesDescriptorSet().Bind(cmdBuffer, vk::PipelineBindPoint::eRayTracingKHR, GetPipelineLayout(), 0, static_cast<std::uint32_t>(cmdBufferIndex * GetCameraUBO().GetInstanceSize()));
        GetImageDescriptorSet(cmdBufferIndex).Bind(cmdBuffer, vk::PipelineBindPoint::eRayTracingKHR, GetPipelineLayout(), 1);

        cmdBuffer.GetHandle().traceRaysKHR(sbtDeviceAddressRegions[0], sbtDeviceAddressRegions[1], sbtDeviceAddressRegions[2], sbtDeviceAddressRegions[3], rtGroups.x, rtGroups.y, rtGroups.z);
    }
}

/**
 * @file   PathIntegrator.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.12.05
 *
 * @brief  Implementation the path tracing integrator.
 */

#include "gfx/PathIntegrator.h"
#include "main.h"
#include <core/resources/ShaderManager.h>
#include <gfx/vk/LogicalDevice.h>
#include <gfx/vk/UniformBufferObject.h>
#include <gfx/vk/wrappers/CommandBuffer.h>
#include <gfx/vk/wrappers/DescriptorSet.h>
#include "materials/material_sample_host_interface.h"

namespace vkfw_app::gfx::rt {

    PathIntegrator::PathIntegrator(vkfw_core::gfx::LogicalDevice* device, const vkfw_core::gfx::PipelineLayout& rtPipelineLayout, const vkfw_core::gfx::UniformBufferObject& cameraUBO,
                               vkfw_core::gfx::DescriptorSet& rtResourcesDescriptorSet, std::vector<vkfw_core::gfx::DescriptorSet>& convergenceImageDescriptorSets)
        : RTIntegrator{"Path Tracing Integrator", "PathTracingPipeline", device, rtPipelineLayout, cameraUBO, rtResourcesDescriptorSet, convergenceImageDescriptorSets}
    {
        // TODO: use correct shaders.

        std::vector<vkfw_core::gfx::RayTracingPipeline::RTShaderInfo> shaders;
        shaders.emplace_back(device->GetShaderManager()->GetResource("shader/rt/path/pathtrace.rgen"), 0);
        shaders.emplace_back(device->GetShaderManager()->GetResource("shader/rt/path/miss.miss"), 0);
        // TODO: use other shaders based on material.
        shaders.emplace_back(device->GetShaderManager()->GetResource("shader/rt/path/closesthit.rchit"), 0);
        shaders.emplace_back(device->GetShaderManager()->GetResource("shader/rt/skipAlpha.rahit"), 0);
        shaders.emplace_back(device->GetShaderManager()->GetResource("shader/rt/path/closesthit_mirror.rchit"), 1);

        materialSBTMapping().resize(static_cast<std::size_t>(materials::MaterialIdentifierApp::TotalMaterialCount), 0);
        materialSBTMapping()[static_cast<std::size_t>(materials::MaterialIdentifierApp::MirrorMaterialType)] = 1;

        GetPipeline().ResetShaders(std::move(shaders));
        GetPipeline().CreatePipeline(1, GetPipelineLayout());
    }

    PathIntegrator::~PathIntegrator() = default;

    void PathIntegrator::TraceRays(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, const glm::u32vec4& rtGroups)
    {
        auto& sbtDeviceAddressRegions = GetPipeline().GetSBTDeviceAddresses();

        GetPipeline().BindPipeline(cmdBuffer);
        GetResourcesDescriptorSet().Bind(cmdBuffer, vk::PipelineBindPoint::eRayTracingKHR, GetPipelineLayout(), 0, static_cast<std::uint32_t>(cmdBufferIndex * GetCameraUBO().GetInstanceSize()));
        GetImageDescriptorSet(cmdBufferIndex).Bind(cmdBuffer, vk::PipelineBindPoint::eRayTracingKHR, GetPipelineLayout(), 1);

        cmdBuffer.GetHandle().traceRaysKHR(sbtDeviceAddressRegions[0], sbtDeviceAddressRegions[1], sbtDeviceAddressRegions[2], sbtDeviceAddressRegions[3], rtGroups.x, rtGroups.y, rtGroups.z);
    }
}

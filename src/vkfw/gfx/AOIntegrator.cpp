/**
 * @file   AOIntegrator.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.12.05
 *
 * @brief  Implementation the ambient occlusion integrator.
 */

#include "gfx/AOIntegrator.h"
#include <main.h>
#include <gfx/vk/LogicalDevice.h>
#include <core/resources/ShaderManager.h>
#include <gfx/vk/wrappers/CommandBuffer.h>
#include <gfx/vk/wrappers/DescriptorSet.h>
#include <gfx/vk/UniformBufferObject.h>
#include "materials/material_sample_host_interface.h"
#include "imgui.h"

namespace vkfw_app::gfx::rt {

    AOIntegrator::AOIntegrator(vkfw_core::gfx::LogicalDevice* device, vkfw_core::gfx::UserControlledCamera* camera, std::size_t framebufferCount)
        : RTIntegrator{"Ambient Occlusion Integrator", "AOPipeline", device, camera, 1, framebufferCount}, m_algorithmUBO{vkfw_core::gfx::UniformBufferObject::Create<scene::rt::ao::AlgorithmParameters>(GetDevice(), framebufferCount)}
    {
        m_algorithmProperties.cosineSampled = 0;
        m_algorithmProperties.maxRange = 10.0f;
        materialSBTMapping().resize(static_cast<std::size_t>(materials::MaterialIdentifierApp::TotalMaterialCount), 0);
        materialSBTMapping()[static_cast<std::size_t>(materials::MaterialIdentifierApp::MirrorMaterialType)] = 1;
    }

    AOIntegrator::~AOIntegrator() = default;

    std::size_t AOIntegrator::CalculateUniformBuffersSize(std::size_t offset)
    {
        auto parentSize = RTIntegrator::CalculateUniformBuffersSize(offset);
        auto algorithmUniformDataOffset = GetDevice()->CalculateUniformBufferAlignment(parentSize);
        return algorithmUniformDataOffset + m_algorithmUBO.GetCompleteSize();
    }

    std::size_t AOIntegrator::AddUBOsToMemoryGroup(vkfw_core::gfx::MemoryGroup& memGroup, unsigned int completeBufferIndex, std::size_t offset)
    {
        auto parentOffset = RTIntegrator::AddUBOsToMemoryGroup(memGroup, completeBufferIndex, offset);

        auto algorithmUniformDataOffset = GetDevice()->CalculateUniformBufferAlignment(parentOffset);
        m_algorithmUBO.AddUBOToBuffer(&memGroup, completeBufferIndex, algorithmUniformDataOffset, m_algorithmProperties);

        return algorithmUniformDataOffset + m_algorithmUBO.GetCompleteSize();
    }

    void AOIntegrator::AddDescriptorLayoutBinding(vkfw_core::gfx::DescriptorSetLayout& rtResourcesDescriptorSetLayout)
    {
        RTIntegrator::AddDescriptorLayoutBinding(rtResourcesDescriptorSetLayout);

        using UniformBufferObject = vkfw_core::gfx::UniformBufferObject;
        using ResBindings = scene::rt::ResSetBindings;
        UniformBufferObject::AddDescriptorLayoutBinding(rtResourcesDescriptorSetLayout, vk::ShaderStageFlagBits::eRaygenKHR, true, static_cast<uint32_t>(ResBindings::AlgorithmProperties));
    }

    void AOIntegrator::FillUBOUploadCmdBuffer(vkfw_core::gfx::CommandBuffer& transferCommandBuffer, std::size_t imageIndex) const
    {
        RTIntegrator::FillUBOUploadCmdBuffer(transferCommandBuffer, imageIndex);
        m_algorithmUBO.FillUploadCmdBuffer<scene::rt::ao::AlgorithmParameters>(transferCommandBuffer, imageIndex);
    }

    void AOIntegrator::FillDescriptorSets(vkfw_core::gfx::DescriptorSet& rtResourcesDescriptorSet, std::vector<vkfw_core::gfx::DescriptorSet>& convergenceImageDescriptorSets)
    {
        RTIntegrator::FillDescriptorSets(rtResourcesDescriptorSet, convergenceImageDescriptorSets);
        using ResBindings = vkfw_app::scene::rt::ResSetBindings;

        std::array<vkfw_core::gfx::BufferRange, 1> algorithmBufferRange;

        m_algorithmUBO.FillBufferRange(algorithmBufferRange[0]);
        GetResourcesDescriptorSet().WriteBufferDescriptor(static_cast<uint32_t>(ResBindings::AlgorithmProperties), 0, algorithmBufferRange, vk::AccessFlagBits2KHR::eShaderRead);
    }

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

    bool gfx::rt::AOIntegrator::RenderGUI()
    {
        bool guiChanged = false;
        bool cosSample = m_algorithmProperties.cosineSampled == 1;
        if (ImGui::Checkbox("Samples Cosine Weigthed", &cosSample)) {
            // add camera changed here.
            guiChanged = true;
        }
        m_algorithmProperties.cosineSampled = cosSample ? 1 : 0;
        if (ImGui::SliderFloat("Max. Range", &m_algorithmProperties.maxRange, 0.1f, 10000.0f)) {
            // add camera changed here.
            guiChanged = true;
        }
        return guiChanged;
    }

    void AOIntegrator::TraceRays(vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex, const glm::u32vec4& rtGroups)
    {
        auto& sbtDeviceAddressRegions = GetPipeline().GetSBTDeviceAddresses();

        GetPipeline().BindPipeline(cmdBuffer);
        GetResourcesDescriptorSet().Bind(cmdBuffer, vk::PipelineBindPoint::eRayTracingKHR, GetPipelineLayout(), 0,
                                         {
                                             static_cast<std::uint32_t>(cmdBufferIndex * GetCameraUBO().GetInstanceSize()),
                                             static_cast<std::uint32_t>(cmdBufferIndex * GetFrameUBO().GetInstanceSize()),
                                             static_cast<std::uint32_t>(cmdBufferIndex * m_algorithmUBO.GetInstanceSize())
                                         });
        GetImageDescriptorSet(cmdBufferIndex).Bind(cmdBuffer, vk::PipelineBindPoint::eRayTracingKHR, GetPipelineLayout(), 1);

        cmdBuffer.GetHandle().traceRaysKHR(sbtDeviceAddressRegions[0], sbtDeviceAddressRegions[1], sbtDeviceAddressRegions[2], sbtDeviceAddressRegions[3], rtGroups.x, rtGroups.y, rtGroups.z);
    }
}

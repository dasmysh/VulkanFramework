/**
 * @file   RTIntegrator.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.12.05
 *
 * @brief  Implementation of the integrator base class.
 */

#include "gfx/RTIntegrator.h"
#include "main.h"
#include <core/resources/ShaderManager.h>
#include <gfx/vk/LogicalDevice.h>
#include <gfx/vk/UniformBufferObject.h>
#include <gfx/vk/wrappers/CommandBuffer.h>
#include <gfx/vk/wrappers/DescriptorSet.h>

namespace vkfw_app::gfx::rt {

    RTIntegrator::RTIntegrator(std::string_view integratorName, std::string_view name, vkfw_core::gfx::LogicalDevice* device, const vkfw_core::gfx::PipelineLayout& rtPipelineLayout,
        const vkfw_core::gfx::UniformBufferObject& cameraUBO, vkfw_core::gfx::DescriptorSet& rtResourcesDescriptorSet, std::vector<vkfw_core::gfx::DescriptorSet>& convergenceImageDescriptorSets)
        : m_integratorName{integratorName}
        , m_cameraUBO{cameraUBO}
        , m_rtResourcesDescriptorSet{rtResourcesDescriptorSet}
        , m_convergenceImageDescriptorSets{convergenceImageDescriptorSets}
        , m_rtPipelineLayout{rtPipelineLayout}
        , m_rtPipeline{device, name, {}}
    {
    }

    RTIntegrator::~RTIntegrator() = default;
}

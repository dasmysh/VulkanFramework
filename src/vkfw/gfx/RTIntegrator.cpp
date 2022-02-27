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
#include <gfx/vk/wrappers/CommandBuffer.h>
#include <gfx/vk/wrappers/DescriptorSet.h>
#include <gfx/camera/UserControlledCamera.h>
#include <glm/gtc/matrix_inverse.hpp>
#include "rt/rt_sample_host_interface.h"

namespace vkfw_app::gfx::rt {
    RTIntegrator::RTIntegrator(std::string_view integratorName, std::string_view pipelineName, vkfw_core::gfx::LogicalDevice* device, vkfw_core::gfx::UserControlledCamera* camera, std::uint32_t maxRecursionDepth,
                               std::size_t framebufferCount)
        : m_integratorName{integratorName}
        , m_maxRecursionDepth{maxRecursionDepth}
        , m_device{device}
        , m_camera{camera}
        , m_rtPipeline{device, pipelineName, {}}
        , m_cameraUBO{vkfw_core::gfx::UniformBufferObject::Create<scene::rt::CameraParameters>(GetDevice(), framebufferCount)}
        , m_frameUBO{vkfw_core::gfx::UniformBufferObject::Create<scene::rt::FrameParameters>(GetDevice(), framebufferCount)}
    {
        m_cameraProperties.viewInverse = glm::inverse(m_camera->GetViewMatrix());
        m_cameraProperties.projInverse = glm::inverse(m_camera->GetProjMatrix());
        m_frameProperties.frameId = 0;
        m_frameProperties.cameraMovedThisFrame = 1;
    }

    RTIntegrator::~RTIntegrator() = default;

    std::size_t RTIntegrator::CalculateUniformBuffersSize(std::size_t offset)
    {
        auto cameraUniformDataOffset = GetDevice()->CalculateUniformBufferAlignment(offset);
        auto frameUniformDataOffset = GetDevice()->CalculateUniformBufferAlignment(cameraUniformDataOffset + m_cameraUBO.GetCompleteSize());
        return frameUniformDataOffset + m_frameUBO.GetCompleteSize();
    }

    std::size_t RTIntegrator::AddUBOsToMemoryGroup(vkfw_core::gfx::MemoryGroup& memGroup, unsigned int completeBufferIndex, std::size_t offset)
    {
        auto cameraUniformDataOffset = GetDevice()->CalculateUniformBufferAlignment(offset);
        auto frameUniformDataOffset = GetDevice()->CalculateUniformBufferAlignment(cameraUniformDataOffset + m_cameraUBO.GetCompleteSize());
        m_cameraUBO.AddUBOToBuffer(&memGroup, completeBufferIndex, cameraUniformDataOffset, m_cameraProperties);
        m_frameUBO.AddUBOToBuffer(&memGroup, completeBufferIndex, frameUniformDataOffset, m_frameProperties);
        return frameUniformDataOffset + m_frameUBO.GetCompleteSize();
    }

    void RTIntegrator::AddDescriptorLayoutBinding(vkfw_core::gfx::DescriptorSetLayout& rtResourcesDescriptorSetLayout)
    {
        using UniformBufferObject = vkfw_core::gfx::UniformBufferObject;
        using ResBindings = scene::rt::ResSetBindings;
        UniformBufferObject::AddDescriptorLayoutBinding(rtResourcesDescriptorSetLayout, vk::ShaderStageFlagBits::eRaygenKHR, true, static_cast<uint32_t>(ResBindings::CameraProperties));
        UniformBufferObject::AddDescriptorLayoutBinding(rtResourcesDescriptorSetLayout, vk::ShaderStageFlagBits::eRaygenKHR, true, static_cast<uint32_t>(ResBindings::FrameProperties));
    }

    void RTIntegrator::FillUBOUploadCmdBuffer(vkfw_core::gfx::CommandBuffer& transferCommandBuffer, std::size_t imageIndex) const
    {
        m_cameraUBO.FillUploadCmdBuffer<scene::rt::CameraParameters>(transferCommandBuffer, imageIndex);
        m_frameUBO.FillUploadCmdBuffer<scene::rt::FrameParameters>(transferCommandBuffer, imageIndex);
    }

    void RTIntegrator::InitializePipeline(const vkfw_core::gfx::PipelineLayout& pipelineLayout)
    {
        m_rtPipelineLayout = &pipelineLayout;
        auto shaders = GetShaders();
        GetPipeline().ResetShaders(std::move(shaders));
        GetPipeline().CreatePipeline(m_maxRecursionDepth, GetPipelineLayout());
    }

    void RTIntegrator::FillDescriptorSets(vkfw_core::gfx::DescriptorSet& rtResourcesDescriptorSet, std::vector<vkfw_core::gfx::DescriptorSet>& convergenceImageDescriptorSets)
    {
        using ResBindings = vkfw_app::scene::rt::ResSetBindings;

        m_rtResourcesDescriptorSet = &rtResourcesDescriptorSet;
        m_convergenceImageDescriptorSets = &convergenceImageDescriptorSets;

        std::array<vkfw_core::gfx::BufferRange, 1> cameraBufferRange;
        std::array<vkfw_core::gfx::BufferRange, 1> frameBufferRange;

        m_cameraUBO.FillBufferRange(cameraBufferRange[0]);
        m_rtResourcesDescriptorSet->WriteBufferDescriptor(static_cast<uint32_t>(ResBindings::CameraProperties), 0, cameraBufferRange, vk::AccessFlagBits2KHR::eShaderRead);
        m_frameUBO.FillBufferRange(frameBufferRange[0]);
        m_rtResourcesDescriptorSet->WriteBufferDescriptor(static_cast<uint32_t>(ResBindings::FrameProperties), 0, frameBufferRange, vk::AccessFlagBits2KHR::eShaderRead);
    }

    void RTIntegrator::FrameMove(std::uint32_t uboIndex, bool sceneChanged)
    {
        static bool firstFrame = true;
        m_cameraProperties.viewInverse = glm::inverse(m_camera->GetViewMatrix());
        m_cameraProperties.projInverse = glm::inverse(m_camera->GetProjMatrix());

        if (m_lastMoveFrame == uboIndex) {
            m_lastMoveFrame = static_cast<std::size_t>(-1);
            m_frameProperties.cameraMovedThisFrame = 0;
        }

        if (uboIndex == 0) { m_frameProperties.frameId += 1; }

        if (sceneChanged) {
            m_frameProperties.cameraMovedThisFrame = 1;
            m_lastMoveFrame = uboIndex;
        }

        m_cameraUBO.UpdateInstanceData(uboIndex, m_cameraProperties);
        m_frameUBO.UpdateInstanceData(uboIndex, m_frameProperties);
    }
}

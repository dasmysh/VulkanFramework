/**
 * @file   Scene.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.04.27
 *
 * @brief  Basic rendering scene for this application.
 */

#pragma once

#include <cstddef>
#include <glm/vec2.hpp>
#include <vulkan/vulkan.hpp>
#include <gfx/vk/wrappers/CommandBuffer.h>

namespace vkfw_core {
    class VKWindow;
}

namespace vkfw_core::gfx {
    class LogicalDevice;
    class UserControlledCamera;
}

namespace vkfw_app::scene {

    class Scene
    {
    public:
        Scene(vkfw_core::gfx::LogicalDevice* t_device, vkfw_core::gfx::UserControlledCamera* t_camera,
              std::size_t t_num_framebuffers);
        virtual ~Scene() = default;

        virtual void CreatePipeline(const glm::uvec2& screenSize, vkfw_core::VKWindow* window) = 0;
        virtual void UpdateCommandBuffer(const vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex,
                                         vkfw_core::VKWindow* window) = 0;
        virtual void FrameMove(float time, float elapsed, const vkfw_core::VKWindow* window) = 0;
        virtual void RenderScene(const vkfw_core::VKWindow* window) = 0;

    protected:
        vkfw_core::gfx::LogicalDevice* GetDevice() const { return m_device; }
        vkfw_core::gfx::UserControlledCamera* GetCamera() const { return m_camera; }
        std::size_t GetNumberOfFramebuffers() const { return m_num_framebuffers; }

    private:
        /** The device to render the scene on. */
        vkfw_core::gfx::LogicalDevice* m_device;
        /** The camera to render the scene into. */
        vkfw_core::gfx::UserControlledCamera* m_camera;
        /** The number of frame buffers used to render this scene. */
        std::size_t m_num_framebuffers;
    };
}

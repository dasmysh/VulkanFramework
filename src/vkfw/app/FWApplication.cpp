/**
 * @file   FWApplication.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.27
 *
 * @brief  Contains the implementation of FWApplication.
 */

#include "app/FWApplication.h"
#include "app_constants.h"

// #include <gfx/vk/GraphicsPipeline.h>
#include <app/VKWindow.h>
#include <gfx/vk/LogicalDevice.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <gfx/vk/Framebuffer.h>
// #include <gfx/vk/buffers/DeviceBuffer.h>
// #include <gfx/vk/buffers/HostBuffer.h>
// #include <gfx/vk/buffers/HostBuffer.h>
// #include <gfx/vk/UniformBufferObject.h>
#include <gfx/camera/ArcballCamera.h>

// #include <glm/gtc/matrix_transform.hpp>
#include "imgui.h"


#include <vulkan/vulkan.hpp>

namespace vkfw_app {

    /**
     * Constructor.
     */
    FWApplication::FWApplication()
        : ApplicationBase{applicationName,
                          applicationVersion,
                          configFileName,
                          {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
                          {VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, VK_NV_RAY_TRACING_EXTENSION_NAME}},
          m_camera{std::make_unique<vkfw_core::gfx::ArcballCamera>(glm::vec3(2.0f, 2.0f, 2.0f), glm::radians(45.0f),
                                                                   static_cast<float>(GetWindow(0)->GetWidth())
                                                                       / static_cast<float>(GetWindow(0)->GetHeight()),
                                                                   0.1f, 10.0f)},
          m_simple_scene{&GetWindow(0)->GetDevice(), m_camera.get(), GetWindow(0)->GetFramebuffers().size()},
    {
        auto fbSize = GetWindow(0)->GetFramebuffers()[0].GetSize();
        Resize(fbSize, GetWindow(0));
    }

    FWApplication::~FWApplication()
    {
        // remove pipeline from command buffer.
        GetWindow(0)->UpdatePrimaryCommandBuffers([](const vk::CommandBuffer&, std::size_t) {});
    }

    void FWApplication::FrameMove(float time, float elapsed, const vkfw_core::VKWindow* window)
    {
        if (window != GetWindow(0)) return;

        m_simple_scene.FrameMove(time, elapsed, window);
    }

    void FWApplication::RenderScene(const vkfw_core::VKWindow* window)
    {
        m_simple_scene.RenderScene(window);
    }

    void FWApplication::RenderGUI(const vkfw_core::VKWindow* )
    {
        static bool show_demo_window = true;
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    bool FWApplication::HandleKeyboard(int key, int scancode, int action, int mods, vkfw_core::VKWindow* sender)
    {
        if (ApplicationBase::HandleKeyboard(key, scancode, action, mods, sender)) return true;
        if (!IsRunning() || IsPaused()) return false;
        auto handled = false;
        return handled;
    }

    bool FWApplication::HandleMouseApp(int button, int action, int, float mouseWheelDelta, vkfw_core::VKWindow* sender)
    {
        auto handled = m_camera->HandleMouse(button, action, mouseWheelDelta, sender);
        return handled;
    }

    void FWApplication::Resize(const glm::uvec2& screenSize, const vkfw_core::VKWindow* window)
    {
        // TODO: handle other windows... [11/9/2016 Sebastian Maisch]
        // TODO: maybe use lambdas to register for resize events...
        if (window != GetWindow(0)) return;

        m_simple_scene.CreatePipeline(screenSize, window);

        window->UpdatePrimaryCommandBuffers([this](const vk::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex)
        {
            m_simple_scene.UpdateCommandBuffer(cmdBuffer, cmdBufferIndex);
        });
    }
}

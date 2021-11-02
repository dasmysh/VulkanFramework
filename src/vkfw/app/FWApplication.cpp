/**
 * @file   FWApplication.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.27
 *
 * @brief  Contains the implementation of FWApplication.
 */

#include "app/FWApplication.h"
#include "app_constants.h"
#include <app/VKWindow.h>
#include <gfx/vk/LogicalDevice.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <gfx/vk/Framebuffer.h>
#include <gfx/camera/ArcballCamera.h>

#include "imgui.h"
#include <vulkan/vulkan.hpp>

namespace vkfw_app {

    void* GetDeviceFeaturesNextChain()
    {
        return nullptr;
    }

    /**
     * Constructor.
     */
    FWApplication::FWApplication()
        : ApplicationBase{applicationName,
                          applicationVersion,
                          configFileName,
                          {},
                          {VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME},
                          GetDeviceFeaturesNextChain()},
          m_camera{std::make_unique<vkfw_core::gfx::ArcballCamera>(glm::vec3(2.0f, 2.0f, 2.0f), glm::radians(45.0f),
                                                                   static_cast<float>(GetWindow(0)->GetWidth())
                                                                       / static_cast<float>(GetWindow(0)->GetHeight()),
                                                                   0.1f, 10.0f)},
          m_simple_scene{&GetWindow(0)->GetDevice(), m_camera.get(), GetWindow(0)->GetFramebuffers().size()},
          m_rt_scene{&GetWindow(0)->GetDevice(), m_camera.get(), GetWindow(0)->GetFramebuffers().size()}
    {
        auto fbSize = GetWindow(0)->GetFramebuffers()[0].GetSize();
        Resize(fbSize, GetWindow(0));
    }

    FWApplication::~FWApplication()
    {
        // remove pipeline from command buffer.
        GetWindow(0)->UpdatePrimaryCommandBuffers([](const vkfw_core::gfx::CommandBuffer&, std::size_t) {});
    }

    void FWApplication::FrameMove(float time, float elapsed, vkfw_core::VKWindow* window)
    {
        if (window != GetWindow(0)) return;

        m_camera->UpdateCamera(elapsed, window);

        switch (m_scene_to_render) {
        case 0: m_simple_scene.FrameMove(time, elapsed, window); break;
        case 1: m_rt_scene.FrameMove(time, elapsed, window); break;
        default: break;
        }
    }

    void FWApplication::RenderScene(vkfw_core::VKWindow* window)
    {
        switch (m_scene_to_render) {
        case 0: m_simple_scene.RenderScene(window); break;
        case 1: m_rt_scene.RenderScene(window); break;
        default: break;
        }
    }

    void FWApplication::RenderGUI(vkfw_core::VKWindow* window)
    {
        bool changed = false;
        ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(220, 90), ImGuiCond_Always);
        if (ImGui::Begin("Render Control")) {
            if (ImGui::RadioButton("Render Simple Scene", &m_scene_to_render, 0)) changed = true;
            if (ImGui::RadioButton("Render RayTracing Scene", &m_scene_to_render, 1)) changed = true;
        }
        ImGui::End();

        switch (m_scene_to_render) {
        case 0: {
            changed = changed || m_simple_scene.RenderGUI(window);
            break;
        }
        case 1: {
            changed = changed || m_rt_scene.RenderGUI(window);
            break;
        }
        default: break;
        }

        if (changed) { window->ForceResizeEvent(); }
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

    void FWApplication::Resize(const glm::uvec2& screenSize, vkfw_core::VKWindow* window)
    {
        // TODO: handle other windows... [11/9/2016 Sebastian Maisch]
        // TODO: maybe use lambdas to register for resize events...
        if (window != GetWindow(0)) return;

        switch (m_scene_to_render) {
        case 0: m_simple_scene.CreatePipeline(screenSize, window); break;
        case 1: m_rt_scene.CreatePipeline(screenSize, window); break;
        default: break;
        }

        window->UpdatePrimaryCommandBuffers(
            [this, window](vkfw_core::gfx::CommandBuffer& cmdBuffer, std::size_t cmdBufferIndex) {
                switch (m_scene_to_render) {
                case 0: {
                    m_simple_scene.RenderScene(cmdBuffer, cmdBufferIndex, window);
                    break;
                }
                case 1: {
                    m_rt_scene.RenderScene(cmdBuffer, cmdBufferIndex, window);
                    break;
                }
                default: break;
                }
            });
    }

}

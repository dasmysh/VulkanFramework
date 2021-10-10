/**
 * @file   Scene.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.04.27
 *
 * @brief  Implementation of a basic scene.
 */

#include "app/Scene.h"
#include "imgui.h"

namespace vkfw_app::scene {

    Scene::Scene(vkfw_core::gfx::LogicalDevice* t_device, vkfw_core::gfx::UserControlledCamera* t_camera,
                 std::size_t t_num_framebuffers)
        : m_device{t_device}, m_camera{t_camera}, m_num_framebuffers{t_num_framebuffers}
    {}

    bool Scene::RenderGUI(const vkfw_core::VKWindow*)
    {
        static bool show_demo_window = true;
        ImGui::ShowDemoWindow(&show_demo_window);
        return false;
    }
}

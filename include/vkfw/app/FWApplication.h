/**
 * @file   FWApplication.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.27
 *
 * @brief  Contains the definition of FWApplication.
 */

#pragma once

#include <app/ApplicationBase.h>
#include "app/SimpleScene.h"
#include "app/RaytracingScene.h"

namespace vkfw_core::gfx {
    class UserControlledCamera;
}

namespace vkfw_app {

    class FWApplication final : public vkfw_core::ApplicationBase
    {
    public:
        explicit FWApplication();
        virtual ~FWApplication() override;

        bool HandleKeyboard(int key, int scancode, int action, int mods, vkfw_core::VKWindow* sender) override;
        bool HandleMouseApp(int button, int action, int mods, float mouseWheelDelta,
            vkfw_core::VKWindow* sender) override;
        void Resize(const glm::uvec2& screenSize, vkfw_core::VKWindow* window) override;

    private:
        /** The camera model used. */
        std::unique_ptr<vkfw_core::gfx::UserControlledCamera> m_camera;

        scene::simple::SimpleScene m_simple_scene;
        scene::rt::RaytracingScene m_rt_scene;

    protected:
        void FrameMove(float time, float elapsed, const vkfw_core::VKWindow* window) override;
        void RenderScene(const vkfw_core::VKWindow* window) override;
        void RenderGUI(const vkfw_core::VKWindow* window) override;
    };
}

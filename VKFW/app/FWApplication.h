/**
 * @file   FWApplication.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.27
 *
 * @brief  Contains the definition of FWApplication.
 */

#pragma once

#include "app/ApplicationBase.h"

namespace vkuapp {


    class FWApplication final : public vku::ApplicationBase
    {
    public:
        explicit FWApplication();
        virtual ~FWApplication();

        bool HandleKeyboard(int key, int scancode, int action, int mods, vku::VKWindow* sender) override;
        bool HandleMouseApp(int button, int action, int mods, float mouseWheelDelta, vku::VKWindow* sender) override;
        void Resize(const glm::uvec2& screenSize) override;

    private:

        /** Holds the screen text to render fps. */
        // std::unique_ptr<ScreenText> fpsText_;

    protected:
        void FrameMove(float time, float elapsed) override;
        void RenderScene(const vku::VKWindow* window) override;
        void RenderGUI() override;
    };
}

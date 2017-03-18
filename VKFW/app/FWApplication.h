/**
 * @file   FWApplication.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.27
 *
 * @brief  Contains the definition of FWApplication.
 */

#pragma once

#include "app/ApplicationBase.h"
#include "gfx/VertexFormats.h"
#include "gfx/vk/buffers/BufferGroup.h"

namespace vku::gfx {
    class DeviceBuffer;
    class GraphicsPipeline;
}

namespace vkuapp {

    struct MVPMatrixUBO
    {
        glm::mat4 model_;
        glm::mat4 view_;
        glm::mat4 proj_;
    };

    class FWApplication final : public vku::ApplicationBase
    {
    public:
        explicit FWApplication();
        virtual ~FWApplication();

        bool HandleKeyboard(int key, int scancode, int action, int mods, vku::VKWindow* sender) override;
        bool HandleMouseApp(int button, int action, int mods, float mouseWheelDelta, vku::VKWindow* sender) override;
        void Resize(const glm::uvec2& screenSize, const vku::VKWindow* window) override;

    private:
        /** Holds the descriptor set layout for the demo pipeline. */
        vk::DescriptorSetLayout vkDescriptorSetLayout_;
        /** Holds the pipeline layout for demo rendering. */
        vk::PipelineLayout vkPipelineLayout_;
        /** Holds the descriptor pool for the UBO binding. */
        vk::DescriptorPool vkUBODescriptorPool_;
        /** Holds the descriptor set for the UBO binding. */
        std::vector<vk::DescriptorSet> vkUBODescritorSets_;
        /** Holds the graphics pipeline for demo rendering. */
        std::unique_ptr<vku::gfx::GraphicsPipeline> demoPipeline_;
        /** Holds vertex information. */
        std::vector<SimpleVertex> vertices_;
        /** Holds index information. */
        std::vector<std::uint32_t> indices_;
        /** Holds a single buffer for all vertex, index and uniform data. */
        std::unique_ptr<vku::gfx::DeviceBuffer> completeBuffer_;
        /** Holds the vertex and index buffer. */
        vku::gfx::BufferGroup buffers_;
        /** Holds the vertex buffer. */
        // std::unique_ptr<vku::gfx::DeviceBuffer> vtxBuffer_;
        /** Holds the index buffer. */
        // std::unique_ptr<vku::gfx::DeviceBuffer> idxBuffer_;
        /** Holds the local uniform buffer for transfer. */
        std::unique_ptr<vku::gfx::HostBuffer> uboTransferBuffer_;
        /** Holds the command buffers for transferring the uniform buffers. */
        std::vector<vk::CommandBuffer> vkTransferCommandBuffers_;



        /** Holds the screen text to render fps. */
        // std::unique_ptr<ScreenText> fpsText_;

    protected:
        void FrameMove(float time, float elapsed, const vku::VKWindow* window) override;
        void RenderScene(const vku::VKWindow* window) override;
        void RenderGUI() override;
    };
}

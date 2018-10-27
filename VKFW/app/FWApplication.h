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
#include "gfx/vk/memory/MemoryGroup.h"
#include "gfx/vk/UniformBufferObject.h"

namespace vku::gfx {
    class DeviceBuffer;
    class GraphicsPipeline;
    class Texture2D;
    class AssImpScene;
    class Mesh;
    class UniformBufferObject;
}

namespace vkuapp {

    struct CameraMatrixUBO
    {
        glm::mat4 view_;
        glm::mat4 proj_;
    };

    class FWApplication final : public vku::ApplicationBase
    {
    public:
        explicit FWApplication();
        virtual ~FWApplication() override;

        bool HandleKeyboard(int key, int scancode, int action, int mods, vku::VKWindow* sender) override;
        bool HandleMouseApp(int button, int action, int mods, float mouseWheelDelta, vku::VKWindow* sender) override;
        void Resize(const glm::uvec2& screenSize, const vku::VKWindow* window) override;

    private:
        /** Holds the descriptor set layouts for the demo pipeline. */
        vk::DescriptorSetLayout vkImageSampleDescriptorSetLayout_;
        /** Holds the pipeline layout for demo rendering. */
        vk::UniquePipelineLayout vkPipelineLayout_;
        /** Holds the descriptor pool for the UBO binding. */
        vk::UniqueDescriptorPool vkUBODescriptorPool_;
        /** Holds the descriptor set for the image/sampler binding. */
        vk::DescriptorSet vkImageSamplerDescritorSet_;
        /** Holds the graphics pipeline for demo rendering. */
        std::unique_ptr<vku::gfx::GraphicsPipeline> demoPipeline_;
        /** Holds vertex information. */
        std::vector<SimpleVertex> vertices_;
        /** Holds index information. */
        std::vector<std::uint32_t> indices_;
        /** Holds the vertex and index buffer. */
        vku::gfx::MemoryGroup memGroup_;
        /** Holds the memory group index of the complete buffer. */
        unsigned int completeBufferIdx_ = vku::gfx::MemoryGroup::INVALID_INDEX;

        /** The uniform buffer object for the camera matrices. */
        vku::gfx::UniformBufferObject cameraUBO_;
        /** The uniform buffer object for the world matrices. */
        vku::gfx::UniformBufferObject worldUBO_;

        /** The command pool for the transfer cmd buffers. */
        vk::UniqueCommandPool transferCmdPool_; // TODO: at some point we may need one per buffer? [10/19/2018 Sebastian Maisch]
        /** Holds the command buffers for transferring the uniform buffers. */
        std::vector<vk::UniqueCommandBuffer> vkTransferCommandBuffers_;

        /** Holds the texture used. */
        std::shared_ptr<vku::gfx::Texture2D> demoTexture_;
        /** Holds the texture sampler. */
        vk::UniqueSampler vkDemoSampler_;

        /** Holds the AssImp demo model. */
        std::shared_ptr<vku::gfx::AssImpScene> meshInfo_;
        /** Holds the mesh to be rendered. */
        std::unique_ptr<vku::gfx::Mesh> mesh_;

        /** Holds the screen text to render fps. */
        // std::unique_ptr<ScreenText> fpsText_;

    protected:
        void FrameMove(float time, float elapsed, const vku::VKWindow* window) override;
        void RenderScene(const vku::VKWindow* window) override;
        void RenderGUI() override;
    };
}

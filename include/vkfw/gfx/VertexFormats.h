/**
 * @file   VertexFormats.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.09
 *
 * @brief  Contains declarations of vertices used in this application.
 */

#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <vulkan/vulkan.hpp>

namespace vkfw_core::gfx {
    class MeshInfo;
    struct Material;
}

namespace vkfw_app {

    struct SimpleVertex
    {
        glm::vec3 position_;
        glm::vec3 color_;
        glm::vec2 texCoord_;

        SimpleVertex() = default;
        SimpleVertex(const glm::vec3& position, const glm::vec3& color, const glm::vec2& texCoord) :
            position_{ position }, color_{ color }, texCoord_{ texCoord } {};
        SimpleVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index);
        static vk::VertexInputBindingDescription bindingDescription_;
        static std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions_;
    };

    struct SimpleMaterial
    {
        SimpleMaterial(const vkfw_core::gfx::Material&) {}
    };
}

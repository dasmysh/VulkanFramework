/**
 * @file   VertexFormats.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.09
 *
 * @brief  Contains declarations of vertices used in this application.
 */

#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace vku::gfx {
    class MeshInfo;
    struct Material;
}

namespace vkuapp {

    struct SimpleVertex
    {
        glm::vec3 position_;
        glm::vec3 color_;
        glm::vec2 texCoord_;

        SimpleVertex() {};
        SimpleVertex(const glm::vec3& position, const glm::vec3& color, const glm::vec2& texCoord) :
            position_{ position }, color_{ color }, texCoord_{ texCoord } {};
        SimpleVertex(const vku::gfx::MeshInfo* mi, std::size_t index);
        static vk::VertexInputBindingDescription bindingDescription_;
        static std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions_;
    };

    struct SimpleMaterial
    {
        SimpleMaterial(const vku::gfx::Material&) {}
    };
}

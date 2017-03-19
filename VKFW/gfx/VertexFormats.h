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

namespace vkuapp {

    struct SimpleVertex
    {
        glm::vec2 position_;
        glm::vec3 color_;
        glm::vec2 texCoord_;

        static vk::VertexInputBindingDescription bindingDescription_;
        static std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions_;
    };
}

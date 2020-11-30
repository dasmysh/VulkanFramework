/**
 * @file   VertexFormats.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.09
 *
 * @brief  Contains declarations of vertices used in this application.
 */

#pragma once

#include <glm/vec4.hpp>
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
        glm::vec3 m_position;
        glm::vec3 m_color;
        glm::vec2 m_texCoord;

        SimpleVertex() = default;
        SimpleVertex(const glm::vec3& position, const glm::vec3& color, const glm::vec2& texCoord) :
            m_position{ position }, m_color{ color }, m_texCoord{ texCoord } {};
        SimpleVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index);
        static vk::VertexInputBindingDescription m_bindingDescription;
        static std::array<vk::VertexInputAttributeDescription, 3> m_attributeDescriptions;
    };

    struct SimpleMaterial
    {
        SimpleMaterial(const vkfw_core::gfx::Material&) {}
    };

    struct RayTracingVertex
    {
        glm::vec3 m_position;
        glm::vec3 m_normal;
        glm::vec2 m_texCoord;
        glm::vec4 m_color;

        RayTracingVertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec4& color, const glm::vec2& texCoord)
            : m_position{position}, m_normal{normal}, m_color{color}, m_texCoord{texCoord} {};
        RayTracingVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index);
        static vk::VertexInputBindingDescription m_bindingDescription;
        static std::array<vk::VertexInputAttributeDescription, 4> m_attributeDescriptions;
    };
}

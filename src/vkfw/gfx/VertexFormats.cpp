/**
 * @file   VertexFormats.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.10
 *
 * @brief  Definitions for the vertex formats used in this application.
 */

#include "gfx/VertexFormats.h"
#include "gfx/meshes/MeshInfo.h"

namespace vkfw_app {

    vk::VertexInputBindingDescription SimpleVertex::m_bindingDescription{ 0, sizeof(SimpleVertex), vk::VertexInputRate::eVertex };
    std::array<vk::VertexInputAttributeDescription, 3> SimpleVertex::m_attributeDescriptions{ {
        { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(SimpleVertex, m_position) },
        { 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(SimpleVertex, m_color) },
        { 2, 0, vk::Format::eR32G32Sfloat, offsetof(SimpleVertex, m_texCoord) } } };

    SimpleVertex::SimpleVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index) :
        m_position{ mi->GetVertices()[index] },
        m_color{ mi->GetColors().empty() ? glm::vec3(0.0f) : mi->GetColors()[0][index] },
        m_texCoord{ mi->GetTexCoords()[0][index] }
    {
    }

    vk::VertexInputBindingDescription RayTracingVertex::m_bindingDescription{0, sizeof(RayTracingVertex),
                                                                         vk::VertexInputRate::eVertex};
    std::array<vk::VertexInputAttributeDescription, 4> RayTracingVertex::m_attributeDescriptions{
        {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(RayTracingVertex, m_position)},
         {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(RayTracingVertex, m_normal)},
         {2, 0, vk::Format::eR32G32Sfloat, offsetof(RayTracingVertex, m_texCoord)},
         {3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(RayTracingVertex, m_color)}}};

    RayTracingVertex::RayTracingVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index)
        : m_position{mi->GetVertices()[index]},
          m_normal{mi->GetNormals()[index]},
          m_color{mi->GetColors().empty() ? glm::vec4(0.0f) : mi->GetColors()[0][index]},
          m_texCoord{mi->GetTexCoords()[0][index]}
    {
    }

}

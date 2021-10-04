/**
 * @file   VertexFormats.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.10
 *
 * @brief  Definitions for the vertex formats used in this application.
 */

#include "gfx/VertexFormats.h"
#include "mesh/mesh_sample_host_interface.h"
#include "rt/rt_sample_host_interface.h"
#include "gfx/meshes/MeshInfo.h"

namespace mesh_sample {

    vk::VertexInputBindingDescription SimpleVertex::m_bindingDescription{0, sizeof(SimpleVertex), vk::VertexInputRate::eVertex};
    std::array<vk::VertexInputAttributeDescription, 3> SimpleVertex::m_attributeDescriptions{
        {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(SimpleVertex, inPosition)}, {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(SimpleVertex, inColor)}, {2, 0, vk::Format::eR32G32Sfloat, offsetof(SimpleVertex, inTexCoord)}}};

    SimpleVertex::SimpleVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index)
        : inPosition{mi->GetVertices()[index]}, inColor{mi->GetColors().empty() ? glm::vec3(0.0f) : mi->GetColors()[0][index]}, inTexCoord{mi->GetTexCoords()[0][index]}
    {
    }
}

namespace rt_sample {

    vk::VertexInputBindingDescription RayTracingVertex::m_bindingDescription{0, sizeof(RayTracingVertex),
                                                                         vk::VertexInputRate::eVertex};
    std::array<vk::VertexInputAttributeDescription, 4> RayTracingVertex::m_attributeDescriptions{
        {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(RayTracingVertex, position)},
         {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(RayTracingVertex, normal)},
         {2, 0, vk::Format::eR32G32Sfloat, offsetof(RayTracingVertex, texCoords)},
         {3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(RayTracingVertex, color)}}};

    RayTracingVertex::RayTracingVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index)
        : position{mi->GetVertices()[index]}
        , normal{mi->GetNormals()[index]}
        , color{mi->GetColors().empty() ? glm::vec4(0.0f) : mi->GetColors()[0][index]}
        , texCoords{mi->GetTexCoords()[0][index]}
    {
    }

}

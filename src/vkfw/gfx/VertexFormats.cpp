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

    vk::VertexInputBindingDescription SimpleVertex::bindingDescription_{ 0, sizeof(SimpleVertex), vk::VertexInputRate::eVertex };
    std::array<vk::VertexInputAttributeDescription, 3> SimpleVertex::attributeDescriptions_{ {
        { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(SimpleVertex, position_) },
        { 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(SimpleVertex, color_) },
        { 2, 0, vk::Format::eR32G32Sfloat, offsetof(SimpleVertex, texCoord_) } } };

    SimpleVertex::SimpleVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index) :
        position_{ mi->GetVertices()[index] },
        color_{ mi->GetColors().empty() ? glm::vec3(0.0f) : mi->GetColors()[0][index] },
        texCoord_{ mi->GetTexCoords()[0][index] }
    {
    }
 }

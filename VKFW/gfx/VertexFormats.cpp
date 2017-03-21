/**
 * @file   VertexFormats.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.10
 *
 * @brief  Definitions for the vertex formats used in this application.
 */

#include "VertexFormats.h"

namespace vkuapp {

    vk::VertexInputBindingDescription SimpleVertex::bindingDescription_{ 0, sizeof(SimpleVertex), vk::VertexInputRate::eVertex };
    std::array<vk::VertexInputAttributeDescription, 3> SimpleVertex::attributeDescriptions_{ {
        { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(SimpleVertex, position_) },
        { 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(SimpleVertex, color_) },
        { 2, 0, vk::Format::eR32G32Sfloat, offsetof(SimpleVertex, texCoord_) } } };
}

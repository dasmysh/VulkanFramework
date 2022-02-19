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

    struct SimpleMaterial
    {
        SimpleMaterial(const vkfw_core::gfx::Material&) {}
    };
}

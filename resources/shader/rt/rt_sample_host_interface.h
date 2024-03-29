#ifndef RT_SAMPLE_HOST_INTERFACE
#define RT_SAMPLE_HOST_INTERFACE

#include "rt/ray_tracing_host_interface.h"
#include "materials/material_sample_host_interface.h"

BEGIN_INTERFACE(vkfw_app::scene::rt)

BEGIN_CONSTANTS(BindingSets)
    RTResourcesSet = 0,
    ConvergenceSet = 1
END_CONSTANTS()

BEGIN_CONSTANTS(ResSetBindings)
    AccelerationStructure = 0,
    CameraProperties = 1,
    Vertices = 2,
    Indices = 3,
    InstanceInfos = 4,
    PhongBumpMaterialInfos = 5,
    MirrorMaterialInfos = 6,
    Textures = 7,
    ResSetBindingsSize = 8
END_CONSTANTS()

BEGIN_CONSTANTS(ConvSetBindings)
    ResultImage = 0,
    ConvSetBindingsSize = 1
END_CONSTANTS()

struct RayTracingVertex
{
    vec3 position;
    vec3 normal;
    vec2 texCoords;
    vec4 color;

#ifdef __cplusplus
    RayTracingVertex(const vec3& position_, const vec3& normal_, const vec4& color_, const vec2& texCoords_) : position{position_}, normal{normal_}, color{color_}, texCoords{texCoords_} {};
    RayTracingVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index);
    static vk::VertexInputBindingDescription m_bindingDescription;
    static std::array<vk::VertexInputAttributeDescription, 4> m_attributeDescriptions;
#endif
};

struct CameraParameters
{
    mat4 viewInverse;
    mat4 projInverse;
    uint frameId;
    uint cameraMovedThisFrame;
    uint cosineSampled;
    float maxRange;
};

BEGIN_UNIFORM_BLOCK(set = RTResourcesSet, binding = CameraProperties, CameraPropertiesBuffer)
    CameraParameters cam;
END_UNIFORM_BLOCK()

END_INTERFACE()

#endif // RT_SAMPLE_HOST_INTERFACE

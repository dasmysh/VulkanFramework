#ifndef RT_SAMPLE_HOST_INTERFACE
#define RT_SAMPLE_HOST_INTERFACE

#include "rt/ray_tracing_host_interface.h"

BEGIN_INTERFACE(vkfw_app::scene::rt)

BEGIN_CONSTANTS(RTBindings)
    AccelerationStructure = 0,
    ResultImage = 1,
    CameraProperties = 2,
    Vertices = 3,
    Indices = 4,
    InstanceInfos = 5,
    MaterialInfos = 6,
    DiffuseTextures = 7,
    BumpTextures = 8,
    BindingsSize = 9
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

BEGIN_UNIFORM_BLOCK(set = 0, binding = CameraProperties, CameraPropertiesBuffer)
mat4 viewInverse;
mat4 projInverse;
uint frameId;
uint cameraMovedThisFrame;
uint cosineSampled;
float maxRange;
END_UNIFORM_BLOCK(cam)

END_INTERFACE()

#endif // RT_SAMPLE_HOST_INTERFACE

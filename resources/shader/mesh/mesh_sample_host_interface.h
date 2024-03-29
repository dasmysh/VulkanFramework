#ifndef MESH_SAMPLE_HOST_INTERFACE
#define MESH_SAMPLE_HOST_INTERFACE

#include "mesh/mesh_host_interface.h"

BEGIN_INTERFACE(mesh_sample)

BEGIN_CONSTANTS(MeshInputBindings)
Position = 0,
Color = 1,
TexCoords = 2
END_CONSTANTS()

BEGIN_CONSTANTS(MeshBindings)
CameraProperties = 0
END_CONSTANTS()

BEGIN_INPUT_BLOCK(SimpleVertex)
INPUT_ELEMENT(Position) vec3 inPosition;
INPUT_ELEMENT(Color) vec3 inColor;
INPUT_ELEMENT(TexCoords) vec2 inTexCoord;

#ifdef __cplusplus
SimpleVertex() = default;
SimpleVertex(const glm::vec3& position_, const glm::vec3& color_, const glm::vec2& texCoord_) : inPosition{position_}, inColor{color_}, inTexCoord{texCoord_} {};
SimpleVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index);
static vk::VertexInputBindingDescription m_bindingDescription;
static std::array<vk::VertexInputAttributeDescription, 3> m_attributeDescriptions;
#endif
END_INPUT_BLOCK()

BEGIN_UNIFORM_BLOCK(set = 2, binding = CameraProperties, CameraUniformBufferObject)
mat4 view;
mat4 proj;
END_UNIFORM_NAMED_BLOCK(camera_ubo)

END_INTERFACE()

#endif // MESH_SAMPLE_HOST_INTERFACE

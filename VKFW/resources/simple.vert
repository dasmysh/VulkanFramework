#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(set = 3, binding = 0) uniform CameraUniformBufferObject {
    mat4 view;
    mat4 proj;
} camera_ubo;

layout(set = 0, binding = 0) uniform WorldUniformBufferObject {
    mat4 model;
    mat4 normalMatrix;
} world_ubo;

void main() {
    gl_Position = camera_ubo.proj * camera_ubo.view * world_ubo.model * vec4(inPosition, 1.0);

    fragColor = inColor;
    fragTexCoord = inTexCoord;
}

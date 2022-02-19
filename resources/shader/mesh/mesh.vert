#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "mesh_sample_host_interface.h"

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = camera_ubo.proj * camera_ubo.view * world_ubo.model * vec4(inPosition, 1.0);

    fragColor = inColor;
    fragTexCoord = inTexCoord;
}

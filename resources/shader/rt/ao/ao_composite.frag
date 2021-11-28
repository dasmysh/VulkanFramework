#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "ao_composite_shader_interface.h"

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = ConvergenceSet, binding = AccumulatedImage) uniform sampler2D accumulatedImage;

void main()
{
    vec4 accumulatedColor = texture(accumulatedImage, fragTexCoord);
    outColor = vec4(vec3(accumulatedColor.r / accumulatedColor.a), 1.0f);
}

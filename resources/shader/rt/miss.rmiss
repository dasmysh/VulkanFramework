#version 460
#extension GL_EXT_ray_tracing : require

#include "ray.glsl"

layout(location = 0) rayPayloadInEXT RayPayload hitValue;

void main()
{
    if (hitValue.done == 1) {
        hitValue.attenuation = vec3(1.0f);
    } else {
        hitValue.attenuation = vec3(0.0, 0.2, 0.2);
    }
    hitValue.done = 1;
}

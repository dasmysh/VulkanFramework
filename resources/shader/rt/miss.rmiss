#version 460
#extension GL_EXT_ray_tracing : require

#include "ray.glsl"

void main()
{
    hitValue.attenuation = vec3(1, 0, 1);
    hitValue.done = -1;
}

#ifndef SHADER_RT_LIGHTS
#define SHADER_RT_LIGHTS

#include "rayTraversal.glsl"

vec3 next_event_estimation(vec3 origin, , in accelerationStructureEXT topLevelAS)
{
    const vec3 light_position = vec3(0.0f, 2.0f, 0.0f);
    const vec3 I = vec3(0.9882f, 0.0588f, 0.7529f);

    vec3 d = light_position - origin;
    vec3 attenuation;

    bool hit = queryShadowRayHit(origin, d, topLevelAS, 1.0f);
    if (!hit) {
        return (I * brdf() * dot(normal, normalize(d))) / dot(d, d);
    }
    return vec3(0.0f);
}

#endif // SHADER_RT_LIGHTS

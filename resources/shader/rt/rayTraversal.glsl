#ifndef SHADER_RT_RAYTRAVERSAL
#define SHADER_RT_RAYTRAVERSAL

#include "ray.glsl"

bool findNextNonSpecularHit(inout vec3 origin, inout vec3 direction, out vec3 normal, in accelerationStructureEXT topLevelAS, float tmax)
{
    const uint maxSpecularDepth = 10;
    uint rayFlags = gl_RayFlagsNoneEXT;
    uint cullMask = 0xff;
    float tmin = 0.001;

    hitValue.attenuation = vec3(1.0f);
    hitValue.rayDirection = direction.xyz;
    hitValue.rayOrigin = origin.xyz;
    hitValue.done = 0;

    uint specularDepth = 0;
    while (hitValue.done == 0 && specularDepth < maxSpecularDepth)
    {
        traceRayEXT(topLevelAS, rayFlags, cullMask, 0, 0, 0, hitValue.rayOrigin, tmin, hitValue.rayDirection, tmax, 0);
        specularDepth += 1;
    }
    if (hitValue.done < 1)
    {
        return false;
    }

    origin = hitValue.rayOrigin;
    direction = hitValue.rayDirection;
    normal = hitValue.attenuation;
    return true;
}

#endif // SHADER_RT_RAYTRAVERSAL

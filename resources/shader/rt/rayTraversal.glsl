#include "ray.glsl"

layout(binding = AccelerationStructure, set = 0) uniform accelerationStructureEXT topLevelAS;

bool findNextNonSpecularHit(inout vec3 origin, inout vec3 direction, out vec3 normal, float tmax)
{
    const uint maxSpecularDepth = 10;
    uint rayFlags = gl_RayFlagsNoneEXT;
    uint cullMask = 0xff;
    float tmin = 0.001;

    hitValue.attenuation = vec3(1.0f);
    hitValue.rayDirection = direction.xyz;
    hitValue.rayOrigin = origin.xyz;
    hitValue.done = 0;

    // uint specularDepth = 0;
    // while (hitValue.done == 0 && specularDepth < maxSpecularDepth)
    // {
        traceRayEXT(topLevelAS, rayFlags, cullMask, 0, 0, 0, hitValue.rayOrigin, tmin, hitValue.rayDirection, tmax, 0);
    //     specularDepth += 1;
    // }
    if (hitValue.done == -1)
    {
        normal = vec3(0.0f, 1.0f, 0.0f);
        return false;
    }

    origin = hitValue.rayOrigin;
    // direction = hitValue.rayDirection;
    normal = hitValue.attenuation;
    // normal = vec3(0.0f, 0.0f, 1.0f);
    if (hitValue.done >= 100)
    {
        normal = vec3(1, 0, 1);
        return false;
    }
    return true;
}

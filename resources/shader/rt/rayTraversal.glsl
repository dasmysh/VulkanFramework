#ifndef SHADER_RT_RAYTRAVERSAL
#define SHADER_RT_RAYTRAVERSAL

#include "ray.glsl"

bool findNextNonSpecularHit(inout vec3 origin, inout vec3 direction, out vec3 payload, in accelerationStructureEXT topLevelAS, float tmax)
{
    const uint maxSpecularDepth = 10;
    uint rayFlags = gl_RayFlagsNoneEXT;
    uint cullMask = 0xff;
    float tmin = 0.001;

    hitValue.attenuation = payload;
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
    payload = hitValue.attenuation;
    return true;
}

bool queryShadowRayHit(vec3 origin, vec3 direction, in accelerationStructureEXT topLevelAS, float tmax)
{
    uint rayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    uint cullMask = 0xff;
    float tmin = 0.001;

    shadowHit = true;

    traceRayEXT(topLevelAS, rayFlags, cullMask, 0, 0, 1, origin, tmin, direction, tmax, 1);
    return shadowHit;
}

bool findNextHit(in vec3 origin, in vec3 direction,
#ifdef ITERATIVE_PATH_TRACING
    out vec3 barycentrics, out int instanceId, out int primitiveId,
#else
    inout TraceResult result,
#endif
    in accelerationStructureEXT topLevelAS, float tmax)
{
#ifdef ITERATIVE_PATH_TRACING
    hitValueMaterial.instancePrimitive = ivec2(-1, -1);
    uint rayFlags = gl_RayFlagsNoneEXT;
#else
    uint rayFlags = gl_RayFlagsNoneEXT;
    hitValueMaterial.rayDirection = result.rayDirection;
    hitValueMaterial.rayOrigin = result.rayOrigin;
    hitValueMaterial.attenuation = result.attenuation;
    hitValueMaterial.L = result.L;
    hitValueMaterial.specularBounce = result.specularBounce;
    hitValueMaterial.done = result.done;
#endif
    uint cullMask = 0xff;
    float tmin = 0.001;

    traceRayEXT(topLevelAS, rayFlags, cullMask, 0, 0, 0, origin, tmin, direction, tmax, 2);

#ifdef ITERATIVE_PATH_TRACING
    barycentrics = vec3(1.0f - hitValueMaterial.barycentrics.x - hitValueMaterial.barycentrics.y, hitValueMaterial.barycentrics.x, hitValueMaterial.barycentrics.y);
    instanceId = hitValueMaterial.instancePrimitive.x;
    primitiveId = hitValueMaterial.instancePrimitive.y;
    return instanceId != -1 && primitiveId != -1;
#else
    result.rayDirection = hitValueMaterial.rayDirection;
    result.rayOrigin = hitValueMaterial.rayOrigin;
    result.attenuation = hitValueMaterial.attenuation;
    result.L = hitValueMaterial.L;
    result.specularBounce = hitValueMaterial.specularBounce;
    result.done = hitValueMaterial.done;
    return result.done > 0;
#endif
}

#endif // SHADER_RT_RAYTRAVERSAL

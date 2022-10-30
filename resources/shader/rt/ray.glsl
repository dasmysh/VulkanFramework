#ifndef SHADER_RT_RAY
#define SHADER_RT_RAY

struct RayPayloadGeneric {
    vec3 attenuation;
    vec3 rayDirection; // direction of next ray
    vec3 rayOrigin; // origin of next ray
    int done;
    // miss: done = -1
    // specular hit: done += 0
    // other: done += 1
};

#ifdef ITERATIVE_PATH_TRACING
struct RayPayloadMaterials {
    vec2 barycentrics;
    ivec2 instancePrimitive;
};
#else
struct TraceResult {
    vec3 rayDirection; // direction of next ray
    vec3 rayOrigin; // origin of next ray
    vec3 attenuation;
    vec3 L;
    bool specularBounce;
    int done;
    // miss: done = -1
    // specular hit: done += 0
    // other: done += 1
};

struct RayPayloadMaterials {
    vec3 rayDirection; // direction of next ray
    vec3 rayOrigin; // origin of next ray
    vec3 attenuation;
    vec3 L;
    bool specularBounce;
    int done;
};
#endif

#ifdef RAYGEN
#define rayPayload rayPayloadEXT
#else
#define rayPayload rayPayloadInEXT
#endif

layout(location = 0) rayPayload RayPayloadGeneric hitValue;
layout(location = 1) rayPayload bool shadowHit;
layout(location = 2) rayPayload RayPayloadMaterials hitValueMaterial;

#endif // SHADER_RT_RAY

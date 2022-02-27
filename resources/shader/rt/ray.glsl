#ifndef SHADER_RT_RAY
#define SHADER_RT_RAY

struct RayPayload {
    vec3 attenuation;
    vec3 rayDirection;
    vec3 rayOrigin;
    int done;
    // miss: done = -1
    // specular hit: done += 0
    // other: done += 1
};

#ifdef RAYGEN
layout(location = 0) rayPayloadEXT RayPayload hitValue;
#else
layout(location = 0) rayPayloadInEXT RayPayload hitValue;
#endif

#endif // SHADER_RT_RAY

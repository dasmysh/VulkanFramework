
#ifndef RAY_TRACING_SHADER_HOST_INTERFACE
#define RAY_TRACING_SHADER_HOST_INTERFACE

#include "../core/shader_interface.h"

START_INTERFACE(ray_tracing)

START_CONSTANTS(RTBindings)
    AccelerationStructure = 0,
    ResultImage = 1,
    CameraProperties = 2,
    Vertices = 3,
    Indices = 4,
    InstanceInfos = 5,
    DiffuseTextures = 6,
    BumpTextures = 7 
END_CONSTANTS();

END_INTERFACE

#endif // RAY_TRACING_SHADER_HOST_INTERFACE

#ifndef PATH_HOST_INTERFACE
#define PATH_HOST_INTERFACE

#include "../rt_sample_host_interface.h"

BEGIN_INTERFACE(vkfw_app::scene::rt::path)

struct AlgorithmParameters
{
    uint maxDepth;
    float rrThreshold;
};

BEGIN_UNIFORM_BLOCK(set = RTResourcesSet, binding = AlgorithmProperties, AlgorithmPropertiesBuffer)
    AlgorithmParameters path;
END_UNIFORM_BLOCK()

END_INTERFACE()

#endif // PATH_HOST_INTERFACE

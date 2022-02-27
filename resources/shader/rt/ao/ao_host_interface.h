#ifndef AO_HOST_INTERFACE
#define AO_HOST_INTERFACE

#include "../rt_sample_host_interface.h"

BEGIN_INTERFACE(vkfw_app::scene::rt::ao)

struct AlgorithmParameters
{
    uint cosineSampled;
    float maxRange;
};

BEGIN_UNIFORM_BLOCK(set = RTResourcesSet, binding = AlgorithmProperties, AlgorithmPropertiesBuffer)
    AlgorithmParameters ao;
END_UNIFORM_BLOCK()

END_INTERFACE()

#endif // AO_COMPOSITE_HOST_INTERFACE

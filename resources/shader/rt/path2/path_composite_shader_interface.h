#ifndef PATH_COMPOSITE_HOST_INTERFACE
#define PATH_COMPOSITE_HOST_INTERFACE

#include "shader_interface.h"

BEGIN_INTERFACE(vkfw_app::scene::rt)

BEGIN_CONSTANTS(CompositeBindingSets)
    ConvergenceSet = 0
END_CONSTANTS()

BEGIN_CONSTANTS(CompositeConvSetBindings)
    AccumulatedImage = 0,
    ConvSetBindingsSize = 1
END_CONSTANTS()

END_INTERFACE()

#endif // PATH_COMPOSITE_HOST_INTERFACE

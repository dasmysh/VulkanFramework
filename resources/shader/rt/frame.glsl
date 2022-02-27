#ifndef SHADER_RT_FRAME
#define SHADER_RT_FRAME

#include "shader_interface.h"

BEGIN_INTERFACE(vkfw_app::scene::rt)

struct FrameParameters
{
    uint frameId;
    uint cameraMovedThisFrame;
};

END_INTERFACE()

#endif // SHADER_RT_FRAME

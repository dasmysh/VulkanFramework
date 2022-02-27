#ifndef SHADER_RT_CAMERA
#define SHADER_RT_CAMERA

#include "shader_interface.h"

BEGIN_INTERFACE(vkfw_app::scene::rt)

struct CameraParameters
{
    mat4 viewInverse;
    mat4 projInverse;
};

END_INTERFACE()

#endif // SHADER_RT_CAMERA

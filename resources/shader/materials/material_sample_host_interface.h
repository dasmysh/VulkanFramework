#ifndef MATERIAL_SAMPLE_HOST_INTERFACE
#define MATERIAL_SAMPLE_HOST_INTERFACE

#include "materials/material_host_interface.h"

BEGIN_INTERFACE(materials)

#ifndef __cplusplus
    #define CALC_MATERIAL_ENUM(i) ApplicationMaterialsStart + i
#else
    #define CALC_MATERIAL_ENUM(i) static_cast<int>(MaterialIdentifierCore::ApplicationMaterialsStart) + i
#endif

BEGIN_CONSTANTS(MaterialIdentifierApp)
    MirrorMaterialType = CALC_MATERIAL_ENUM(0),
    GlassMaterialType = CALC_MATERIAL_ENUM(1),
    TotalMaterialCount = CALC_MATERIAL_ENUM(2)
END_CONSTANTS()

struct MirrorMaterial
{
    vec3 Kr;
};

END_INTERFACE()

#endif // MATERIAL_SAMPLE_HOST_INTERFACE

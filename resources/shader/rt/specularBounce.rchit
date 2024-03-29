#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

#include "ray.glsl"
#include "rt_sample_host_interface.h"

hitAttributeEXT vec2 attribs;

layout(scalar, binding = Vertices, set = 0) buffer VerticesBuffer { RayTracingVertex v[]; } vertices[];
layout(binding = Indices, set = 0) buffer IndicesBuffer { uint i[]; } indices[];
layout(scalar, binding = InstanceInfos, set = 0) buffer InstanceInfosBuffer { InstanceDesc i[]; } instances;
layout(scalar, binding = PhongBumpMaterialInfos, set = RTResourcesSet) buffer PhongMaterialInfosBuffer { PhongBumpMaterial m[]; } phongMaterials;
layout(scalar, binding = MirrorMaterialInfos, set = RTResourcesSet) buffer MirrorMaterialInfosBuffer { MirrorMaterial m[]; } mirrorMaterials;
layout(binding = Textures, set = 0) uniform sampler2D textures[];

void main()
{
    if (hitValue.done == 1) {
        hitValue.attenuation = vec3(0.0f);
        return;
    }

    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    uint vertexSize = instances.i[gl_InstanceID].vertexSize;
    // if (vertexSize != 48) {
    //     hitValue.attenuation = barycentricCoords;
    //     hitValue.done = 1;
    //     return;
    // }

    uint bufferIndex = instances.i[gl_InstanceID].bufferIndex;
    uint indexOffset = instances.i[gl_InstanceID].indexOffset;
    mat4 transform = instances.i[gl_InstanceID].transform;
    mat4 transformInverseTranspose = instances.i[gl_InstanceID].transformInverseTranspose;
    // uint objId = scnDesc.i[gl_InstanceID].objId;

    ivec3 ind = ivec3(indices[nonuniformEXT(bufferIndex)].i[indexOffset + 3 * gl_PrimitiveID + 0],
                      indices[nonuniformEXT(bufferIndex)].i[indexOffset + 3 * gl_PrimitiveID + 1],
                      indices[nonuniformEXT(bufferIndex)].i[indexOffset + 3 * gl_PrimitiveID + 2]);

    RayTracingVertex v0 = vertices[nonuniformEXT(bufferIndex)].v[ind.x];
    RayTracingVertex v1 = vertices[nonuniformEXT(bufferIndex)].v[ind.y];
    RayTracingVertex v2 = vertices[nonuniformEXT(bufferIndex)].v[ind.z];


    vec3 normal = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
    normal = normalize(vec3(transformInverseTranspose * vec4(normal, 0.0)));

    vec3 worldPos = v0.position * barycentricCoords.x + v1.position * barycentricCoords.y + v2.position * barycentricCoords.z;
    worldPos = vec3(transform * vec4(worldPos, 1.0));

    vec2 texCoords = v0.texCoords * barycentricCoords.x + v1.texCoords * barycentricCoords.y + v2.texCoords * barycentricCoords.z;

    uint materialIndex = instances.i[gl_InstanceID].materialIndex;
    uint materialType = instances.i[gl_InstanceID].materialType;
    vec3 attenuation = vec3(1.0f);
    if (materialType == PhongBumpMaterialType) {
        uint diffuseTextureIndex = phongMaterials.m[nonuniformEXT(materialIndex)].diffuseTextureIndex;
        attenuation = texture(textures[nonuniformEXT(diffuseTextureIndex)], texCoords).rgb;
    }

    hitValue.rayOrigin = worldPos;
    hitValue.rayDirection = normal;
    hitValue.attenuation = attenuation;
    hitValue.done = 1;
}

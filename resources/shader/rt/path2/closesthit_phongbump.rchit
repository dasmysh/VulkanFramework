#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

#include "../ray.glsl"
#include "../rt_sample_host_interface.h"

hitAttributeEXT vec2 attribs;

layout(scalar, binding = Vertices, set = RTResourcesSet) buffer VerticesBuffer { RayTracingVertex v[]; } vertices[];
layout(binding = Indices, set = RTResourcesSet) buffer IndicesBuffer { uint i[]; } indices[];
layout(scalar, binding = InstanceInfos, set = RTResourcesSet) buffer InstanceInfosBuffer { InstanceDesc i[]; } instances;
layout(scalar, binding = PhongBumpMaterialInfos, set = RTResourcesSet) buffer PhongMaterialInfosBuffer { PhongBumpMaterial m[]; } phongMaterials;
layout(binding = Textures, set = RTResourcesSet) uniform sampler2D textures[];

void main()
{
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    uint bufferIndex = instances.i[gl_InstanceID].bufferIndex;
    uint indexOffset = instances.i[gl_InstanceID].indexOffset;
    mat4 transform = instances.i[gl_InstanceID].transform;
    mat4 transformInverseTranspose = instances.i[gl_InstanceID].transformInverseTranspose;

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

    hitValue.rayOrigin = worldPos;
    hitValue.attenuation = normal;
    hitValue.done += 1;
}

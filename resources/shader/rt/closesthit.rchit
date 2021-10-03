#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

#include "ray.glsl"
#include "ray_tracing_shader_host_interface.h"

layout(location = 0) rayPayloadInEXT RayPayload hitValue;
hitAttributeEXT vec2 attribs;

struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 texCoords;
    vec4 color;
};

struct InstanceDesc
{
    uint vertexSize;
    uint bufferIndex;
    uint materialIndex;
    uint textureIndex;
    uint indexOffset;
    mat4 transform;
    mat4 transformInverseTranspose;
};

layout(scalar, binding = Vertices, set = 0) buffer VerticesBuffer { Vertex v[]; } vertices[];
layout(binding = Indices, set = 0) buffer IndicesBuffer { uint i[]; } indices[];
layout(scalar, binding = InstanceInfos, set = 0) buffer InstanceInfosBuffer { InstanceDesc i[]; } instances;
layout(binding = DiffuseTextures, set = 0) uniform sampler2D diffuseTextures[];

void main()
{
    if (hitValue.done == 1) {
        hitValue.attenuation = vec3(0.0f);
        return;
    }

    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    uint vertexSize = instances.i[gl_InstanceID].vertexSize;
    if (vertexSize != 48) {
        hitValue.attenuation = barycentricCoords;
        hitValue.done = 1;
        return;
    }

    uint bufferIndex = instances.i[gl_InstanceID].bufferIndex;
    uint indexOffset = instances.i[gl_InstanceID].indexOffset;
    mat4 transform = instances.i[gl_InstanceID].transform;
    mat4 transformInverseTranspose = instances.i[gl_InstanceID].transformInverseTranspose;
    // uint objId = scnDesc.i[gl_InstanceID].objId;

    ivec3 ind = ivec3(indices[nonuniformEXT(bufferIndex)].i[indexOffset + 3 * gl_PrimitiveID + 0],
                      indices[nonuniformEXT(bufferIndex)].i[indexOffset + 3 * gl_PrimitiveID + 1],
                      indices[nonuniformEXT(bufferIndex)].i[indexOffset + 3 * gl_PrimitiveID + 2]);

    Vertex v0 = vertices[nonuniformEXT(bufferIndex)].v[ind.x];
    Vertex v1 = vertices[nonuniformEXT(bufferIndex)].v[ind.y];
    Vertex v2 = vertices[nonuniformEXT(bufferIndex)].v[ind.z];


    vec3 normal = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
    normal = normalize(vec3(transformInverseTranspose * vec4(normal, 0.0)));

    vec3 worldPos = v0.position * barycentricCoords.x + v1.position * barycentricCoords.y + v2.position * barycentricCoords.z;
    worldPos = vec3(transform * vec4(worldPos, 1.0));

    hitValue.rayOrigin = worldPos;
    hitValue.rayDirection = normal;
}

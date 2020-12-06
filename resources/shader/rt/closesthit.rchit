#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;
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
    uint bufferIndex;
    uint materialIndex;
    uint textureIndex;
    uint indexOffset;
    mat4 transform;
    mat4 transformInverseTranspose;
};

layout(scalar, binding = 3, set = 0) buffer Vertices { Vertex v[]; } vertices[];
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices[];
layout(scalar, binding = 5, set = 0) buffer InstanceInfos { InstanceDesc i[]; } instances;

void main()
{
    uint bufferIndex = instances.i[gl_InstanceID].bufferIndex;
    uint indexOffset = instances.i[gl_InstanceID].indexOffset;
    // uint objId = scnDesc.i[gl_InstanceID].objId;

    ivec3 ind = ivec3(indices[nonuniformEXT(bufferIndex)].i[indexOffset + 3 * gl_PrimitiveID + 0],
                      indices[nonuniformEXT(bufferIndex)].i[indexOffset + 3 * gl_PrimitiveID + 1],
                      indices[nonuniformEXT(bufferIndex)].i[indexOffset + 3 * gl_PrimitiveID + 2]);

    Vertex v0 = vertices[nonuniformEXT(bufferIndex)].v[ind.x];
    Vertex v1 = vertices[nonuniformEXT(bufferIndex)].v[ind.y];
    Vertex v2 = vertices[nonuniformEXT(bufferIndex)].v[ind.z];

    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 normal = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
    normal = normalize(vec3(vec4(normal, 0.0))); // TODO: transform to world.

    hitValue = normal;
}

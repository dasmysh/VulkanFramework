/*
 * Implementation is taken from Ray Tracing Gems II
 */

#ifndef SHADER_CORE_RANDOM
#define SHADER_CORE_RANDOM

uint jenkinsHash(uint x)
{
    x += x << 10;
    x ^= x >> 6;
    x += x << 3;
    x ^= x >> 11;
    x += x << 15;
    return x;
}

uint initRNG(uvec2 pixel, uvec2 resolution, uint frame)
{
    uint rngState = uint(dot(pixel, uvec2(1, resolution.x))) ^ jenkinsHash(frame);
    return jenkinsHash(rngState);
}

float uintToFloat(uint x)
{
    return uintBitsToFloat(0x3F800000 | (x >> 9)) - 1.0f;
}

uint xorshift(inout uint rngState)
{
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return rngState;
}

float rand(inout uint rngState)
{
    return uintToFloat(xorshift(rngState));
}

#endif // SHADER_CORE_RANDOM

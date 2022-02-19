#ifndef SHADER_CORE_SAMPLING
#define SHADER_CORE_SAMPLING

#include "random.glsl"

const float M_PI = 3.14159265359;

void sampleCameraRay(out vec3 origin, out vec3 direction, CameraParameters cam, inout uint rngState) {
    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(rand(rngState), rand(rngState));
    const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec2 d = inUV * 2.0 - 1.0;

    origin = (cam.viewInverse * vec4(0,0,0,1)).xyz;
    vec4 target = cam.projInverse * vec4(d.x, d.y, 1, 1);
    direction = (cam.viewInverse * vec4(normalize(target.xyz / target.w), 0)).xyz;
}

vec3 sampleUniformHemisphere(vec3 normal, vec3 tangent, vec3 binormal, inout uint rngState) {
    float r1 = rand(rngState);
    float r2 = rand(rngState);
    float sq = sqrt(1.0 - r2);

    vec3 direction = vec3(cos(2 * M_PI * r1) * sq, sin(2 * M_PI * r1) * sq, sqrt(r2));
    direction      = direction.x * tangent + direction.y * binormal + direction.z * normal;

    return direction;
}

float uniformHemispherePDF() {
    return 1.0f / (2.0f * M_PI);
}

vec2 sampleUniformDiskConcentric(inout uint rngState) {
    // from PBRT v4
    // Map _u_ to $[-1,1]^2$ and handle degeneracy at the origin
    vec2 u = vec2(rand(rngState), rand(rngState));
    vec2 uOffset = 2.0f * u - vec2(1.0f, 1.0f);
    if (uOffset.x == 0.0f && uOffset.y == 0.0f)
        return vec2(0.0f);

    // Apply concentric mapping to point
    float theta, r;
    if (abs(uOffset.x) > abs(uOffset.y)) {
        r = uOffset.x;
        theta = (M_PI / 4.0f) * (uOffset.y / uOffset.x);
    } else {
        r = uOffset.y;
        theta = (M_PI / 2.0f) - (M_PI / 4.0f) * (uOffset.x / uOffset.y);
    }
    return r * vec2(cos(theta), sin(theta));
}

vec3 sampleCosineHemisphere(vec3 normal, vec3 tangent, vec3 binormal, inout uint rngState) {
    // from PBRT v4
    vec2 d = sampleUniformDiskConcentric(rngState);
    float z2 = min(1.0f, dot(d, d));
    float z = sqrt(1.0f - z2);

    vec3 direction = d.x * tangent + d.y * binormal + z * normal;
    return direction;
}

float cosineHemispherePDF(float cosTheta) {
    return cosTheta / M_PI;
}

#endif // SHADER_CORE_SAMPLING

#ifndef SHADER_RT_UTILS
#define SHADER_RT_UTILS

vec3 face_forward(vec3 direction, vec3 normal)
{
    if (dot(normal, direction) > 0.0f) normal *= -1;
    return normal;
}

void compute_default_basis(const vec3 normal, out vec3 tangent, out vec3 binormal)
{
  if(abs(normal.x) > abs(normal.y))
    tangent = vec3(normal.z, 0, -normal.x) / sqrt(normal.x * normal.x + normal.z * normal.z);
  else
    tangent = vec3(0, -normal.z, normal.y) / sqrt(normal.y * normal.y + normal.z * normal.z);
  binormal = cross(normal, tangent);
}

#endif // SHADER_RT_UTILS

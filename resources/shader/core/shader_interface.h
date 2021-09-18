
#ifndef SHADER_HOST_INTERFACE
#define SHADER_HOST_INTERFACE

#ifdef __cplusplus
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

// GLSL Type
namespace shader_types {
    using vec2 = glm::vec2;
    using vec3 = glm::vec3;
    using vec4 = glm::vec4;
    using mat4 = glm::mat4;
    using uint = unsigned int;
}

#define CONSTANT constexpr
#define START_INTERFACE(name) \
namespace name { \
    using namespace shader_types;
#define END_INTERFACE }
#define START_CONSTANTS(name) \
    enum class name : uint32_t {
#define END_CONSTANTS() }

#else

#define CONSTANT const
#define START_INTERFACE(name)
#define END_INTERFACE
#define START_BINDING(a) CONSTANT uint
#define END_BINDING()

#endif

#endif // SHADER_HOST_INTERFACE

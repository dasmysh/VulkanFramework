cmake_minimum_required(VERSION 3.15)

project(VKFW CXX)

set(APPLICATION_NAME vkfw)

include(extern/vkfw_core/cmake/StandardProjectSettings.cmake)

# enable doxygen
include(extern/vkfw_core/cmake/Doxygen.cmake)
enable_doxygen()

# allow for static analysis options
include(extern/vkfw_core/cmake/StaticAnalyzers.cmake)

option(BUILD_SHARED_LIBS "Enable compilation of shared libraries" OFF)
option(ENABLE_TESTING "Enable Test Builds" ON)

option(ENABLE_PCH "Enable Precompiled Headers" OFF)
if (ENABLE_PCH)
  # This sets a global PCH parameter, each project will build its own PCH, which
  # is a good idea if any #define's change
  # 
  # 
  target_precompile_headers(vkfw_options INTERFACE <vector> <string> <map> <utility>)
endif()

set(CONAN_DISABLE_CHECK_COMPILER ON)
include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)


get_filename_component(VKFW_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR} REALPATH)
set(VKFW_WINDOW_WIDTH "1920" CACHE STRING "Application main window width.")
set(VKFW_WINDOW_HEIGHT "1080" CACHE STRING "Application main window height.")
set(VKFW_RESOURCE_DIR "resources" CACHE STRING "Resources directory in project structure.")
set(VKFW_EVAL_DIR "evaluation" CACHE STRING "Base path for evaluation.")
set(VKFW_RESOURCE_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${VKFW_RESOURCE_DIR})
set(VKFW_EVAL_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${VKFW_EVAL_DIR})

add_subdirectory(extern/vkfw_core)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.xml")
   configure_file("${CMAKE_SOURCE_DIR}/${PROJECT_NAME}Config.xml" "${PROJECT_NAME}Config.xml")
endif()

if(ENABLE_TESTING)
  enable_testing()
  message(
    "Building Tests. Be sure to check out test/constexpr_tests for constexpr testing"
  )
  add_subdirectory(test)
endif()

add_subdirectory(src)

get_property(GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(GENERATOR_IS_MULTI_CONFIG)
    foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
        configure_file("RDSettings.cap" "${CONFIG_TYPE}Settings.cap")
    endforeach()
endif()

file(GLOB_RECURSE MYSHBIN_FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/${VKFW_RESOURCE_DIR}/models/*.myshbin)
add_custom_target("clean_binary" COMMAND ${CMAKE_COMMAND} -E remove ${MYSHBIN_FILES} ${COMPILED_SHADERS})

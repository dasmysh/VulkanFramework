file(RELATIVE_PATH PROJECT_REL_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE RES_FILES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.*)
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.vert
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.tesc
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.tese
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.geom
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.frag
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.comp
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.mesh
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.task
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.rgen
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.rint
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.rahit
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.rchit
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.rmiss
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.rcall)
file(GLOB_RECURSE GEN_SHADER_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.vert
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.tesc
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.tese
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.geom
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.frag
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.comp
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.mesh
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.task
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.rgen
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.rint
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.rahit
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.rchit
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.rmiss
    ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.gen.rcall)
if(NOT "${GEN_SHADER_FILES}" STREQUAL "")
    list(REMOVE_ITEM SHADER_FILES ${GEN_SHADER_FILES})
    list(REMOVE_ITEM RES_FILES ${GEN_SHADER_FILES})
    source_group(TREE ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR} PREFIX "resources\\generated_shaders" FILES ${GEN_SHADER_FILES})
endif()
set(COMPILED_SHADERS "")
foreach(SHADER ${SHADER_FILES})
    compile_spirv_shader("${SHADER}" "${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/shader;${VKFWCORE_RESOURCE_BASE_PATH}/shader")
    list(APPEND COMPILED_SHADERS ${COMPILE_SPIRV_SHADER_RETURN})
endforeach()
if(NOT "${COMPILED_SHADERS}" STREQUAL "")
    list(REMOVE_ITEM RES_FILES ${COMPILED_SHADERS})
    source_group(TREE ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR} PREFIX "resources\\compiled_shaders" FILES ${COMPILED_SHADERS})
endif()
source_group(TREE ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR} PREFIX "resources" FILES ${RES_FILES})


file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    *.h
    *.hpp
    *.inl
    *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_FILES})


file(GLOB_RECURSE INCLUDE_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}/*.h
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}/*.hpp
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}/*.inl)
source_group(TREE ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH} FILES ${INCLUDE_FILES})

file(GLOB TOP_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/Readme.md
    ${PROJECT_SOURCE_DIR}/extern/vkfw_core/.clang-tidy
    ${PROJECT_SOURCE_DIR}/extern/vkfw_core/.clang-format
    ${CMAKE_BINARY_DIR}/glm.natvis
    ${CMAKE_BINARY_DIR}/imgui.natvis
    ${CMAKE_BINARY_DIR}/imgui.gdb
    ${CMAKE_BINARY_DIR}/imgui.natstepfilter)
source_group(" " FILES ${TOP_FILES})


file(GLOB_RECURSE RES_OBJ_FILES ${PROJECT_SOURCE_DIR}/${VKFW_RESOURCE_DIR}/*.obj)
foreach(f ${RES_OBJ_FILES})
    set_source_files_properties(${f} PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()

add_custom_target(compile_shaders DEPENDS ${COMPILED_SHADERS})

add_executable(${APPLICATION_NAME} ${SRC_FILES} ${INCLUDE_FILES} ${EXTERN_SOURCES} ${TOP_FILES} ${RES_FILES})
target_link_libraries(${APPLICATION_NAME} PRIVATE vk_framework_core)
add_dependencies(${APPLICATION_NAME} compile_shaders)
target_include_directories(${APPLICATION_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}
    ${PROJECT_SOURCE_DIR}/resources/shader
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/extern/imgui/cpp)
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${APPLICATION_NAME})
set_target_properties(${APPLICATION_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

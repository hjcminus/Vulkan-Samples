file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

set(SHADER_DIR "../../shaders/cascaded_shadow_maps")
file(GLOB SHADERS "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag" "${SHADER_DIR}/*.glsl")

source_group("shaders" FILES ${SHADERS})
add_executable(cascaded_shadow_maps ${SOURCES} ${HEADERS} ${SHADERS})
set_property(TARGET cascaded_shadow_maps PROPERTY FOLDER "samples")

target_include_directories(cascaded_shadow_maps PRIVATE $ENV{VULKAN_SDK}/Include)
target_include_directories(cascaded_shadow_maps PRIVATE ${PROJECT_SOURCE_DIR}/third_party/glm-1.0.1)
target_link_directories(cascaded_shadow_maps PRIVATE $ENV{VULKAN_SDK}/Lib)

target_link_libraries(cascaded_shadow_maps PRIVATE common vulkan-1)

if (WIN32)
  # set using UNICODE characterset
  add_definitions(-DUNICODE -D_UNICODE)
endif()

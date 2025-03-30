file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

set(SHADER_DIR "../../data/shaders/shadow_map")
file(GLOB SHADERS "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag")

source_group("shaders" FILES ${SHADERS})
add_executable(shadow_map ${SOURCES} ${HEADERS} ${SHADERS})
set_property(TARGET shadow_map PROPERTY FOLDER "samples")

target_include_directories(shadow_map PRIVATE $ENV{VULKAN_SDK}/Include)
target_include_directories(shadow_map PRIVATE ${PROJECT_SOURCE_DIR}/third_party/glm-1.0.1)
target_link_directories(shadow_map PRIVATE $ENV{VULKAN_SDK}/Lib)

target_link_libraries(shadow_map PRIVATE common vulkan-1)

if (WIN32)
  # set using UNICODE characterset
  add_definitions(-DUNICODE -D_UNICODE)
endif()

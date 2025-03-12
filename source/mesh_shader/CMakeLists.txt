file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

set(SHADER_DIR "../../data/shaders/mesh_shader")
file(GLOB SHADERS "${SHADER_DIR}/*.task" "${SHADER_DIR}/*.mesh" "${SHADER_DIR}/*.frag")

source_group("shaders" FILES ${SHADERS})
add_executable(mesh_shader ${SOURCES} ${HEADERS} ${SHADERS})

target_include_directories(mesh_shader PRIVATE $ENV{VULKAN_SDK}/Include)
target_include_directories(mesh_shader PRIVATE ${PROJECT_SOURCE_DIR}/third_party/glm-1.0.1)
target_link_directories(mesh_shader PRIVATE $ENV{VULKAN_SDK}/Lib)

target_link_libraries(mesh_shader PRIVATE common)

if (WIN32)
  # set using UNICODE characterset
  add_definitions(-DUNICODE -D_UNICODE)
endif()

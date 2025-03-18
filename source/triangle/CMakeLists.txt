file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

set(SHADER_DIR "../../data/shaders/triangle")
file(GLOB SHADERS "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag")

source_group("shaders" FILES ${SHADERS})
add_executable(triangle ${SOURCES} ${HEADERS} ${SHADERS})

target_include_directories(triangle PRIVATE $ENV{VULKAN_SDK}/Include)
target_include_directories(triangle PRIVATE ${PROJECT_SOURCE_DIR}/third_party/glm-1.0.1)
target_link_directories(triangle PRIVATE $ENV{VULKAN_SDK}/Lib)

target_link_libraries(triangle PRIVATE common vulkan-1)

if (WIN32)
  # set using UNICODE characterset
  add_definitions(-DUNICODE -D_UNICODE)
endif()

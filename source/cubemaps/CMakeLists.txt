file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

set(SHADER_DIR "../../shaders/cubemaps")
file(GLOB SHADERS "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag" "${SHADER_DIR}/*.glsl")

source_group("shaders" FILES ${SHADERS})
add_executable(cubemaps ${SOURCES} ${HEADERS} ${SHADERS})
set_property(TARGET cubemaps PROPERTY FOLDER "samples")	# sampler filter

target_include_directories(cubemaps PRIVATE $ENV{VULKAN_SDK}/Include)
target_include_directories(cubemaps PRIVATE ${PROJECT_SOURCE_DIR}/third_party/glm-1.0.1)
target_link_directories(cubemaps PRIVATE $ENV{VULKAN_SDK}/Lib)

target_link_libraries(cubemaps PRIVATE common vulkan-1)

if (WIN32)
  # set using UNICODE characterset
  add_definitions(-DUNICODE -D_UNICODE)
endif()

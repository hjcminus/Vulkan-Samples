file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_library(common SHARED ${SOURCES} ${HEADERS})

add_definitions(-DCOMMON_MODULE)

target_include_directories(common PRIVATE $ENV{VULKAN_SDK}/Include)
target_include_directories(common PRIVATE ${PROJECT_SOURCE_DIR}/third_party/glm-1.0.1)
target_link_directories(common PRIVATE $ENV{VULKAN_SDK}/Lib)

if (WIN32)
  # set using UNICODE characterset
  add_definitions(-DUNICODE -D_UNICODE)
endif()

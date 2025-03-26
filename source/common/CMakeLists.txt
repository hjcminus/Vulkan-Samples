if (WIN32)
  # set using UNICODE characterset
  add_definitions(-DUNICODE -D_UNICODE)
endif()

file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")
list(REMOVE_ITEM SOURCES "./test.cpp")

add_library(common SHARED ${SOURCES} ${HEADERS})
set_property(TARGET common PROPERTY FOLDER "common")

target_compile_definitions(common PRIVATE -DCOMMON_MODULE)

target_include_directories(common PRIVATE $ENV{VULKAN_SDK}/Include)
target_include_directories(common PRIVATE ${PROJECT_SOURCE_DIR}/third_party/glm-1.0.1)
target_include_directories(common PRIVATE ${PROJECT_SOURCE_DIR}/third_party/lpng1647)
target_include_directories(common PRIVATE ${PROJECT_BINARY_DIR}/third_party/lpng1647)
target_link_directories(common PRIVATE $ENV{VULKAN_SDK}/Lib)
target_link_libraries(common PRIVATE vulkan-1 png_static)

add_executable(test "./test.cpp")
set_property(TARGET test PROPERTY FOLDER "common")

target_include_directories(test PRIVATE $ENV{VULKAN_SDK}/Include)
target_include_directories(test PRIVATE ${PROJECT_SOURCE_DIR}/third_party/glm-1.0.1)
target_include_directories(test PRIVATE ${PROJECT_SOURCE_DIR}/third_party/lpng1647)
target_include_directories(test PRIVATE ${PROJECT_BINARY_DIR}/third_party/lpng1647)
target_link_directories(test PRIVATE $ENV{VULKAN_SDK}/Lib)
target_link_libraries(test PRIVATE common)

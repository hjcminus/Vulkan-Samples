file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_library(common SHARED ${SOURCES} ${HEADERS})

add_definitions(-DCOMMON_MODULE)

if (WIN32)
  # set using UNICODE characterset
  add_definitions(-DUNICODE -D_UNICODE)
endif()

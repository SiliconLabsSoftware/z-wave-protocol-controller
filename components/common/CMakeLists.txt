# Common library
# Provides project-wide headers like sl_status.h and zpc_version.h
# Also provides clock_platform implementation
set(COMMON_SRC
    src/clock_platform.cpp
)

add_library(common OBJECT ${COMMON_SRC})

target_include_directories(
  common
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:include>)


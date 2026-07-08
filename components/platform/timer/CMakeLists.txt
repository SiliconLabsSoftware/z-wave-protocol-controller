add_library(timer STATIC src/timer.cpp)

target_include_directories(
  timer
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  PRIVATE src)

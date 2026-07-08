# Main component provides header-only interface (init_builder.hpp)
set(MAIN_INCLUDE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/>)
add_library(main INTERFACE)
target_include_directories(
  main
  INTERFACE ${MAIN_INCLUDE} $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

# Dependencies for init_builder.hpp
target_link_libraries(
  main
  INTERFACE
    log
    threading)
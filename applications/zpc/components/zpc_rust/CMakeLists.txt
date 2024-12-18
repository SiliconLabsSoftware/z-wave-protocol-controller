# Use libunify from dep

if(NOT DEFINED COMMON_LOCATION)
  set(COMMON_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/../../../../_deps/unifysdk-src")
endif()

# TODO: prefer https://cmake.org/cmake/help/latest/command/add_custom_target.html
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory
  ${CMAKE_SOURCE_DIR}/components
)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
  ${COMMON_LOCATION}/components/uic_rust
  ${CMAKE_SOURCE_DIR}/components/uic_rust
)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
  ${COMMON_LOCATION}/components/unify_attribute_poll
  ${CMAKE_SOURCE_DIR}/components/unify_attribute_poll
)

cargo_build(NAME zpc_rust_build MANIFEST_PATH ${CMAKE_CURRENT_LIST_DIR}
            DISABLE_TESTS)
cargo_as_cmake_target(zpc_rust_build zpc_rust)

# TODO: include UNIFY_BINARY_DIR
add_dependencies(zpc_rust unify)

install(
  FILES zwave_poll_config.yaml
  DESTINATION share/uic
  COMPONENT uic-zpc)

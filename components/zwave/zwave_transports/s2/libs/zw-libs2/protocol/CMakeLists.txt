include_directories( ${CMAKE_CURRENT_BINARY_DIR} . )

# Add protocol unit tests if testing is enabled
if(NOT DEFINED SKIP_TESTING OR CMAKE_BUILD_TYPE STREQUAL Test)
  add_subdirectory(test)
endif()

include_directories( ${CMAKE_CURRENT_BINARY_DIR} . )

# Add protocol unit tests if testing is enabled
if(UNIT_TEST OR CMAKE_BUILD_TYPE STREQUAL Test
   OR CMAKE_PROJECT_NAME STREQUAL "libs2")
  add_subdirectory(test)
endif()

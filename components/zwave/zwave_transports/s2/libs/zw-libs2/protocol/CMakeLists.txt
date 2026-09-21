include_directories( ${CMAKE_CURRENT_BINARY_DIR} . )

if(UNIT_TEST)
  add_subdirectory(test)
endif()

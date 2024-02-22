SET(SOURCES unity.c)

# unity target may be imported be several projects, e.g. zw-protocol and libs2 simultaneously.
# This test ensures that the target named 'unity' will only be declared if it is not already declared.
if (NOT TARGET unity)
  add_library(unity ${SOURCES} )
  target_compile_definitions(unity PUBLIC "-DUNITY_EXCLUDE_FLOAT")
  target_include_directories(unity PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif (NOT TARGET unity)


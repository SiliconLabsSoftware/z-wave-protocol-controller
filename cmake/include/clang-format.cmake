# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-License-Identifier: ZLIB

file(GLOB_RECURSE sources_list *.c *.cpp *.h *.hpp)
list(FILTER sources_list EXCLUDE REGEX ".git/.*")
list(FILTER sources_list EXCLUDE REGEX "${CMAKE_BINARY_DIR}/.*")
list(FILTER sources_list EXCLUDE REGEX ".*/ZW_classcmd.h")
if(NOT ENV{uncrustify_args})
  set(uncrustify_args --replace)
endif()

string(REPLACE ";" "\n" sources_list_text "${sources_list}" )
file(WRITE "${CMAKE_BINARY_DIR}/sources.lst" ${sources_list_text})

add_custom_target(lint
  COMMAND clang-format -i  --files="${CMAKE_BINARY_DIR}/sources.lst"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"    
  DEPENDS "${CMAKE_BINARY_DIR}/sources.lst"
)

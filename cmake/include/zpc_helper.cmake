set(DIR_OF_ZPC_HELPER_CMAKE
    ${CMAKE_CURRENT_LIST_DIR}
    CACHE INTERNAL "DIR_OF_ZPC_HELPER_CMAKE")

function(collect_object_files TARGET TARGET_OBJS EXCLUDES)
  set(TARGET_OBJS
      $<TARGET_OBJECTS:${TARGET}>
      PARENT_SCOPE)
  if(NOT "${EXCLUDES}" STREQUAL "")
    list(LENGTH EXCLUDES length)
    math(EXPR list_last "${length} - 1")
    list(GET EXCLUDES ${list_last} last_element)

    foreach(ex ${EXCLUDES})
      string(REPLACE "." "\\." cleaned_ex ${ex})
      string(APPEND regex ".*${cleaned_ex}\\.o.*")
      if(${length} GREATER 1 AND NOT "${last_element}" STREQUAL "${ex}")
        string(APPEND regex "|")
      endif()
    endforeach()

    set(TARGET_OBJS
        $<FILTER:$<REMOVE_DUPLICATES:$<TARGET_OBJECTS:${TARGET}>>,EXCLUDE,${regex}>
        PARENT_SCOPE)
  endif()
endfunction()

# helper tags to pack static libraries as a whole library. meaning all symbols
# are imported.
if(APPLE)
  set(WHOLE_ARCHIVE_START -Wl,-undefined -Wl,dynamic_lookup -Wl,-all_load)
  set(WHOLE_ARCHIVE_END "")
else()
  set(WHOLE_ARCHIVE_START -Wl,--whole-archive)
  set(WHOLE_ARCHIVE_END -Wl,--no-whole-archive)
endif()

# This function is able to collect a bunch of static liberries (.a) and bundle
# them into a shared library. All libraries must be defined cmake targets, ie
# defined using the add_library() command
#
# Usage: zpc_add_shared_library( <output_lib> <lib1> <lib2> ... <libN>)
function(zpc_add_shared_library)
  # Parse the arguments
  list(POP_FRONT ARGV LIBNAME)
  set(LIBS ${ARGV})
  # Gather a list of genrator expressions which can resolve the achive file
  # belonging to the target
  foreach(L ${LIBS})
    get_target_property(TARGET_TYPE ${L} TYPE)
    if(NOT (${TARGET_TYPE} MATCHES "INTERFACE_LIBRARY"))
      list(APPEND GENERATOR_EXPRS "$<TARGET_OBJECTS:${L}>")
    endif()

    get_target_property(includes ${L} INTERFACE_INCLUDE_DIRECTORIES)
    if(includes)
      list(APPEND target-includes "${includes}")
    endif()

    set_property(TARGET ${L} PROPERTY POSITION_INDEPENDENT_CODE 1)
  endforeach()

  list(REMOVE_DUPLICATES target-includes)

  # We just make an empty C file to keep Cmake happy
  set(DUMMYFILE ${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}_dummy.cpp)
  file(TOUCH ${DUMMYFILE})
  add_library(${LIBNAME} SHARED ${DUMMYFILE})
  add_dependencies(${LIBNAME} ${LIBS})
  target_include_directories(${LIBNAME} PUBLIC ${target-includes})
  target_link_options(${LIBNAME} PRIVATE ${WHOLE_ARCHIVE_START}
                      ${GENERATOR_EXPRS} ${WHOLE_ARCHIVE_END})
endfunction()
if(NOT TARGET sl_status_strings_lib)
  set(SL_STATUS_STRINGS_SCRIPT "")
  set(SL_STATUS_STRINGS_DIR "")

  if(EXISTS ${COMMON_LOCATION})
    set(SL_STATUS_STRINGS_SCRIPT ${COMMON_LOCATION}/scripts/generate_sl_status_strings/main.py)
    set(SL_STATUS_STRINGS_DIR ${COMMON_LOCATION}/components/common/include)
  else()
    set(SL_STATUS_STRINGS_SCRIPT ${DIR_OF_ZPC_HELPER_CMAKE}/../../scripts/generate_sl_status_strings/main.py)
    set(SL_STATUS_STRINGS_DIR ${DIR_OF_ZPC_HELPER_CMAKE}/../../components/common/include)
  endif()

  find_package(Python3 COMPONENTS Interpreter REQUIRED)

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src)

  message(NOTICE "-- Generating sl_status_strings")
  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${SL_STATUS_STRINGS_SCRIPT} ${SL_STATUS_STRINGS_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}
    RESULT_VARIABLE SL_STATUS_STRINGS_RESULT
    ERROR_VARIABLE SL_STATUS_STRINGS_ERROR)
  if(NOT SL_STATUS_STRINGS_RESULT EQUAL 0)
    message(FATAL_ERROR "sl_status_strings generation failed: ${SL_STATUS_STRINGS_ERROR}")
  endif()

  add_library(sl_status_strings_lib STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/src/sl_status_strings.c
  )
  target_include_directories(sl_status_strings_lib PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${SL_STATUS_STRINGS_DIR}
  )
  if(TARGET common)
    target_link_libraries(sl_status_strings_lib PUBLIC common)
  endif()
endif()

# Compile FetchContent / vendored third-party code without ZPC flags
# (-Werror, sanitizers, auto-var-init, stack protector).
#
# FetchContent_MakeAvailable() uses add_subdirectory() and inherits this
# directory's COMPILE_OPTIONS / LINK_OPTIONS plus CMAKE_<LANG>_FLAGS*.
# Push/pop around those adds so Paho, fmt, SQLite, gtest, and Unity keep
# their own compiler settings. Same build tree; not a second CMake project.

if(COMMAND zpc_push_external_compiler_flags)
  return()
endif()

macro(zpc_push_external_compiler_flags)
  if(DEFINED _ZPC_EXT_FLAGS_PUSHED)
    message(FATAL_ERROR "zpc_push_external_compiler_flags is already active")
  endif()
  set(_ZPC_EXT_FLAGS_PUSHED TRUE)

  get_directory_property(_ZPC_EXT_COMPILE_OPTIONS COMPILE_OPTIONS)
  get_directory_property(_ZPC_EXT_LINK_OPTIONS LINK_OPTIONS)
  if(NOT _ZPC_EXT_COMPILE_OPTIONS)
    set(_ZPC_EXT_COMPILE_OPTIONS "")
  endif()
  if(NOT _ZPC_EXT_LINK_OPTIONS)
    set(_ZPC_EXT_LINK_OPTIONS "")
  endif()
  set_directory_properties(PROPERTIES COMPILE_OPTIONS "" LINK_OPTIONS "")

  set(_ZPC_EXT_CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  set(_ZPC_EXT_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(_ZPC_EXT_CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
  set(_ZPC_EXT_CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  set(_ZPC_EXT_CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
  set(_ZPC_EXT_CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

  set(CMAKE_C_FLAGS "")
  set(CMAKE_CXX_FLAGS "")
  set(CMAKE_C_FLAGS_DEBUG "-g")
  set(CMAKE_CXX_FLAGS_DEBUG "-g")
  set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endmacro()

macro(zpc_pop_external_compiler_flags)
  if(NOT DEFINED _ZPC_EXT_FLAGS_PUSHED)
    message(FATAL_ERROR "zpc_pop_external_compiler_flags without a matching push")
  endif()

  set(CMAKE_C_FLAGS "${_ZPC_EXT_CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${_ZPC_EXT_CMAKE_CXX_FLAGS}")
  set(CMAKE_C_FLAGS_DEBUG "${_ZPC_EXT_CMAKE_C_FLAGS_DEBUG}")
  set(CMAKE_CXX_FLAGS_DEBUG "${_ZPC_EXT_CMAKE_CXX_FLAGS_DEBUG}")
  set(CMAKE_C_FLAGS_RELEASE "${_ZPC_EXT_CMAKE_C_FLAGS_RELEASE}")
  set(CMAKE_CXX_FLAGS_RELEASE "${_ZPC_EXT_CMAKE_CXX_FLAGS_RELEASE}")

  set_directory_properties(
    PROPERTIES COMPILE_OPTIONS "${_ZPC_EXT_COMPILE_OPTIONS}"
               LINK_OPTIONS "${_ZPC_EXT_LINK_OPTIONS}")

  unset(_ZPC_EXT_FLAGS_PUSHED)
  unset(_ZPC_EXT_COMPILE_OPTIONS)
  unset(_ZPC_EXT_LINK_OPTIONS)
  unset(_ZPC_EXT_CMAKE_C_FLAGS)
  unset(_ZPC_EXT_CMAKE_CXX_FLAGS)
  unset(_ZPC_EXT_CMAKE_C_FLAGS_DEBUG)
  unset(_ZPC_EXT_CMAKE_CXX_FLAGS_DEBUG)
  unset(_ZPC_EXT_CMAKE_C_FLAGS_RELEASE)
  unset(_ZPC_EXT_CMAKE_CXX_FLAGS_RELEASE)
endmacro()

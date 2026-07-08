if(NOT VERSION_CMAKE_FILE)
  set(VERSION_CMAKE_FILE ${CMAKE_CURRENT_LIST_DIR})
else()
  return()
endif()

# -------- ZPC Version and Git SHA Extraction Logic --------
#
# 1. If GIT_VERSION is not already set, try to obtain it using "git describe"
#    and GIT_VERSION_SHA using "git rev-parse --short HEAD".
#    If Git is not available, configuration fails with a FATAL_ERROR.
#
# 2. GIT_VERSION must match the pattern v<MAJOR>.<MINOR>.<REV>
#    (e.g. "v1.2.3"). If it does not, configuration fails with a FATAL_ERROR.
#
# 3. Extract MAJOR, MINOR, REV values from GIT_VERSION for CMAKE_PROJECT_VERSION.

if("${GIT_VERSION}" STREQUAL "")
  find_package(Git)
  if(Git_FOUND)
    message(STATUS "Git found: ${GIT_EXECUTABLE}")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe
      OUTPUT_VARIABLE GIT_VERSION
      ERROR_VARIABLE GIT_DESCRIBE_ERROR
      RESULT_VARIABLE GIT_DESCRIBE_RESULT
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      OUTPUT_VARIABLE GIT_VERSION_SHA
      ERROR_QUIET
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # Remove newline characters from GIT_VERSION and GIT_VERSION_SHA
    string(REGEX REPLACE "\n$" "" GIT_VERSION "${GIT_VERSION}")
    string(REGEX REPLACE "\n$" "" GIT_VERSION_SHA "${GIT_VERSION_SHA}")
    string(REGEX REPLACE "\n$" "" GIT_DESCRIBE_ERROR "${GIT_DESCRIBE_ERROR}")

    if(NOT GIT_DESCRIBE_RESULT EQUAL 0)
      message(FATAL_ERROR "git describe failed (exit code ${GIT_DESCRIBE_RESULT}): ${GIT_DESCRIBE_ERROR}")
    endif()
  else()
    message(FATAL_ERROR "Git not found")
  endif()
endif()

# If GIT_VERSION is not in format v1.2.3
string(REGEX MATCH "v[0-9]+\\.[0-9]+\\.[0-9]+" OUT "${GIT_VERSION}")
if("${OUT}" STREQUAL "")
  message(FATAL_ERROR "Invalid tag format: ${GIT_VERSION}. It should be v<major>.<minor>.<revision> For e.g. v1.2.3")
else()
  #Generate MAJOR MINOR REV and PATCH from GIT_VERSION for CMAKE_PROJECT_VERSION
  string(REGEX REPLACE ".*v([0-9]+).*" "\\1" VERSION_MAJOR "${GIT_VERSION}")
  string(REGEX REPLACE ".*v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${GIT_VERSION}")
  string(REGEX REPLACE ".*v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_REV "${GIT_VERSION}")
endif()

message(NOTICE "-- Build: ${GIT_VERSION}")
message(NOTICE "-- Git SHA: ${GIT_VERSION_SHA}")

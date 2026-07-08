# Fetch and build Paho MQTT C++ library using FetchContent (static)
#
# Sets the following variables:
# PAHO_MQTT_CPP_INCLUDE_DIR - where to find mqtt/client.h, etc.
# PAHO_MQTT_CPP_LIBRARIES   - List of libraries (CMake targets).
# PAHO_MQTT_CPP_FOUND       - True if Paho MQTT C++ was fetched and built.
# PAHO_MQTT_C_INCLUDE_DIR   - where to find MQTTClient.h, etc.

if(PAHO_MQTT_CPP_FOUND)
  return()
endif()

message(NOTICE "-- Finding Paho MQTT C++")

include(FetchContent)

FetchContent_Declare(
  paho-mqtt-cpp
  GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.cpp.git
  GIT_TAG        v1.5.3
  GIT_SHALLOW    TRUE
)

# Avoid re-downloading on subsequent configures
set(FETCHCONTENT_QUIET TRUE)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

# Configure build options before making available
set(PAHO_BUILD_SHARED FALSE CACHE BOOL "Build shared library" FORCE)
set(PAHO_BUILD_STATIC TRUE CACHE BOOL "Build static library" FORCE)
set(PAHO_WITH_SSL TRUE CACHE BOOL "Build with SSL support" FORCE)
set(PAHO_BUILD_EXAMPLES FALSE CACHE BOOL "Build examples" FORCE)
set(PAHO_BUILD_TESTS FALSE CACHE BOOL "Build tests" FORCE)
set(PAHO_WITH_MQTT_C TRUE CACHE BOOL "Build bundled Paho C library" FORCE)

# Temporarily disable deprecation warnings for external dependencies
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.4")
  cmake_diagnostic(GET CMD_DEPRECATED _PAHO_SAVE_DEPRECATED_ACTION)
  cmake_diagnostic(SET CMD_DEPRECATED IGNORE RECURSE)
else()
  if(CMAKE_WARN_DEPRECATED)
    set(_PAHO_SAVE_WARN_DEPRECATED ${CMAKE_WARN_DEPRECATED})
  else()
    set(_PAHO_SAVE_WARN_DEPRECATED ON)
  endif()
  set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(paho-mqtt-cpp)

# Restore deprecation warnings for our own code
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.4")
  cmake_diagnostic(SET CMD_DEPRECATED ${_PAHO_SAVE_DEPRECATED_ACTION} RECURSE)
else()
  set(CMAKE_WARN_DEPRECATED ${_PAHO_SAVE_WARN_DEPRECATED} CACHE BOOL "" FORCE)
endif()

# --- Resolve C++ target (prefer static) ---
set(PAHO_CPP_TARGET "")
if(TARGET paho-mqttpp3-static)
  set(PAHO_CPP_TARGET paho-mqttpp3-static)
elseif(TARGET paho-mqttpp3)
  set(PAHO_CPP_TARGET paho-mqttpp3)
endif()

# --- Resolve C target (prefer static async with SSL) ---
# The C++ library's mqtt::async_client wraps MQTTAsync_* from the async C library.
# We must link against the async variant (paho-mqtt3as = async+SSL, paho-mqtt3a = async).
# Linking the synchronous variant (paho-mqtt3c) causes duplicate common symbols
# (protocol, socket, signal handling) that corrupt async thread/signal management,
# breaking Ctrl+C handling and MQTT publish operations.
set(PAHO_C_TARGET "")
if(TARGET paho-mqtt3as-static)
  set(PAHO_C_TARGET paho-mqtt3as-static)
elseif(TARGET paho-mqtt3a-static)
  set(PAHO_C_TARGET paho-mqtt3a-static)
elseif(TARGET paho-mqtt3as)
  set(PAHO_C_TARGET paho-mqtt3as)
elseif(TARGET paho-mqtt3a)
  set(PAHO_C_TARGET paho-mqtt3a)
endif()

if(NOT PAHO_CPP_TARGET OR NOT PAHO_C_TARGET)
  message(FATAL_ERROR "Failed to build Paho MQTT C++ from source. Could not find required targets.")
endif()

# --- Include directories ---
FetchContent_GetProperties(paho-mqtt-cpp SOURCE_DIR PAHO_MQTT_CPP_SOURCE_DIR)
set(PAHO_MQTT_CPP_INCLUDE_DIR "${PAHO_MQTT_CPP_SOURCE_DIR}/include")

get_target_property(PAHO_MQTT_C_INCLUDE_DIR ${PAHO_C_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
if(NOT PAHO_MQTT_C_INCLUDE_DIR OR PAHO_MQTT_C_INCLUDE_DIR STREQUAL "PAHO_MQTT_C_INCLUDE_DIR-NOTFOUND")
  set(PAHO_MQTT_C_INCLUDE_DIR "${PAHO_MQTT_CPP_SOURCE_DIR}/externals/paho.mqtt.c/src")
endif()

# --- Suppress warnings for all paho-mqtt build targets ---
# Recursively find and suppress warnings in all FetchContent subdirectories.
# INTERFACE libraries are skipped because they have no compilation step and
# CMake does not allow PRIVATE compile options on them.
function(_paho_suppress_warnings_in dir)
  get_property(_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(_t ${_targets})
    get_target_property(_type ${_t} TYPE)
    if(NOT _type STREQUAL "INTERFACE_LIBRARY")
      target_compile_options(${_t} PRIVATE -w)
    endif()
  endforeach()
  get_property(_subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
  foreach(_sub ${_subdirs})
    _paho_suppress_warnings_in("${_sub}")
  endforeach()
endfunction()

FetchContent_GetProperties(paho-mqtt-cpp SOURCE_DIR _paho_src_dir)
_paho_suppress_warnings_in("${_paho_src_dir}")

# --- Export results ---
set(PAHO_MQTT_CPP_FOUND TRUE)
set(PAHO_MQTT_CPP_LIBRARIES ${PAHO_CPP_TARGET} ${PAHO_C_TARGET})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PahoMQTTCpp DEFAULT_MSG
  PAHO_MQTT_CPP_LIBRARIES PAHO_MQTT_CPP_INCLUDE_DIR PAHO_MQTT_C_INCLUDE_DIR)

mark_as_advanced(PAHO_MQTT_CPP_INCLUDE_DIR PAHO_MQTT_C_INCLUDE_DIR)

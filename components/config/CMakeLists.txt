# Dependencies
find_package(Yaml-cpp REQUIRED)
message(STATUS "YAML_CPP_LIBRARIES=${YAML_CPP_LIBRARIES}")

# Get include directories - try modern target first, then fallback
if(TARGET yaml-cpp::yaml-cpp)
  # Modern target-based approach - includes are automatically handled
  set(YAML_CPP_TARGET yaml-cpp::yaml-cpp)
else()
  # Fallback: find include directory manually
  if(YAML_CPP_INCLUDE_DIRS)
    set(YAML_CPP_INCLUDE_DIR ${YAML_CPP_INCLUDE_DIRS})
  else()
    # Try common installation paths
    find_path(YAML_CPP_INCLUDE_DIR yaml-cpp/yaml.h
      PATHS
        /opt/homebrew/include
        /usr/local/include
        /usr/include
        /opt/local/include
    )
    if(NOT YAML_CPP_INCLUDE_DIR)
      message(FATAL_ERROR "yaml-cpp include directory not found. Please install yaml-cpp development package.")
    endif()
  endif()
endif()

if(NOT YAML_CPP_LIBRARIES)
  # On most machines the yaml-cpp configuration doesnt set YAML_CPP_LIBRARIES,
  # YAML_CPP_FOUND and YAML_CPP_INCL_DIRS even though it seems that a find
  # package config is found and some environment is set. explicitly set
  # '-lyaml-cpp' so that we can fallback on PKG-CONFIG to resolve our dependency
  set(YAML_CPP_LIBRARIES "yaml-cpp")
endif()
if(APPLE)
  set(YAML_CPP_LIB_FILE "/opt/homebrew/lib/libyaml-cpp.dylib")
  message(STATUS "TODO: https://github.com/jbeder/yaml-cpp/issues/1202#issuecomment-3132557141 ${YAML_CPP_LIBRARIES}")
  if(${YAML_CPP_LIBRARIES} STREQUAL "yaml-cpp"
    AND EXISTS ${YAML_CPP_LIB_FILE})
    set(YAML_CPP_LIBRARIES ${YAML_CPP_LIB_FILE})
  endif()
endif()
message(STATUS "TODO: ${YAML_CPP_LIBRARIES} https://github.com/jbeder/yaml-cpp/issues/1202#")

# Library
set(CONFIG_SRC platform/${COMPATIBLE_PLATFORM}/config.cpp src/zpc_config.c platform/${COMPATIBLE_PLATFORM}/device_id.c)
set(CONFIG_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/include/)
set(CONFIG_DEPS ${YAML_CPP_LIBRARIES})
add_library(config ${CONFIG_SRC})
target_include_directories(
  config
  PUBLIC $<BUILD_INTERFACE:${CONFIG_INCLUDE}>
  PRIVATE platform/${COMPATIBLE_PLATFORM})
# Add yaml-cpp include directories if not using target-based approach
if(YAML_CPP_INCLUDE_DIR AND NOT TARGET yaml-cpp::yaml-cpp)
  target_include_directories(config PRIVATE ${YAML_CPP_INCLUDE_DIR})
endif()
# Link yaml-cpp - use target if available, otherwise use library variable
if(TARGET yaml-cpp::yaml-cpp)
  target_link_libraries(config yaml-cpp::yaml-cpp zwave_definitions common log)
else()
  target_link_libraries(config ${CONFIG_DEPS} zwave_definitions common log)
endif()
target_compile_definitions(
  config PRIVATE CMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}" CMAKE_PROJECT_VERSION="${CMAKE_PROJECT_VERSION}")

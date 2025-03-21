if(NOT ZPC_PACKAGE_HELPER_CMAKE)
  set(ZPC_PACKAGE_HELPER_CMAKE ${CMAKE_CURRENT_LIST_DIR})
else()
  return()
endif()

set(CPACK_COMPONENTS_GROUPING "IGNORE")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
set(CPACK_DEB_PACKAGE_COMPONENT ON)

# TODO: Not aligned to debian arch
if(NOT DEFINED FILE_NAME_VERSIONING_ARCH)
  set(FILE_NAME_VERSIONING_ARCH "${CMAKE_PROJECT_VERSION}_${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(CPACK_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}_${FILE_NAME_VERSIONING_ARCH}"
)

# Common CPACK configuration
set(CPACK_PACKAGING_INSTALL_PREFIX "")

# Generate source tarball
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
    "${PROJECT_SOURCE_DIR}/build.*/"
    "${PROJECT_SOURCE_DIR}/GeckoSDK.*/"
    "Jenkinsfile" "Earthfile"
    "*.nix" "justfile" ".envrc" "flake.lock"
    "\\\\.git/"
    "\\\\.gitattributes"
    "\\\\.gitignore"
    "\\\\.gitmodules"
    "\\\\.pre-commit-config.yaml"
    "${PROJECT_SOURCE_DIR}/externals"
    "sonar-project\\\\.properties"
    "${PROJECT_SOURCE_DIR}/scripts/ci"
    "${PROJECT_SOURCE_DIR}/scripts/internal"
    ".*\\\\.internal\\\\.md"
    "${PROJECT_SOURCE_DIR}/applications/examples/applications/example_mqtt_device")

# Generate Debian package
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Silicon Labs")

# Install headers of the specified target into the specified package. only .h
# files that reside inside the project root will be added, other items will be
# omitted.
function(install_headers TARGET DESTINATION PACKAGE)
  get_target_property(includes ${TARGET} INTERFACE_INCLUDE_DIRECTORIES)
  foreach(include ${includes})
    # strip out any includes that wrapped in BUILD_INTERFACE expressions if present
  string(REGEX REPLACE "\\s*(:?\\\$<BUILD_INTERFACE:)?([A-Za-z0-9_/\\\${}-]+)>?"
    "\\2" include
    "${include}")

    # omit include directories which are not part of this project. There are
    # cases where system directories are included. we obviously dont want to
    # export these headers.
    if((include MATCHES "${PROJECT_SOURCE_DIR}") OR (include MATCHES
                                                     "${PROJECT_BINARY_DIR}"))
      file(GLOB new_headers "${include}/*.h")
      file(GLOB new_hppheaders "${include}/*.hpp")
      list(APPEND headers ${new_headers} ${new_hppheaders})
    endif()
  endforeach()

  install(
    FILES ${headers}
    DESTINATION "include/${DESTINATION}"
    COMPONENT ${PACKAGE})
endfunction()

# Function arguments 1. PKG_NAME      component package Name 2. PKG_FILE_NAME
# Debian package filename for the component 3. PKG_DEPNDS    Component Debian
# package dependencies. 4. PKG_EXTRA     Component Debian package Control extra
# info See https://cmake.org/cmake/help/v3.6/module/CPackDeb.html for more info
macro(add_component_to_uic PKG_NAME PKG_DESCRIPTION PKG_FILE_NAME PKG_DEPNDS PKG_REPLACES PKG_EXTRA)
  if(NOT APPLE)
    string(TOUPPER ${PKG_NAME} PKG_NAME_UPPER)
    string(
      CONCAT CPACK_DEBIAN_${PKG_NAME_UPPER}_FILE_NAME
      "${PKG_FILE_NAME}_"
      "${FILE_NAME_VERSIONING_ARCH}"
      ".deb")

    if (NOT "${PKG_DEPNDS}" STREQUAL "")
      # Removing spaces generated by identation below
      string(REGEX REPLACE " " "" PKG_DEPNDS "${PKG_DEPNDS}")
      set(CPACK_DEBIAN_${PKG_NAME_UPPER}_PACKAGE_DEPENDS      ${PKG_DEPNDS}                               CACHE STRING "Package dependencies for ${PKG_NAME}: ${PKG_DEPNDS}"                    FORCE)
    endif()

    if (NOT "${PKG_REPLACES}" STREQUAL "")
      set(CPACK_DEBIAN_${PKG_NAME_UPPER}_PACKAGE_PROVIDES   ${PKG_REPLACES}                             CACHE STRING "Package ${PKG_NAME} provides ${PKG_REPLACES}"                           FORCE)
      set(CPACK_DEBIAN_${PKG_NAME_UPPER}_PACKAGE_REPLACES   ${PKG_REPLACES}                             CACHE STRING "Package ${PKG_NAME} replaces ${PKG_REPLACES}"                           FORCE)
      set(CPACK_DEBIAN_${PKG_NAME_UPPER}_PACKAGE_CONFLICTS  ${PKG_REPLACES}                             CACHE STRING "Package ${PKG_NAME} conflicts ${PKG_REPLACES}"                           FORCE)
    endif()

    set(CPACK_DEBIAN_${PKG_NAME_UPPER}_FILE_NAME            ${CPACK_DEBIAN_${PKG_NAME_UPPER}_FILE_NAME} CACHE STRING "Filename for ${PKG_NAME}: ${CPACK_DEBIAN_${PKG_NAME_UPPER}_FILE_NAME}"  FORCE)
    set(CPACK_DEBIAN_${PKG_NAME_UPPER}_PACKAGE_NAME         ${PKG_NAME}                                 CACHE STRING "Package name for ${PKG_NAME}"                                           FORCE)
    set(CPACK_DEBIAN_${PKG_NAME_UPPER}_DESCRIPTION_SUMMARY  "Unify component ${PKG_NAME}"               CACHE STRING "Summary for ${PKG_NAME}: Unify component ${PKG_NAME}"                   FORCE)
    set(CPACK_DEBIAN_${PKG_NAME_UPPER}_DESCRIPTION          "${PKG_DESCRIPTION}"                        CACHE STRING "Description for ${PKG_NAME}: ${PKG_DESCRIPTION}"                        FORCE)

    # Removing spaces generated by identation below
    # Saving the string back into itself does not work for some reason, hence TMP_EXTRA.
    string(REPLACE " " "" TMP_EXTRA "${PKG_EXTRA}")
    set(CPACK_DEBIAN_${PKG_NAME_UPPER}_PACKAGE_CONTROL_EXTRA ${TMP_EXTRA}                               CACHE STRING "Extras for ${PKG_NAME}: ${TMP_EXTRA}"                                   FORCE)

    #Remove the item incase it was already in list incase there was "ninja package" done before
    list(REMOVE_ITEM CPACK_COMPONENTS_ALL "${PKG_NAME}")
    list(APPEND CPACK_COMPONENTS_ALL "${PKG_NAME}")
    set(CPACK_COMPONENTS_ALL                                ${CPACK_COMPONENTS_ALL}                     CACHE STRING "Packages that will have Debian packages built: ${CPACK_COMPONENTS_ALL}" FORCE)

    install(
      FILES "${CMAKE_SOURCE_DIR}/copyright"
      DESTINATION share/doc/${PKG_NAME}
      COMPONENT ${PKG_NAME})
  endif()
endmacro()

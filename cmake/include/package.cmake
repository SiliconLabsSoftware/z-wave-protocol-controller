message(STATUS "Components of Unify which will have deb packages"
               ": ${CPACK_COMPONENTS_ALL}")

# TODO: Not aligned to debian arch
if(NOT DEFINED FILE_NAME_VERSIONING_ARCH)
  set(FILE_NAME_VERSIONING_ARCH "${CMAKE_PROJECT_VERSION}_${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Generate Debian package
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Silicon Labs")
set(CPACK_COMPONENTS_GROUPING "IGNORE")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
set(CPACK_DEB_PACKAGE_COMPONENT ON)
set(CPACK_SOURCE_GENERATOR "TGZ")
set(DEB_PACKAGE_FOLDER "${CMAKE_PROJECT_NAME}_${FILE_NAME_VERSIONING_ARCH}")
set(DEB_PACKAGE_ZIP "${DEB_PACKAGE_FOLDER}.zip")
set(DIST_FOLDER "dist")

add_custom_target(
  package_archive
  DEPENDS package
  COMMAND mkdir -p "${CMAKE_BINARY_DIR}/${DEB_PACKAGE_FOLDER}"
  COMMAND mv "${CMAKE_BINARY_DIR}/\*.deb" "${CMAKE_BINARY_DIR}/${DEB_PACKAGE_FOLDER}/"
  COMMAND cd "${CMAKE_BINARY_DIR}/${DEB_PACKAGE_FOLDER}"
    && dpkg-scanpackages . /dev/null > Packages
    && cd ..
  COMMAND zip --recurse-paths "${DEB_PACKAGE_ZIP}" "${DEB_PACKAGE_FOLDER}"
  COMMAND mkdir -p "${DIST_FOLDER}" && mv "${DEB_PACKAGE_ZIP}" "${DIST_FOLDER}"
  COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
  "${CPACK_GENERATOR} packages archived in ${DIST_FOLDER}/${DEB_PACKAGE_FOLDER}.zip"
)

if(PROJECT_IS_TOP_LEVEL)
  message(STATUS "cpack: Included from ${CMAKE_SOURCE_DIR}")
  include(CPack)

  foreach(PKG_NAME IN LISTS CPACK_COMPONENTS_ALL)
    string(TOUPPER ${PKG_NAME} PKG_NAME_UPPER)
    cpack_add_component(
      PKG_NAME
      DISPLAY_NAME ${PKG_NAME}
      DESCRIPTION ${CPACK_DEBIAN_${PKG_NAME_UPPER}_DESCRIPTION}
      INSTALL_TYPES Full)
  endforeach()
endif()

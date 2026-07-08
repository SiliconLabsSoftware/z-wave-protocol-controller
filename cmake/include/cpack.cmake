# ##############################################################################
# CPack: Debian .deb packaging (run: cmake --build <build-dir> --target package)
# Requires: CMAKE_INSTALL_PREFIX=/usr (env or -D) for FHS layout and
#           /var/lib/zpc install rule in applications/zpc/CMakeLists.txt.
# ##############################################################################

if(ZPC_CPACK_CONFIGURED)
  return()
endif()
set(ZPC_CPACK_CONFIGURED TRUE)

set(CPACK_PACKAGE_NAME "zpc")
set(CPACK_PACKAGE_VENDOR "Silicon Laboratories Inc.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Z-Wave Protocol Controller (zpc)")
set(CPACK_PACKAGE_DESCRIPTION
    "Z-Wave Protocol Controller (zpc) — application, libraries, and data paths for Z-Wave network management."
)
set(CPACK_PACKAGE_CONTACT "https://www.silabs.com")
set(CPACK_PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REV}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "zpc")

if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE.md")
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.md")
endif()

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Silicon Laboratories <https://www.silabs.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "electronics")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://www.silabs.com")

# Runtime deps: default is conservative for Debian bookworm/trixie; override with
# -D CPACK_DEBIAN_PACKAGE_DEPENDS=... at configure time if needed.
# SQLite is compiled from the official amalgamation via FetchContent (see
# cmake/modules/FindSQLite3.cmake), not linked as libsqlite3-0.
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libc6 (>= 2.31), libssl3 (>= 3.0.0), libyaml-cpp0.8 | libyaml-cpp0.7, libfmt9 | libfmt10"
    CACHE STRING "Debian package runtime dependencies (comma-separated list)")

# Architecture: cross toolchains (cmake/arm64_debian.cmake, armhf_debian.cmake) set this;
# native Linux builds resolve via dpkg when available.
if(NOT DEFINED CPACK_DEBIAN_PACKAGE_ARCHITECTURE OR CPACK_DEBIAN_PACKAGE_ARCHITECTURE STREQUAL "")
  if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    execute_process(
      COMMAND dpkg --print-architecture
      OUTPUT_VARIABLE _zpc_cpack_deb_arch
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
      RESULT_VARIABLE _zpc_dpkg_arch_rc)
    if(_zpc_dpkg_arch_rc EQUAL 0 AND _zpc_cpack_deb_arch)
      set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${_zpc_cpack_deb_arch}")
    endif()
  endif()
  if(NOT DEFINED CPACK_DEBIAN_PACKAGE_ARCHITECTURE OR CPACK_DEBIAN_PACKAGE_ARCHITECTURE STREQUAL "")
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
      set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
      set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE arm64)
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv7l|armv6l|arm")
      set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE armhf)
    else()
      set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${CMAKE_HOST_SYSTEM_PROCESSOR})
    endif()
  endif()
endif()

# Output: zpc-<arch>.deb (e.g. zpc-amd64.deb); CPack adds .deb for the DEB generator
set(CPACK_PACKAGE_FILE_NAME "zpc-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

set(CPACK_GENERATOR "DEB" CACHE STRING "CPack generator(s); DEB for Debian package")

# Single .deb containing the zpc component, bash-completion, shared libs, and other install() rules.
set(CPACK_MONOLITHIC_INSTALL ON)

include(CPack)

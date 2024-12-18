# Build options will determine if the applications will be included and build

if(NOT DEFINED BUILD_ZPC)
  set(BUILD_ZPC ON)
endif()

if(BUILD_ZPC MATCHES ON)
  add_subdirectory(zpc)
  message(STATUS "ZPC will build")
else()
  message(STATUS "ZPC will NOT build")
endif()

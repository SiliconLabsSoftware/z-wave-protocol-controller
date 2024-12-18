# Build options enabling different permutations and exluding unwanted software

option(BUILD_TESTING "WARNING: Bypass all tests (for integration purpose only)" OFF)

# applications
option(BUILD_ZPC "Package the ZPC" ON)
option(BUILD_DEV_GUI "Package the developer GUI" OFF)
option(BUILD_UIC_DEMO "Package the Unify demo" OFF)
option(BUILD_UPVL "Package the UIC-UPVL" OFF)
option(BUILD_GMS "Package the UIC-GMS" OFF)
option(BUILD_IMAGE_PROVIDER "Build the UIC-IMAGE-PROVIDER" OFF)
option(BUILD_NAL "Package the Name and location service" OFF)
option(BUILD_UPTI_CAP "Build the UIC-UPTI-CAP" OFF)
option(BUILD_UPTI_WRITER "Build the UIC-UPTI-WRITER" OFF)
option(BUILD_MATTER_BRIDGE "Build the Matter Bridge" OFF)
option(BUILD_EPC "Build EPC (Example Protocol Controller)" OFF)
option(BUILD_EED "Build EED (Emulated End Device)" OFF)

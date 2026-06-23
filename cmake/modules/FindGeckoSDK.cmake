# Find the Silicon Labs Gecko SDK location
# =============================================================================
# Usage:
#
#   find_package(GeckoSDK)
#
# Variables used by this module:
# * GSDK_LOCATION or ENV{GSDK_LOCATION} - Optional override for a preinstalled
#   Gecko SDK checkout.
#
# Variables defined by this module:
# * GeckoSDK_FOUND - True if the Gecko SDK location is found
# * GeckoSDK_VERSION - Version of the Gecko SDK found
# * GeckoSDK_ROOT_DIR - Path to the Gecko SDK base directory
# =============================================================================

if(GSDK_LOCATION)
  set(FETCHCONTENT_SOURCE_DIR_GECKO_SDK "${GSDK_LOCATION}")
  message(STATUS "GeckoSDK - using provided GSDK_LOCATION from ${GSDK_LOCATION}")
elseif(DEFINED ENV{GSDK_LOCATION})
  set(FETCHCONTENT_SOURCE_DIR_GECKO_SDK "$ENV{GSDK_LOCATION}")
  message(STATUS "GeckoSDK - using provided GSDK_LOCATION from $ENV{GSDK_LOCATION}")
else()
  if(NOT FETCH_GSDK_VERSION)
    set(FETCH_GSDK_VERSION "v2024.6.0")
  endif()
  message(STATUS "Fetching GeckoSDK ${FETCH_GSDK_VERSION} from public repository")
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare(
  gecko_sdk
  GIT_REPOSITORY "https://github.com/SiliconLabs/simplicity_sdk.git"
  GIT_SUBMODULES_RECURSE True
  GIT_SHALLOW True
  GIT_PROGRESS True
  GIT_TAG ${FETCH_GSDK_VERSION}
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/externals/GeckoSDK"
  SUBBUILD_DIR "${CMAKE_SOURCE_DIR}/externals/cache/GeckoSDK-subbuild")
FetchContent_MakeAvailable(gecko_sdk)

set(GeckoSDK_ROOT_DIR ${gecko_sdk_SOURCE_DIR})
mark_as_advanced(GeckoSDK_ROOT_DIR)
message(STATUS "GeckoSDK_ROOT_DIR is ${GeckoSDK_ROOT_DIR}")

file(
  STRINGS "${gecko_sdk_SOURCE_DIR}/simplicity_sdk.slcs" _ver_line
  REGEX "sdk_version: +\"[0-9\\.]+\""
  LIMIT_COUNT 1)
string(REGEX MATCH "[0-9\\.]+" GeckoSDK_VERSION "${_ver_line}")
message(STATUS "GeckoSDK version ${GeckoSDK_VERSION}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  GeckoSDK
  REQUIRED_VARS GeckoSDK_ROOT_DIR GeckoSDK_VERSION
  VERSION_VAR GeckoSDK_VERSION)

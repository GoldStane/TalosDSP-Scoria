include(FetchContent)
# SOURCE_SUBDIR prevents configuring third-party examples or SDK tools.
FetchContent_Declare(iplug2
  GIT_REPOSITORY https://github.com/iPlug2/iPlug2.git
  GIT_TAG d82b2e99b17c8f723d3f6e2a88ffd8b656f28e5d
  GIT_SUBMODULES ""
  SOURCE_SUBDIR scoria-unused)
FetchContent_MakeAvailable(iplug2)
set(IPLUG2_DIR "${iplug2_SOURCE_DIR}")
FetchContent_Declare(vst3sdk
  GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
  GIT_TAG 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
  GIT_SUBMODULES base pluginterfaces public.sdk
  SOURCE_DIR "${IPLUG2_DIR}/Dependencies/IPlug/VST3_SDK"
  SOURCE_SUBDIR scoria-unused)
FetchContent_MakeAvailable(vst3sdk)
if(NOT EXISTS "${vst3sdk_SOURCE_DIR}/pluginterfaces/base/funknown.h")
  message(FATAL_ERROR "Pinned VST3 SDK is incomplete")
endif()

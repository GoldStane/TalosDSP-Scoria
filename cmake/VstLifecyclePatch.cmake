# Upstream publishes offline mode only on the first process() call, too late for
# non-realtime preparation in OnReset(). Publish it during setupProcessing too.
file(READ "${IPLUG2_DIR}/IPlug/VST3/IPlugVST3_ProcessorBase.cpp" source)
set(needle "  SetSampleRate(setup.sampleRate);")
string(FIND "${source}" "${needle}" position)
if(position EQUAL -1)
  message(FATAL_ERROR "Review VST3 lifecycle patch against updated iPlug2")
endif()
string(REPLACE "${needle}" "  // Scoria: expose offline mode before lifecycle preparation.\n  SetRenderingOffline(setup.processMode == Steinberg::Vst::kOffline);\n${needle}" source "${source}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/IPlugVST3_ProcessorBase.cpp" "${source}")
get_target_property(sources iPlug2::VST3 INTERFACE_SOURCES)
list(FILTER sources EXCLUDE REGEX "/IPlugVST3_ProcessorBase\\.cpp$")
set_property(TARGET iPlug2::VST3 PROPERTY INTERFACE_SOURCES "${sources}" "${CMAKE_CURRENT_BINARY_DIR}/IPlugVST3_ProcessorBase.cpp")

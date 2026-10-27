iplug_add_plugin(Scoria
  SOURCES Scoria.cpp Scoria.h config.h
  FORMATS VST3 AU
  LINK scoria_core)
if(APPLE)
  include("${PROJECT_SOURCE_DIR}/cmake/AuStatePatch.cmake")
endif()
if(APPLE AND BUILD_TESTING)
  add_executable(scoria_au_tests ../../tests/AuHostTests.cpp)
  target_link_libraries(scoria_au_tests PRIVATE "-framework AudioUnit" "-framework CoreFoundation")
  add_dependencies(scoria_au_tests Scoria-au)
  add_test(NAME scoria_au_host COMMAND scoria_au_tests "$<TARGET_BUNDLE_DIR:Scoria-au>")
endif()
foreach(format vst3 au)
  if(TARGET Scoria-${format})
    if(APPLE)
      set(resource_dir "$<TARGET_BUNDLE_DIR:Scoria-${format}>/Contents/Resources")
    else()
      set(resource_dir "${CMAKE_BINARY_DIR}/out/Scoria.vst3/Contents/Resources")
    endif()
    add_custom_command(TARGET Scoria-${format} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets" "${resource_dir}/assets")
  endif()
endforeach()

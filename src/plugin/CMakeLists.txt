iplug_add_plugin(Scoria
  SOURCES Scoria.cpp Scoria.h config.h
  FORMATS VST3 AU
  LINK scoria_core)
if(APPLE AND BUILD_TESTING)
  add_executable(scoria_au_tests ../../tests/AuHostTests.cpp)
  target_link_libraries(scoria_au_tests PRIVATE "-framework AudioUnit" "-framework CoreFoundation")
  add_dependencies(scoria_au_tests Scoria-au)
  add_test(NAME scoria_au_host COMMAND scoria_au_tests "$<TARGET_BUNDLE_DIR:Scoria-au>")
endif()

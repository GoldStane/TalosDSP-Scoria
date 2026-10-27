include(FetchContent)
FetchContent_Declare(nam
  GIT_REPOSITORY https://github.com/sdatkinson/NeuralAmpModelerCore.git
  GIT_TAG 1f42f88535884450104b8711d7595019afa0495b
  GIT_SUBMODULES Dependencies/eigen
  SOURCE_SUBDIR scoria-unused)
FetchContent_MakeAvailable(nam)
# Match upstream's source inventory; OBJECT preserves model registry initializers.
file(GLOB NAM_SOURCES CONFIGURE_DEPENDS "${nam_SOURCE_DIR}/NAM/*.cpp" "${nam_SOURCE_DIR}/NAM/*.c" "${nam_SOURCE_DIR}/NAM/*/*.cpp")
add_library(scoria_nam OBJECT ${NAM_SOURCES})
set_target_properties(scoria_nam PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(scoria_nam SYSTEM PUBLIC "${nam_SOURCE_DIR}" "${nam_SOURCE_DIR}/Dependencies/eigen" "${nam_SOURCE_DIR}/Dependencies/nlohmann")
target_compile_definitions(scoria_nam PUBLIC EIGEN_MPL2_ONLY NAM_ENABLE_A2_FAST=1 NOMINMAX)
# Enables explicit Eigen allocation guards in debug headless tests; normal processing
# leaves the guard enabled for allocation (preparation runs on another thread).
target_compile_definitions(scoria_nam PUBLIC EIGEN_RUNTIME_NO_MALLOC)

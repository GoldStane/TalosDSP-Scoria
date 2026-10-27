# Building and validating Scoria 0.0.2

## Prerequisites

CMake 3.25+, Git, and a C++17 compiler. Windows requires Visual Studio 2022 Desktop development with C++ and the Windows SDK. macOS requires Xcode Command Line Tools (or Xcode) and a macOS SDK. No prebuilt Skia libraries are needed: IGraphics uses NanoVG/Metal on macOS and NanoVG/OpenGL 2 on Windows. First configuration needs GitHub and GitLab access (pinned NAM Core/Eigen); subsequent builds can be offline using `-DFETCHCONTENT_UPDATES_DISCONNECTED=ON` after dependencies are populated. Dependencies are development-only.

## macOS (Universal)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DIPLUG_DEPLOY_PLUGINS=OFF
cmake --build build --target Scoria-vst3 scoria_au_tests scoria_tests scoria_dsp_tests --parallel 4
ctest --test-dir build --output-on-failure
lipo -archs build/out/Scoria.vst3/Contents/MacOS/Scoria
lipo -archs build/out/Scoria.component/Contents/MacOS/Scoria
```

The AU integration test loads the built bundle and registers its factory only inside the test process. It needs macOS AudioComponent service access; restrictive sandboxes can reject registration. No local plugin installation is required for this test. Intel slices can be built on Apple Silicon, but that alone is not an Intel runtime test.

## Windows x64 (Developer PowerShell)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DIPLUG_DEPLOY_PLUGINS=OFF
cmake --build build --config Release --target Scoria-vst3 scoria_tests scoria_dsp_tests --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

Output is `build/out/Scoria.vst3` on both platforms and `build/out/Scoria.component` on macOS. Always copy/archive the entire bundle directory, never only its inner binary. CI archives include README, license and dependency notices. Builds are unsigned development builds, not notarized distributions.

## Core only / sanitizers / formatting

```sh
cmake -S . -B build-core -DSCORIA_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --config Release
ctest --test-dir build-core -C Release --output-on-failure
cmake -S . -B build-sanitize -DSCORIA_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Debug '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined'
cmake --build build-sanitize
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-sanitize --output-on-failure
clang-format -i src/audio/*.h src/dsp/*.h src/dsp/*.cpp src/models/*.h src/models/*.cpp src/plugin/*.h src/plugin/*.cpp tests/*.cpp
```

On macOS, AddressSanitizer leak detection is unsupported; `detect_leaks=0` disables that check while ASan and UBSan remain active.

## Opt-in local installation and validators

After closing your DAW, copy `Scoria.vst3` to `~/Library/Audio/Plug-Ins/VST3/` and `Scoria.component` to `~/Library/Audio/Plug-Ins/Components/` on macOS. On Windows copy `Scoria.vst3` to `C:\Program Files\Common Files\VST3\` (administrator privileges may be required). Keep LICENSE.md and THIRD_PARTY_NOTICES.md with distributed archives. Reopen the host and rescan. Alternatively enable `-DIPLUG_DEPLOY_PLUGINS=ON` explicitly during configuration for upstream local deployment.

```sh
auval -v aufx Scor TlDs
pluginval --strictness-level 5 --validate /absolute/path/to/Scoria.vst3
```

`auval` requires the AU in a discovery directory. CI installs into its disposable runner's user Components directory solely for validation. Pluginval is optional and not downloaded by this build.

## Manual REAPER acceptance

Use your own dry guitar DI at **48 kHz**, starting at low monitor volume. The sole development preset uses a captured amp-and-cab rig, gate and boost; separate CAB defaults off. Visit all four pages and verify each implemented control in the host automation list. Confirm gate threshold/release, boost drive/tone/level, NAM input/output trims, cabinet bypass/level and global controls are audible and save correctly. Crunch/Clean and modulation/time effects remain unavailable.

Enable the separate IR deliberately for convolution testing; this adds a second cabinet coloration to the bundled model. With every stage disabled and both gains at 0 dB, output should null against dry DI. Global bypass must preserve dry audio regardless of effects. Change parameters during sustained audio and listen for clicks. Test mono→mono, mono→stereo and distinct stereo input; save/reopen projects, recall the development preset, and migrate an M0 project.

At 44.1/88.2/96 kHz, NAM is explicitly unsupported and an enabled amp is silent. Disable AMP to test gate/boost/cab, or use Global bypass. Return to 48 kHz and wait for the worker to prepare the model. Exercise buffers 16–2048 plus irregular sizes, reset/restart, offline render and repeated insert/remove/editor reopen. Short silence after reset is an acknowledged limitation. Record results in VALIDATION.md; synthetic tests do not prove guitar tone quality or glitch-free host performance.

## Assets and packaging

Configuration verifies every manifest SHA-256. The build copies `assets/` into **both** native bundle Resources directories. Keep these directories intact. Do not substitute arbitrary `.nam` files: this development build accepts only the audited bundled model hash. NAM Core and Eigen are fetched at pinned commits during development, not by end users. The plugin works offline after installation.

To reproduce timing output, run `build/scoria_dsp_tests /absolute/path/to/assets` (Windows: `build/Release/scoria_dsp_tests.exe`). This is an offline synthetic-input benchmark, not a live DAW callback guarantee. Debug/sanitizer runs are unsuitable for production CPU estimates.

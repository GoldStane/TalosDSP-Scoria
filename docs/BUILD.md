# Building and validating Scoria 0.0.1

## Prerequisites

CMake 3.25+, Git, and a C++17 compiler. Windows requires Visual Studio 2022 Desktop development with C++ and the Windows SDK. macOS requires Xcode Command Line Tools (or Xcode) and a macOS SDK. No prebuilt Skia libraries are needed: IGraphics uses NanoVG/Metal on macOS and NanoVG/OpenGL 2 on Windows. First configuration needs GitHub access; subsequent builds can be offline using `-DFETCHCONTENT_UPDATES_DISCONNECTED=ON` after dependencies are populated. Dependencies are development-only.

## macOS (Universal)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DIPLUG_DEPLOY_PLUGINS=OFF
cmake --build build --target Scoria-vst3 scoria_au_tests scoria_tests --parallel 4
ctest --test-dir build --output-on-failure
lipo -archs build/out/Scoria.vst3/Contents/MacOS/Scoria
lipo -archs build/out/Scoria.component/Contents/MacOS/Scoria
```

The AU integration test loads the built bundle and registers its factory only inside the test process. It needs macOS AudioComponent service access; restrictive sandboxes can reject registration. No local plugin installation is required for this test. Intel slices can be built on Apple Silicon, but that alone is not an Intel runtime test.

## Windows x64 (Developer PowerShell)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DIPLUG_DEPLOY_PLUGINS=OFF
cmake --build build --config Release --target Scoria-vst3 scoria_tests --parallel 4
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
ctest --test-dir build-sanitize --output-on-failure
clang-format -i src/audio/*.h src/plugin/*.h src/plugin/*.cpp tests/*.cpp
```

## Opt-in local installation and validators

After closing your DAW, copy `Scoria.vst3` to `~/Library/Audio/Plug-Ins/VST3/` and `Scoria.component` to `~/Library/Audio/Plug-Ins/Components/` on macOS. On Windows copy `Scoria.vst3` to `C:\Program Files\Common Files\VST3\` (administrator privileges may be required). Keep LICENSE.md and THIRD_PARTY_NOTICES.md with distributed archives. Reopen the host and rescan. Alternatively enable `-DIPLUG_DEPLOY_PLUGINS=ON` explicitly during configuration for upstream local deployment.

```sh
auval -v aufx Scor TlDs
pluginval --strictness-level 5 --validate /absolute/path/to/Scoria.vst3
```

`auval` requires the AU in a discovery directory. CI installs into its disposable runner's user Components directory solely for validation. Pluginval is optional and not downloaded by this build.

## Manual REAPER acceptance

Use a new test project and legal dry DI or synthetic audio at safe monitor levels. Scan the installed plugin, add it to a track, open the editor, and visit all four tabs. Verify the three global controls appear in the host automation list; future effects must remain unavailable. With both gains at 0 dB the output should null against the input after routing alignment. Change each gain, toggle bypass, and confirm smooth transitions and dry bypass. Mono→stereo should carry the same sample on both channels; stereo should retain distinct left/right content.

Save/reopen a project with nonzero gains and bypass set; confirm exact control recall. Exercise 44.1/48/88.2/96 kHz and buffer sizes 16–2048, playback stop/start, offline render, repeated insert/remove and editor reopen. Measure worst-case callback timing and listen for glitches, not only average CPU. Record actual results in VALIDATION.md. M0 should sound like the source with gain, not like an amplifier.

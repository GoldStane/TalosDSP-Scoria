# M0 validation — 27 October 2026

Local environment: macOS 26.7.1, Apple Silicon, AppleClang 21.0.0, Xcode Command Line Tools (full Xcode is not installed). This is a foundation build, not a validated metal tone or public v1 release.

## Commands and results

All paths below are relative to the repository root. The final native build uses clean dependency downloads, not a preinstalled SDK.

```sh
cmake -S . -B build-clean -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
cmake --build build-clean --target Scoria-vst3 scoria_au_tests scoria_tests -j 6
ctest --test-dir build-clean --output-on-failure
arch -x86_64 build-clean/scoria_tests
arch -x86_64 build-clean/src/plugin/scoria_au_tests "$PWD/build-clean/out/Scoria.component"
lipo -archs build-clean/out/Scoria.vst3/Contents/MacOS/Scoria
lipo -archs build-clean/out/Scoria.component/Contents/MacOS/Scoria
```

Configure and both plugin builds passed. Both bundles contain x86_64 and arm64 slices. CTest's core and AU-host tests passed on Apple Silicon; the core and AU-host executables also passed under Rosetta using their x86_64 slices. Rosetta is not physical Intel hardware. SDK downloads required network permission. AU registration requires access to macOS AudioComponent services: the first sandboxed run failed at registration, and the same test passed outside the sandbox. No local plugin installation was performed.

The AU test loads the actual bundle, registers its exported factory in the test process, and checks factory-preset recall, all three parameter values through save/load, input/output gain, bypass, zero reported latency, 1→1/1→2/2→2 layouts, 44.1/48/88.2/96 kHz, 16–2048-frame rendering and repeated create/reset/uninitialize/dispose operations. It uses synthetic constants and does not open the editor.

```sh
cmake -S . -B build-core -DSCORIA_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core -j 4
ctest --test-dir build-core --output-on-failure
cmake -S . -B build-sanitize -DSCORIA_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Debug '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined'
cmake --build build-sanitize -j 4
ctest --test-dir build-sanitize --output-on-failure
```

Release core and AddressSanitizer/UndefinedBehaviorSanitizer checks passed. Float/double tests cover pass-through, gain, bypass, smoothing, in-place processing, layouts, zero/one/16–2048 frames, four sample rates, non-finite/subnormal inputs, missing input, rejected layouts and repeated parameter toggling. The initial test compilation exposed a null-pointer template deduction issue in the test call sites; explicit template arguments fixed it before the passing runs.

```sh
/tmp/scoria-format/clang_format/data/bin/clang-format --dry-run --Werror src/audio/*.h src/plugin/*.h src/plugin/*.cpp tests/*.cpp
git diff --check
plutil -lint src/plugin/resources/Scoria-AU-Info.plist src/plugin/resources/Scoria-VST3-Info.plist
```

Passed. clang-format 18.1.8 was installed only into a temporary developer-tools directory. There is no Python/formatter runtime requirement for users. Initial native compilation reported upstream VST3 integer-to-double conversion and AU variable-length-array warnings; no Scoria compile errors remained.

## Not verified

- Windows compilation/runtime and GitHub Actions: workflow added, not dispatched or claimed green in this local task.
- REAPER plugin loading, GUI interaction, DAW automation, saved-project recall, offline bounce, and subjective listening. REAPER launch encountered an existing missing-project error referring to an external volume; the user's session/settings were left unchanged.
- Visual GUI rendering and all page interactions: compiled, not visually validated.
- `auval`: available, but not run because Scoria was not installed in a component discovery directory. CI includes installation and auval on its disposable runner.
- Pluginval: not installed locally; manual command documented.
- Physical Intel and Windows machines; older supported macOS versions.
- Worst-case callback timing, audio-device glitches/CPU, signing, notarization, installers and public release readiness.

Before M1 integration, complete native CI and the manual REAPER/validator checklist in BUILD.md. Then implement safe NAM preparation and asset ownership with a proven-redistributable high-gain capture/cab IR, gate and overdrive, followed by a level-matched DI recording.

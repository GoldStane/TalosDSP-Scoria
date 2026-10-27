# Validation record — M1 development

M1 acceptance is **not complete**. The default is an explicitly labeled development rig, not Tight Rhythm. The licensed model includes a cabinet; an amp-only capture and real guitar listening/tuning remain necessary. Current Windows CI and manual REAPER acceptance are not yet verified.

## Baseline inspected

At M0 commit `c1a39dbbb36b923891f69430a2b6203baa5e4535`, local native build and both CTest tests passed. GitHub Actions run `40561427926`: Windows job `122391177344` passed configure/build/tests/package; macOS job `122391177743` passed Universal builds and CTest, but failed AU discovery (`didn't find the component`, exit 2). The workflow now restarts the disposable runner's registrar and retries validation, still failing if validation never succeeds. This workflow change is not proof of a passing new CI run.

## Local environment and commands

macOS 26.7.1, Apple Silicon, AppleClang 21, CMake, Xcode Command Line Tools. Dependencies were inspected at the exact pins in `cmake/Dependencies.cmake` and `cmake/Nam.cmake`. Initial local configuration reused the checked-out pinned NAM/Eigen and M0 iPlug2/VST3 trees:

```sh
cmake -S . -B build-m1 -DSCORIA_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_NAM="$PWD/third_party/NeuralAmpModelerCore"
cmake --build build-m1 -j4
ctest --test-dir build-m1 --output-on-failure
cmake -S . -B build-m1-native -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DFETCHCONTENT_SOURCE_DIR_NAM="$PWD/third_party/NeuralAmpModelerCore" -DFETCHCONTENT_SOURCE_DIR_IPLUG2="$PWD/build-clean/_deps/iplug2-src" -DFETCHCONTENT_SOURCE_DIR_VST3SDK="$PWD/build-clean/_deps/iplug2-src/Dependencies/IPlug/VST3_SDK"
cmake --build build-m1-native --target Scoria-vst3 scoria_au_tests scoria_tests scoria_dsp_tests -j4
ctest --test-dir build-m1-native --output-on-failure
```

Release headless tests and the in-process AU host test passed. Universal VST3/AU builds succeeded. `auval -v aufx Scor TlDs` passed all checks against the Universal AU build; it reported one non-fatal warning that the preset name is not retained in retrieved class data. The AU host test must run outside this environment's filesystem/service sandbox; sandbox registration failure is not reported as a plugin test success.

## Coverage

- Original float/double gain, bypass, zero-length, layouts, missing channels, in-place and parameter-ramp regression tests at 44.1/48/88.2/96 kHz, sizes 1–2048.
- Gate opening/closing, DC rejection, measurable boost nonlinearity and dry bypass.
- Boost 4× versus 1× normalized third-harmonic foldback measurement; this is one synthetic test, not a complete aliasing audit.
- FIR output against a reference convolution; recorded WAV preparation at four rates; malformed WAV rejection.
- Actual bundled NAM inference at 48 kHz, sizes 1/16/32/64/127/128/256/511/512/1024/2048, deterministic output across block boundaries, transactional failed replacement, missing/unsupported/altered model rejection.
- Worker generation handoffs through rate changes and unchanged-rate requests retaining prepared assets. NAM rejects 44.1/88.2/96 kHz rather than running at the wrong rate.
- Full NAM+IR stereo in-place processing, global exact dry bypass and input/output placement around nonlinearity.
- Version-1 migration, version-2 state roundtrip, truncation/nonfinite/unknown-asset rejection before mutation.
- Actual AU instantiation, factory recall, all 16 parameters roundtrip, M0 migration and invalid state rejection, layouts/rates/gain/bypass/lifecycle, and NAM+IR rendering from bundle-relative resources.
- `auval` open/initialization, default formats, required/recommended properties, custom UI, factory preset, 16 published parameters, reset retention, render at 22.05–192 kHz, mono/stereo layouts, parameter scheduling and MIDI checks.
- C++ allocation counters around NAM/pipeline processing. Debug tests additionally disable Eigen allocations around NAM processing. This does not instrument every possible system allocation or prove host-level real-time performance.

## Remaining acceptance checks

Run the new Windows workflow, pluginval, REAPER editor/automation/session/offline-render checks, and level-matched listening with an actual legal DI recording. No production tone quality, live glitch-free operation, Windows M1 binary, signing or notarization is claimed. Known limitations: 48 kHz-only NAM, temporary silence after reset while preparation completes, no arbitrary model import, a single IR, direct-convolution CPU cost, and incomplete default amp-only preset.

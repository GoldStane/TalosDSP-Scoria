# Validation — 0.0.3 M1 acceptance candidate

## Current completion pass (2026-10-28)

Baseline `19fecf7` was clean and GitHub native run `40573164711` passed Windows/macOS. That result predates these changes. Completion commit `63eaa95` passed native run `40763626791`: Windows x64 VST3 and macOS Universal VST3/AU, CTest, AU validation and complete bundles with checksum verification. A subsequent FIR loop optimization requires its own CI result below.

Local Apple M4 / macOS 26.7.1 / AppleClang 21 Release Universal VST3 and AU builds pass. Four CTest tests pass: completion/registry/resampling/state fixtures, original core regressions, DSP/assets, and actual AU host integration. ASan+UBSan portable tests passed 3/3 (83.01 seconds) before the final lifecycle adjustment; final sanitizer status must be recorded separately. C++ allocation counters cover adapted NAM and the complete pipeline; Debug additionally rejects Eigen allocations.

`pluginval` 1.0.4 strictness 5 passed VST3 scanning, editor, processing at 44.1/48/96 kHz and buffers 64–1024, state and automation. Apple `auval -v aufx Scor TlDs` passed the installed AU. Repeat after final lifecycle changes.

Polyphase impulse peaks exactly match declared latency (127/0/186/196 samples at 44.1/48/88.2/96 kHz). A 1 kHz roundtrip sine has maximum absolute error below 4.2e-7. NAM renders match across 17-frame and 256-frame internal boundaries. These checks do not substitute for a full converter spectral audit.

Offline full stereo chain, Herbert full WaveNet, 256 frames, 100 timed blocks, Release arm64: 44.1 kHz mean 1181.86 / p99 1270.96 / max 1324.50 µs; 48 kHz 1110.61 / 1139.04 / 1149.08 µs; 88.2 kHz 1800.41 / 1864.00 / 1868.71 µs; 96 kHz 1923.22 / 2013.58 / 2019.25 µs. An earlier run while a native build was active reached 11536.2 µs at 44.1 kHz. Neither run establishes glitch-free live performance; direct FIR cost is significant at 96 kHz. No FFT rewrite was made.

The user supplied a private 19.349-second, mono 24-bit/44.1 kHz guitar DI for local testing. It is not committed. The full offline pipeline rendered 853304 frames after latency compensation, peak 0.352955 (about −9.05 dBFS), RMS 0.0574685. The user listened and reported **“Usable rhythm tone; continue validation.”** This is user musical feedback on one recording, not an agent claim to have heard the audio or an exhaustive pickup/tuning comparison.

REAPER 7.34 arm64 loaded the updated VST3, recalled the legacy model and +12 dB NAM trim from an existing project, and successfully selected Tight Rhythm (new model, cabinet on, output −12 dB). Validation uses a separate `/tmp` project; the original project and recording are untouched. Real v2/v3 state payloads containing only IDs/numeric parameters are committed as fixtures. An initial repeated fast bounce exposed variable startup silence while assets prepared. The fix prepares on the worker and waits only during offline lifecycle setup; never in ProcessBlock. After the lifecycle fix, two independent REAPER bounces contained exactly identical 24-bit stereo PCM: 474803 frames, first nonzero sample 0, PCM SHA-256 `08c56bbfa562e8f4a593a1e95a5495d1c16f8f692ddb06c66505049a73b09ee3`. The prior broken renders began at samples 15876 and 26052. REAPER also rendered at 96 kHz with FX processing at the render rate: 1033584 frames / 10.7665 seconds, non-silent. The project reopened successfully with the new factory state.

Exact commands used:

```sh
cmake --build build-m1-native --parallel 4
ctest --test-dir build-m1-native --output-on-failure
cmake --build build-m1-sanitize --parallel 4
ctest --test-dir build-m1-sanitize --output-on-failure
build-m1-native/scoria_completion_tests assets
build-m1/scoria_render_test assets /tmp/scoria-di.f64 /tmp/scoria-tight.f32 44100
pluginval --strictness-level 5 --validate /absolute/path/build-m1-native/out/Scoria.vst3
auval -v aufx Scor TlDs
```

The AU test/validators require access to macOS services outside the execution sandbox. Sandbox registration failure was rerun successfully outside it.

Remaining: final FIR optimization CI/sanitizer rerun, broader live buffer/rate/automation and multiple-instance listening matrix. Live model changes are not gapless; initial async preparation remains visible. Signing/notarization and physical Windows host listening are not claimed.

## Historical 0.0.2 validation (superseded limitations)

# Validation record — M1 development

M1 acceptance is **not complete**. The default is an explicitly labeled development rig, not Tight Rhythm. The licensed model includes a cabinet; an amp-only capture and real guitar listening/tuning remain necessary. Native CI run `40572630277` passed for commit `ec1f078` on Windows x64 and macOS Universal; manual REAPER acceptance and listening remain unverified.

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

## CI result

GitHub Actions run `40572630277` (commit `ec1f078`) passed both jobs. `windows-2022` configured, built `Scoria-vst3`, `scoria_tests` and `scoria_dsp_tests`, ran CTest and packaged the VST3. `macos-14` built the Universal VST3/AU, ran all three CTest tests, passed `auval -v aufx Scor TlDs` and packaged both bundles. This proves build, automated test and AU validation on GitHub-hosted runners, not a manual DAW session.

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

Run pluginval, REAPER editor/automation/session/offline-render checks, and level-matched listening with an actual legal DI recording. No production tone quality, live glitch-free operation, manual Windows VST3 host validation, signing or notarization is claimed. Known limitations: 48 kHz-only NAM, temporary silence after reset while preparation completes, no arbitrary model import, a single IR, direct-convolution CPU cost, and incomplete default amp-only preset.

## Final hardening results

The lifecycle-corrected version passed pluginval strictness 5 again and ASan+UBSan 3/3 in 91.15 seconds. The converter's 96→48 kHz rejection test produces RMS 9.144e-7 for a unit-amplitude 30 kHz input (about −118 dB relative to the input RMS). This is a selected stopband measurement, not an exhaustive quality score.

Profiling justified a small direct-FIR optimization, rather than FFT convolution: mirrored history and four accumulators. On the same Apple M4 Release full-stereo 256-frame test, 100 blocks: 44.1 kHz mean 722.455 / p99 1005.96 / max 1168.92 µs; 48 kHz 578.688 / 723.75 / 757.75 µs; 88.2 kHz 792.673 / 1020.04 / 1023.75 µs; 96 kHz 804.747 / 1029.08 / 1080.38 µs. Output peaks are unchanged to the printed precision. Scheduling noise remains; these are offline measurements, not guaranteed live deadlines.

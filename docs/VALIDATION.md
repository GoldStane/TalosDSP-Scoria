# M2 validation — 0.0.4 candidate

## Resumed completion pass (2026-11-15)

The branch starts from M1 completion `52a211e`. Its latest CI run `40902507147` passed Windows and failed macOS configuration because GitLab refused the pinned Eigen submodule clone due to server load, before compilation. The preceding code revision passed both native runners (`40880652083`). M2 native CI must be recorded independently.

The original M2 implementation pass built Universal VST3/AU and passed 5/5 native CTest tests; pluginval 1.0.4 strictness 5 also passed. After resuming and strengthening AU coverage to restore all four new effects enabled and Clean selected, `cmake --build build-m1-native --parallel 4` and `ctest --test-dir build-m1-native --output-on-failure` pass again: **5/5, 2.76 seconds**. Sandbox-only AU registration failure was rerun with macOS service access. ASan+UBSan passed 4/4 portable tests in 135.43 seconds on this resumed pass. Final package/host checks remain separate.

New tests cover static allpass impulse energy, modulation periodicity, extreme phaser feedback, chorus fractional-delay timing, exact ping-pong echoes/feedback, delay time transitions and bypass tails, finite decorrelated decaying reverb, parameter changes, all four rates, block sizes 1/17/64/128/256/511/2048, deterministic output, in-place mono downmix, aligned global bypass and C++ allocation counters. Registry tests exercise new captures at all supported rates, worker voice replacement and invalid model indices. Binary schemas 1–4 load; all old fixtures keep new effects off. AU tests round-trip all 35 parameters including enabled new effects and voice selection. Existing M1/core tests continue running.

The full private DI rendered through Tight Rhythm byte-for-byte identically to the saved M1 float32 output before temporary files were cleared. The original new-preset stereo renders were finite: Modern Lead peak 0.254692, Ambient Clean 0.108345, Crunch Rhythm 0.260755. The user liked Modern Lead and Ambient Clean and reported harshness in Crunch Rhythm. The user found a −6 dB input revision too clean, then approved the midpoint: −3 dB input and +0.6 dB output. The final render peak is 0.267606 and RMS 0.0545111, close to the original 0.0539988 RMS. Modern Lead and Ambient Clean are unchanged. This is a preset tuning change only, not a hidden replacement of the capture or the M1 sound.

Offline Apple M4 / AppleClang Release, full Herbert WaveNet + cabinet, 1000 timed blocks after 50 warmup blocks per case: at 48 kHz stereo/256 frames effects-off mean/p95/p99/max = 548.246/562.166/574.916/589.208 µs, all-effects-on = 566.782/584.583/602.792/653.875 µs. At 96 kHz stereo/256: off 751.485/771.042/790.75/843.083 µs, on 776.292/797.833/833.5/885.25 µs. All four rates, mono/stereo input and 64/128/256 frames were measured. Mean overhead in most cases was approximately 3–5%; one noisy 44.1 kHz mono/64 baseline exceeded the effects-on time. These are offline observations, not guaranteed live deadlines or an isolated pre-M2 binary benchmark. Reproduce with `build-m1-native/scoria_effects_benchmark assets`.

M2 effects and amplifier expansion are engineering-complete with user-approved preset renders, installed editor/REAPER recall and deterministic offline rendering. Full extended live-host acceptance is not claimed. The broad M1 live-monitoring/multiple-buffer/Windows physical-host acceptance matrix remains outstanding. New effects have not been tested on a physical Windows DAW or Intel Mac. New models have explicit MIT provenance and require no runtime downloads. Signing/notarization are not claimed.

## Installed-host completion checks

The final DSP/preset binary passed pluginval 1.0.4 strictness 5 and installed `auval -v aufx Scor TlDs` on 2026-11-15. Both bundles contain arm64 and x86_64 slices. REAPER 7.34 arm64 loaded 0.0.4, restored the legacy v2 project, displayed all five factory presets, and recalled Modern Lead. A separate `/tmp` project was saved; private source projects and DI were not committed. The device was at 16 kHz, which correctly showed an unsupported-rate status, so live monitoring was not claimed.

Two independent full-speed offline REAPER renders at 44.1 kHz, 24-bit stereo, Modern Lead, with three seconds of tail, were byte-identical PCM: 607103 frames; SHA-256 `6a274006ecb36c9626869c5c85e9a4eece7c2468a5f75df55d7d2fec8ae79b47`. REAPER reported no clipped samples. The editor inspection found undersized pedal bypass faces; a layout-only follow-up moves their labels alongside full-height buttons. The installed follow-up was visually checked: pedal bypass faces are full height and the FX panel has readable, non-overlapping controls. After quitting and reopening REAPER, the saved project restored Modern Lead with delay 360 ms / 28% feedback / 18% mix and both post-effects enabled. Final local build and native CTest passed again: 5/5 in 3.34 seconds.

Native run `43652851829` for `4fc746c` passed Windows x64 build/tests/package; macOS was cancelled, not failed or verified. Run `43755041502` also passed Windows, but macOS and its retry were cancelled before any steps with `macos-14-arm64-us-default is not supported`. GitHub [retired macOS 14 on November 2](https://github.blog/changelog/2026-10-01/github-actions-macos-14-runner-image-retirement/); the workflow now uses `macos-15`, retaining both Universal architectures. Replacement [run `43756216752`](https://github.com/GoldStane/TalosDSP-Scoria/actions/runs/43756216752), commit `b322349`, passed both Windows x64 and macOS 15 Universal jobs: native builds, CTest, macOS auval, asset verification and complete package upload. Extended live monitoring, REAPER automation recording, multiple active instances and physical Windows host listening remain manual acceptance items. The conservative 560-second tail report can overextend automatic exports; use explicit tail length.

The final installed AU passed `auval -v aufx Scor TlDs` again after the layout change. The local `build-m1-native/Scoria-0.0.4-macOS.zip` contains both complete Universal bundles, README, MIT license and documentation. All five assets in each archived bundle match their manifest SHA-256; no private DI or REAPER project is included. Reproduce packaging with `cmake -E tar cf ../Scoria-0.0.4-macOS.zip --format=zip .` from `build-m1-native/out` after copying current README/license/docs there.

## Historical M1 record

# Validation — 0.0.3 M1 acceptance candidate

## Current completion pass (2026-10-28)

Baseline `19fecf7` was clean and GitHub native run `40573164711` passed Windows/macOS. That result predates these changes. Completion commit `63eaa95` passed native run `40763626791`: Windows x64 VST3 and macOS Universal VST3/AU, CTest, AU validation and complete bundles with checksum verification. Final code commit `8283288` passed run `40880652083` on both native runners, including the FIR optimization and asset-status fix.

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

Remaining: broader live buffer/rate/automation and multiple-instance listening matrix. Live model changes are not gapless; initial async preparation remains visible. Signing/notarization and physical Windows host listening are not claimed.

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

## Final code verification (2026-10-29)

Commit `8283288d82b35225b5608836fe7a83b4609e91c5`, branch `codex/m1-completion`, passed [native CI run 40880652083](https://github.com/GoldStane/TalosDSP-Scoria/actions/runs/40880652083): Windows x64 VST3 and macOS Universal VST3/AU, all CTest checks, macOS auval, packaged assets and artifact publication. Local Universal slices verified with `lipo -archs`: x86_64 and arm64 for both bundles.

Final local native suite: 4/4 passed, 3.78 seconds. Final ASan+UBSan portable suite: 3/3 passed, 77.96 seconds. Pluginval 1.0.4 strictness 5 passed the optimized VST3. Source and both packaged asset manifests passed SHA-256 verification. Formatting and `git diff --check` passed. The asset-status fix now derives readiness from the completed request token, avoiding a request/worker status-write race. Fractional unsupported sample rates are explicitly rejected.

Delivered scope: self-contained amp-only Tight Rhythm, original legacy capture/state preservation, common-rate adaptation, gate/boost/cabinet, host-visible parameters and preset recall, actual REAPER DI rendering and user-approved tone. No required model, IR, sample-rate implementation or native build is missing.

Full acceptance is not claimed for the entire requested live-host matrix: sustained live-input performance, manual REAPER automation recording/playback, exhaustive buffer/rate switching while playing, and extended multi-instance listening remain to be checked. Host automation is exercised by pluginval, and rate/layout/reset behavior by native AU tests. Live asset changes can briefly mute; no gapless model-switch guarantee, unsigned builds, no Intel runtime or physical Windows DAW listening verification. These limits must remain visible rather than being inferred as passing from CI.

The final optimized bundles were installed into the user VST3/Components folders with rollback copies retained in `/tmp`; `auval -v aufx Scor TlDs` passed again. The local ZIP contains both complete Universal bundles, README, MIT license and documentation. No DI audio or private project is packaged.

# AGENTS.md — TalosDSP Scoria

> Repository-level instructions for Codex and other coding agents. Read this entire file before making changes. Keep the product goal, real-time safety, and an installable end-user experience above architectural novelty.

## 1. Product contract

- **Publisher/brand:** TalosDSP.
- **Product:** Scoria (display name: **TalosDSP Scoria**).
- **Description:** A free, open-source, self-contained modern-metal guitar amplifier and effects suite for real music production, not primarily a tutorial or DSP experiment.
- **Primary use case:** Plug a guitar DI into a DAW, load Scoria, select a preset, and immediately get a convincing djent/modern-metal tone. Focus on excellent rhythm tones first, then leads, clean and ambient sounds.
- **Experience:** Inspired by the navigational *workflow* of modern amp suites such as Archetype: Gojira (pedals -> amplifier -> cabinet -> effects), while using entirely original visuals, names, presets, and assets. Do not reproduce proprietary code, assets, graphics, branding, or captures.
- **Mandatory release platforms:** Windows x64 VST3; macOS VST3 + Audio Unit (AUv2), with native Apple Silicon and Intel support. Other formats/platforms are optional after these work.
- **Primary test DAW:** REAPER. Do not assume that passing a standalone test proves DAW compatibility.
- **Non-negotiable:** End users must not need Docker, Python, an external neural-amp plugin, manual dependency compilation, or an internet connection to use a published release.

### Product success criteria

1. A legitimate, redistributable factory amp/cab configuration produces a musically usable high-gain rhythm tone by default.
2. Users can switch presets and configure/bypass every implemented effect using a simple native GUI; the DAW can automate parameters and restore saved sessions.
3. The plugin handles expected channel layouts, sample rates, host buffer sizes, and lifecycle changes without crashes, audio glitches, silence, or unreasonable CPU spikes.
4. Public release artifacts contain the plugin, every required runtime asset, installation instructions, licensing notices, and working Windows/macOS builds.
5. Sound quality in real guitar recordings matters more than feature count or algorithm originality.

## 2. Locked-in product/UX decisions

### Identity and interface

- Product identity is **TalosDSP Scoria**. Repository name can be `scoria`; don't invent a second brand.
- GUI is minimalist and functional: vector/text controls, toggles, sliders, numeric values, drop-downs. No mandatory artwork, photorealistic pedals, 3D animations, browser UI, or fancy skins.
- Main navigation has **PEDALS | AMP | CAB | FX** pages, plus a persistent preset selector and practical input/output controls.
- Every *implemented* effect has a bypass toggle and meaningful controls. Unimplemented modules must be clearly labeled/disabled, not silently presented as working features.
- The amplifier section provides distinct voicings: **Distortion (high gain)** is the flagship amp; **Crunch** and **Clean** are planned additional amp voices. These are amp choices, **not** extra distortion-pedal effects.
- The separate **Overdrive** is a pre-amp pedal/boost; there is no standalone “Distortion” stompbox in the initial specification.
- The CAB page selects bundled IRs (and may later support custom IR loading); allow cab bypass for troubleshooting.
- Use a few curated factory presets (e.g. Tight Rhythm, Modern Lead, Ambient Clean) only when the underlying amp/effects support them honestly. Do not label a passthrough or generic clipper a production-ready preset.

### Fixed initial processing chain

```text
Guitar DI / input trim
  -> noise gate
  -> [optional later: octaver]
  -> overdrive / boost
  -> phaser
  -> chorus
  -> amp model (Distortion / Crunch / Clean; one active voice)
  -> cabinet impulse-response convolution
  -> stereo delay
  -> stereo reverb
  -> output gain
```

- The initial routing is intentionally fixed. Freely drag-reordering modules and pre/post-amp switching are **not** required for v1.0.
- Gate and input trim are utility controls; the remaining effects have their own UI and independent bypass.
- The octaver is deferred until after a usable public release; do not let polyphonic pitch shifting delay v1.0.
- Phaser and chorus start pre-amp; delay and reverb are post-cab. This is a deliberate first-release choice, not a claim that other orders are musically invalid.
- Audio input is normally mono guitar DI; stereo output is supported where the host permits it. Host channel negotiation must be explicit: do not silently drop channels, double signals, or read nonexistent channels.

### Minimal initial parameter targets

| Module | Controls to plan for |
|---|---|
| Global | preset, input trim, output level, global bypass |
| Noise gate | enabled, threshold, release (possibly attack later) |
| Overdrive pedal | enabled, drive, tone, level |
| Phaser | enabled, rate, depth, feedback, mix |
| Chorus | enabled, rate, depth, mix; stereo width optional |
| Amplifier | amp choice, gain/input drive, bass, mid, treble, presence, master/output where meaningful |
| Cabinet | enabled, IR selection, output, optional low/high cut |
| Delay | enabled, time, feedback, wet/dry; host-tempo sync later if necessary |
| Reverb | enabled, decay, damping, wet/dry; size optional |
| Octaver (post-v1) | enabled, octave blend, tracking/mix |

**Important NAM qualification:** Ordinary NAM captures can encode fixed amp settings. Do not claim a GUI slider faithfully moves a physical amp's original gain/EQ knob when it actually applies pre/post filtering. Name and document controls according to their real behavior. Switching amp voicings may require separate models.

## 3. Technical approach

### Intended stack

- **Language:** Modern C++ (prefer C++17 initially for compatibility; change the standard only with a demonstrated dependency requirement).
- **Audio plugin framework:** iPlug2, preferably using the upstream **iPlug2OOS** template. Verify the current template/API before adapting; do not guess outdated methods or CMake target names.
- **UI:** iPlug2 IGraphics with plain native/vector controls.
- **Build:** CMake, using the template's proven build conventions, toolchains, and explicit plugin format targets.
- **Amplifier inference:** NeuralAmpModelerCore (MIT); integrate only after baseline host loading and audio I/O are demonstrated.
- **Conventional effects:** Start with small auditable C++ DSP modules and evaluate permissively licensed components such as DaisySP on a case-by-case basis. Check each included file/submodule's precise license, not merely a library's top-level README.
- **Cabinet:** FIR convolution with bundled, proven-redistributable IRs; optimize using partitioned convolution only if measurement justifies it.
- **Tests:** CTest/unit tests on framework-independent DSP, plus pluginval and macOS `auval` when available, plus manual REAPER regression testing.
- **CI/CD:** GitHub Actions on real Windows and macOS runners. A Linux container can help unit testing but **cannot replace** native Windows/macOS builds or macOS signing/notarization.
- **Licensing target:** MIT for original Scoria code, subject to actual compatibility of incorporated dependencies and assets. Maintain third-party notices and avoid copying code with incompatible copyleft obligations into the source.

### Source organization (adapt pragmatically to the chosen template)

```text
AGENTS.md
README.md
CMakeLists.txt
CMakePresets.json              # only if compatible with actual template
src/
  plugin/                     # iPlug2 adapter, editor, parameter registration, host state
  audio/                      # AudioEngine, AudioGraph, parameter smoothing, buffers
  dsp/                        # independent gate, OD, modulations, delay, reverb, cab
  models/                     # NAM engine adapter, model metadata/loading
  presets/                    # preset schema/factory settings
assets/
  models/                     # only redistributable models
  irs/                        # only redistributable IRs
  manifest.*                  # source, permissions, checksums, model/IR rates
resources/                     # minimal UI resources where required by iPlug2
include/                       # public project headers if useful
tests/                         # headless DSP/state/asset tests
docs/
  ARCHITECTURE.md
  BUILD.md
  LICENSING.md
.github/workflows/              # native build, test, release
third_party/                    # pinned dependencies or submodules, with licenses
```

Do not force this directory shape at the expense of a working upstream template. **Keep plugin host glue separate from portable DSP** so processing can be unit-tested without loading a DAW.

### Implementation conventions

1. Use `clang-format` and consistent naming. Prefer descriptive identifiers, small cohesive classes, `const` correctness, RAII, and explicit ownership.
2. Minimize new dependencies. Pin versions or commit SHAs. Never copy a full third-party project into our source when a clean vendored/submodule or CMake integration will do.
3. Keep signal processing independent of editor widgets; UI sends parameter changes through the framework's proper host-safe mechanism.
4. Centralize parameter definitions (stable IDs, names, ranges, units, skew/defaults). Do not reuse/reorder a released parameter ID casually; add migration for schema changes.
5. Use physical units where possible (dB, Hz, ms, seconds, %); map UI normalized values to DSP values deliberately, especially logarithmic frequency/time controls.
6. Maintain a versioned preset/state representation and preserve all host state needed to recall a session, including selected amp, cabinet, model references/assets, effect toggles, and parameters.
7. Source DSP classes must support initialization/reset on sample-rate and block-size changes and be testable without a GUI.
8. Host-reported latency must match actual processing latency; never advertise zero latency merely for marketing.
9. No hidden network access, tracking, telemetry, background downloads, or account requirement. Bundled factory assets must work offline.
10. Prefer code that builds and can be verified over speculative abstractions, prematurely optimized SIMD, or a plugin that displays fake controls.

## 4. Hard real-time audio rules

In the audio callback / per-buffer processing path:

- **Never** perform file/network I/O, model or IR loading, blocking mutex waits, UI calls, logging, thread spawning/joining, heap allocations, or dynamic container growth.
- Preallocate required buffers and DSP state. Build/validate heavy assets off-thread, then exchange immutable processing states using a documented audio-safe lifetime strategy.
- Smooth user-automated parameters where sudden changes could click; account for transitions when bypassing effects or switching models.
- Avoid exceptions crossing the audio callback; ensure invalid assets/parameters cause safe and explicitly documented behavior.
- Sanitize NaNs/infinities, handle denormals, avoid clipping internal intermediates unnecessarily, and keep wet/dry gain structure deliberate.
- Support silence, tiny and zero-length blocks where hosts use them, and expected sample rates (at least 44.1/48 kHz; also test 88.2/96 kHz when feasible).
- Test varied host buffer sizes (e.g. 16/32/64/128/256/512/1024/2048 frames), tempo/transport changes, offline bounce, and repeated activate/deactivate/reset operations.
- Measure **worst-case callback time**, glitches, reported latency, CPU consumption, and output levels. A low average time or a single fast benchmark does not prove real-time safety.

## 5. Sound-quality requirements

- A convincing modern-metal rhythm tone is the first audio milestone. Favor an authentic, documented, licensed high-gain NAM capture instead of building a simplistic `tanh` waveshaper and calling it an amp.
- Choose a cabinet IR that complements the amp. Verify whether a NAM model already includes cabinet/speaker coloration to prevent accidentally applying two cabs.
- Tune gate and overdrive together for tight, palm-muted low-tuned guitar. Avoid excessive gain and low-frequency mud.
- Add suitable oversampling/anti-aliasing around **new** nonlinear stages where listening/measurement reveals aliasing; do not blindly oversample the whole plugin.
- Check impulse/sweep behavior, gain response, silence/noise floor, feedback stability, bypass equality, and subjective A/B listening with level matching.
- Test on recorded dry DI, chugs, sustained notes, chords, and double-tracked rhythm parts. Keep a small legal test set or synthetic signals for CI.
- Do not claim a captured NAM amp's knobs behave identically to the original hardware unless that behavior has been implemented and validated.

## 6. Licensing, legal and asset provenance

- Source dependencies and assets are separate licensing problems. The NAM inference engine's MIT license does **not** grant rights to arbitrary `.nam` captures or cabinet IRs.
- Do not bundle models/IRs downloaded from sharing sites unless the exact redistribution rights are clearly documented. Store author, URL, license, permission evidence, hash, and attribution in an asset manifest.
- Prefer our own captures/IRs or explicitly permissive/redistributable assets. Do not commit unlicensed copyrighted music/test recordings, product screenshots, proprietary plugin artwork, or copied commercial presets.
- Inspect transitively included third-party license files, including any LGPL/GPL components. Document notices and resulting redistribution requirements before merging.
- Verify trademark and naming risks before public branding; **Scoria** is the working product name chosen by the project owner, not a claim of trademark clearance.

## 7. Testing and definition of done

Each PR or Codex task should report:

- What changed and why (including any DSP/audio ramifications).
- Exact configure/build/test/validation commands executed and their pass/fail results.
- The platforms and DAWs actually tested; distinguish local results, CI results, and tests that could not be run.
- Any new dependencies and license findings.
- Manual listening instructions where relevant, and remaining blockers.

At minimum, verify:

- CMake configure/build works on the available native runner; run both Windows and macOS CI as soon as workflows exist.
- State round-trip: save/load restores parameter values, selected model/IR, effect toggles, and preset selection.
- Bypass: pass-through is correct (allowing documented latency/tails); controls have audible effect only when a module is truly implemented.
- Channel and sample-rate changes do not crash or read beyond buffers.
- No NaNs/infinities for ordinary bounded inputs or repeated toggling; effects with feedback stay stable in their documented ranges.
- VST3 checked with pluginval where feasible; macOS AU with `auval`; actual manual load and audio processing in REAPER before declaring user-facing readiness.
- Every factory asset exists in the released package with legal provenance, and presets work entirely offline.

**Never state that code compiles, the plugin loads in REAPER, or audio sounds good without actually testing that claim.** If an environment cannot run a macOS AU host or a Windows VST3 host, say so.

## 8. Milestone plan (implement incrementally)

### M0 — Foundation: first Codex task

- Inspect the existing repository first. If empty, bootstrap using a **pinned** current iPlug2/iPlug2OOS starting point, retaining only needed targets (VST3 on both OSes, AUv2 on macOS).
- Create actual `TalosDSP Scoria` manufacturer/product metadata with stable plugin identifiers (avoid default template identifiers).
- Build a **real pass-through audio effect**, with operational input/output trim and global bypass, handling mono/stereo host arrangements safely.
- Add the simple **PEDALS | AMP | CAB | FX** GUI skeleton, preset display placeholder, and routing/data model. Any future DSP controls must be visibly disabled or marked not implemented until backed by working DSP.
- Separate a headless DSP/core target from framework-specific host/UI glue where practical.
- Include a README, build instructions, a first CTest smoke test for pass-through/gain/bypass, a `.clang-format`, gitignore, and basic Windows/macOS CI configuration. Do not pretend to ship a complete amp in M0.
- **Acceptance:** builds on supported runner(s), pass-through tests succeed, plugin format targets are produced on those runners, and editor/parameter changes are host-visible. Record untested hosts/platforms plainly.

### M1 — First genuinely playable metal tone

- Integrate NAM Core with safe loading/preparation; select a properly licensed **Distortion (high-gain)** amp model.
- Implement cabinet IR convolution with at least one licensed 4x12-style IR and asset manifest.
- Add a functional noise gate and Tube Screamer-style overdrive boost.
- Tune and ship at least one **Tight Rhythm** default preset that sounds good from a dry DI without external setup.
- Test sample rates, gain staging, parameter smoothing, model/IR errors, and offline rendering.
- **Acceptance:** a repeatable recording in REAPER uses only Scoria and demonstrates a convincing modern-metal guitar sound.

### M2 — Requested effects and amp choices

- Implement phaser and chorus on the PEDALS page, then stereo delay and reverb on FX.
- Add **Crunch** and **Clean** amp voices when appropriately licensed and tested captures are available.
- Implement effect bypasses, DAW automation, preset serialization, and polished defaults.
- **Acceptance:** each advertised effect can be audibly controlled, bypassed, saved, restored, and switched without clicks/instability.

### M3 — Public v1.0 hardening

- Profile and optimize measured real-time bottlenecks; validate host restart/reconfigure scenarios.
- Add pluginval/auval checks, native Windows/macOS CI, manual REAPER compatibility matrix, and clear issue reporting.
- Package self-contained Windows VST3 and macOS Universal (Intel + Apple Silicon) VST3/AU bundles on GitHub Releases.
- Provide ZIP installation at minimum; a Windows installer and macOS `.pkg` are desirable if reliable. Mac distribution signing/notarization requires configured Apple credentials; never claim unsigned artifacts are notarized.
- Check license notices, assets, installers, first-run preset behavior, documentation, and version metadata.
- **Acceptance:** a new user can install a release, load it into a supported DAW, and record without downloading anything else.

### Post-v1 (explicitly deferred)

Polyphonic or monophonic octaver, custom model/IR importing, flexible effect routing, more cabs, additional stereo options, standalone app, Linux/CLAP/AAX, intricate graphics, advanced model training. Prioritize based on musicians' needs rather than feature-count parity.

## 9. Distribution conventions

- Native OS builds are the source of public binaries: Windows runner -> `.vst3`; macOS runner -> `.vst3` and `.component`.
- Use GitHub Actions for PR checks and tag-triggered release packaging. Use `vMAJOR.MINOR.PATCH` tags and matching plugin metadata.
- Package the **entire bundle directory**, not only the inner shared library. Include presets/assets in verified platform-appropriate locations.
- Disable automatic deployment into CI runner audio-plugin directories unless needed for validation; make local installation opt-in/documented.
- Signing/notarization workflows must be optional until valid secrets are provided. Do not commit certificates, secrets, or tokens.
- Docker is optional for Linux-side tests or tooling; end-user plugins are **not** Docker containers.

## 10. Agent execution rules

1. Read this file, inspect `git status`, inspect existing files and current dependency versions before editing. Never assume a blank repo.
2. For each task, propose the smallest meaningful implementation slice and tests; then **implement**, don't only produce a speculative plan.
3. Preserve working files and user changes. Do not rewrite unrelated history or silently delete infrastructure.
4. Do not introduce placeholder audio algorithms, fake factory models, fictional performance claims, or mock features presented as working.
5. Do not import new frameworks or change the architecture merely because an alternative is fashionable. Explain needed deviations in `docs/ARCHITECTURE.md`.
6. If a platform SDK, dependency, signing identity, licensed asset, or DAW isn't available, finish the achievable pieces and state what remains unverified.
7. At the end, give a compact summary: changed files; commands/tests and results; known issues; next milestone. Do not leave broad TODO comments in place of the requested functionality.

## 11. Useful upstream references

- iPlug2: https://github.com/iPlug2/iPlug2
- iPlug2OOS template: https://github.com/iPlug2/iPlug2OOS
- iPlug2 CMake docs: https://iplug2.github.io/docs/md_cmake.html
- NeuralAmpModelerCore: https://github.com/sdatkinson/NeuralAmpModelerCore
- DaisySP: https://github.com/daisyaudio/DaisySP
- pluginval: https://github.com/Tracktion/pluginval

These URLs are references, not permission to copy arbitrary assets or to assume that all dependencies currently build together. Check versions, licenses, and toolchains at integration time.

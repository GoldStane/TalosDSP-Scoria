# TalosDSP Scoria

Scoria is a free, open-source modern-metal guitar amplifier and effects suite in development by **TalosDSP**.

**Version 0.0.1 is the M0 foundation:** a real pass-through effect with input trim, output level, smooth global bypass, and a native PEDALS | AMP | CAB | FX interface. It does **not** yet contain an amplifier, cabinet simulation, noise gate, or other effects. All planned modules are explicitly marked unavailable. The only preset is **M0 - Pass-through**.

- Windows x64: VST3 target.
- macOS 13 or later: Universal Apple Silicon/Intel VST3 and AUv2 targets.
- Mono → mono, mono → stereo (same sample at unity on each channel), stereo → stereo.
- Input/output: −60 to +24 dB each; bypass ignores both trims after a 10 ms transition.
- Zero processing latency; automation and versioned host state for the three global controls.
- Native C++17/iPlug2; no runtime downloads, accounts, Python, or external amp plugins.

## Build and test

See [BUILD.md](docs/BUILD.md) for prerequisites, native builds, installation, validation and manual listening steps. Dependencies are fetched at pinned commits during development configuration. End users install complete plugin bundles only.

```sh
cmake -S . -B build-core -DSCORIA_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --config Release
ctest --test-dir build-core -C Release --output-on-failure
```

Native Windows/macOS GitHub Actions compile the plugin targets, run tests, and archive complete bundles with notices. These are development artifacts, not a production-ready amp release. See [VALIDATION.md](docs/VALIDATION.md) for what has actually been tested.

## Roadmap and contributing

[AGENTS.md](AGENTS.md) is the product and engineering specification. [ARCHITECTURE.md](docs/ARCHITECTURE.md) describes the M0 implementation and real-time boundary.

M1 adds safely prepared NAM inference, a documented redistributable high-gain **Distortion amplifier**, a complementary cabinet IR, a noise gate and a separate overdrive boost. M2 adds modulation, delay, reverb and additional amp voices. No model, IR, commercial preset or proprietary artwork is bundled in M0.

Original code is [MIT licensed](LICENSE.md), preserving the existing copyright holder. Dependencies have separate notices in [LICENSING.md](docs/LICENSING.md) and [THIRD_PARTY_NOTICES.md](docs/THIRD_PARTY_NOTICES.md). Scoria is a working product name; trademark clearance is not claimed.

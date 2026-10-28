# TalosDSP Scoria

Scoria is an open-source modern-metal guitar amplifier for Windows x64 VST3 and macOS Universal VST3/AUv2, built with C++17 and iPlug2.

**0.0.3 — M1 acceptance candidate.** Tight Rhythm now combines an MIT-licensed Lum Herbert amp-only NAM capture with the CC0 Jester Cookie Monster 4×12 IR, noise gate and overdrive. It works at 44.1, 48, 88.2 and 96 kHz. The user assessed a render of their guitar DI as a usable rhythm tone. See [VALIDATION.md](docs/VALIDATION.md) for exact host/build results and remaining acceptance checks; this is not a v1.0 production-readiness claim.

## Play a guitar DI

1. Install the **whole** `Scoria.vst3` directory into `C:\Program Files\Common Files\VST3\` on Windows, or `~/Library/Audio/Plug-Ins/VST3/` on macOS. For AU, install the whole `Scoria.component` into `~/Library/Audio/Plug-Ins/Components/`. Preserve Resources and assets. Binaries are unsigned.
2. Rescan in REAPER. Use a supported sample rate and insert Scoria on a mono DI track, or monitor a mono instrument/Hi-Z input. Avoid simultaneous direct dry monitoring from the interface.
3. New instances select **Tight Rhythm**. Wait for **Assets ready** before live playback. All required models, IRs and licenses are bundled; no downloads, Python, dependency compilation or internet access are needed.
4. Start with DI peaks around −24 to −12 dBFS and quiet monitoring. Input trim changes drive into the chain; Output level defaults to −12 dB for headroom. Hot pickups or added gain may need attenuation. There is no hidden output limiter.
5. PEDALS controls gate and boost; AMP selects the audited NAM capture and its processing trims; CAB controls the IR. **Distortion is the amplifier voice**, not a distortion pedal. Phaser, Chorus, Crunch, Clean, Delay and Reverb remain unavailable.
6. Save/reopen the project to recall the complete configuration. Global bypass is dry with compensated conversion latency. Use matched listening levels when comparing.

The old **Legacy development rig** remains available for existing sessions. Its RiffSmith capture includes speaker coloration, so its factory preset leaves the extra cabinet off. Explicit cabinet choices in old projects are preserved.

Tight Rhythm defaults: gate −52 dBFS / 120 ms; boost drive 3%, tone 45%, level +3 dB; global input and NAM trims 0 dB; cabinet on at 0 dB; global output −12 dB. These are starting settings, not universal pickup calibration.

## Development and licensing

See [BUILD.md](docs/BUILD.md), [ARCHITECTURE.md](docs/ARCHITECTURE.md) and [VALIDATION.md](docs/VALIDATION.md). Models are restricted to an audited registry; arbitrary model/IR import is deferred. Initial preparation and live model/rate changes can temporarily mute processing. Offline lifecycle preparation is handled before processing, not by blocking the audio callback.

Original code is [MIT](LICENSE.md). Dependencies and assets have separate notices: [LICENSING.md](docs/LICENSING.md), [THIRD_PARTY_NOTICES.md](docs/THIRD_PARTY_NOTICES.md), [asset manifest](assets/manifest.json). No commercial plugin graphics, proprietary code or guitar recordings are distributed.

Finish the documented M1 host/performance acceptance checks before adding M2 effects.

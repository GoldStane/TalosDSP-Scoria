# TalosDSP Scoria

Scoria is an open-source guitar amplifier and effects plugin for Windows x64 VST3 and macOS Universal VST3/AUv2, built with C++17 and iPlug2.

**Current status: 0.0.2, M1 development — not production-ready and not full M1 acceptance.** Real NAM inference, a recorded cabinet IR, noise gate and overdrive are implemented. The bundled MIT RiffSmith Distorted capture already includes a cabinet, so the development preset leaves the separate cabinet bypassed. A licensed amp-only high-gain capture, tuned Tight Rhythm preset, listening validation and current Windows validation remain required.

## Try it with a guitar DI

1. Install the **whole** `Scoria.vst3` bundle. On Windows use `C:\Program Files\Common Files\VST3\`. On macOS use `~/Library/Audio/Plug-Ins/VST3/`, or install the whole `Scoria.component` under `~/Library/Audio/Plug-Ins/Components/` for AU. Keep the bundled Resources directory intact. These development binaries are unsigned.
2. Rescan plugins in REAPER and set the audio device/project to **48 kHz**. The bundled NAM does not run at other rates; the status bar reports this and the enabled amp stays silent.
3. Put Scoria on a mono audio track containing your own dry guitar DI, or arm a mono input from your interface's instrument/Hi-Z socket. Enable REAPER input monitoring; avoid simultaneously hearing the interface's direct dry monitoring. Start with quiet monitors and conservative interface gain, without input clipping.
4. Select the sole **Development - Distortion rig (48 kHz)** configuration. Wait for the asset-ready status. It enables gate, overdrive and NAM. It does not require another plugin, asset download, Python or internet access.
5. Adjust Input trim for the signal entering the effects, then Output level for listening volume. PEDALS exposes gate and boost. AMP exposes NAM input/output trims, **not physical amp knobs**. The default capture already contains a cabinet: leave CAB off unless deliberately testing an extra IR.
6. Toggle Global bypass for the original DI. Save/reopen the project to test recall. Record palm mutes, sustained notes and chords; compare at matched volume. Report glitches, silence, levels, CPU, sample rate and buffer size with any issue.

The four pages are **PEDALS | AMP | CAB | FX**. Phaser, chorus, delay, reverb, Crunch and Clean remain unavailable. Distortion is the amplifier voice, not a distortion pedal. There is no promised Tight Rhythm preset yet.

## Build and validation

See [BUILD.md](docs/BUILD.md) for native commands, installation and manual checks; [VALIDATION.md](docs/VALIDATION.md) records what was actually tested. [ARCHITECTURE.md](docs/ARCHITECTURE.md) describes DSP, rate limitations, state migration and audio-safe ownership.

Original code is MIT: [LICENSE.md](LICENSE.md). Dependencies and audio assets have separate licenses: [LICENSING.md](docs/LICENSING.md), [THIRD_PARTY_NOTICES.md](docs/THIRD_PARTY_NOTICES.md), and the [asset manifest](assets/manifest.json). No proprietary plugin resources or copyrighted music are included.

The next milestone task is to finish M1 acceptance: obtain an explicitly redistributable amp-only modern-metal capture, pair and tune the recorded IR, validate the Tight Rhythm preset on real DI in REAPER, and verify Windows/native CI. Effects expansion comes afterward.

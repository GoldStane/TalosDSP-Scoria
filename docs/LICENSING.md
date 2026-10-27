# Licensing and provenance

Original Scoria code is MIT, copyright 2026 Radmir. LICENSE.md preserves the original LICENSE verbatim; the original file remains intact. The Scoria brand is a working name, not a trademark-clearance claim.

## Pinned build inputs

- iPlug2: https://github.com/iPlug2/iPlug2/tree/d82b2e99b17c8f723d3f6e2a88ffd8b656f28e5d — zlib-like license. Its checked-in dependencies inherit this exact revision, not floating package versions.
- iPlug2OOS reference: https://github.com/iPlug2/iPlug2OOS/tree/9b214c1121a0da1e8db14b86a565c942a3bfea7e — inspected for CMake conventions; no commercial example art, fonts or installer imagery copied.
- Steinberg VST3 SDK: https://github.com/steinbergmedia/vst3sdk/tree/3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96 — MIT. `base`, `pluginterfaces`, and `public.sdk` submodules are resolved to that commit's gitlinks. Their individual LICENSE.txt files were inspected and are MIT. This finding applies to this selected revision, not to historical SDK releases.

The used transitive code includes Cockos WDL (zlib-like), NanoVG and FontStash (zlib), NanoSVG (zlib), MetalNanoVG (MIT), stb image/truetype (public-domain notices in the older bundled revisions), stb textedit/image_write (MIT alternative selected), Khronos OpenGL platform headers (MIT-style), and Destroy FX AU utilities (BSD 3-clause). Full relevant notices are reproduced in THIRD_PARTY_NOTICES.md. The generated GL loader is GLAD 0.1.28; its generator's MIT notice and generated-code provenance are included there. These components do not impose a copyleft license on original Scoria code.

The broad iPlug2 checkout includes optional software which is not selected by Scoria. No Skia, Yoga/FlexBox, RTAudio/RTMidi standalone, WebView, AAX or CLAP code is linked by the explicit plugin targets. Do not infer that any arbitrary optional file has the same license as iPlug2. Re-audit notices when changing linked targets/backends.

## M1 dependencies

- NeuralAmpModelerCore v0.5.4, commit `1f42f88535884450104b8711d7595019afa0495b`: MIT, Steven Atkinson. Compiled directly from its pinned source via CMake; no copied framework source. `assets/licenses/NAM-Core-MIT.txt` contains the notice.
- Eigen gitlink `bc3b39870ecb690a623a3f49149a358b95c5781d`: primarily **MPL-2.0**, with compatible third-party notices. This is a file-level copyleft dependency, not an MIT dependency. `EIGEN_MPL2_ONLY` is enabled; no unsupported modules are selected. Eigen is unmodified. Covered source remains available at the pinned source/archive links in `assets/licenses/Eigen-SOURCE.md`, which accompanies binary bundles with MPL2 and ancillary notices. Original Scoria source remains MIT.
- nlohmann/json 3.12.0, as vendored in the pinned NAM revision: MIT, Niels Lohmann; its Grisu2 conversion includes Florian Loitsch's MIT notice. See `assets/licenses/nlohmann-MIT.txt`.
- NAM's AudioDSPTools submodule is not fetched or linked. No new WAV/audio library is required.

## Bundled audio assets

`assets/manifest.json` records exact URLs, stable IDs, SHA-256, sample rates, creators and permission evidence. CMake verifies hashes. Neither free download nor NAM Core's MIT license is used as a substitute for asset permission.

**RiffSmith Distorted**, by Matt Hartman: author-owned repository commit `228b801ccc8b518d0a1fb53a8e63355a2fbd153f` explicitly includes its four captures under MIT. The exact LICENSE and relevant README permission statements are preserved in `assets/licenses`. The unmodified model runs at 48 kHz. Its metadata explicitly says `amp_cab`, and its hardware identity is not documented. We do not claim a particular physical amp or an amp-only capture. It is a development rig, not a listening-validated Tight Rhythm preset.

**Jester Cookie Monster**, by Bastian Karschewski / Jester Dyne Productions: original pack 1.0 handbook page 3 explicitly dedicates the pack to **CC0**. Page 5 identifies patch 1 as Vintage 30 / SM57; page 6 identifies the modified oversized BG412S cabinet. The original mono 48 kHz WAV is included unchanged. Offline preparation trims/fades the room tail and resamples for the host; those transformations are documented in ARCHITECTURE.md. The original handbook is included as permission evidence. No endorsement by hardware manufacturers is implied.

Arial is requested from the operating system, not redistributed. No commercial plugin artwork, copyrighted music or guitar recordings are bundled. Test audio consists of original synthetic signals and the licensed assets above. Scoria's SHA-256 and DSP implementations are original code; no proprietary pedal implementation is copied.

CI packages include LICENSE.md, THIRD_PARTY_NOTICES.md and complete plugin bundles with asset licenses. macOS packages remain unsigned/not notarized. Retain the Eigen source availability notice when redistributing binaries.

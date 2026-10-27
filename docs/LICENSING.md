# Licensing and provenance

Original Scoria code is MIT, copyright 2026 Radmir. LICENSE.md preserves the original LICENSE verbatim; the original file remains intact. The Scoria brand is a working name, not a trademark-clearance claim.

## Pinned build inputs

- iPlug2: https://github.com/iPlug2/iPlug2/tree/d82b2e99b17c8f723d3f6e2a88ffd8b656f28e5d — zlib-like license. Its checked-in dependencies inherit this exact revision, not floating package versions.
- iPlug2OOS reference: https://github.com/iPlug2/iPlug2OOS/tree/9b214c1121a0da1e8db14b86a565c942a3bfea7e — inspected for CMake conventions; no commercial example art, fonts or installer imagery copied.
- Steinberg VST3 SDK: https://github.com/steinbergmedia/vst3sdk/tree/3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96 — MIT. `base`, `pluginterfaces`, and `public.sdk` submodules are resolved to that commit's gitlinks. Their individual LICENSE.txt files were inspected and are MIT. This finding applies to this selected revision, not to historical SDK releases.

The used transitive code includes Cockos WDL (zlib-like), NanoVG and FontStash (zlib), NanoSVG (zlib), MetalNanoVG (MIT), stb image/truetype (public-domain notices in the older bundled revisions), stb textedit/image_write (MIT alternative selected), Khronos OpenGL platform headers (MIT-style), and Destroy FX AU utilities (BSD 3-clause). Full relevant notices are reproduced in THIRD_PARTY_NOTICES.md. The generated GL loader is GLAD 0.1.28; its generator's MIT notice and generated-code provenance are included there. These components do not impose a copyleft license on original Scoria code.

The broad iPlug2 checkout includes optional software which is not selected by Scoria. No Skia, Yoga/FlexBox, RTAudio/RTMidi standalone, WebView, AAX, CLAP, or NAM code is linked by the explicit M0 plugin targets. Do not infer that any arbitrary optional file has the same license as iPlug2. Re-audit notices when changing linked targets/backends.

Arial is requested from the operating system, not redistributed. No model, IR, capture, music, test recording, or proprietary plugin imagery is bundled. Synthetic constants in tests are original. M1 must add a separate asset manifest with authors, exact redistribution rights, permission evidence, source URLs, hashes, and sample-rate/model metadata before any audio asset is committed. NAM Core's license alone does not license captures.

CI packages include LICENSE.md and THIRD_PARTY_NOTICES.md beside the complete plugin bundles. macOS packages are unsigned/not notarized until a later distribution workflow has actual signing credentials.

`m1-v2.bin` is the 144-byte Scoria state payload extracted from the user's actual
REAPER `scoria.RPP` project on 2026-10-28. It contains version 2, stable asset IDs
and the original 16 numeric parameters only. No recording, project name, path,
track name, or other personal project data is included. Cabinet was explicitly
enabled in this saved state: migration must preserve that choice even though the
legacy capture already contains speaker coloration.

`m1-v3.bin` is the 152-byte state payload saved by REAPER after selecting Tight Rhythm in the updated VST3. It contains only schema/asset/preset IDs and the 17 parameter values. Tests compare it against the complete factory configuration.

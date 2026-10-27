Source: https://github.com/hartmamt/riffsmith/blob/228b801ccc8b518d0a1fb53a8e63355a2fbd153f/README.md

The MIT license in `LICENSE` covers RiffSmith's own code and the bundled RiffSmith Distorted, RiffSmith Clean, RiffSmith Bass Clean and RiffSmith Bass Distorted captures. Bundled third-party audio and engine assets carry their own licenses, listed here.

Four NAM captures are bundled, all trained by Matt Hartman on his own rigs with the TONE3000 trainer and released with the repo under the MIT license: **RiffSmith Distorted** (`public/nam/models/riffsmith-distorted.nam`, the default guitar amp), **RiffSmith Clean** (`riffsmith-clean.nam`, a clean channel; set `tight` to 0 to keep the boost pedal out of it), **RiffSmith Bass Clean** (`riffsmith-bass-clean.nam`, the default bass channel, running in its own NAM instance) and **RiffSmith Bass Distorted** (`riffsmith-bass-distorted.nam`, the driven bass; `set_rig { bass_model }`). Users can load any other `.nam` file.

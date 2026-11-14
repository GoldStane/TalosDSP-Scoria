#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace scoria {
// Released IDs: append only. Asset indices map to stable IDs in assets/manifest.json.
enum ParameterId {
  InputGain = 0,
  OutputGain = 1,
  GlobalBypass = 2,
  GateEnabled,
  GateThreshold,
  GateRelease,
  OverdriveEnabled,
  OverdriveDrive,
  OverdriveTone,
  OverdriveLevel,
  AmpEnabled,
  AmpInput,
  AmpOutput,
  CabinetEnabled,
  CabinetSelection,
  CabinetOutput,
  ModelSelection,
  PhaserEnabled,
  PhaserRate,
  PhaserDepth,
  PhaserFeedback,
  PhaserMix,
  ChorusEnabled,
  ChorusRate,
  ChorusDepth,
  ChorusMix,
  DelayEnabled,
  DelayTime,
  DelayFeedback,
  DelayMix,
  ReverbEnabled,
  ReverbDecay,
  ReverbDamping,
  ReverbMix,
  AmplifierVoice,
  ParameterCount
};
constexpr double kMinimumGainDb = -60., kMaximumGainDb = 24.;
constexpr std::int32_t kStateVersion = 4;
struct ParameterDefinition {
  const char *name;
  const char *unit;
  double minimum, maximum, initial, step;
};
inline constexpr std::array<ParameterDefinition, ParameterCount> kDefinitions{
    {{"Input trim", "dB", -60, 24, 0, .1},
     {"Output level", "dB", -60, 24, 0, .1},
     {"Global bypass", "", 0, 1, 0, 1},
     {"Gate enabled", "", 0, 1, 0, 1},
     {"Gate threshold", "dBFS", -80, 0, -50, .1},
     {"Gate release", "ms", 10, 1000, 100, 1},
     {"Overdrive enabled", "", 0, 1, 0, 1},
     {"Overdrive drive", "%", 0, 100, 10, .1},
     {"Overdrive tone", "%", 0, 100, 50, .1},
     {"Overdrive level", "dB", -24, 24, 6, .1},
     {"Amplifier enabled", "", 0, 1, 0, 1},
     {"NAM input trim", "dB", -24, 24, 0, .1},
     {"NAM output level", "dB", -60, 24, 0, .1},
     {"Cabinet enabled", "", 0, 1, 0, 1},
     {"Cabinet IR", "", 0, 1, 1, 1},
     {"Cabinet output", "dB", -60, 24, 0, .1},
     {"Distortion model", "", 0, 1, 1, 1},
     {"Phaser enabled", "", 0, 1, 0, 1},
     {"Phaser rate", "Hz", .05, 10, .3, .01},
     {"Phaser depth", "%", 0, 100, 65, .1},
     {"Phaser feedback", "%", -90, 90, 20, .1},
     {"Phaser mix", "%", 0, 100, 50, .1},
     {"Chorus enabled", "", 0, 1, 0, 1},
     {"Chorus rate", "Hz", .05, 8, .8, .01},
     {"Chorus depth", "%", 0, 100, 35, .1},
     {"Chorus mix", "%", 0, 100, 35, .1},
     {"Delay enabled", "", 0, 1, 0, 1},
     {"Delay time", "ms", 1, 2000, 360, 1},
     {"Delay feedback", "%", 0, 95, 30, .1},
     {"Delay mix", "%", 0, 100, 20, .1},
     {"Reverb enabled", "", 0, 1, 0, 1},
     {"Reverb decay", "s", .2, 15, 2.5, .01},
     {"Reverb damping", "%", 0, 100, 50, .1},
     {"Reverb mix", "%", 0, 100, 15, .1},
     {"Amplifier voice", "", 0, 2, 0, 1}}};
struct Parameters {
  double inputDb = 0., outputDb = 0.;
  bool bypass = false;
  bool gate = false;
  double thresholdDb = -50., releaseMs = 100.;
  bool overdrive = false;
  double drive = .1, tone = .5, overdriveDb = 6.;
  bool amplifier = false;
  double ampInputDb = 0., ampOutputDb = 0.;
  bool cabinet = false;
  int cabinetId = 1;
  double cabinetDb = 0.;
  int modelIndex = 0;
  bool phaser = false;
  double phaserRate = .3, phaserDepth = .65, phaserFeedback = .2, phaserMix = .5;
  bool chorus = false;
  double chorusRate = .8, chorusDepth = .35, chorusMix = .35;
  bool delay = false;
  double delayMs = 360., delayFeedback = .3, delayMix = .2;
  bool reverb = false;
  double reverbDecay = 2.5, reverbDamping = .5, reverbMix = .15;
};
inline double sanitizeGain(double db) noexcept {
  return std::isfinite(db) ? std::clamp(db, kMinimumGainDb, kMaximumGainDb) : 0.;
}
inline double boundedSelection(double value) noexcept {
  return std::isfinite(value) ? std::clamp(value, 0., 1.) : 0.;
}
inline Parameters fromValues(const std::array<double, ParameterCount> &v) noexcept {
  return {v[0],
          v[1],
          v[2] > .5,
          v[3] > .5,
          v[4],
          v[5],
          v[6] > .5,
          v[7] * .01,
          v[8] * .01,
          v[9],
          v[10] > .5,
          v[11],
          v[12],
          v[13] > .5,
          int(boundedSelection(v[14])),
          v[15],
          v[34] == 1.   ? 3
          : v[34] == 2. ? 2
                        : int(boundedSelection(v[16])),
          v[17] > .5,
          v[18],
          v[19] * .01,
          v[20] * .01,
          v[21] * .01,
          v[22] > .5,
          v[23],
          v[24] * .01,
          v[25] * .01,
          v[26] > .5,
          v[27],
          v[28] * .01,
          v[29] * .01,
          v[30] > .5,
          v[31],
          v[32] * .01,
          v[33] * .01};
}
inline bool validateValues(const std::array<double, ParameterCount> &values) noexcept {
  for (int i = 0; i < ParameterCount; ++i) {
    const auto &d = kDefinitions[i];
    if (!std::isfinite(values[i]) || values[i] < d.minimum || values[i] > d.maximum ||
        (d.step == 1. && (d.maximum <= 1. || i == AmplifierVoice) &&
         values[i] != std::floor(values[i])))
      return false;
  }
  return true;
}
enum class AmpVoice { Distortion, Crunch, Clean };
} // namespace scoria

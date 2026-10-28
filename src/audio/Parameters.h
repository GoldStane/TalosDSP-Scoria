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
  ParameterCount
};
constexpr double kMinimumGainDb = -60., kMaximumGainDb = 24.;
constexpr std::int32_t kStateVersion = 3;
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
     {"Distortion enabled", "", 0, 1, 0, 1},
     {"NAM input trim", "dB", -24, 24, 0, .1},
     {"NAM output level", "dB", -60, 24, 0, .1},
     {"Cabinet enabled", "", 0, 1, 0, 1},
     {"Cabinet IR", "", 0, 1, 1, 1},
     {"Cabinet output", "dB", -60, 24, 0, .1},
     {"Distortion model", "", 0, 1, 1, 1}}};
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
};
inline double sanitizeGain(double db) noexcept {
  return std::isfinite(db) ? std::clamp(db, kMinimumGainDb, kMaximumGainDb) : 0.;
}
inline Parameters fromValues(const std::array<double, ParameterCount> &v) noexcept {
  return {v[0],      v[1],       v[2] > .5,  v[3] > .5, v[4],       v[5],
          v[6] > .5, v[7] * .01, v[8] * .01, v[9],      v[10] > .5, v[11],
          v[12],     v[13] > .5, int(v[14]), v[15],     int(v[16])};
}
inline bool validateValues(const std::array<double, ParameterCount> &values) noexcept {
  for (int i = 0; i < ParameterCount; ++i) {
    const auto &d = kDefinitions[i];
    if (!std::isfinite(values[i]) || values[i] < d.minimum || values[i] > d.maximum ||
        (d.step == 1. && d.maximum <= 1. && values[i] != std::floor(values[i])))
      return false;
  }
  return true;
}
enum class AmpVoice { Distortion, Crunch, Clean };
} // namespace scoria

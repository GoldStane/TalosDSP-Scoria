#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace scoria {
// Released automation IDs: append only.
enum ParameterId { InputGain = 0, OutputGain = 1, GlobalBypass = 2, ParameterCount = 3 };
constexpr double kMinimumGainDb = -60.0;
constexpr double kMaximumGainDb = 24.0;
constexpr std::int32_t kStateVersion = 1;
struct Parameters {
  double inputDb = 0.0;
  double outputDb = 0.0;
  bool bypass = false;
};
inline double sanitizeGain(double db) noexcept {
  return std::isfinite(db) ? std::clamp(db, kMinimumGainDb, kMaximumGainDb) : 0.0;
}
// Future processing stages, in fixed signal order. No effect is active in M0.
enum class Stage {
  InputTrim,
  Gate,
  Overdrive,
  Phaser,
  Chorus,
  Amplifier,
  Cabinet,
  Delay,
  Reverb,
  OutputGain
};
enum class AmpVoice { Distortion, Crunch, Clean };
} // namespace scoria

#pragma once
#include "Parameters.h"
#include "models/ModelRegistry.h"
#include <cstring>
namespace scoria {
inline constexpr std::uint32_t kModelId = 0x52464431, kCabinetId = 0x4a434d31, kPresetId = 0;
// Schema 1: three doubles. Schema 2: legacy IDs +16 doubles.
// Schema 3: IDs +17 doubles. Schema 4: IDs +35 doubles. Asset IDs never depend on paths.
inline int decodeState(const unsigned char *data, int size, int position,
                       std::array<double, ParameterCount> &result,
                       std::uint32_t *restoredPreset = nullptr) noexcept {
  if (!data || position < 0 || size < position)
    return -1;
  auto read = [&](auto &value) {
    if (size - position < int(sizeof(value)))
      return false;
    std::memcpy(&value, data + position, sizeof(value));
    position += sizeof(value);
    return true;
  };
  std::int32_t version = 0;
  if (!read(version) || version < 1 || version > 4)
    return -1;
  std::uint32_t model = kModelId, cab = kCabinetId, preset = 0;
  if (version >= 2 && (!read(model) || !read(cab) || !read(preset) || cab != kCabinetId))
    return -1;
  if (version == 2 && (model != kModelId || preset != 0))
    return -1;
  if (version >= 3 && (!modelById(model) || preset > (version == 3 ? 1u : 4u)))
    return -1;
  std::array<double, ParameterCount> candidate{};
  for (int i = 0; i < ParameterCount; ++i)
    candidate[i] = kDefinitions[i].initial;
  candidate[ModelSelection] = 0.;
  const int count = version == 1 ? 3 : version == 2 ? 16 : version == 3 ? 17 : ParameterCount;
  for (int i = 0; i < count; ++i)
    if (!read(candidate[i]))
      return -1;
  if (!validateValues(candidate))
    return -1;
  if (version >= 3 && kModels[fromValues(candidate).modelIndex].id != model)
    return -1;
  result = candidate;
  if (restoredPreset)
    *restoredPreset = preset;
  return position;
}
inline std::array<double, ParameterCount> factoryValues(bool tight) noexcept {
  std::array<double, ParameterCount> values{};
  for (int i = 0; i < ParameterCount; ++i)
    values[i] = kDefinitions[i].initial;
  values[GateEnabled] = values[OverdriveEnabled] = values[AmpEnabled] = 1.;
  values[ModelSelection] = tight ? 1. : 0.;
  values[CabinetEnabled] = tight ? 1. : 0.;
  if (tight) {
    values[OverdriveDrive] = 3.;
    values[OverdriveTone] = 45.;
    values[OverdriveLevel] = 3.;
    values[OutputGain] = -12.;
    values[GateThreshold] = -52.;
    values[GateRelease] = 120.;
  }
  return values;
}
inline std::array<double, ParameterCount> presetValues(std::uint32_t id) noexcept {
  auto values = factoryValues(id != 0);
  if (id == 2) {
    values[DelayEnabled] = values[ReverbEnabled] = 1.;
    values[DelayTime] = 360.;
    values[DelayFeedback] = 28.;
    values[DelayMix] = 18.;
    values[ReverbDecay] = 2.4;
    values[ReverbDamping] = 60.;
    values[ReverbMix] = 12.;
  }
  if (id == 3 || id == 4) {
    values[OverdriveEnabled] = 0.;
    values[AmplifierVoice] = id == 3 ? 2. : 1.;
    values[GateThreshold] = -65.;
    values[GateRelease] = 250.;
    if (id == 4) {
      // Reduce capture excitation rather than disguising harshness with output attenuation.
      values[AmpInput] = -3.;
      values[AmpOutput] = .6;
    }
    if (id == 3) {
      values[ChorusEnabled] = values[DelayEnabled] = values[ReverbEnabled] = 1.;
      values[ChorusMix] = 25.;
      values[DelayTime] = 480.;
      values[DelayFeedback] = 40.;
      values[DelayMix] = 22.;
      values[ReverbDecay] = 5.;
      values[ReverbDamping] = 55.;
      values[ReverbMix] = 28.;
    }
  }
  return values;
}
inline const char *presetName(std::uint32_t id) noexcept {
  return id == 4   ? "Crunch Rhythm"
         : id == 3 ? "Ambient Clean"
         : id == 2 ? "Modern Lead"
         : id == 1 ? "Tight Rhythm"
                   : "Legacy development rig";
}
} // namespace scoria

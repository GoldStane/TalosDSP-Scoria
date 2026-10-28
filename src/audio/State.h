#pragma once
#include "Parameters.h"
#include "models/ModelRegistry.h"
#include <cstring>
namespace scoria {
inline constexpr std::uint32_t kModelId = 0x52464431, kCabinetId = 0x4a434d31, kPresetId = 0;
// Schema 1: three doubles. Schema 2: legacy IDs +16 doubles.
// Schema 3: audited model ID, cab ID, preset ID,17 doubles. IDs never depend on paths.
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
  if (!read(version) || version < 1 || version > 3)
    return -1;
  std::uint32_t model = kModelId, cab = kCabinetId, preset = 0;
  if (version >= 2 && (!read(model) || !read(cab) || !read(preset) || cab != kCabinetId))
    return -1;
  if (version == 2 && (model != kModelId || preset != 0))
    return -1;
  if (version == 3 && (!modelById(model) || preset > 1))
    return -1;
  std::array<double, ParameterCount> candidate{};
  for (int i = 0; i < ParameterCount; ++i)
    candidate[i] = kDefinitions[i].initial;
  candidate[ModelSelection] = 0.;
  const int count = version == 1 ? 3 : version == 2 ? 16 : ParameterCount;
  for (int i = 0; i < count; ++i)
    if (!read(candidate[i]))
      return -1;
  if (!validateValues(candidate))
    return -1;
  if (version == 3 && kModels[int(candidate[ModelSelection])].id != model)
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
} // namespace scoria

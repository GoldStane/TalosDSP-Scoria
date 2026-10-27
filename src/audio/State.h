#pragma once
#include "Parameters.h"
#include <cstring>
namespace scoria {
inline constexpr std::uint32_t kModelId = 0x52464431, kCabinetId = 0x4a434d31, kPresetId = 0;
// Wire format retains M0 native little-endian doubles (all supported platforms).
// Decode into a candidate and commit only after complete validation.
inline int decodeState(const unsigned char *data, int size, int position,
                       std::array<double, ParameterCount> &result) noexcept {
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
  if (!read(version) || (version != 1 && version != 2))
    return -1;
  if (version == 2) {
    std::uint32_t model = 0, cab = 0, preset = 0;
    if (!read(model) || !read(cab) || !read(preset) || model != kModelId || cab != kCabinetId ||
        preset != kPresetId)
      return -1;
  }
  std::array<double, ParameterCount> candidate{};
  for (int i = 0; i < ParameterCount; ++i)
    candidate[i] = kDefinitions[i].initial;
  for (int i = 0; i < (version == 1 ? 3 : ParameterCount); ++i)
    if (!read(candidate[i]))
      return -1;
  if (!validateValues(candidate))
    return -1;
  result = candidate;
  return position;
}
} // namespace scoria

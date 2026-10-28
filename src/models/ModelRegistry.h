#pragma once
#include <array>
#include <cmath>
#include <cstdint>
namespace scoria {
struct ModelDefinition {
  std::uint32_t id;
  const char *name;
  const char *file;
  const char *sha256;
  const char *license;
  int sampleRate;
  bool includesCabinet;
};
// Append-only registry. Exact provenance and permission evidence live in assets/manifest.json.
inline constexpr std::array<ModelDefinition, 2> kModels{
    {{0x52464431, "RiffSmith Distorted (legacy amp + cab)", "riffsmith-distorted.nam",
      "ec52e16296f96fdd1d07d1aaa699e33e17df253f026f785f135080e1eb5db007", "MIT", 48000, true},
     {0x48425434, "Distortion - Lum Herbert 4 (amp only)", "volum-herbert-4.nam",
      "cb5c16b8ebb21efd778fba39eb231e27d40646048af105806772903453ff8f4b", "MIT", 48000, false}}};
inline const ModelDefinition *modelById(std::uint32_t id) noexcept {
  for (const auto &m : kModels)
    if (m.id == id)
      return &m;
  return nullptr;
}
inline bool supportedRate(double rate) noexcept {
  return rate == 44100. || rate == 48000. || rate == 88200. || rate == 96000.;
}
inline int inferenceLatency(double rate) noexcept {
  return !supportedRate(rate) || rate == 48000. ? 0 : 4 + int(std::ceil(64. + 64. * rate / 48000.));
}
} // namespace scoria

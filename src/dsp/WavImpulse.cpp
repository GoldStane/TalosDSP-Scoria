#include "WavImpulse.h"
#include "Control.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
namespace scoria {
std::vector<double> loadImpulse(const std::string &path, double rate) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file || file.tellg() < 44 || file.tellg() > 4 * 1024 * 1024)
    throw std::runtime_error("Missing or oversized IR WAV");
  file.seekg(0);
  std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), {});
  auto u16 = [&](size_t p) {
    if (p + 2 > bytes.size())
      throw std::runtime_error("Truncated WAV");
    return unsigned(bytes[p]) | (unsigned(bytes[p + 1]) << 8);
  };
  auto u32 = [&](size_t p) {
    if (p + 4 > bytes.size())
      throw std::runtime_error("Truncated WAV");
    return uint32_t(bytes[p]) | (uint32_t(bytes[p + 1]) << 8) | (uint32_t(bytes[p + 2]) << 16) |
           (uint32_t(bytes[p + 3]) << 24);
  };
  if (std::memcmp(bytes.data(), "RIFF", 4) || std::memcmp(bytes.data() + 8, "WAVE", 4))
    throw std::runtime_error("Expected RIFF WAVE");
  unsigned format = 0, channels = 0, bits = 0, sourceRate = 0, align = 0;
  size_t data = 0, size = 0;
  for (size_t p = 12; p + 8 <= bytes.size();) {
    const size_t n = u32(p + 4), start = p + 8;
    if (n > bytes.size() - start)
      throw std::runtime_error("Truncated WAV chunk");
    if (!std::memcmp(bytes.data() + p, "fmt ", 4)) {
      if (n < 16)
        throw std::runtime_error("Invalid WAV format");
      format = u16(start);
      channels = u16(start + 2);
      sourceRate = u32(start + 4);
      align = u16(start + 12);
      bits = u16(start + 14);
    } else if (!std::memcmp(bytes.data() + p, "data", 4)) {
      data = start;
      size = n;
    }
    p = start + n + (n & 1);
  }
  if (channels != 1 || !data || !size || sourceRate < 8000 || sourceRate > 192000 ||
      !std::isfinite(rate) || rate < 8000 || rate > 96000 ||
      !((format == 1 && (bits == 16 || bits == 24 || bits == 32)) || (format == 3 && bits == 32)) ||
      align != bits / 8 || size % align)
    throw std::runtime_error("Unsupported mono WAV format/rate");
  std::vector<double> source(size / align);
  for (size_t i = 0; i < source.size(); ++i) {
    const size_t p = data + i * align;
    if (format == 3) {
      uint32_t word = u32(p);
      float value;
      std::memcpy(&value, &word, 4);
      source[i] = value;
    } else if (bits == 16)
      source[i] = static_cast<int16_t>(u16(p)) / 32768.;
    else if (bits == 24) {
      int32_t word =
          int32_t(bytes[p]) | (int32_t(bytes[p + 1]) << 8) | (int32_t(bytes[p + 2]) << 16);
      if (word & 0x800000)
        word -= 0x1000000;
      source[i] = word / 8388608.;
    } else
      source[i] = static_cast<int32_t>(u32(p)) / 2147483648.;
    if (!std::isfinite(source[i]))
      throw std::runtime_error("Nonfinite IR sample");
  }
  // Preserve early cabinet response, omit late room tail. 42.67 ms, maximum4096 taps.
  const size_t count =
      std::min<size_t>(4096, std::min<size_t>(size_t(std::ceil(source.size() * rate / sourceRate)),
                                              size_t(rate * 2048. / 48000.)));
  if (!count)
    throw std::runtime_error("Empty prepared IR");
  std::vector<double> result(count);
  constexpr double pi = 3.14159265358979323846;
  const double cutoff = std::min(1., rate / sourceRate);
  for (size_t i = 0; i < count; ++i) {
    double value = 0.;
    const double position = i * sourceRate / rate;
    if (rate == sourceRate)
      value = source[i];
    else
      for (int k = -32; k <= 32; ++k) {
        const long index = long(std::floor(position)) + k;
        if (index < 0 || size_t(index) >= source.size())
          continue;
        const double distance = position - index;
        if (std::abs(distance) > 32.)
          continue;
        const double z = pi * distance * cutoff;
        const double sinc = std::abs(z) < 1e-12 ? 1. : std::sin(z) / z;
        value += source[size_t(index)] * cutoff * sinc * (.5 + .5 * std::cos(pi * distance / 32.));
      }
    // Resampled impulse coefficients include time-step normalization.
    value *= sourceRate / rate;
    const size_t fade = std::min<size_t>(128, count);
    if (i >= count - fade)
      value *= .5 + .5 * std::cos(pi * double(i - (count - fade) + 1) / fade);
    result[i] = value;
  }
  return result;
}
} // namespace scoria

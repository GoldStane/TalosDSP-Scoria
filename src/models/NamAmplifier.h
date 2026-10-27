#pragma once
#include <memory>
#include <string>
namespace scoria {
class NamAmplifier {
public:
  NamAmplifier();
  ~NamAmplifier();
  NamAmplifier(const NamAmplifier &) = delete;
  NamAmplifier &operator=(const NamAmplifier &) = delete;
  // Caller must suspend processing before loading/replacing. Construction, validation,
  // prewarming and destruction are never performed by process().
  bool load(const std::string &path, double sampleRate, std::string &error) noexcept;
  bool ready() const noexcept;
  double sampleRate() const noexcept;
  void process(double *left, double *right, int channels, int frames) noexcept;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace scoria

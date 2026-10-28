#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <vector>
namespace scoria {
// Causal rational polyphase windowed-sinc converter. Coefficients are prepared
// off-thread; integer phase accumulation preserves duration across any block split.
class Resampler {
public:
  void prepare(int inputRate, int outputRate, double delay = 64.) {
    if (inputRate < 8000 || outputRate < 8000 || delay < 64. || delay > 70.)
      throw std::runtime_error("Unsupported resampler configuration");
    const int divisor = std::gcd(inputRate, outputRate);
    up_ = outputRate / divisor;
    down_ = inputRate / divisor;
    if (up_ > 640 || down_ > 640)
      throw std::runtime_error("Unsupported rational rate");
    taps_ = int(std::ceil(delay + 64.)) + 1;
    coefficients_.resize(size_t(up_) * taps_);
    constexpr double pi = 3.14159265358979323846;
    const double cutoff = .94 * std::min(1., double(outputRate) / inputRate);
    for (int phase = 0; phase < up_; ++phase) {
      double sum = 0.;
      for (int k = 0; k < taps_; ++k) {
        const double distance = k + double(phase) / up_ - delay;
        const double z = pi * cutoff * distance;
        // Four-term Blackman-Harris window: low sidelobes, 128-sample support.
        const double a = pi * distance / 64.;
        const double window = std::abs(distance) > 64.
                                  ? 0.
                                  : .35875 + .48829 * std::cos(a) + .14128 * std::cos(2 * a) +
                                        .01168 * std::cos(3 * a);
        const double value = cutoff * (std::abs(z) < 1e-12 ? 1. : std::sin(z) / z) * window;
        coefficients_[size_t(phase) * taps_ + k] = value;
        sum += value;
      }
      for (int k = 0; k < taps_; ++k)
        coefficients_[size_t(phase) * taps_ + k] /= sum;
    }
    reset();
  }
  void reset() noexcept {
    history_.fill(0.);
    inputIndex_ = 0;
    next_ = 0;
  }
  template <class Emit> void push(double value, Emit &&emit) noexcept {
    history_[inputIndex_ % history_.size()] = value;
    while (next_ / up_ <= inputIndex_) {
      const auto index = next_ / up_;
      const int phase = int(next_ % up_);
      double sum = 0.;
      for (int k = 0; k < taps_; ++k)
        if (index >= static_cast<unsigned>(k))
          sum += history_[(index - k) % history_.size()] * coefficients_[size_t(phase) * taps_ + k];
      emit(sum);
      next_ += down_;
    }
    ++inputIndex_;
  }

private:
  std::array<double, 256> history_{};
  std::vector<double> coefficients_;
  std::uint64_t inputIndex_ = 0, next_ = 0;
  int up_ = 1, down_ = 1, taps_ = 129;
};
class SampleDelay {
public:
  void reset(int length) noexcept {
    length_ = length;
    position_ = 0;
    data_.fill(0.);
  }
  double process(double value) noexcept {
    if (!length_)
      return value;
    const double out = data_[position_];
    data_[position_] = value;
    if (++position_ == length_)
      position_ = 0;
    return out;
  }

private:
  std::array<double, 512> data_{};
  int length_ = 0, position_ = 0;
};
} // namespace scoria

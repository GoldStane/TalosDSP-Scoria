#pragma once
#include "DelayLine.h"
#include <array>
namespace scoria {
// Pre-amplifier mono chorus; independent input channels retain independent histories.
class Chorus {
public:
  void reset(double rate, bool enabled = false, double hz = .8, double depth = .35,
             double mix = .35) noexcept {
    rate_ = bounded(rate, 8000., 384000., 48000.);
    phase_ = 0.;
    delay_.reset();
    auto v = values(enabled, hz, depth, mix);
    for (size_t i = 0; i < v.size(); ++i)
      controls_[i].reset(v[i], rate_);
  }
  void configure(bool on, double hz, double depth, double mix) noexcept {
    auto v = values(on, hz, depth, mix);
    for (size_t i = 0; i < v.size(); ++i)
      controls_[i].set(v[i]);
  }
  double process(double input) noexcept {
    input = finite(input);
    const double on = controls_[0].next(), hz = controls_[1].next(), depth = controls_[2].next();
    const double mix = controls_[3].next() * on;
    phase_ += hz / rate_;
    phase_ -= std::floor(phase_);
    const double wet =
        delay_.read(rate_ * (.012 + .004 * depth * std::sin(6.283185307179586 * phase_)));
    delay_.push(input);
    return mix == 0. ? input : finite(input + (wet - input) * mix);
  }

private:
  static std::array<double, 4> values(bool on, double hz, double depth, double mix) noexcept {
    return {on ? 1. : 0., bounded(hz, .05, 8., .8), bounded(depth, 0., 1., .35),
            bounded(mix, 0., 1., .35)};
  }
  DelayLine delay_{6146};
  double rate_ = 48000., phase_ = 0.;
  std::array<Ramp, 4> controls_;
};
} // namespace scoria

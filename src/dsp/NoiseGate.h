#pragma once
#include "Control.h"
namespace scoria {
class NoiseGate {
public:
  void reset(double rate) noexcept {
    rate_ = rate;
    envelope_ = gain_ = 0.;
    open_ = false;
    attack_ = std::exp(-1. / (.0005 * rate));
  }
  void configure(double thresholdDb, double releaseMs) noexcept {
    threshold_ = std::pow(10., bounded(thresholdDb, -80., 0., -50.) / 20.);
    release_ = std::exp(-1. / (.001 * bounded(releaseMs, 10., 1000., 100.) * rate_));
  }
  double process(double x) noexcept {
    const double magnitude = std::abs(x);
    const double coefficient = magnitude > envelope_ ? attack_ : release_;
    envelope_ = finite(magnitude + coefficient * (envelope_ - magnitude));
    if (envelope_ >= threshold_)
      open_ = true;
    else if (envelope_ < threshold_ * .5)
      open_ = false;
    const double target = open_ ? 1. : 0.;
    gain_ = finite(target + (open_ ? attack_ : release_) * (gain_ - target));
    return x * gain_;
  }

private:
  double rate_ = 48000., envelope_ = 0., gain_ = 0., attack_ = 0., release_ = 0., threshold_ = .003;
  bool open_ = false;
};
} // namespace scoria

#pragma once
#include "Control.h"
#include <array>
namespace scoria {
// Six first-order allpasses. Feedback is bounded and input-normalized.
class Phaser {
public:
  void reset(double rate, bool enabled = false, double hz = .3, double depth = .65,
             double feedback = .2, double mix = .5) noexcept {
    rate_ = bounded(rate, 8000., 384000., 48000.);
    phase_ = last_ = 0.;
    history_.fill(0.);
    auto v = values(enabled, hz, depth, feedback, mix);
    for (size_t i = 0; i < v.size(); ++i)
      controls_[i].reset(v[i], rate_);
  }
  void configure(bool enabled, double hz, double depth, double feedback, double mix) noexcept {
    auto v = values(enabled, hz, depth, feedback, mix);
    for (size_t i = 0; i < v.size(); ++i)
      controls_[i].set(v[i]);
  }
  double process(double input) noexcept {
    input = finite(input);
    const double on = controls_[0].next(), hz = controls_[1].next(), depth = controls_[2].next();
    const double feedback = controls_[3].next(), mix = controls_[4].next() * on;
    phase_ += hz / rate_;
    phase_ -= std::floor(phase_);
    if (on == 0.) {
      history_.fill(0.);
      last_ = 0.;
      return input;
    }
    const double frequency = 700. * std::exp2(2.5 * depth * std::sin(6.283185307179586 * phase_));
    const double t = std::tan(3.141592653589793 * frequency / rate_), a = (t - 1.) / (t + 1.);
    double x = input * (1. - std::abs(feedback)) + feedback * last_;
    for (auto &z : history_) {
      const double y = a * x + z;
      z = finite(x - a * y);
      x = y;
    }
    last_ = finite(x);
    return finite(input + (last_ - input) * mix);
  }

private:
  static std::array<double, 5> values(bool on, double hz, double depth, double feedback,
                                      double mix) noexcept {
    return {on ? 1. : 0., bounded(hz, .05, 10., .3), bounded(depth, 0., 1., .65),
            bounded(feedback, -.9, .9, .2), bounded(mix, 0., 1., .5)};
  }
  double rate_ = 48000., phase_ = 0., last_ = 0.;
  std::array<double, 6> history_{};
  std::array<Ramp, 5> controls_;
};
} // namespace scoria

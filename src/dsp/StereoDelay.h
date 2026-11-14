#pragma once
#include "DelayLine.h"
#include <array>
namespace scoria {
// Ping-pong feedback. Mono sources enter left only, stereo sources enter both sides.
// Time changes crossfade two fixed read heads over 30 ms; rapid edits coalesce.
class StereoDelay {
public:
  void reset(double rate, bool enabled = false, double ms = 360., double feedback = .3,
             double mix = .2) noexcept {
    active_ = enabled;
    silence_ = 0;
    rate_ = bounded(rate, 8000., 384000., 48000.);
    for (auto &d : lines_)
      d.reset();
    current_ = next_ = requested_ = bounded(ms, 1., 2000., 360.) * rate_ * .001;
    transition_ = 0;
    length_ = std::max(1, int(rate_ * .03));
    auto v = values(enabled, feedback, mix);
    for (size_t i = 0; i < v.size(); ++i)
      controls_[i].reset(v[i], rate_);
  }
  void configure(bool on, double ms, double feedback, double mix) noexcept {
    if (on)
      active_ = true;
    requested_ = bounded(ms, 1., 2000., 360.) * rate_ * .001;
    auto v = values(on, feedback, mix);
    for (size_t i = 0; i < v.size(); ++i)
      controls_[i].set(v[i]);
  }
  void process(double &left, double &right, bool mono) noexcept {
    left = finite(left);
    right = finite(right);
    const double on = controls_[0].next(), feedback = controls_[1].next(),
                 mix = controls_[2].next();
    if (!active_)
      return;
    if (!transition_ && requested_ != current_) {
      next_ = requested_;
      transition_ = length_;
    }
    const double blend = transition_ ? 1. - double(transition_) / length_ : 0.;
    double wet[2];
    for (int c = 0; c < 2; ++c) {
      wet[c] = lines_[c].read(current_);
      if (transition_)
        wet[c] += (lines_[c].read(next_) - wet[c]) * blend;
    }
    if (transition_ && !--transition_)
      current_ = next_;
    lines_[0].push(on * (mono ? .5 * (left + right) : left) + feedback * wet[1]);
    lines_[1].push(on * (mono ? 0. : right) + feedback * wet[0]);
    if (on == 0. && std::abs(wet[0]) + std::abs(wet[1]) < 1e-15) {
      if (++silence_ > int(rate_ * 2.04))
        active_ = false;
    } else
      silence_ = 0;
    // Bypass restores unity dry and stops injection, but retains existing echoes.
    left = finite(left * (1. - mix * on) + mix * wet[0]);
    right = finite(right * (1. - mix * on) + mix * wet[1]);
  }

private:
  static std::array<double, 3> values(bool on, double feedback, double mix) noexcept {
    return {on ? 1. : 0., bounded(feedback, 0., .95, .3), bounded(mix, 0., 1., .2)};
  }
  std::array<DelayLine, 2> lines_{DelayLine(768002), DelayLine(768002)};
  bool active_ = false;
  int silence_ = 0;
  std::array<Ramp, 3> controls_;
  double rate_ = 48000., current_ = 17280., next_ = 17280., requested_ = 17280.;
  int transition_ = 0, length_ = 1440;
};
} // namespace scoria

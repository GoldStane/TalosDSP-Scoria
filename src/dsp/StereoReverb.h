#pragma once
#include "DelayLine.h"
#include <array>
namespace scoria {
// Eight-line Householder FDN with input allpass diffusion and orthogonal stereo taps.
// Per-line gains correspond to -60 dB at Decay seconds; damping only removes energy.
class StereoReverb {
public:
  void reset(double rate, bool enabled = false, double decay = 2.5, double damping = .5,
             double mix = .15) noexcept {
    active_ = enabled;
    silence_ = 0;
    rate_ = bounded(rate, 8000., 384000., 48000.);
    for (auto &d : lines_)
      d.reset();
    for (auto &d : diffusers_)
      d.reset();
    low_.fill(0.);
    previous_.fill(0.);
    high_.fill(0.);
    tick_ = 0;
    constexpr int primes[]{1493, 1601, 1747, 1867, 1999, 2131, 2281, 2411};
    for (int i = 0; i < 8; ++i)
      lengths_[i] = std::round(primes[i] * rate_ / 48000.);
    dc_ = std::exp(-6.283185307179586 * 15. / rate_);
    auto v = values(enabled, decay, damping, mix);
    for (size_t i = 0; i < v.size(); ++i)
      controls_[i].reset(v[i], rate_);
    coefficients(v[1], v[2]);
  }
  void configure(bool on, double decay, double damping, double mix) noexcept {
    if (on)
      active_ = true;
    auto v = values(on, decay, damping, mix);
    for (size_t i = 0; i < v.size(); ++i)
      controls_[i].set(v[i]);
  }
  void process(double &left, double &right) noexcept {
    left = finite(left);
    right = finite(right);
    const double on = controls_[0].next(), decay = controls_[1].next(),
                 damping = controls_[2].next(), mix = controls_[3].next();
    if (!active_)
      return;
    if (tick_++ % 32 == 0)
      coefficients(decay, damping);
    double input[2]{left * on, right * on};
    constexpr double times[]{.0047, .0073, .0111, .0137};
    for (int c = 0; c < 2; ++c)
      for (int stage = 0; stage < 4; ++stage) {
        auto &d = diffusers_[4 * c + stage];
        const double delayed = d.read(std::round(rate_ * times[stage] * (c ? 1.07 : 1.)));
        const double y = delayed - .6 * input[c];
        d.push(input[c] + .6 * y);
        input[c] = y;
      }
    constexpr double signL[]{1, 1, 1, 1, -1, -1, -1, -1};
    constexpr double signR[]{1, -1, 1, -1, 1, -1, 1, -1};
    double taps[8], sum = 0., wetL = 0., wetR = 0.;
    for (int i = 0; i < 8; ++i) {
      const double raw = lines_[i].read(lengths_[i]);
      high_[i] = finite((raw - previous_[i]) * (1. + dc_) * .5 + dc_ * high_[i]);
      previous_[i] = raw;
      low_[i] = finite(low_[i] + dampCoefficient_ * (high_[i] - low_[i]));
      taps[i] = low_[i] * gain_[i];
      sum += taps[i];
      wetL += raw * signL[i];
      wetR += raw * signR[i];
    }
    for (int i = 0; i < 8; ++i)
      lines_[i].push(taps[i] - .25 * sum + .25 * (input[0] * signL[i] + input[1] * signR[i]));
    double energy = 0.;
    for (double tap : taps)
      energy += std::abs(tap);
    if (on == 0. && energy < 1e-15) {
      if (++silence_ > int(rate_ * .12))
        active_ = false;
    } else
      silence_ = 0;
    left = finite(left * (1. - mix * on) + mix * wetL * .25);
    right = finite(right * (1. - mix * on) + mix * wetR * .25);
  }

private:
  static std::array<double, 4> values(bool on, double decay, double damping, double mix) noexcept {
    return {on ? 1. : 0., bounded(decay, .2, 15., 2.5), bounded(damping, 0., 1., .5),
            bounded(mix, 0., 1., .15)};
  }
  void coefficients(double decay, double damping) noexcept {
    for (int i = 0; i < 8; ++i)
      gain_[i] = std::exp(-6.907755278982137 * lengths_[i] / (rate_ * decay));
    dampCoefficient_ =
        1. - std::exp(-6.283185307179586 * (16000. * std::pow(.0625, damping)) / rate_);
  }
  std::array<DelayLine, 8> lines_{DelayLine(20002), DelayLine(20002), DelayLine(20002),
                                  DelayLine(20002), DelayLine(20002), DelayLine(20002),
                                  DelayLine(20002), DelayLine(20002)};
  std::array<DelayLine, 8> diffusers_{DelayLine(6146), DelayLine(6146), DelayLine(6146),
                                      DelayLine(6146), DelayLine(6146), DelayLine(6146),
                                      DelayLine(6146), DelayLine(6146)};
  std::array<double, 8> lengths_{}, gain_{}, low_{}, previous_{}, high_{};
  bool active_ = false;
  int silence_ = 0;
  std::array<Ramp, 4> controls_;
  double rate_ = 48000., dc_ = 0., dampCoefficient_ = 0.;
  unsigned tick_ = 0;
};
} // namespace scoria

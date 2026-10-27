#pragma once
#include "Control.h"
namespace scoria {
// Original frequency-shaped boost, not an exact hardware emulation. Four substeps
// reduce nonlinear aliasing; the post-filter is also the decimation low-pass.
class Overdrive {
public:
  void reset(double rate, int oversampling = 4) noexcept {
    factor_ = std::clamp(oversampling, 1, 16);
    rate_ = rate * factor_;
    low_ = toneState_ = dc_ = previous_ = 0.;
    highpass_ = 1. - std::exp(-6.283185307179586 * 720. / rate_);
    dcCoefficient_ = 1. - std::exp(-6.283185307179586 * 15. / rate_);
  }
  double process(double x, double drive, double tone, double level) noexcept {
    const double gain = 1. + 30. * drive;
    const double lp = 1. - std::exp(-6.283185307179586 * (900. + 6000. * tone) / rate_);
    double out = 0.;
    for (int i = 1; i <= factor_; ++i) {
      const double up = previous_ + (x - previous_) * (double(i) / factor_);
      low_ = finite(low_ + highpass_ * (up - low_));
      const double clipped = std::tanh((up - low_ * .85) * gain);
      toneState_ = finite(toneState_ + lp * (clipped - toneState_));
      dc_ = finite(dc_ + dcCoefficient_ * (toneState_ - dc_));
      out += toneState_ - dc_;
    }
    previous_ = x;
    return out / factor_ * level;
  }

private:
  int factor_ = 4;
  double rate_ = 192000., low_ = 0., toneState_ = 0., dc_ = 0., previous_ = 0., highpass_ = 0.,
         dcCoefficient_ = 0.;
};
} // namespace scoria

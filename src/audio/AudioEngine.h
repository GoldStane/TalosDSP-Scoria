#pragma once
#include "Parameters.h"
#include <limits>

namespace scoria {
class AudioEngine {
public:
  void reset(double sampleRate, Parameters parameters = {}) noexcept {
    rampSamples_ = static_cast<int>(
        (std::isfinite(sampleRate) && sampleRate >= 8000.0 && sampleRate <= 768000.0 ? sampleRate
                                                                                     : 48000.0) *
        0.01);
    gain_ = target_ = effectiveGain(parameters);
    remaining_ = 0;
  }

  template <typename Sample>
  void process(const Sample *const *inputs, int inputChannels, Sample *const *outputs,
               int outputChannels, int frames, Parameters parameters) noexcept {
    if (frames <= 0 || !outputs)
      return;
    const double nextTarget = effectiveGain(parameters);
    if (nextTarget != target_) {
      target_ = nextTarget;
      remaining_ = rampSamples_;
      step_ = (target_ - gain_) / remaining_;
    }
    const bool validLayout = (inputChannels == 1 && (outputChannels == 1 || outputChannels == 2)) ||
                             (inputChannels == 2 && outputChannels == 2);
    for (int frame = 0; frame < frames; ++frame) {
      if (remaining_ > 0) {
        gain_ += step_;
        if (--remaining_ == 0)
          gain_ = target_;
      }
      // Read both channels before writing: supports in-place mono expansion and stereo.
      const Sample left = validLayout && inputs && inputs[0] ? clean(inputs[0][frame]) : Sample{};
      const Sample right =
          validLayout && inputChannels == 2 && inputs && inputs[1] ? clean(inputs[1][frame]) : left;
      for (int channel = 0; channel < outputChannels; ++channel) {
        if (outputs[channel])
          outputs[channel][frame] =
              clean(static_cast<Sample>((channel == 0 ? left : right) * gain_));
      }
    }
  }

private:
  template <typename Sample> static Sample clean(Sample value) noexcept {
    return std::isfinite(value) &&
                   (value == Sample{} || std::abs(value) >= std::numeric_limits<Sample>::min())
               ? value
               : Sample{};
  }
  static double effectiveGain(Parameters p) noexcept {
    return p.bypass ? 1.0
                    : std::pow(10.0, (sanitizeGain(p.inputDb) + sanitizeGain(p.outputDb)) / 20.0);
  }
  double gain_ = 1.0, target_ = 1.0, step_ = 0.0;
  int remaining_ = 0, rampSamples_ = 480;
};
} // namespace scoria

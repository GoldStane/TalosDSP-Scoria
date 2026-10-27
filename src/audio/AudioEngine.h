#pragma once
#include "Parameters.h"
#include "dsp/NoiseGate.h"
#include "dsp/Overdrive.h"
#include "models/AssetLoader.h"
#include <array>
namespace scoria {
class AudioEngine {
public:
  void reset(double sampleRate, Parameters p = {}) noexcept {
    rate_ = bounded(sampleRate, 8000., 96000., 48000.);
    availability_[0].reset(0., rate_);
    availability_[1].reset(0., rate_);
    for (auto &gate : gates_)
      gate.reset(rate_);
    for (auto &drive : drives_)
      drive.reset(rate_);
    const auto values = targets(p);
    for (size_t i = 0; i < controls_.size(); ++i)
      controls_[i].reset(values[i], rate_);
  }
  // Assets are exclusively audio-owned between worker handoffs. Never destroyed here.
  void setAssets(ProcessingAssets *assets) noexcept {
    if (assets != assets_) {
      availability_[0].reset(0., rate_);
      availability_[1].reset(0., rate_);
    }
    assets_ = assets;
  }
  template <typename Sample>
  void process(const Sample *const *inputs, int inputChannels, Sample *const *outputs,
               int outputChannels, int frames, Parameters p) noexcept {
    if (frames <= 0 || !outputs)
      return;
    const bool valid = (inputChannels == 1 && (outputChannels == 1 || outputChannels == 2)) ||
                       (inputChannels == 2 && outputChannels == 2);
    const auto values = targets(p);
    for (size_t i = 0; i < controls_.size(); ++i)
      controls_[i].set(values[i]);
    for (auto &gate : gates_)
      gate.configure(p.thresholdDb, p.releaseMs);
    const bool ampReady = assets_ && assets_->rate == rate_ && assets_->amplifier.ready();
    const bool cabReady = assets_ && assets_->rate == rate_ && assets_->cabinets[0].ready();
    availability_[0].set(ampReady ? 1. : 0.);
    availability_[1].set(cabReady ? 1. : 0.);
    for (int base = 0; base < frames; base += 256) {
      const int count = std::min(256, frames - base);
      for (int i = 0; i < count; ++i) {
        for (size_t k = 0; k < controls_.size(); ++k)
          modulation_[k][i] = controls_[k].next();
        // Read both channels before any output writes, including mono expansion.
        dry_[0][i] = valid && inputs && inputs[0] ? finite(inputs[0][base + i]) : 0.;
        dry_[1][i] = valid && inputChannels == 2 && inputs && inputs[1]
                         ? finite(inputs[1][base + i])
                         : dry_[0][i];
        for (int c = 0; c < (inputChannels == 2 ? 2 : 1); ++c) {
          double x = dry_[c][i] * modulation_[In][i];
          const double gated = gates_[c].process(x);
          x += (gated - x) * modulation_[Gate][i];
          const double driven = drives_[c].process(x, modulation_[Drive][i], modulation_[Tone][i],
                                                   modulation_[Level][i]);
          x += (driven - x) * modulation_[OD][i];
          preAmp_[c][i] = x;
          wet_[c][i] = x * modulation_[AmpIn][i];
        }
      }
      if (ampReady)
        assets_->amplifier.process(wet_[0].data(), wet_[1].data(), inputChannels == 2 ? 2 : 1,
                                   count);
      for (int i = 0; i < count; ++i) {
        double result[2]{};
        const double ampMix = availability_[0].next(), cabMix = availability_[1].next();
        for (int c = 0; c < (inputChannels == 2 ? 2 : 1); ++c) {
          // An enabled unavailable amp is silent, never disguised as working dry DSP.
          const double amplified = ampReady ? wet_[c][i] * modulation_[AmpOut][i] * ampMix : 0.;
          double x = preAmp_[c][i] + (amplified - preAmp_[c][i]) * modulation_[Amp][i];
          const double cabinet =
              cabReady ? assets_->cabinets[c].process(x) * modulation_[CabOut][i] * cabMix : 0.;
          x += (cabinet - x) * modulation_[Cab][i];
          x *= modulation_[Out][i];
          result[c] = !valid ? 0.
                      : modulation_[Bypass][i] == 1.
                          ? dry_[c][i]
                          : finite(x + (dry_[c][i] - x) * modulation_[Bypass][i]);
        }
        if (inputChannels != 2)
          result[1] = result[0];
        for (int c = 0; c < outputChannels; ++c)
          if (outputs[c]) {
            const auto value = static_cast<Sample>(finite(c < 2 ? result[c] : 0.));
            outputs[c][base + i] = std::isfinite(value) ? value : Sample{};
          }
      }
    }
  }

private:
  enum Control {
    In,
    Out,
    Bypass,
    Gate,
    OD,
    Drive,
    Tone,
    Level,
    Amp,
    AmpIn,
    AmpOut,
    Cab,
    CabOut,
    Count
  };
  static std::array<double, Count> targets(Parameters p) noexcept {
    return {dbGain(p.inputDb),           dbGain(p.outputDb),
            p.bypass ? 1. : 0.,          p.gate ? 1. : 0.,
            p.overdrive ? 1. : 0.,       bounded(p.drive, 0., 1.),
            bounded(p.tone, 0., 1., .5), dbGain(p.overdriveDb),
            p.amplifier ? 1. : 0.,       dbGain(p.ampInputDb),
            dbGain(p.ampOutputDb),       p.cabinet && p.cabinetId == 1 ? 1. : 0.,
            dbGain(p.cabinetDb)};
  }
  double rate_ = 48000.;
  ProcessingAssets *assets_ = nullptr;
  Ramp availability_[2];
  NoiseGate gates_[2];
  Overdrive drives_[2];
  std::array<Ramp, Count> controls_;
  std::array<std::array<double, 256>, Count> modulation_{};
  std::array<std::array<double, 256>, 2> dry_{}, preAmp_{}, wet_{};
};
} // namespace scoria

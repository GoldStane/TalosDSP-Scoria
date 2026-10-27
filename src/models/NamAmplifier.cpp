#include "NamAmplifier.h"
#include "NAM/get_dsp.h"
#include "Sha256.h"
#include "dsp/Control.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
namespace scoria {
struct NamAmplifier::Impl {
  std::unique_ptr<nam::DSP> models[2];
  double rate = 0.;
};
NamAmplifier::NamAmplifier() = default;
NamAmplifier::~NamAmplifier() = default;
bool NamAmplifier::load(const std::string &path, double rate, std::string &error) noexcept {
  try {
    if (!std::isfinite(rate) || rate < 8000. || rate > 192000.)
      throw std::runtime_error("Invalid sample rate");
    if (std::filesystem::file_size(path) > 32 * 1024 * 1024)
      throw std::runtime_error("Model exceeds 32 MiB limit");
    std::ifstream file(path, std::ios::binary);
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), {});
    // Only the audited bundled model is supported in M1. Reject untrusted model
    // configurations before upstream parsers (some assume valid weight counts).
    if (sha256(bytes) != "ec52e16296f96fdd1d07d1aaa699e33e17df253f026f785f135080e1eb5db007")
      throw std::runtime_error(
          "Unsupported or corrupted NAM asset; expected riffsmith-distorted-v1");
    const auto document = nlohmann::json::parse(bytes);
    auto candidate = std::make_unique<Impl>();
    for (auto &model : candidate->models) {
      model = nam::get_dsp(document, nam::DspLoadOptions{false});
      if (!model || model->NumInputChannels() != 1 || model->NumOutputChannels() != 1)
        throw std::runtime_error(
            "Only mono NAM models are supported (stereo uses independent instances)");
      if (model->GetExpectedSampleRate() != rate)
        throw std::runtime_error("NAM sample rate mismatch; no resampling is performed");
      model->SetPrewarmOnReset(true);
      model->Reset(rate, 256);
    }
    candidate->rate = rate;
    impl_ = std::move(candidate);
    error.clear();
    return true;
  } catch (const std::exception &e) {
    error = e.what();
    return false;
  } catch (...) {
    error = "Unknown NAM preparation error";
    return false;
  }
}
bool NamAmplifier::ready() const noexcept { return bool(impl_); }
double NamAmplifier::sampleRate() const noexcept { return impl_ ? impl_->rate : 0.; }
void NamAmplifier::process(double *left, double *right, int channels, int frames) noexcept {
  if (!impl_ || frames <= 0 || !left || channels < 1 || channels > 2 || (channels == 2 && !right))
    return;
  for (int offset = 0; offset < frames; offset += 256) {
    const int count = std::min(256, frames - offset);
    for (int channel = 0; channel < channels; ++channel) {
      double *samples = (channel ? right : left) + offset;
      // All inference buffers were sized by Reset; fixed-rate mono models only.
      impl_->models[channel]->process(&samples, &samples, count);
      for (int i = 0; i < count; ++i)
        samples[i] = finite(samples[i]);
    }
  }
}
} // namespace scoria

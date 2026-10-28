#include "NamAmplifier.h"
#include "ModelRegistry.h"
#include "NAM/get_dsp.h"
#include "Sha256.h"
#include "dsp/Control.h"
#include "dsp/Resampler.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
namespace scoria {
struct NamAmplifier::Impl {
  std::unique_ptr<nam::DSP> models[2];
  double rate = 0.;
  Resampler toModel[2], toHost[2];
  std::array<std::array<double, 512>, 2> native{};
  std::array<std::array<double, 2048>, 2> queue{};
  size_t read[2]{0, 0}, write[2]{4, 4};
  bool adapted = false;
};
NamAmplifier::NamAmplifier() = default;
NamAmplifier::~NamAmplifier() = default;
bool NamAmplifier::load(const std::string &path, double rate, std::string &error) noexcept {
  try {
    if (!supportedRate(rate))
      throw std::runtime_error("Invalid sample rate");
    if (std::filesystem::file_size(path) > 32 * 1024 * 1024)
      throw std::runtime_error("Model exceeds 32 MiB limit");
    std::ifstream file(path, std::ios::binary);
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), {});
    const auto hash = sha256(bytes);
    const ModelDefinition *definition = nullptr;
    for (const auto &item : kModels)
      if (hash == item.sha256)
        definition = &item;
    if (!definition)
      throw std::runtime_error("Unsupported or corrupted NAM asset");
    const auto document = nlohmann::json::parse(bytes);
    auto candidate = std::make_unique<Impl>();
    for (auto &model : candidate->models) {
      model = nam::get_dsp(document, nam::DspLoadOptions{false});
      if (!model || model->NumInputChannels() != 1 || model->NumOutputChannels() != 1)
        throw std::runtime_error(
            "Only mono NAM models are supported (stereo uses independent instances)");
      if (model->GetExpectedSampleRate() != definition->sampleRate)
        throw std::runtime_error("NAM metadata disagrees with audited registry");
      model->SetPrewarmOnReset(true);
      model->Reset(definition->sampleRate, 256);
    }
    candidate->rate = rate;
    candidate->adapted = rate != definition->sampleRate;
    if (candidate->adapted)
      for (int c = 0; c < 2; ++c) {
        candidate->toModel[c].prepare(int(rate), definition->sampleRate);
        const double delay = (inferenceLatency(rate) - 4 - 64) * definition->sampleRate / rate;
        candidate->toHost[c].prepare(definition->sampleRate, int(rate), delay);
      }
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
      if (!impl_->adapted) {
        impl_->models[channel]->process(&samples, &samples, count);
      } else {
        int nativeCount = 0;
        for (int i = 0; i < count; ++i)
          impl_->toModel[channel].push(
              samples[i], [&](double x) { impl_->native[channel][nativeCount++] = x; });
        for (int base = 0; base < nativeCount; base += 256) {
          double *block = impl_->native[channel].data() + base;
          impl_->models[channel]->process(&block, &block, std::min(256, nativeCount - base));
        }
        for (int i = 0; i < nativeCount; ++i)
          impl_->toHost[channel].push(impl_->native[channel][i], [&](double x) {
            impl_->queue[channel][impl_->write[channel]++ % 2048] = x;
          });
        for (int i = 0; i < count; ++i)
          samples[i] = impl_->read[channel] < impl_->write[channel]
                           ? impl_->queue[channel][impl_->read[channel]++ % 2048]
                           : 0.;
      }
      for (int i = 0; i < count; ++i)
        samples[i] = finite(samples[i]);
    }
  }
}
} // namespace scoria

#include "audio/AudioEngine.h"
#include "audio/State.h"
#include "dsp/WavImpulse.h"
#include <Eigen/Core>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <numeric>
thread_local bool measureAllocations = false;
thread_local size_t audioAllocations = 0;
void *operator new(std::size_t size) {
  if (measureAllocations)
    ++audioAllocations;
  if (auto *p = std::malloc(size ? size : 1))
    return p;
  throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
void require(bool ok, const char *message) {
  if (!ok) {
    std::cerr << message << '\n';
    std::exit(1);
  }
}
int main(int argc, char **argv) {
  require(argc == 2, "asset path");
  const auto root = std::filesystem::path(argv[1]);
  {
    std::ifstream file(root.parent_path() / "tests/fixtures/m1-v2.bin", std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), {});
    std::array<double, scoria::ParameterCount> values{};
    require(bytes.size() == 144 &&
                scoria::decodeState(bytes.data(), int(bytes.size()), 0, values) == 144,
            "real REAPER v2 state fixture");
    require(values[scoria::ModelSelection] == 0 && values[scoria::CabinetEnabled] == 1,
            "legacy model and explicit cab choice retained");
  }
  {
    std::ifstream file(root.parent_path() / "tests/fixtures/m1-v3.bin", std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), {});
    std::array<double, scoria::ParameterCount> values{};
    std::uint32_t preset = 0;
    require(bytes.size() == 152 &&
                scoria::decodeState(bytes.data(), int(bytes.size()), 0, values, &preset) == 152,
            "real REAPER v3 state fixture");
    require(values == scoria::factoryValues(true) && preset == 1,
            "Tight Rhythm exact saved-state recall");
  }
  for (double rate : {44100., 48000., 88200., 96000.}) {
    {
      scoria::AudioEngine bypass;
      scoria::Parameters parameters;
      parameters.bypass = true;
      bypass.reset(rate, parameters);
      std::array<double, 2048> left{}, right{}, outputLeft{}, outputRight{};
      left[0] = .25;
      right[0] = -.125;
      const double *in[]{left.data(), right.data()};
      double *out[]{outputLeft.data(), outputRight.data()};
      bypass.process(in, 2, out, 2, 2048, parameters);
      for (int i = 0; i < 2048; ++i)
        require(outputLeft[i] == (i == scoria::inferenceLatency(rate) ? .25 : 0.) &&
                    outputRight[i] == (i == scoria::inferenceLatency(rate) ? -.125 : 0.),
                "global bypass impulse timing and stereo routing");
    }
    if (rate != 48000.) {
      scoria::Resampler down, up;
      down.prepare(int(rate), 48000);
      up.prepare(48000, int(rate), (scoria::inferenceLatency(rate) - 4 - 64) * 48000. / rate);
      std::vector<double> result(4, 0.);
      for (int i = 0; i < 4096; ++i)
        down.push(i == 0 ? 1. : 0.,
                  [&](double x) { up.push(x, [&](double y) { result.push_back(y); }); });
      const auto peak = std::max_element(result.begin(), result.end(), [](double a, double b) {
        return std::abs(a) < std::abs(b);
      });
      require(peak - result.begin() == scoria::inferenceLatency(rate),
              "resampler impulse alignment");
      require(result.size() >= 4096 && result.size() <= 4102, "resampler duration");
      down.reset();
      up.reset();
      result.assign(4, 0.);
      for (int i = 0; i < int(rate); ++i)
        down.push(std::sin(6.283185307179586 * 1000 * i / rate),
                  [&](double x) { up.push(x, [&](double y) { result.push_back(y); }); });
      double error = 0.;
      for (int i = 1000; i < int(rate); ++i)
        error = std::max(
            error, std::abs(result[i] - std::sin(6.283185307179586 * 1000 *
                                                 (i - scoria::inferenceLatency(rate)) / rate)));
      std::cout << "rate=" << rate << " latency=" << scoria::inferenceLatency(rate)
                << " sine_error=" << error << '\n';
      require(error < .001, "resampler pitch and amplitude");
    }
    const auto model = (root / "models/volum-herbert-4.nam").string();
    scoria::ProcessingAssets assets;
    assets.rate = rate;
    assets.modelIndex = 1;
    std::string error;
    if (!assets.amplifier.load(model, rate, error)) {
      std::cerr << error << '\n';
      return 1;
    }
    scoria::NamAmplifier alternate;
    require(alternate.load(model, rate, error), "second model preparation");
    std::vector<double> first(8192), second(8192);
    for (size_t i = 0; i < first.size(); ++i)
      first[i] = second[i] = .1 * std::sin(i * .13);
    measureAllocations = true;
    Eigen::internal::set_is_malloc_allowed(false);
    assets.amplifier.process(first.data(), nullptr, 1, int(first.size()));
    for (int i = 0; i < int(second.size()); i += 17)
      alternate.process(second.data() + i, nullptr, 1, std::min(17, int(second.size()) - i));
    measureAllocations = false;
    Eigen::internal::set_is_malloc_allowed(true);
    require(audioAllocations == 0, "resampled NAM has no C++/Eigen allocations");
    double energy = 0.;
    for (size_t i = 0; i < first.size(); ++i) {
      require(std::isfinite(first[i]), "finite NAM");
      require(std::abs(first[i] - second[i]) < 1e-5, "NAM independent of block boundaries");
      energy += std::abs(first[i]);
    }
    require(energy > .1, "amp-only model audible output");
    auto taps = scoria::loadImpulse((root / "irs/jester-cookie-monster.wav").string(), rate);
    for (auto &cab : assets.cabinets)
      cab.prepare(taps);
    auto p = scoria::fromValues(scoria::factoryValues(true));
    scoria::AudioEngine engine;
    engine.reset(rate, p);
    engine.setAssets(&assets);
    std::array<double, 2048> l{}, r{};
    const double *in[]{l.data(), r.data()};
    double *out[]{l.data(), r.data()};
    std::vector<double> times;
    double peak = 0.;
    for (int n = 0; n < 100; ++n) {
      for (int i = 0; i < 256; ++i)
        l[i] = r[i] = .1 * std::sin((n * 256 + i) * .13);
      auto start = std::chrono::steady_clock::now();
      measureAllocations = true;
      engine.process(in, 2, out, 2, 256, p);
      measureAllocations = false;
      require(audioAllocations == 0, "resampled full chain has no C++ allocations");
      times.push_back(
          std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start)
              .count());
      for (int i = 0; i < 256; ++i) {
        require(std::isfinite(l[i]), "finite full chain");
        peak = std::max(peak, std::abs(l[i]));
      }
    }
    std::sort(times.begin(), times.end());
    require(peak > 1e-5, "full factory chain non-silent");
    std::cout << "Herbert full stereo chain rate=" << rate << " block=256 mean_us="
              << std::accumulate(times.begin(), times.end(), 0.) / times.size()
              << " p99_us=" << times[98] << " max_us=" << times.back() << " peak=" << peak << '\n';
  }
}

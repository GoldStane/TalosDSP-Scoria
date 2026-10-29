#include "audio/AudioEngine.h"
#include "audio/State.h"
#include "dsp/WavImpulse.h"
#include "models/Sha256.h"
#include <Eigen/Core>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <numeric>
#include <thread>
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
void check(bool ok, const char *message) {
  if (!ok) {
    std::cerr << message << '\n';
    std::exit(1);
  }
}
int main(int argc, char **argv) {
#ifdef NDEBUG
  constexpr int repeats = 200;
#else
  constexpr int repeats = 8; // Debug builds check correctness, not performance.
#endif
  check(argc == 2, "asset root argument");
  const auto root = std::filesystem::path(argv[1]);
  for (double rate : {44100., 48000., 88200., 96000.}) {
    scoria::NoiseGate gate;
    gate.reset(rate);
    gate.configure(-40., 30.);
    double output = 0.;
    for (int i = 0; i < int(rate * .1); ++i)
      output = gate.process(.1);
    check(output > .099, "gate opens");
    for (int i = 0; i < int(rate); ++i)
      output = gate.process(.0001);
    check(std::abs(output) < 1e-8, "gate closes");
    scoria::Overdrive od;
    od.reset(rate);
    double difference = 0., sum = 0.;
    for (int i = 0; i < int(rate); ++i) {
      const double x = .2 * std::sin(6.28318530718 * 440 * i / rate);
      const double y = od.process(x, .6, .5, 2.);
      check(std::isfinite(y), "finite overdrive");
      difference += std::abs(y - x);
      sum += y;
    }
    check(difference / rate > .05, "overdrive changes signal");
    check(std::abs(sum / rate) < .005, "overdrive DC rejection");
    auto impulse = scoria::loadImpulse((root / "irs/jester-cookie-monster.wav").string(), rate);
    check(!impulse.empty() && impulse.size() <= 4096, "IR rate preparation");
    scoria::Cabinet cab;
    const std::vector<double> taps{.2, -.1, .7, 0., -.2};
    cab.prepare(taps);
    std::array<double, 100> signal{};
    for (size_t i = 0; i < signal.size(); ++i) {
      signal[i] = std::sin(i * .31);
      double reference = 0.;
      for (size_t k = 0; k < taps.size() && k <= i; ++k)
        reference += taps[k] * signal[i - k];
      check(std::abs(reference - cab.process(signal[i])) < 1e-12, "FIR matches reference");
    }
    cab.reset();
    check(cab.process(1.) == .2, "FIR reset");
    scoria::AudioEngine engine;
    scoria::Parameters p;
    p.overdrive = true;
    engine.reset(rate, p);
    double x = .3, y = 0.;
    const double *in[]{&x};
    double *out[]{&y};
    for (int i = 0; i < 1000; ++i)
      engine.process(in, 1, out, 1, 1, p);
    p.overdrive = false;
    for (int i = 0; i < 2000; ++i)
      engine.process(in, 1, out, 1, 1, p);
    check(std::abs(y - x) < 1e-12, "overdrive bypass settles dry");
  }
  check(scoria::sha256({}) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "SHA256 empty known vector");
  check(scoria::sha256({'a', 'b', 'c'}) ==
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "SHA256 abc known vector");
  std::vector<unsigned char> state;
  auto put = [&](const auto &value) {
    const auto *p = reinterpret_cast<const unsigned char *>(&value);
    state.insert(state.end(), p, p + sizeof(value));
  };
  std::array<double, scoria::ParameterCount> restored{};
  put(std::int32_t(1));
  put(6.);
  put(-3.);
  put(1.);
  check(scoria::decodeState(state.data(), int(state.size()), 0, restored) == int(state.size()),
        "M0 schema migration");
  check(restored[0] == 6. && restored[1] == -3. && restored[2] == 1. &&
            restored[scoria::AmpEnabled] == 0. && restored[scoria::OverdriveEnabled] == 0.,
        "M0 preserves pass-through");
  restored[scoria::GateEnabled] = 1.;
  restored[scoria::AmpEnabled] = 1.;
  restored[scoria::CabinetEnabled] = 1.;
  const auto expected = restored;
  state.clear();
  put(scoria::kStateVersion);
  put(scoria::kModelId);
  put(scoria::kCabinetId);
  put(scoria::kPresetId);
  for (double x : expected)
    put(x);
  check(scoria::decodeState(state.data(), int(state.size()), 0, restored) > 0 &&
            restored == expected,
        "M1 state roundtrip");
  for (int size = 0; size < int(state.size()); ++size)
    check(scoria::decodeState(state.data(), size, 0, restored) < 0 && restored == expected,
          "truncated state is transactional");
  state[4] ^= 1;
  check(scoria::decodeState(state.data(), int(state.size()), 0, restored) < 0 &&
            restored == expected,
        "unknown asset rejected");
  state[4] ^= 1;
  double nan = std::numeric_limits<double>::quiet_NaN();
  std::memcpy(state.data() + 16, &nan, sizeof(nan));
  check(scoria::decodeState(state.data(), int(state.size()), 0, restored) < 0 &&
            restored == expected,
        "nonfinite state rejected");
  // Third-harmonic foldback from 15 kHz to 3 kHz at 48 kHz.
  double aliases[2]{};
  for (int mode = 0; mode < 2; ++mode) {
    scoria::Overdrive pedal;
    pedal.reset(48000., mode ? 4 : 1);
    double fundamentalReal = 0., fundamentalImag = 0., aliasReal = 0., aliasImag = 0.;
    for (int i = 0; i < 49000; ++i) {
      double y = pedal.process(.5 * std::sin(6.283185307179586 * 15000 * i / 48000.), .8, .5, 1.);
      if (i < 1000)
        continue;
      fundamentalReal += y * std::cos(6.283185307179586 * 15000 * i / 48000.);
      fundamentalImag += y * std::sin(6.283185307179586 * 15000 * i / 48000.);
      aliasReal += y * std::cos(6.283185307179586 * 3000 * i / 48000.);
      aliasImag += y * std::sin(6.283185307179586 * 3000 * i / 48000.);
    }
    aliases[mode] = std::hypot(aliasReal, aliasImag) / std::hypot(fundamentalReal, fundamentalImag);
  }
  check(aliases[1] < aliases[0], "4x boost reduces measured foldback relative to 1x");
  std::cout << "OD alias/fundamental 1x=" << aliases[0] << " 4x=" << aliases[1] << '\n';
  scoria::Overdrive drive1, drive2;
  drive1.reset(48000.);
  drive2.reset(48000.);
  double nonlinear = 0.;
  for (int i = 0; i < 4800; ++i) {
    double x = .3 * std::sin(i * .1);
    nonlinear += std::abs(drive2.process(2 * x, .8, .5, 1.) - 2 * drive1.process(x, .8, .5, 1.));
  }
  check(nonlinear > 1., "boost violates linear scaling as expected from soft clipping");
  const auto model = (root / "models/riffsmith-distorted.nam").string();
  scoria::NamAmplifier nam;
  std::string error;
  const auto invalid =
      std::filesystem::temp_directory_path() /
      ("scoria-invalid-" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".nam");
  {
    std::ofstream file(invalid);
    file << "{broken NAM";
  }
  check(!nam.load(invalid.string(), 48000., error),
        "malformed model rejected before inference construction");
  bool invalidWav = false;
  try {
    scoria::loadImpulse(invalid.string(), 48000.);
  } catch (...) {
    invalidWav = true;
  }
  check(invalidWav, "invalid WAV rejected");
  std::filesystem::remove(invalid);
  check(!nam.load("/missing/scoria.nam", 48000., error), "missing NAM rejected");
  check(!nam.load(model, 32000., error), "wrong NAM rate rejected");
  if (!nam.load(model, 48000., error)) {
    std::cerr << error << '\n';
    return 1;
  }
  std::array<double, 2048> l{}, r{};
  std::vector<double> times;
  for (int size : {1, 16, 32, 64, 127, 128, 256, 511, 512, 1024, 2048}) {
    times.clear();
    for (int repeat = 0; repeat < repeats; ++repeat) {
      for (int i = 0; i < size; ++i) {
        l[i] = .1 * std::sin((repeat * size + i) * .13);
        r[i] = -.07 * std::sin((repeat * size + i) * .07);
      }
      auto start = std::chrono::steady_clock::now();
      Eigen::internal::set_is_malloc_allowed(false);
      measureAllocations = true;
      nam.process(l.data(), r.data(), 2, size);
      measureAllocations = false;
      Eigen::internal::set_is_malloc_allowed(true);
      check(audioAllocations == 0, "NAM callback does not allocate C++ storage");
      times.push_back(
          std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start)
              .count());
      for (int i = 0; i < size; ++i)
        check(std::isfinite(l[i]) && std::isfinite(r[i]), "NAM finite output");
    }
    std::sort(times.begin(), times.end());
    std::cout << "NAM stereo block=" << size
              << " mean_us=" << std::accumulate(times.begin(), times.end(), 0.) / times.size()
              << " p99_us=" << times[(times.size() - 1) * 99 / 100] << " max_us=" << times.back()
              << '\n';
  }
  // Freshly prepared instances must render identically regardless of block boundaries.
  scoria::NamAmplifier second;
  check(second.load(model, 48000., error), "reload valid model");
  check(nam.load(model, 48000., error), "reset via offline preparation");
  for (int i = 0; i < 2048; ++i)
    l[i] = r[i] = .1 * std::sin(i * .1);
  nam.process(l.data(), nullptr, 1, 2048);
  for (int i = 0; i < 2048; i += 17)
    second.process(r.data() + i, nullptr, 1, std::min(17, 2048 - i));
  double energy = 0.;
  for (int i = 0; i < 2048; ++i) {
    check(std::abs(l[i] - r[i]) < 1e-5, "deterministic block-independent NAM");
    energy += std::abs(l[i]);
  }
  check(energy > .01, "NAM non-silent");
  // Active models survive failed replacement; preparation never corrupts a working instance.
  check(!nam.load("missing.nam", 48000., error) && nam.ready(), "transactional NAM replacement");
  scoria::AssetLoader loader(model, (root / "irs/jester-cookie-monster.wav").string());
  for (double rate : {48000., 44100., 96000., 48000.}) {
    loader.request(rate);
    scoria::ProcessingAssets *assets = nullptr;
    for (int i = 0; i < 1000 && !assets; ++i) {
      assets = loader.poll();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    check(assets && assets->rate == rate, "worker reset handoff");
    check(assets->amplifier.ready(), "unsupported NAM rate explicit");
  }
  loader.request(48000., 1, true);
  check(loader.waitForPreparation(), "offline lifecycle prepares before rendering");
  auto *fresh = loader.poll();
  check(fresh && fresh->modelIndex == 1 && fresh->amplifier.ready(), "new model worker handoff");
  loader.request(44100.5, 1);
  check(!loader.waitForPreparation() && loader.status() == scoria::AssetLoader::UnsupportedRate,
        "fractional unsupported rates never masquerade as ready");
  loader.request(48000.);
  check(loader.waitForPreparation(), "return to supported legacy model");
  // Retirement may still be occupied until the next worker polling iteration.
  for (int i = 0; i < 100 && !loader.poll(); ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  auto *retained = loader.poll();
  loader.request(48000.);
  check(retained && loader.poll() == retained, "unchanged-rate request keeps prepared assets");
  scoria::ProcessingAssets processing;
  check(processing.amplifier.load(model, 48000., error), "pipeline model load");
  processing.rate = 48000.;
  const auto ir = scoria::loadImpulse((root / "irs/jester-cookie-monster.wav").string(), 48000.);
  for (auto &c : processing.cabinets)
    c.prepare(ir);
  scoria::AudioEngine engine;
  scoria::Parameters parameters;
  parameters.amplifier = true;
  parameters.cabinet = true;
  engine.reset(48000., parameters);
  engine.setAssets(&processing);
  const double *in[]{l.data(), r.data()};
  double *out[]{l.data(), r.data()};
  for (int i = 0; i < 2048; ++i)
    l[i] = r[i] = .1 * std::sin(i * .17);
  engine.process(in, 2, out, 2, 2048, parameters);
  for (int i = 0; i < 2048; ++i)
    check(std::isfinite(l[i]) && std::abs(l[i] - r[i]) < 1e-10, "full stereo in-place pipeline");
  parameters.bypass = true;
  for (int i = 0; i < 2048; ++i)
    l[i] = .25, r[i] = -.1;
  engine.process(in, 2, out, 2, 2048, parameters);
  check(l.back() == .25 && r.back() == -.1, "full chain global dry bypass");
  // Gain order: drive a nonlinear stage with input trim, attenuate after it.
  scoria::AudioEngine before, after;
  scoria::Parameters a, b;
  a.overdrive = b.overdrive = true;
  a.inputDb = 12.;
  a.outputDb = -12.;
  before.reset(48000., a);
  after.reset(48000., b);
  double difference = 0.;
  for (int i = 0; i < 4096; ++i) {
    double x = .2 * std::sin(i * .14), y = 0., z = 0.;
    const double *input[]{&x};
    double *o1[]{&y};
    double *o2[]{&z};
    before.process(input, 1, o1, 1, 1, a);
    after.process(input, 1, o2, 1, 1, b);
    difference += std::abs(y - z);
  }
  check(difference > 1., "input and output gains straddle nonlinear processing");
  // Offline whole-chain benchmark (stereo, 48 kHz, 256 frames, recorded 2048-tap IR).
  parameters.bypass = false;
  parameters.gate = true;
  parameters.overdrive = true;
  engine.reset(48000., parameters);
  times.clear();
  for (int repeat = 0; repeat < repeats; ++repeat) {
    for (int i = 0; i < 256; ++i)
      l[i] = r[i] = .1 * std::sin((repeat * 256 + i) * .17);
    auto start = std::chrono::steady_clock::now();
    measureAllocations = true;
    engine.process(in, 2, out, 2, 256, parameters);
    measureAllocations = false;
    check(audioAllocations == 0, "pipeline does not allocate C++ storage");
    times.push_back(
        std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start)
            .count());
  }
  std::sort(times.begin(), times.end());
  std::cout << "Full chain stereo block=256 mean_us="
            << std::accumulate(times.begin(), times.end(), 0.) / times.size()
            << " p99_us=" << times[(times.size() - 1) * 99 / 100] << " max_us=" << times.back()
            << " deadline_us=5333.33\n";
  std::cout << "DSP and asset checks passed\n";
}

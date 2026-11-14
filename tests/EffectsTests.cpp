#include "audio/AudioEngine.h"
#include "audio/State.h"
#include "dsp/WavImpulse.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <vector>
static bool counting = false;
static size_t allocations = 0;
void *operator new(std::size_t n) {
  if (counting)
    ++allocations;
  if (auto *p = std::malloc(n ? n : 1))
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
void check(bool ok, const char *what) {
  if (!ok) {
    std::cerr << what << '\n';
    std::exit(1);
  }
}
int main(int argc, char **argv) {
  check(argc == 2, "asset directory");
  auto root = std::filesystem::path(argv[1]);
  for (double rate : {44100., 48000., 88200., 96000.}) {
    scoria::Phaser phaser;
    phaser.reset(rate, true, 1., 0., 0., 1.);
    double energy = 0.;
    for (int n = 0; n < int(rate); ++n) {
      double y = phaser.process(n == 0 ? 1. : 0.);
      energy += y * y;
    }
    check(std::abs(energy - 1.) < 1e-10, "static phaser allpass conserves impulse energy");
    for (double feedback : {-.9, .9}) {
      phaser.reset(rate, true, 10., 1., feedback, .5);
      for (int n = 0; n < int(rate); ++n)
        check(std::abs(phaser.process(.2 * std::sin(n * .1))) < 2., "phaser feedback stability");
    }
    phaser.reset(rate);
    for (int n = 0; n < 2000; ++n)
      check(phaser.process(.1) == .1, "phaser bypass equality");
    scoria::Chorus chorus;
    chorus.reset(rate, true, .8, 0., 1.);
    double moment = 0., sum = 0.;
    for (int n = 0; n < int(rate * .03); ++n) {
      double y = chorus.process(n == 0 ? 1. : 0.);
      sum += y;
      moment += n * y;
    }
    check(std::abs(sum - 1.) < 1e-12 && std::abs(moment / sum - rate * .012) < 1e-9,
          "chorus fractional delay timing");
    // 1 Hz modulation repeats after one second; compare steady cycles at 1 kHz.
    chorus.reset(rate, true, 1., 1., .5);
    phaser.reset(rate, true, 1., 1., 0., .5);
    std::vector<double> cp(size_t(rate), 0.), pp(size_t(rate), 0.);
    double periodicError = 0., modulationDifference = 0.;
    for (int n = 0; n < int(rate * 3); ++n) {
      const double x = .1 * std::sin(6.283185307179586 * 1000. * n / rate);
      const double c = chorus.process(x), p = phaser.process(x);
      if (n >= int(rate) && n < int(rate * 2)) {
        cp[n - int(rate)] = c;
        pp[n - int(rate)] = p;
      }
      if (n >= int(rate * 2))
        periodicError = std::max(periodicError, std::abs(c - cp[n - int(rate * 2)]) +
                                                    std::abs(p - pp[n - int(rate * 2)]));
      modulationDifference += std::abs(c - x) + std::abs(p - x);
    }
    check(periodicError < 1e-7 && modulationDifference > 1.,
          "phaser/chorus modulation period and audible processing");
    scoria::StereoDelay delay;
    delay.reset(rate, true, 10., .5, 1.);
    const int interval = int(rate * .01);
    for (int n = 0; n < interval * 4; ++n) {
      double l = n == 0 ? 1. : 0., r = l;
      delay.process(l, r, true);
      const double el = n == interval ? 1. : n == interval * 3 ? .25 : 0.;
      const double er = n == interval * 2 ? .5 : 0.;
      check(std::abs(l - el) < 1e-10 && std::abs(r - er) < 1e-10,
            "ping-pong exact timing and feedback");
    }
    delay.reset(rate, true, 100., .5, 1.);
    for (int n = 0; n < int(rate * .31); ++n) {
      if (n == 1)
        delay.configure(false, 100., .5, 1.);
      double l = n == 0 ? 1. : 0., r = l;
      delay.process(l, r, true);
      if (n == int(rate * .1))
        check(std::abs(l - 1.) < 1e-9, "delay bypass preserves stored echo");
    }
    delay.reset(rate, true, 10., .95, .7);
    double previous = 0.;
    for (int n = 0; n < int(rate); ++n) {
      if (n % 127 == 0)
        delay.configure(true, (n % 2000) + 1., .95, .7);
      double l = .1, r = .1;
      delay.process(l, r, true);
      check(std::isfinite(l) && std::abs(l) < 3. && std::abs(l - previous) < .2,
            "delay time automation bounded continuity");
      previous = l;
    }
    scoria::StereoReverb reverb;
    reverb.reset(rate, true, .5, .5, 1.);
    double early = 0., late = 0., difference = 0.;
    for (int n = 0; n < int(rate * 3); ++n) {
      double l = n == 0 ? 1. : 0., r = l;
      reverb.process(l, r);
      check(std::isfinite(l) && std::isfinite(r) && std::abs(l) < 2., "reverb finite impulse");
      if (n < rate)
        early += l * l + r * r;
      else if (n > rate * 2)
        late += l * l + r * r;
      difference += std::abs(l - r);
    }
    check(early > 1e-4 && late < early * 1e-7 && difference > .01,
          "reverb decays and decorrelates stereo");
    reverb.reset(rate, true, 15., 0., 1.);
    for (int n = 0; n < int(rate * 2); ++n) {
      if (n % 193 == 0)
        reverb.configure(true, n % 2 ? .2 : 15., n % 2 ? 1. : 0., 1.);
      double l = .1 * std::sin(n * .13), r = -l;
      reverb.process(l, r);
      check(std::isfinite(l) && std::abs(l) < 3., "reverb automation at maximum decay stable");
    }
    // Same deterministic chain with all new effects, arbitrary chunking, no NAM asset required.
    scoria::Parameters p;
    p.phaser = p.chorus = p.delay = p.reverb = true;
    p.delayMs = 5.;
    const int frames = int(rate * .15);
    std::vector<double> input(frames), referenceL(frames), referenceR(frames), left(frames),
        right(frames);
    for (int n = 0; n < frames; ++n)
      input[n] = .1 * std::sin(n * .07);
    scoria::AudioEngine engine;
    engine.reset(rate, p);
    const double *in[]{input.data()};
    double *out[]{referenceL.data(), referenceR.data()};
    counting = true;
    engine.process(in, 1, out, 2, frames, p);
    counting = false;
    check(allocations == 0, "full new-effects pipeline must not allocate");
    for (int block : {1, 17, 64, 128, 256, 511, 2048}) {
      engine.reset(rate, p);
      for (int base = 0; base < frames; base += block) {
        const double *i[]{input.data() + base};
        double *o[]{left.data() + base, right.data() + base};
        engine.process(i, 1, o, 2, std::min(block, frames - base), p);
      }
      check(left == referenceL && right == referenceR,
            "all effects independent of host block boundaries");
    }
    engine.reset(rate, p);
    auto inplace = input;
    const double *ip[]{inplace.data()};
    double *op[]{inplace.data()};
    engine.process(ip, 1, op, 1, frames, p);
    for (int n = 0; n < frames; ++n)
      check(std::abs(inplace[n] - .5 * (referenceL[n] + referenceR[n])) < 1e-14,
            "mono downmix and in-place equality");
    p.bypass = true;
    engine.reset(rate, p);
    engine.process(in, 1, out, 2, frames, p);
    const int latency = scoria::inferenceLatency(rate);
    for (int n = 0; n < frames; ++n)
      check(referenceL[n] == (n < latency ? 0. : input[n - latency]) &&
                referenceL[n] == referenceR[n],
            "global bypass excludes all effects");
    std::cout << "Effects passed rate=" << rate << '\n';
  }
  for (int model : {2, 3})
    for (double rate : {44100., 48000., 88200., 96000.}) {
      scoria::ProcessingAssets assets;
      assets.rate = rate;
      assets.modelIndex = model;
      std::string error;
      check(assets.amplifier.load((root / "models" / scoria::kModels[model].file).string(), rate,
                                  error),
            "new audited amp loads at every rate");
      auto ir = scoria::loadImpulse((root / "irs/jester-cookie-monster.wav").string(), rate);
      for (auto &c : assets.cabinets)
        c.prepare(ir);
      scoria::AudioEngine engine;
      auto p = scoria::fromValues(scoria::presetValues(model == 2 ? 3 : 4));
      engine.reset(rate, p);
      engine.setAssets(&assets);
      double input[256], left[256], right[256];
      double energy = 0.;
      for (int block = 0; block < 20; ++block) {
        for (int n = 0; n < 256; ++n)
          input[n] = .03 * std::sin((block * 256 + n) * .07);
        const double *in[]{input};
        double *out[]{left, right};
        counting = true;
        engine.process(in, 1, out, 2, 256, p);
        counting = false;
        for (double x : left) {
          check(std::isfinite(x), "new preset finite");
          energy += x * x;
        }
      }
      check(energy > 1e-8 && allocations == 0, "new amp and cabinet audible without allocations");
    }
  for (const char *file : {"m0-v1.bin", "m1-v2.bin", "m1-v3.bin", "m2-v4.bin"}) {
    std::ifstream stream(root.parent_path() / "tests/fixtures" / file, std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(stream)), {});
    std::array<double, scoria::ParameterCount> values{};
    std::uint32_t preset = 0;
    check(scoria::decodeState(bytes.data(), int(bytes.size()), 0, values, &preset) ==
              int(bytes.size()),
          "binary state fixture migration");
    if (std::string(file) == "m2-v4.bin")
      check(values == scoria::presetValues(2), "Modern Lead fixture exact recall");
    else
      for (int id : {scoria::PhaserEnabled, scoria::ChorusEnabled, scoria::DelayEnabled,
                     scoria::ReverbEnabled})
        check(values[id] == 0., "old projects do not enable new effects");
  }
  {
    scoria::AssetLoader worker((root / "models/riffsmith-distorted.nam").string(),
                               (root / "irs/jester-cookie-monster.wav").string());
    for (int index : {2, 3, 1, 0, 3, 2}) {
      worker.request(48000., index, true);
      check(worker.waitForPreparation(), "expanded registry worker prepares selected voice");
      auto *assets = worker.poll();
      check(assets && assets->modelIndex == index, "worker transfers correct voice ownership");
    }
    worker.request(48000., 999, true);
    check(!worker.waitForPreparation() && worker.status() == scoria::AssetLoader::MissingAsset &&
              !worker.poll(),
          "invalid registry index fails safely");
  }
  // Schema 4 roundtrip and transactional truncation rejection.
  auto values = scoria::presetValues(2);
  values[scoria::PhaserEnabled] = 1.;
  std::vector<unsigned char> bytes(16 + 8 * scoria::ParameterCount);
  const std::uint32_t header[]{4, scoria::kModels[1].id, scoria::kCabinetId, 2};
  std::memcpy(bytes.data(), header, 16);
  std::memcpy(bytes.data() + 16, values.data(), values.size() * 8);
  std::array<double, scoria::ParameterCount> restored{};
  check(scoria::decodeState(bytes.data(), int(bytes.size()), 0, restored) == int(bytes.size()) &&
            restored == values,
        "v4 roundtrip");
  for (int n = 0; n < int(bytes.size()); ++n) {
    auto sentinel = restored;
    check(scoria::decodeState(bytes.data(), n, 0, restored) < 0 && restored == sentinel,
          "truncated v4 must be transactional");
  }
  std::cout << "M2 effect and routing tests passed\n";
}

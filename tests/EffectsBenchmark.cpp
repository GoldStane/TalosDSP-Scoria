#include "audio/AudioEngine.h"
#include "audio/State.h"
#include "dsp/WavImpulse.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <vector>
int main(int argc, char **argv) {
  if (argc != 2)
    return 1;
  auto root = std::filesystem::path(argv[1]);
  for (double rate : {44100., 48000., 88200., 96000.}) {
    scoria::ProcessingAssets assets;
    assets.rate = rate;
    assets.modelIndex = 1;
    std::string error;
    if (!assets.amplifier.load((root / "models" / scoria::kModels[1].file).string(), rate, error))
      return 1;
    auto ir = scoria::loadImpulse((root / "irs/jester-cookie-monster.wav").string(), rate);
    for (auto &c : assets.cabinets)
      c.prepare(ir);
    for (int channels : {1, 2})
      for (int frames : {64, 128, 256})
        for (bool effects : {false, true}) {
          scoria::AudioEngine engine;
          auto p = scoria::fromValues(scoria::factoryValues(true));
          p.phaser = p.chorus = p.delay = p.reverb = effects;
          engine.reset(rate, p);
          engine.setAssets(&assets);
          double l[256], r[256], ol[256], orr[256];
          const double *in[]{l, r};
          double *out[]{ol, orr};
          std::vector<double> timing;
          timing.reserve(1000);
          for (int iteration = 0; iteration < 1050; ++iteration) {
            for (int n = 0; n < frames; ++n) {
              l[n] = .05 * std::sin((iteration * frames + n) * .07);
              r[n] = .04 * std::sin((iteration * frames + n) * .09);
            }
            const auto start = std::chrono::steady_clock::now();
            engine.process(in, channels, out, 2, frames, p);
            const auto end = std::chrono::steady_clock::now();
            if (iteration >= 50)
              timing.push_back(std::chrono::duration<double, std::micro>(end - start).count());
          }
          double mean = 0.;
          for (double t : timing)
            mean += t / timing.size();
          std::sort(timing.begin(), timing.end());
          std::cout << rate << ',' << channels << ',' << frames << ',' << effects << ',' << mean
                    << ',' << timing[949] << ',' << timing[989] << ',' << timing.back() << '\n';
        }
  }
}

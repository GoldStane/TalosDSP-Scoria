// Developer-only offline fixture renderer; never packaged as an application.
#include "audio/AudioEngine.h"
#include "audio/State.h"
#include "dsp/WavImpulse.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "assets input-f64-mono output-f32-mono rate\n";
    return 1;
  }
  const double rate = std::stod(argv[4]);
  std::ifstream input(argv[2], std::ios::binary | std::ios::ate);
  if (!input || input.tellg() <= 0 || input.tellg() > 1024 * 1024 * 1024)
    return 1;
  std::vector<double> samples(size_t(input.tellg()) / sizeof(double));
  input.seekg(0);
  input.read(reinterpret_cast<char *>(samples.data()), samples.size() * sizeof(double));
  scoria::ProcessingAssets assets;
  assets.rate = rate;
  assets.modelIndex = 1;
  auto root = std::filesystem::path(argv[1]);
  std::string error;
  if (!assets.amplifier.load((root / "models/volum-herbert-4.nam").string(), rate, error)) {
    std::cerr << error;
    return 1;
  }
  auto ir = scoria::loadImpulse((root / "irs/jester-cookie-monster.wav").string(), rate);
  for (auto &c : assets.cabinets)
    c.prepare(ir);
  scoria::AudioEngine engine;
  auto p = scoria::fromValues(scoria::factoryValues(true));
  engine.reset(rate, p);
  engine.setAssets(&assets);
  // Settle availability with silence before the DI begins.
  double zero[2048]{};
  const double *silence[]{zero};
  double *silentOut[]{zero};
  engine.process(silence, 1, silentOut, 1, 2048, p);
  const int latency = scoria::inferenceLatency(rate);
  samples.resize(samples.size() + latency, 0.);
  std::ofstream output(argv[3], std::ios::binary);
  double peak = 0., square = 0.;
  for (size_t offset = 0; offset < samples.size(); offset += 256) {
    const int count = int(std::min<size_t>(256, samples.size() - offset));
    const double *in[]{samples.data() + offset};
    double *out[]{samples.data() + offset};
    engine.process(in, 1, out, 1, count, p);
  }
  for (size_t i = latency; i < samples.size(); ++i) {
    const float x = float(samples[i]);
    if (!std::isfinite(x))
      return 1;
    peak = std::max(peak, std::abs(double(x)));
    square += double(x) * x;
    output.write(reinterpret_cast<const char *>(&x), sizeof(x));
  }
  std::cout << "frames=" << samples.size() - latency << " peak=" << peak
            << " rms=" << std::sqrt(square / (samples.size() - latency)) << '\n';
  return output ? 0 : 1;
}

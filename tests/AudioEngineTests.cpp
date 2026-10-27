#include "audio/AudioEngine.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

void require(bool ok, const char *message) {
  if (!ok) {
    std::cerr << message << '\n';
    std::exit(1);
  }
}
template <typename Sample> void test() {
  for (double rate : {44100., 48000., 88200., 96000.}) {
    for (int size : {0, 1, 16, 32, 64, 128, 256, 512, 1024, 2048}) {
      for (auto layout : {std::array<int, 2>{1, 1}, {1, 2}, {2, 2}}) {
        std::array<Sample, 2048> left{}, right{}, outLeft{}, outRight{};
        left.fill(Sample(0.125));
        right.fill(Sample(-0.25));
        const Sample *in[] = {left.data(), right.data()};
        Sample *out[] = {outLeft.data(), outRight.data()};
        scoria::AudioEngine engine;
        for (auto parameters : {scoria::Parameters{}, {6., -3., false}, {24., 24., true}}) {
          engine.reset(rate, parameters);
          engine.process(in, layout[0], out, layout[1], size, parameters);
          const double gain = parameters.bypass
                                  ? 1.
                                  : std::pow(10., (parameters.inputDb + parameters.outputDb) / 20.);
          for (int i = 0; i < size; ++i) {
            require(std::abs(outLeft[i] - 0.125 * gain) < 1e-6, "left gain/pass-through/bypass");
            if (layout[1] == 2)
              require(std::abs(outRight[i] - (layout[0] == 1 ? 0.125 : -0.25) * gain) < 1e-6,
                      "channel routing");
          }
        }
        // Aliased input/output, including mono expansion: avoid applying gain twice.
        Sample *inplace[] = {left.data(), right.data()};
        engine.reset(rate, {6., 0., false});
        engine.process(in, layout[0], inplace, layout[1], size, {6., 0., false});
        for (int i = 0; i < size; ++i)
          require(std::abs(left[i] - 0.125 * std::pow(10., 6. / 20.)) < 1e-6, "in-place gain");
      }
    }
    scoria::AudioEngine engine;
    engine.reset(rate);
    Sample input = 1, output = 0;
    const Sample *in[] = {&input};
    Sample *out[] = {&output};
    double previous = 1.;
    for (int i = 0; i < 2000; ++i) {
      engine.process(in, 1, out, 1, 1, {6., 0., false});
      require(output >= previous - 1e-6 && output - previous < 0.003, "smooth gain transition");
      previous = output;
    }
    for (int i = 0; i < 2000; ++i)
      engine.process(in, 1, out, 1, 1, {6., 0., true});
    require(output == 1, "bypass settles exactly to dry");
    for (auto invalid :
         {std::numeric_limits<Sample>::quiet_NaN(), std::numeric_limits<Sample>::infinity(),
          std::numeric_limits<Sample>::denorm_min()}) {
      input = invalid;
      engine.process(in, 1, out, 1, 1, {});
      require(output == 0, "invalid/denormal input sanitized");
    }
    input = 1;
    engine.process<Sample>(nullptr, 0, out, 1, 1, {});
    require(output == 0, "disconnected input silent");
    engine.process(in, 2, out, 1, 1, {});
    require(output == 0, "unsupported layout silent without reading invalid pointers");
    engine.process<Sample>(nullptr, 0, nullptr, 0, 0, {});
    for (int i = 0; i < 10000; ++i) {
      engine.process(in, 1, out, 1, 1, {i % 2 ? 24. : -60., 24., i % 3 == 0});
      require(std::isfinite(output), "repeated toggles remain finite");
    }
  }
}
int main() {
  test<float>();
  test<double>();
  std::cout << "Scoria core checks passed\n";
}

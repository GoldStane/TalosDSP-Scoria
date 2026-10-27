#pragma once
#include <string>
#include <vector>
namespace scoria {
// Offline mono PCM16/24/32 or IEEE float32 WAV decode and windowed-sinc preparation.
std::vector<double> loadImpulse(const std::string &path, double targetRate);
} // namespace scoria

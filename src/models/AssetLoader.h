#pragma once
#include "NamAmplifier.h"
#include "dsp/Cabinet.h"
#include <atomic>
#include <string>
#include <thread>
namespace scoria {
struct ProcessingAssets {
  NamAmplifier amplifier;
  Cabinet cabinets[2];
  unsigned generation = 0;
  double rate = 0.;
};
// Single producer (worker), single consumer (audio). Audio never deletes an asset.
// Stop processing before destruction. request() is called by suspended host reset.
class AssetLoader {
public:
  AssetLoader(std::string model, std::string impulse);
  ~AssetLoader();
  void request(double rate) noexcept;
  ProcessingAssets *poll() noexcept;
  enum Status { Loading, Ready, UnsupportedRate, MissingAsset };
  Status status() const noexcept { return static_cast<Status>(status_.load()); }

private:
  void work();
  std::string model_, impulse_;
  std::atomic<bool> stop_{false};
  std::atomic<double> rate_{48000.};
  std::atomic<unsigned> generation_{1};
  std::atomic<ProcessingAssets *> pending_{nullptr}, retired_{nullptr};
  std::atomic<int> status_{Loading};
  ProcessingAssets *active_ = nullptr; // audio-owned until destructor after host suspension
  std::thread worker_;
};
} // namespace scoria

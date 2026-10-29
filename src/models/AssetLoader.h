#pragma once
#include "ModelRegistry.h"
#include "NamAmplifier.h"
#include "dsp/Cabinet.h"
#include <atomic>
#include <string>
#include <thread>
namespace scoria {
struct ProcessingAssets {
  NamAmplifier amplifier;
  Cabinet cabinets[2];
  std::uint64_t generation = 0;
  double rate = 0.;
  int modelIndex = 0;
};
// Single producer (worker), single consumer (audio). Audio never deletes an asset.
// Stop processing before destruction. request() uses one lock-free configuration/epoch word; force
// a fresh state at host reset.
class AssetLoader {
public:
  AssetLoader(std::string model, std::string impulse);
  ~AssetLoader();
  void request(double rate, int modelIndex = 0, bool force = false) noexcept;
  // Only for offline host lifecycle setup, never ProcessBlock or realtime reset.
  bool waitForPreparation() noexcept;
  ProcessingAssets *poll() noexcept;
  enum Status { Loading, Ready, UnsupportedRate, MissingAsset };
  Status status() const noexcept {
    return completed_.load() == request_.load() ? static_cast<Status>(status_.load()) : Loading;
  }

private:
  void work();
  std::string model_, impulse_;
  std::atomic<bool> stop_{false};
  std::atomic<std::uint64_t> request_{48000};
  std::atomic<ProcessingAssets *> pending_{nullptr}, retired_{nullptr};
  std::atomic<std::uint64_t> completed_{~std::uint64_t(0)};
  std::atomic<int> status_{Loading};
  ProcessingAssets *active_ = nullptr; // audio-owned until destructor after host suspension
  std::thread worker_;
};
} // namespace scoria

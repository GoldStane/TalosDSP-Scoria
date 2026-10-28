#include "AssetLoader.h"
#include "dsp/WavImpulse.h"
#include <chrono>
#include <filesystem>
namespace scoria {
static_assert(std::atomic<ProcessingAssets *>::is_always_lock_free &&
                  std::atomic<std::uint64_t>::is_always_lock_free,
              "Audio handoff must be lock-free");
AssetLoader::AssetLoader(std::string model, std::string impulse)
    : model_(std::move(model)), impulse_(std::move(impulse)), worker_([this] { work(); }) {}
AssetLoader::~AssetLoader() {
  stop_.store(true);
  worker_.join();
  delete active_;
  delete pending_.exchange(nullptr);
  delete retired_.exchange(nullptr);
}
void AssetLoader::request(double rate, int modelIndex, bool force) noexcept {
  constexpr std::uint64_t mask = (std::uint64_t(1) << 33) - 1;
  const auto configuration =
      (std::uint64_t(modelIndex == 1 ? 1 : 0) << 32) |
      std::uint32_t(std::isfinite(rate) && rate > 0 && rate < 200000 ? rate : 0);
  auto previous = request_.load();
  do {
    if (!force && (previous & mask) == configuration)
      return;
  } while (!request_.compare_exchange_weak(
      previous, ((previous & ~mask) + (std::uint64_t(1) << 33)) | configuration));
  status_.store(Loading);
}
bool AssetLoader::waitForPreparation() noexcept {
  const auto requested = request_.load();
  for (int attempt = 0; attempt < 10000; ++attempt) {
    if (completed_.load() == requested)
      return status() == Ready;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return false;
}
ProcessingAssets *AssetLoader::poll() noexcept {
  if (!retired_.load()) {
    if (auto *next = pending_.exchange(nullptr)) {
      auto *old = active_;
      active_ = next;
      retired_.store(old);
    }
  }
  return active_ && active_->generation == request_.load() ? active_ : nullptr;
}
void AssetLoader::work() {
  std::uint64_t prepared = ~std::uint64_t(0);
  while (!stop_.load()) {
    delete retired_.exchange(nullptr);
    const auto generation = request_.load();
    if (generation != prepared) {
      try {
        auto next = std::make_unique<ProcessingAssets>();
        next->generation = generation;
        const auto request = generation;
        next->rate = double(std::uint32_t(request));
        next->modelIndex = int((request >> 32) & 1);
        std::string error;
        bool cab = false;
        try {
          const auto ir = loadImpulse(impulse_, next->rate);
          for (auto &c : next->cabinets)
            c.prepare(ir);
          cab = true;
        } catch (...) {
        }
        const auto path = next->modelIndex == 0 ? std::filesystem::u8path(model_)
                                                : std::filesystem::u8path(model_).parent_path() /
                                                      kModels[next->modelIndex].file;
        const bool amp = next->amplifier.load(path.u8string(), next->rate, error);
        if (generation == request_.load()) {
          status_.store(amp && cab                   ? Ready
                        : !supportedRate(next->rate) ? UnsupportedRate
                                                     : MissingAsset);
          delete pending_.exchange(next.release());
          prepared = generation;
          completed_.store(generation);
        }
      } catch (...) {
        if (generation == request_.load()) {
          status_.store(MissingAsset);
          prepared = generation;
          completed_.store(generation);
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}
} // namespace scoria

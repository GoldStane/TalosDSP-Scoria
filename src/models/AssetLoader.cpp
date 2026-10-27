#include "AssetLoader.h"
#include "dsp/WavImpulse.h"
#include <chrono>
namespace scoria {
static_assert(std::atomic<ProcessingAssets *>::is_always_lock_free &&
                  std::atomic<unsigned>::is_always_lock_free &&
                  std::atomic<double>::is_always_lock_free,
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
void AssetLoader::request(double rate) noexcept {
  // Do not restart preparation for an unchanged rate that is already loading or ready.
  if (rate == rate_.load() && (status_.load() == Loading || status_.load() == Ready))
    return;
  // Publish Loading before the generation so a fast worker cannot finish first and then be
  // overwritten with a stale status.
  status_.store(Loading);
  rate_.store(rate);
  generation_.fetch_add(1);
}
ProcessingAssets *AssetLoader::poll() noexcept {
  if (!retired_.load()) {
    if (auto *next = pending_.exchange(nullptr)) {
      auto *old = active_;
      active_ = next;
      retired_.store(old);
    }
  }
  return active_ && active_->generation == generation_.load() ? active_ : nullptr;
}
void AssetLoader::work() {
  unsigned prepared = 0;
  while (!stop_.load()) {
    delete retired_.exchange(nullptr);
    const unsigned generation = generation_.load();
    if (generation != prepared) {
      auto next = std::make_unique<ProcessingAssets>();
      next->generation = generation;
      next->rate = rate_.load();
      std::string error;
      bool cab = false;
      try {
        const auto ir = loadImpulse(impulse_, next->rate);
        for (auto &c : next->cabinets)
          c.prepare(ir);
        cab = true;
      } catch (...) {
      }
      const bool amp = next->amplifier.load(model_, next->rate, error);
      if (generation == generation_.load()) {
        status_.store(amp && cab ? Ready : next->rate != 48000. ? UnsupportedRate : MissingAsset);
        delete pending_.exchange(next.release());
        prepared = generation;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}
} // namespace scoria

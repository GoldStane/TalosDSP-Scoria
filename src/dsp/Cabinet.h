#pragma once
#include "Control.h"
#include <stdexcept>
#include <vector>
namespace scoria {
class Cabinet {
public:
  // Preparation/reset only while processing is suspended. No allocation in process.
  void prepare(const std::vector<double> &impulse) {
    if (impulse.empty() || impulse.size() > 4096)
      throw std::runtime_error("IR must contain 1..4096 taps");
    for (double x : impulse)
      if (!std::isfinite(x))
        throw std::runtime_error("Nonfinite IR");
    taps_ = impulse;
    history_.assign(taps_.size(), 0.);
    position_ = 0;
  }
  void reset() noexcept {
    std::fill(history_.begin(), history_.end(), 0.);
    position_ = 0;
  }
  bool ready() const noexcept { return !taps_.empty(); }
  double process(double x) noexcept {
    if (!ready())
      return x;
    history_[position_] = x;
    double sum = 0.;
    size_t index = position_;
    for (double tap : taps_) {
      sum += tap * history_[index];
      index = index ? index - 1 : taps_.size() - 1;
    }
    if (++position_ == taps_.size())
      position_ = 0;
    return finite(sum);
  }

private:
  std::vector<double> taps_, history_;
  size_t position_ = 0;
};
} // namespace scoria

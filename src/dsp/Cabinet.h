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
    history_.assign(taps_.size() * 2, 0.);
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
    // Mirrored history makes the dot product contiguous without a wrap branch
    // per tap. Four independent accumulators reduce the dependency chain.
    const size_t count = taps_.size();
    history_[position_] = history_[position_ + count] = x;
    const double *history = history_.data() + position_;
    double a = 0., b = 0., c = 0., d = 0.;
    size_t i = 0;
    for (; i + 3 < count; i += 4) {
      a += taps_[i] * history[i];
      b += taps_[i + 1] * history[i + 1];
      c += taps_[i + 2] * history[i + 2];
      d += taps_[i + 3] * history[i + 3];
    }
    double sum = (a + b) + (c + d);
    for (; i < count; ++i)
      sum += taps_[i] * history[i];
    position_ = position_ ? position_ - 1 : count - 1;
    return finite(sum);
  }

private:
  std::vector<double> taps_, history_;
  size_t position_ = 0;
};
} // namespace scoria

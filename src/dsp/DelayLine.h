#pragma once
#include "Control.h"
#include <vector>
namespace scoria {
// Storage is allocated once at construction, never during reset or processing.
class DelayLine {
public:
  explicit DelayLine(size_t capacity) : buffer_(capacity, 0.) {}
  void reset() noexcept {
    std::fill(buffer_.begin(), buffer_.end(), 0.);
    position_ = 0;
  }
  double read(double delay) const noexcept {
    delay = std::clamp(delay, 1., double(buffer_.size() - 2));
    const auto whole = size_t(delay);
    const double fraction = delay - double(whole);
    const auto a = (position_ + buffer_.size() - whole) % buffer_.size();
    const auto b = a == 0 ? buffer_.size() - 1 : a - 1;
    return buffer_[a] + fraction * (buffer_[b] - buffer_[a]);
  }
  void push(double x) noexcept {
    buffer_[position_] = finite(x);
    if (++position_ == buffer_.size())
      position_ = 0;
  }

private:
  std::vector<double> buffer_;
  size_t position_ = 0;
};
} // namespace scoria

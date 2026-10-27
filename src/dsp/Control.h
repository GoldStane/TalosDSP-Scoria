#pragma once
#include <algorithm>
#include <cmath>
namespace scoria {
inline double bounded(double x, double lo, double hi, double fallback = 0.) noexcept {
  return std::isfinite(x) ? std::clamp(x, lo, hi) : fallback;
}
inline double finite(double x) noexcept {
  return std::isfinite(x) && std::abs(x) >= 1e-30 ? x : 0.;
}
inline double dbGain(double db) noexcept { return std::pow(10., bounded(db, -60., 24.) / 20.); }
class Ramp {
public:
  void reset(double value, double rate) noexcept {
    value_ = target_ = value;
    count_ = 0;
    length_ = std::max(1, int(rate * .01));
  }
  void set(double value) noexcept {
    if (value != target_) {
      target_ = value;
      count_ = length_;
      step_ = (target_ - value_) / count_;
    }
  }
  double next() noexcept {
    if (count_ && !--count_)
      value_ = target_;
    else if (count_)
      value_ += step_;
    return value_;
  }

private:
  double value_ = 0., target_ = 0., step_ = 0.;
  int count_ = 0, length_ = 480;
};
} // namespace scoria

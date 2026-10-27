#pragma once
#include "IPlug_include_in_plug_hdr.h"
#include "audio/AudioEngine.h"
#include <atomic>
static_assert(std::atomic<double>::is_always_lock_free, "Host parameters must be lock-free");
#ifdef PARAMS_MUTEX
#error Scoria must not enable the iPlug2 parameter mutex in audio callbacks
#endif

class Scoria final : public iplug::Plugin {
public:
  explicit Scoria(const iplug::InstanceInfo &info);
  bool SerializeState(iplug::IByteChunk &chunk) const override;
  int UnserializeState(const iplug::IByteChunk &chunk, int startPos) override;
#if IPLUG_DSP
  void ProcessBlock(iplug::sample **inputs, iplug::sample **outputs, int nFrames) override;
  void OnReset() override;
#endif
private:
  scoria::Parameters parameters() const;
  scoria::AudioEngine engine_;
};

#include <AudioUnit/AudioUnit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

void check(bool ok, const char *message) {
  if (!ok) {
    std::cerr << message << '\n';
    std::exit(1);
  }
}
void status(OSStatus result, const char *message) {
  if (result) {
    std::cerr << message << ": " << result << '\n';
    std::exit(1);
  }
}
OSStatus input(void *, AudioUnitRenderActionFlags *, const AudioTimeStamp *, UInt32, UInt32 frames,
               AudioBufferList *data) {
  for (UInt32 channel = 0; channel < data->mNumberBuffers; ++channel)
    for (UInt32 i = 0; i < frames; ++i)
      static_cast<float *>(data->mBuffers[channel].mData)[i] = channel == 0 ? 0.125f : -0.25f;
  return noErr;
}
int main(int argc, char **argv) {
  check(argc == 2, "AU bundle path required");
  const auto path = std::string(argv[1]);
  auto url = CFURLCreateFromFileSystemRepresentation(
      nullptr, reinterpret_cast<const UInt8 *>(path.data()), path.size(), true);
  auto bundle = CFBundleCreate(nullptr, url);
  check(bundle && CFBundleLoadExecutable(bundle), "load built AU bundle");
  auto factory = reinterpret_cast<AudioComponentFactoryFunction>(
      CFBundleGetFunctionPointerForName(bundle, CFSTR("Scoria_Factory")));
  check(factory, "AU factory export");
  AudioComponentDescription description{'aufx', 'Scor', 'TlDs', 0, 0};
  auto component = AudioComponentRegister(&description, CFSTR("TalosDSP: Scoria test"), 1, factory);
  check(component, "register AU in test process only");
  for (double rate : {44100., 48000., 88200., 96000.}) {
    for (auto channels : {std::array<UInt32, 2>{1, 1}, {1, 2}, {2, 2}}) {
      AudioUnit unit = nullptr;
      status(AudioComponentInstanceNew(component, &unit), "instantiate AU");
      AudioStreamBasicDescription format{};
      format.mSampleRate = rate;
      format.mFormatID = kAudioFormatLinearPCM;
      format.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
      format.mBytesPerPacket = format.mBytesPerFrame = sizeof(float);
      format.mFramesPerPacket = 1;
      format.mBitsPerChannel = 32;
      format.mChannelsPerFrame = channels[0];
      status(AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0,
                                  &format, sizeof(format)),
             "input layout");
      format.mChannelsPerFrame = channels[1];
      status(AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0,
                                  &format, sizeof(format)),
             "output layout");
      UInt32 maximum = 2048;
      status(AudioUnitSetProperty(unit, kAudioUnitProperty_MaximumFramesPerSlice,
                                  kAudioUnitScope_Global, 0, &maximum, sizeof(maximum)),
             "maximum block size");
      AURenderCallbackStruct callback{input, nullptr};
      status(AudioUnitSetProperty(unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input,
                                  0, &callback, sizeof(callback)),
             "input callback");
      status(AudioUnitInitialize(unit), "initialize");
      Float64 latency = -1.;
      UInt32 latencyBytes = sizeof(latency);
      status(AudioUnitGetProperty(unit, kAudioUnitProperty_Latency, kAudioUnitScope_Global, 0,
                                  &latency, &latencyBytes),
             "latency property");
      check(latency == 0., "zero reported latency");
      status(AudioUnitSetParameter(unit, 0, kAudioUnitScope_Global, 0, -12., 0),
             "change before preset recall");
      AUPreset preset{0, CFSTR("M0 - Pass-through")};
      status(AudioUnitSetProperty(unit, kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, 0,
                                  &preset, sizeof(preset)),
             "factory preset recall");
      AudioUnitParameterValue defaultGain = -1.;
      status(AudioUnitGetParameter(unit, 0, kAudioUnitScope_Global, 0, &defaultGain),
             "factory preset value");
      check(defaultGain == 0., "factory preset restores unity");
      status(AudioUnitSetParameter(unit, 0, kAudioUnitScope_Global, 0, 6., 0), "input gain");
      status(AudioUnitSetParameter(unit, 1, kAudioUnitScope_Global, 0, -3., 0), "output gain");
      CFPropertyListRef saved = nullptr;
      UInt32 bytes = sizeof(saved);
      status(AudioUnitGetProperty(unit, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0,
                                  &saved, &bytes),
             "save state");
      status(AudioUnitSetParameter(unit, 0, kAudioUnitScope_Global, 0, -30., 0), "change input");
      status(AudioUnitSetParameter(unit, 2, kAudioUnitScope_Global, 0, 1., 0), "change bypass");
      status(AudioUnitSetProperty(unit, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0,
                                  &saved, sizeof(saved)),
             "restore state");
      CFRelease(saved);
      for (UInt32 id = 0; id < 3; ++id) {
        AudioUnitParameterValue value = 0;
        status(AudioUnitGetParameter(unit, id, kAudioUnitScope_Global, 0, &value),
               "read restored parameter");
        check(value == (id == 0 ? 6.f : id == 1 ? -3.f : 0.f), "state round trip");
      }
      std::array<float, 2048> left{}, right{};
      struct Buffers {
        UInt32 count;
        AudioBuffer buffers[2];
      } buffers{channels[1], {{1, sizeof(left), left.data()}, {1, sizeof(right), right.data()}}};
      AudioTimeStamp time{};
      time.mFlags = kAudioTimeStampSampleTimeValid;
      for (UInt32 size : {2048, 16, 32, 64, 128, 256, 512, 1024, 2048}) {
        AudioUnitRenderActionFlags flags = 0;
        status(AudioUnitRender(unit, &flags, &time, 0, size,
                               reinterpret_cast<AudioBufferList *>(&buffers)),
               "render");
        time.mSampleTime += size;
        const double gain = std::pow(10., 3. / 20.);
        check(std::abs(left[size - 1] - 0.125 * gain) < 1e-6, "AU rendered input/output gain");
        if (channels[1] == 2)
          check(std::abs(right[size - 1] - (channels[0] == 1 ? 0.125 : -0.25) * gain) < 1e-6,
                "AU channel routing");
      }
      status(AudioUnitSetParameter(unit, 2, kAudioUnitScope_Global, 0, 1., 0), "bypass on");
      AudioUnitRenderActionFlags flags = 0;
      status(AudioUnitRender(unit, &flags, &time, 0, 2048,
                             reinterpret_cast<AudioBufferList *>(&buffers)),
             "render bypass");
      check(left.back() == 0.125f, "AU bypass dry");
      status(AudioUnitReset(unit, kAudioUnitScope_Global, 0), "reset");
      status(AudioUnitUninitialize(unit), "uninitialize");
      status(AudioComponentInstanceDispose(unit), "dispose");
    }
  }
  CFRelease(bundle);
  CFRelease(url);
  std::cout << "AU loading, state, gain, bypass, layouts and lifecycle checks passed\n";
}

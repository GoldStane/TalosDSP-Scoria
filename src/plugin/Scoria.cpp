#include "Scoria.h"
#include "IControls.h"
#include "IPlug_include_in_plug_src.h"

using namespace iplug;
using namespace igraphics;

Scoria::Scoria(const InstanceInfo &info) : Plugin(info, MakeConfig(scoria::ParameterCount, 1)) {
  GetParam(scoria::InputGain)
      ->InitDouble("Input trim", 0., scoria::kMinimumGainDb, scoria::kMaximumGainDb, 0.1, "dB");
  GetParam(scoria::OutputGain)
      ->InitDouble("Output level", 0., scoria::kMinimumGainDb, scoria::kMaximumGainDb, 0.1, "dB");
  GetParam(scoria::GlobalBypass)->InitBool("Global bypass", false);
  MakeDefaultPreset("M0 - Pass-through", 1);
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS,
                        GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  mLayoutFunc = [&](IGraphics *g) {
    g->AttachPanelBackground(IColor(255, 27, 29, 32));
    g->LoadFont("Roboto-Regular", "Arial", ETextStyle::Normal);
    const IText text(18, COLOR_WHITE);
    g->AttachControl(new ITextControl(IRECT(20, 10, 780, 50), "TalosDSP Scoria | 0.0.1",
                                      IText(26, COLOR_WHITE)));
    auto *preset = new IVButtonControl(IRECT(20, 60, 780, 95), nullptr,
                                       "Preset: M0 - Pass-through (only preset)");
    preset->SetDisabled(true);
    g->AttachControl(preset);
    g->AttachControl(new IVSliderControl(IRECT(20, 105, 310, 180), scoria::InputGain, "Input trim",
                                         DEFAULT_STYLE, true, EDirection::Horizontal));
    g->AttachControl(new IVSliderControl(IRECT(330, 105, 620, 180), scoria::OutputGain,
                                         "Output level", DEFAULT_STYLE, true,
                                         EDirection::Horizontal));
    g->AttachControl(
        new IVToggleControl(IRECT(640, 105, 780, 180), scoria::GlobalBypass, "Global bypass"));
    const char *pages[4][5] = {{"Noise gate: enable / threshold / release - NOT IMPLEMENTED",
                                "Overdrive boost: enable / drive / tone / level - NOT IMPLEMENTED",
                                "Phaser: enable / rate / depth / feedback / mix - NOT IMPLEMENTED",
                                "Chorus: enable / rate / depth / mix - NOT IMPLEMENTED",
                                "Octaver: deferred until after v1"},
                               {"Distortion (high gain) / Crunch / Clean - NOT IMPLEMENTED",
                                "Amp drive / bass / mid / treble / presence / master: unavailable",
                                "Distortion is an amplifier voice, not a pedal.",
                                "M0 contains no amp model and applies no amp coloration.", ""},
                               {"Cabinet convolution - NOT IMPLEMENTED",
                                "Enable / IR selection / output / low cut / high cut: unavailable",
                                "No cabinet IRs are bundled in M0.", "", ""},
                               {"Stereo delay: enable / time / feedback / mix - NOT IMPLEMENTED",
                                "Stereo reverb: enable / decay / damping / mix - NOT IMPLEMENTED",
                                "Delay and reverb will follow the cabinet.", "", ""}};
    for (int page = 0; page < 4; ++page) {
      for (int row = 0; row < 5; ++row) {
        auto *label = new ITextControl(IRECT(25, 250 + row * 38, 775, 285 + row * 38),
                                       pages[page][row], text);
        g->AttachControl(label, 100 + page * 5 + row);
        label->Hide(page != 0);
      }
    }
    g->AttachControl(new IVTabSwitchControl(
        IRECT(20, 195, 780, 240),
        [g](IControl *control) {
          const int selected = static_cast<int>(control->GetValue() * 3.0 + 0.5);
          for (int page = 0; page < 4; ++page)
            for (int row = 0; row < 5; ++row)
              g->GetControlWithTag(100 + page * 5 + row)->Hide(page != selected);
          g->SetAllControlsDirty();
        },
        {"PEDALS", "AMP", "CAB", "FX"}));
    g->AttachControl(new ITextControl(IRECT(20, 460, 780, 510),
                                      "M0 foundation: input trim -> output level | zero latency",
                                      text));
  };
#endif
}

scoria::Parameters Scoria::parameters() const {
  return {GetParam(scoria::InputGain)->Value(), GetParam(scoria::OutputGain)->Value(),
          GetParam(scoria::GlobalBypass)->Bool()};
}
bool Scoria::SerializeState(IByteChunk &chunk) const {
  return chunk.Put(&scoria::kStateVersion) > 0 && SerializeParams(chunk);
}
int Scoria::UnserializeState(const IByteChunk &chunk, int startPos) {
  if (startPos < 0)
    return -1;
  std::int32_t version = 0;
  const int pos = chunk.Get(&version, startPos);
  if (pos < 0 || version != scoria::kStateVersion ||
      chunk.Size() - pos < 3 * static_cast<int>(sizeof(double)))
    return -1;
  int cursor = pos;
  for (int i = 0; i < scoria::ParameterCount; ++i) {
    double value = 0.;
    cursor = chunk.Get(&value, cursor);
    if (cursor < 0 || !std::isfinite(value))
      return -1;
  }
  return UnserializeParams(chunk, pos);
}
#if IPLUG_DSP
void Scoria::OnReset() { engine_.reset(GetSampleRate(), parameters()); }
void Scoria::ProcessBlock(sample **inputs, sample **outputs, int nFrames) {
  engine_.process(inputs, NInChansConnected(), outputs, NOutChansConnected(), nFrames,
                  parameters());
}
#endif

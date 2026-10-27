#include "Scoria.h"
#include "IControls.h"
#include "IPlugPaths.h"
#include "IPlug_include_in_plug_src.h"
#include "audio/State.h"
#include <filesystem>

using namespace iplug;
using namespace igraphics;

Scoria::Scoria(const InstanceInfo &info) : Plugin(info, MakeConfig(scoria::ParameterCount, 1)) {
  for (int i = 0; i < scoria::ParameterCount; ++i) {
    const auto &d = scoria::kDefinitions[i];
    if (i == scoria::CabinetSelection)
      GetParam(i)->InitEnum(d.name, 1, 2, "", IParam::kFlagsNone, "", "None (dry)",
                            "Jester Cookie Monster");
    else if (d.maximum == 1. && d.step == 1.)
      GetParam(i)->InitBool(d.name, d.initial > .5);
    else
      GetParam(i)->InitDouble(d.name, d.initial, d.minimum, d.maximum, d.step, d.unit);
  }
  GetParam(scoria::GateEnabled)->Set(1.);
  GetParam(scoria::OverdriveEnabled)->Set(1.);
  GetParam(scoria::AmpEnabled)->Set(1.);
  MakeDefaultPreset("Development - Distortion rig (48 kHz)", 1);
  WDL_String resourcePath;
#ifdef OS_WIN
  BundleResourcePath(resourcePath, gHINSTANCE);
#else
  BundleResourcePath(resourcePath, GetBundleID());
#endif
  const auto root = std::filesystem::u8path(resourcePath.Get()) / "assets";
  assets_ =
      std::make_unique<scoria::AssetLoader>((root / "models/riffsmith-distorted.nam").u8string(),
                                            (root / "irs/jester-cookie-monster.wav").u8string());
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS,
                        GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  mLayoutFunc = [&](IGraphics *g) {
    g->AttachPanelBackground(IColor(255, 27, 29, 32));
    g->LoadFont("Roboto-Regular", "Arial", ETextStyle::Normal);
    const auto labelStyle = DEFAULT_STYLE.WithLabelText(IText(16, COLOR_WHITE));
    const auto sliderStyle = labelStyle.WithValueText(IText(14, COLOR_WHITE));
    g->AttachControl(new ITextControl(IRECT(20, 10, 780, 50), "TalosDSP Scoria | 0.0.2",
                                      IText(26, COLOR_WHITE)));
    g->AttachControl(new ITextControl(IRECT(20, 60, 780, 95),
                                      "Preset: Development rig | 48 kHz | Tight Rhythm unavailable",
                                      IText(16, COLOR_WHITE)));
    g->AttachControl(new IVSliderControl(IRECT(20, 105, 310, 180), scoria::InputGain, "Input trim",
                                         sliderStyle, true, EDirection::Horizontal));
    g->AttachControl(new IVSliderControl(IRECT(330, 105, 620, 180), scoria::OutputGain,
                                         "Output level", sliderStyle, true,
                                         EDirection::Horizontal));
    g->AttachControl(new IVToggleControl(IRECT(640, 105, 780, 180), scoria::GlobalBypass,
                                         "Global bypass", labelStyle));
    int counts[4] = {0, 0, 0, 0};
    auto attach = [&](IControl *control, int page) {
      g->AttachControl(control, 100 + page * 30 + counts[page]++);
      control->Hide(page != 0);
    };
    auto label = [&](int page, int y, const char *title) {
      attach(new ITextControl(IRECT(25, y, 775, y + 28), title, IText(16, COLOR_WHITE)), page);
    };
    auto slider = [&](int page, int id, int x, int y, int width) {
      attach(new IVSliderControl(IRECT(x, y, x + width, y + 70), id, scoria::kDefinitions[id].name,
                                 sliderStyle, true, EDirection::Horizontal),
             page);
    };
    auto toggle = [&](int page, int id, int x, int y) {
      attach(new IVToggleControl(IRECT(x, y, x + 140, y + 65), id, scoria::kDefinitions[id].name,
                                 labelStyle),
             page);
    };
    toggle(0, scoria::GateEnabled, 25, 250);
    slider(0, scoria::GateThreshold, 185, 250, 270);
    slider(0, scoria::GateRelease, 475, 250, 295);
    toggle(0, scoria::OverdriveEnabled, 25, 330);
    slider(0, scoria::OverdriveDrive, 185, 330, 180);
    slider(0, scoria::OverdriveTone, 385, 330, 180);
    slider(0, scoria::OverdriveLevel, 585, 330, 185);
    label(0, 420, "Phaser / Chorus: unavailable | Octaver: deferred");
    label(1, 250, "Distortion: RiffSmith Distorted by Matt Hartman (includes cabinet)");
    toggle(1, scoria::AmpEnabled, 25, 295);
    slider(1, scoria::AmpInput, 185, 295, 270);
    slider(1, scoria::AmpOutput, 475, 295, 295);
    label(1, 380, "Scoria trim controls; not captured amplifier knob positions.");
    label(1, 420, "Crunch / Clean: unavailable. NAM requires a 48 kHz session.");
    label(2, 250, "IR: Jester Cookie Monster | BG412S 4x12 / Vintage 30 / SM57");
    toggle(2, scoria::CabinetEnabled, 25, 295);
    slider(2, scoria::CabinetOutput, 475, 295, 295);
    attach(new IVMenuButtonControl(IRECT(185, 300, 455, 355), scoria::CabinetSelection,
                                   "Cabinet IR", labelStyle),
           2);
    label(2, 380, "Separate cab defaults OFF: the bundled NAM already includes a cab.");
    label(2, 420, "Enabling this IR intentionally applies additional cabinet coloration.");
    label(3, 265, "Stereo delay: NOT IMPLEMENTED");
    label(3, 315, "Stereo reverb: NOT IMPLEMENTED");
    g->AttachControl(new IVTabSwitchControl(
        IRECT(20, 195, 780, 240),
        [g, counts](IControl *control) {
          const int selected = static_cast<int>(control->GetValue() * 3.0 + 0.5);
          for (int page = 0; page < 4; ++page)
            for (int row = 0; row < counts[page]; ++row)
              g->GetControlWithTag(100 + page * 30 + row)->Hide(page != selected);
          g->SetAllControlsDirty();
        },
        {"PEDALS", "AMP", "CAB", "FX"}));
    g->AttachControl(new ITextControl(IRECT(20, 460, 780, 510), "Preparing audio assets...",
                                      IText(15, COLOR_WHITE)),
                     900);
  };
#endif
}

void Scoria::OnIdle() {
#if IPLUG_EDITOR
  if (auto *g = GetUI())
    if (auto *label = dynamic_cast<ITextControl *>(g->GetControlWithTag(900))) {
      const char *messages[] = {
          "Preparing assets: enabled amp is silent until ready",
          "Development rig ready | zero added latency | listening validation pending",
          "Unsupported NAM rate: use 48 kHz; global bypass remains dry",
          "Asset loading failed: missing or unreadable bundled asset; reinstall complete "
          "bundle"};
      label->SetStr(messages[assets_->status()]);
    }
#endif
}
scoria::Parameters Scoria::parameters() const {
  std::array<double, scoria::ParameterCount> values{};
  for (int i = 0; i < scoria::ParameterCount; ++i)
    values[i] = GetParam(i)->Value();
  return scoria::fromValues(values);
}
bool Scoria::SerializeState(IByteChunk &chunk) const {
  // Stable asset IDs and development preset ID, independent of installation paths.
  const std::uint32_t model = 0x52464431, cabinet = 0x4a434d31, preset = 0;
  return chunk.Put(&scoria::kStateVersion) > 0 && chunk.Put(&model) > 0 &&
         chunk.Put(&cabinet) > 0 && chunk.Put(&preset) > 0 && SerializeParams(chunk);
}
int Scoria::UnserializeState(const IByteChunk &chunk, int startPos) {
  std::array<double, scoria::ParameterCount> values{};
  const int cursor = scoria::decodeState(chunk.GetData(), chunk.Size(), startPos, values);
  if (cursor < 0)
    return -1;
  IByteChunk validated;
  for (double value : values)
    validated.Put(&value);
  if (UnserializeParams(validated, 0) < 0)
    return -1;
  return cursor;
}
#if IPLUG_DSP
void Scoria::OnReset() {
  engine_.reset(GetSampleRate(), parameters());
  assets_->request(GetSampleRate());
}
void Scoria::ProcessBlock(sample **inputs, sample **outputs, int nFrames) {
  engine_.setAssets(assets_->poll());
  engine_.process(inputs, NInChansConnected(), outputs, NOutChansConnected(), nFrames,
                  parameters());
}
#endif

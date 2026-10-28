#include "Scoria.h"
#include "IControls.h"
#include "IPlugPaths.h"
#include "IPlug_include_in_plug_src.h"
#include "audio/State.h"
#include <filesystem>

using namespace iplug;
using namespace igraphics;

Scoria::Scoria(const InstanceInfo &info) : Plugin(info, MakeConfig(scoria::ParameterCount, 2)) {
  for (int i = 0; i < scoria::ParameterCount; ++i) {
    const auto &d = scoria::kDefinitions[i];
    if (i == scoria::CabinetSelection)
      GetParam(i)->InitEnum(d.name, 1, 2, "", IParam::kFlagsNone, "", "None (dry)",
                            "Jester Cookie Monster");
    else if (i == scoria::ModelSelection)
      GetParam(i)->InitEnum(d.name, 1, 2, "", IParam::kFlagsNone, "",
                            "RiffSmith (legacy, includes cab)", "Lum Herbert 4 (amp only)");
    else if (d.maximum == 1. && d.step == 1.)
      GetParam(i)->InitBool(d.name, d.initial > .5);
    else
      GetParam(i)->InitDouble(d.name, d.initial, d.minimum, d.maximum, d.step, d.unit);
  }
  for (int preset = 0; preset < 2; ++preset) {
    presetId_.store(preset == 0 ? 1 : 0);
    const auto values = scoria::factoryValues(preset == 0);
    for (int i = 0; i < scoria::ParameterCount; ++i)
      GetParam(i)->Set(values[i]);
    MakeDefaultPreset(preset == 0 ? "Tight Rhythm" : "Legacy development rig", 1);
  }
  RestorePreset(0);
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
    g->AttachControl(new ITextControl(IRECT(20, 10, 780, 50), "TalosDSP Scoria | 0.0.3",
                                      IText(26, COLOR_WHITE)));
    g->AttachControl(
        new IVBakedPresetManagerControl(IRECT(20, 55, 780, 95), "Factory preset", labelStyle), 901);
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
    label(1, 250, "Distortion: audited high-gain NAM captures");
    toggle(1, scoria::AmpEnabled, 25, 295);
    slider(1, scoria::AmpInput, 185, 295, 270);
    slider(1, scoria::AmpOutput, 475, 295, 295);
    attach(new IVMenuButtonControl(IRECT(25, 365, 770, 410), scoria::ModelSelection,
                                   "NAM model (Scoria trims above)", labelStyle),
           1);
    label(1, 420, "Crunch / Clean: unavailable. Trims do not reproduce hardware knobs.");
    label(2, 250, "IR: Jester Cookie Monster | BG412S 4x12 / Vintage 30 / SM57");
    toggle(2, scoria::CabinetEnabled, 25, 295);
    slider(2, scoria::CabinetOutput, 475, 295, 295);
    attach(new IVMenuButtonControl(IRECT(185, 300, 455, 355), scoria::CabinetSelection,
                                   "Cabinet IR", labelStyle),
           2);
    label(2, 380, "Tight Rhythm: amp-only capture + cabinet ON.");
    label(2, 420, "Legacy RiffSmith includes a cab: leave separate cabinet OFF.");
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
  if (auto *g = GetUI()) {
    if (auto *selector = dynamic_cast<IVBakedPresetManagerControl *>(g->GetControlWithTag(901))) {
      const auto factory = scoria::factoryValues(presetId_.load() == 1);
      bool modified = false;
      for (int i = 0; i < scoria::ParameterCount; ++i)
        modified |= GetParam(i)->Value() != factory[i];
      const std::string title =
          std::string(presetId_.load() == 1 ? "Tight Rhythm" : "Legacy development rig") +
          (modified ? " (modified)" : "");
      selector->ForAllChildrenFunc([&](int index, IControl *control) {
        if (index == 2)
          if (auto *button = dynamic_cast<IVButtonControl *>(control))
            button->SetLabelStr(title.c_str());
      });
    }
    if (auto *label = dynamic_cast<ITextControl *>(g->GetControlWithTag(900))) {
      const char *messages[] = {
          "Preparing assets: enabled amp is silent until ready",
          "Assets ready | Distortion / cabinet | 44.1 / 48 / 88.2 / 96 kHz",
          "Unsupported rate: use 44.1 / 48 / 88.2 / 96 kHz",
          "Asset loading failed: missing or unreadable bundled asset; reinstall complete "
          "bundle"};
      label->SetStr(messages[assets_->status()]);
    }
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
  const std::uint32_t model = scoria::kModels[GetParam(scoria::ModelSelection)->Int()].id,
                      cabinet = scoria::kCabinetId, preset = presetId_.load();
  return chunk.Put(&scoria::kStateVersion) > 0 && chunk.Put(&model) > 0 &&
         chunk.Put(&cabinet) > 0 && chunk.Put(&preset) > 0 && SerializeParams(chunk);
}
int Scoria::UnserializeState(const IByteChunk &chunk, int startPos) {
  std::array<double, scoria::ParameterCount> values{};
  std::uint32_t preset = 0;
  const int cursor = scoria::decodeState(chunk.GetData(), chunk.Size(), startPos, values, &preset);
  if (cursor < 0)
    return -1;
  IByteChunk validated;
  for (double value : values)
    validated.Put(&value);
  if (UnserializeParams(validated, 0) < 0)
    return -1;
  presetId_.store(preset);
  SetCurrentPresetIdx(preset == 1 ? 0 : 1);
  return cursor;
}
#if IPLUG_DSP
void Scoria::OnReset() {
  engine_.reset(GetSampleRate(), parameters());
  SetLatency(scoria::inferenceLatency(GetSampleRate()));
  assets_->request(GetSampleRate(), parameters().modelIndex, true);
  if (GetRenderingOffline()) {
    assets_->waitForPreparation();
    engine_.setAssets(assets_->poll());
  }
}
void Scoria::ProcessBlock(sample **inputs, sample **outputs, int nFrames) {
  const auto p = parameters();
  assets_->request(GetSampleRate(), p.modelIndex);
  engine_.setAssets(assets_->poll());
  engine_.process(inputs, NInChansConnected(), outputs, NOutChansConnected(), nFrames, p);
}
#endif

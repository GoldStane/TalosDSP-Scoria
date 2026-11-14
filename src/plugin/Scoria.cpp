#include "Scoria.h"
#include "IControls.h"
#include "IPlugPaths.h"
#include "IPlug_include_in_plug_src.h"
#include "audio/State.h"
#include <filesystem>

using namespace iplug;
using namespace igraphics;

Scoria::Scoria(const InstanceInfo &info) : Plugin(info, MakeConfig(scoria::ParameterCount, 5)) {
  for (int i = 0; i < scoria::ParameterCount; ++i) {
    const auto &d = scoria::kDefinitions[i];
    if (i == scoria::CabinetSelection)
      GetParam(i)->InitEnum(d.name, 1, 2, "", IParam::kFlagsNone, "", "None (dry)",
                            "Jester Cookie Monster");
    else if (i == scoria::AmplifierVoice)
      GetParam(i)->InitEnum(d.name, 0, 3, "", IParam::kFlagsNone, "", "Distortion", "Crunch",
                            "Clean");
    else if (i == scoria::ModelSelection)
      GetParam(i)->InitEnum(d.name, 1, 2, "", IParam::kFlagsNone, "",
                            "RiffSmith (legacy, includes cab)", "Lum Herbert 4 (amp only)");
    else if (d.maximum == 1. && d.step == 1.)
      GetParam(i)->InitBool(d.name, d.initial > .5);
    else
      GetParam(i)->InitDouble(d.name, d.initial, d.minimum, d.maximum, d.step, d.unit);
  }
  for (int preset = 0; preset < 5; ++preset) {
    presetId_.store(preset == 0 ? 1 : preset == 1 ? 0 : preset);
    const auto values = scoria::presetValues(presetId_.load());
    for (int i = 0; i < scoria::ParameterCount; ++i)
      GetParam(i)->Set(values[i]);
    MakeDefaultPreset(scoria::presetName(presetId_.load()), 1);
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
    g->AttachControl(new ITextControl(IRECT(20, 10, 780, 50), "TalosDSP Scoria | 0.0.4",
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
    auto selected = std::make_shared<std::array<int, 3>>(std::array<int, 3>{0, 0, 0});
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
    auto effectControls = [&](int effect, std::initializer_list<int> ids) {
      int tag = 400 + effect * 10;
      int index = 0;
      for (int id : ids) {
        const int x = 200 + (index % 2) * 290, y = 300 + (index / 2) * 75;
        auto *slider =
            new IVSliderControl(IRECT(x, y, x + 275, y + 70), id, scoria::kDefinitions[id].name,
                                sliderStyle, true, EDirection::Horizontal);
        g->AttachControl(slider, tag++);
        slider->Hide(effect != 0);
        ++index;
      }
    };
    const int pedalIds[]{scoria::GateEnabled, scoria::OverdriveEnabled, scoria::PhaserEnabled,
                         scoria::ChorusEnabled};
    const char *pedalNames[]{"Gate", "Overdrive", "Phaser", "Chorus"};
    for (int i = 0; i < 4; ++i)
      attach(new IVToggleControl(IRECT(25, 300 + i * 36, 180, 333 + i * 36), pedalIds[i],
                                 pedalNames[i], labelStyle),
             0);
    attach(new IVToggleControl(IRECT(25, 310, 180, 365), scoria::DelayEnabled, "Delay", labelStyle),
           3);
    attach(
        new IVToggleControl(IRECT(25, 385, 180, 440), scoria::ReverbEnabled, "Reverb", labelStyle),
        3);
    effectControls(0, {scoria::GateThreshold, scoria::GateRelease});
    effectControls(1, {scoria::OverdriveDrive, scoria::OverdriveTone, scoria::OverdriveLevel});
    effectControls(
        2, {scoria::PhaserRate, scoria::PhaserDepth, scoria::PhaserFeedback, scoria::PhaserMix});
    effectControls(3, {scoria::ChorusRate, scoria::ChorusDepth, scoria::ChorusMix});
    effectControls(4, {scoria::DelayTime, scoria::DelayFeedback, scoria::DelayMix});
    effectControls(5, {scoria::ReverbDecay, scoria::ReverbDamping, scoria::ReverbMix});
    auto updateEffects = [g, selected]() {
      for (int effect = 0; effect < 6; ++effect)
        for (int item = 0; item < 5; ++item)
          if (auto *control = g->GetControlWithTag(400 + effect * 10 + item))
            control->Hide(!(effect < 4 ? (*selected)[0] == 0 && (*selected)[1] == effect
                                       : (*selected)[0] == 3 && (*selected)[2] + 4 == effect));
      g->SetAllControlsDirty();
    };
    attach(new IVTabSwitchControl(IRECT(25, 250, 775, 290),
                                  [selected, updateEffects](IControl *c) {
                                    (*selected)[1] = int(c->GetValue() * 3. + .5);
                                    updateEffects();
                                  },
                                  {"Gate", "Overdrive", "Phaser", "Chorus"}),
           0);
    attach(new IVTabSwitchControl(IRECT(25, 250, 775, 290),
                                  [selected, updateEffects](IControl *c) {
                                    (*selected)[2] = int(c->GetValue() + .5);
                                    updateEffects();
                                  },
                                  {"Delay", "Reverb"}),
           3);
    attach(new IVMenuButtonControl(IRECT(25, 250, 770, 285), scoria::AmplifierVoice,
                                   "Amplifier voice", labelStyle),
           1);
    toggle(1, scoria::AmpEnabled, 25, 295);
    slider(1, scoria::AmpInput, 185, 295, 270);
    slider(1, scoria::AmpOutput, 475, 295, 295);
    attach(new IVMenuButtonControl(IRECT(25, 365, 770, 410), scoria::ModelSelection,
                                   "Distortion capture (used by Distortion voice)", labelStyle),
           1);
    attach(new ITextControl(IRECT(25, 420, 775, 450),
                            "NAM trims are digital gain, not physical amp knobs.",
                            IText(16, COLOR_WHITE)),
           1);
    label(2, 250, "IR: Jester Cookie Monster | BG412S 4x12 / Vintage 30 / SM57");
    toggle(2, scoria::CabinetEnabled, 25, 295);
    slider(2, scoria::CabinetOutput, 475, 295, 295);
    attach(new IVMenuButtonControl(IRECT(185, 300, 455, 355), scoria::CabinetSelection,
                                   "Cabinet IR", labelStyle),
           2);
    label(2, 380, "Tight Rhythm: amp-only capture + cabinet ON.");
    label(2, 420, "Legacy RiffSmith includes a cab: leave separate cabinet OFF.");

    g->AttachControl(new IVTabSwitchControl(
        IRECT(20, 195, 780, 240),
        [g, counts, selected, updateEffects](IControl *control) {
          const int pageIndex = static_cast<int>(control->GetValue() * 3.0 + 0.5);
          (*selected)[0] = pageIndex;
          for (int page = 0; page < 4; ++page)
            for (int row = 0; row < counts[page]; ++row)
              g->GetControlWithTag(100 + page * 30 + row)->Hide(page != pageIndex);
          updateEffects();
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
      const auto factory = scoria::presetValues(presetId_.load());
      bool modified = false;
      for (int i = 0; i < scoria::ParameterCount; ++i)
        modified |= GetParam(i)->Value() != factory[i];
      const std::string title =
          std::string(scoria::presetName(presetId_.load())) + (modified ? " (modified)" : "");
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
      const std::string status =
          assets_->status() == scoria::AssetLoader::Ready
              ? std::string("Ready | ") + scoria::kModels[parameters().modelIndex].name
              : messages[assets_->status()];
      label->SetStr(status.c_str());
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
  const std::uint32_t model = scoria::kModels[parameters().modelIndex].id,
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
  SetCurrentPresetIdx(preset == 1 ? 0 : preset == 0 ? 1 : int(preset));
  return cursor;
}
#if IPLUG_DSP
void Scoria::OnReset() {
  engine_.reset(GetSampleRate(), parameters());
  SetLatency(scoria::inferenceLatency(GetSampleRate()));
  SetTailSize(int(GetSampleRate() * 560.));
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

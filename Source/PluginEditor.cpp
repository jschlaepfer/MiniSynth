/*
    File: PluginEditor.cpp
    Project: MiniSynth
    Revision: 1.0.0
    Date: 2025-09-03
    Description:
        Implements the editor: builds controls, binds APVTS attachments,
        presets toolbar, and compact/full layouts. (Patched to remove deprecated
        AlertWindow::runModalLoop and old showOkCancelBox signature.)
*/


#include "PluginEditor.h"
#include "BinaryData.h"


using namespace juce;

void MiniSynthAudioProcessorEditor::KnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                                      float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                                                                      juce::Slider& slider)
{
    // Draw JUCE default rotary knob first
    this->juce::LookAndFeel_V4::drawRotarySlider (g, x, y, width, height,
                                                  sliderPosProportional, rotaryStartAngle, rotaryEndAngle, slider);

    // Then overlay the current value centered inside the knob
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    auto inner  = bounds.reduced (bounds.getWidth() * 0.18f);

    g.setColour (juce::Colours::white);
    auto font = g.getCurrentFont().withHeight (juce::jmin (14.0f, inner.getHeight() * 0.28f));
    g.setFont (font);

    juce::String valueText = slider.getTextFromValue (slider.getValue());
    if (valueText.length() > 6)
        valueText = valueText.substring (0, 6);

    g.drawFittedText (valueText, inner.toNearestInt(), juce::Justification::centred, 1);
}

static void styleKnob (juce::Slider& s, MiniSynthAudioProcessorEditor* owner) {
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    if (owner != nullptr) s.setLookAndFeel (owner->getKnobLookAndFeel());
}

void MiniSynthAudioProcessorEditor::addKnobLabel (Slider& s, const String& text)
{
    auto* lb = new juce::Label();
    lb->setText (text, juce::dontSendNotification);
    lb->setJustificationType (juce::Justification::centred);
    lb->setInterceptsMouseClicks (false, false);
    lb->setColour (juce::Label::textColourId, juce::Colours::white);
    lb->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    auto f = lb->getFont().withHeight (12.0f); // non-déprécié
    lb->setFont (f);
    addAndMakeVisible (lb);
    lb->toFront (true); // s'assurer qu'il passe devant
    knobLabels.add (lb);
    knobLabelLinks.add ({ &s, lb });
}

void MiniSynthAudioProcessorEditor::positionKnobLabels()
{
    for (auto link : knobLabelLinks)
    {
        if (link.slider != nullptr && link.label != nullptr)
        {
            const bool vis = link.slider->isVisible();
            link.label->setVisible (vis);
            if (! vis) continue;

            auto b = link.slider->getBounds();
            auto labelArea = juce::Rectangle<int> (b.getX(), b.getBottom() - 20, b.getWidth(), 16);
            link.label->setBounds (labelArea);
        }
    }
}

void MiniSynthAudioProcessorEditor::addControlLabel (Component& c, const String& text)
{
    auto* lb = new juce::Label();
    lb->setText (text, juce::dontSendNotification);
    lb->setJustificationType (juce::Justification::centred);
    lb->setInterceptsMouseClicks (false, false);
    lb->setColour (juce::Label::textColourId, juce::Colours::white);
    lb->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    auto f = lb->getFont().withHeight (12.0f);
    lb->setFont (f);
    addAndMakeVisible (lb);
    lb->toFront (true);
    controlLabels.add (lb);
    controlLabelLinks.add ({ &c, lb });
}

void MiniSynthAudioProcessorEditor::positionControlLabels()
{
    for (auto link : controlLabelLinks)
    {
        if (link.comp != nullptr && link.label != nullptr)
        {
            const bool vis = link.comp->isVisible();
            link.label->setVisible (vis);
            if (! vis) continue;

            auto b = link.comp->getBounds();
            auto labelArea = juce::Rectangle<int> (b.getX(), b.getBottom() - 20, b.getWidth(), 16);
            link.label->setBounds (labelArea);
        }
    }
}

MiniSynthAudioProcessorEditor::MiniSynthAudioProcessorEditor (MiniSynthAudioProcessor& p)
: AudioProcessorEditor (&p), processor (p)
{
    setSize (1300, 900);

    // Presets bar
    addAndMakeVisible (presetBox);
    addAndMakeVisible (saveBtn); addAndMakeVisible (deleteBtn); addAndMakeVisible (reloadBtn);

    // ---- SAVE (async FileChooser, pas de modal loop bloquante) ----
    saveBtn.onClick = [this] {
        // Propose un nom par défaut; l’emplacement choisi n’est pas utilisé pour enregistrer,
        // on extrait seulement le nom de fichier (processor.saveUserPreset(name)).
        auto defaultFile = processor.getUserPresetDir().getChildFile ("MyPreset.minisynth.xml");

        FileChooser chooser ("Save preset as...", defaultFile, "*.xml;*.json");
        chooser.launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles,
                             [this](const FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f.getFullPathName().isNotEmpty())
            {
                auto name = f.getFileNameWithoutExtension().trim();
                if (name.isNotEmpty())
                {
                    if (processor.saveUserPreset (name))
                        refreshPresetBox();
                }
            }
        });
    };

    // ---- DELETE (NativeMessageBox avec signature complète) ----
    deleteBtn.onClick = [this] {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx < 0) return;
        auto name = presetBox.getText();

        const bool ok = juce::NativeMessageBox::showOkCancelBox(
            juce::MessageBoxIconType::WarningIcon,
            "Delete preset",
            "Supprimer '" + name + "' ?",
            this,        // associatedComponent (optional)
            nullptr      // callback (nullptr = modal, requires JUCE_MODAL_LOOPS_PERMITTED)
        );

        if (ok) {
            if (processor.deleteUserPreset (name))
                refreshPresetBox();
        }
    };

    reloadBtn.onClick = [this] { refreshPresetBox(); };
    presetBox.onChange = [this] { auto idx = presetBox.getSelectedItemIndex(); if (idx >= 0) processor.applyPresetByIndex (idx); };
    refreshPresetBox();

    // Branding image (optional): looks for brand.png or logo.png in user preset dir
    addAndMakeVisible (brandImage);
    refreshBrandImage();

    // Compact
    // addAndMakeVisible (compactToggle);
    // compactToggle.setToggleState (false, dontSendNotification);
    // compactToggle.setVisible (false);
    // compactToggle.onClick = [this]{ /* full layout always on */ };
    // isCompact = false;
    
      addAndMakeVisible (compactToggle); compactToggle.setToggleState (true, dontSendNotification);
    compactToggle.onClick = [this]{ isCompact = compactToggle.getToggleState(); resized(); };
  

    // Always visible
    for (auto* cb : { &w1, &w2, &w3, &filtType }) addAndMakeVisible (*cb);
    for (auto* s  : { &mix1, &mix2, &mix3, &cutoff, &resonance, &gain }) { addAndMakeVisible (*s); styleKnob (*s, this); }
    w1.addItemList (StringArray{ "Sine","Saw+","Pulse","Tri","NoiseW","Saw-","Fold","HalfS" }, 1);
    w2.addItemList (StringArray{ "Sine","Saw+","Pulse","Tri","NoiseW","Saw-","Fold","HalfS" }, 1);
    w3.addItemList (StringArray{ "Sine","Saw+","Pulse","Tri","NoiseW","Saw-","Fold","HalfS" }, 1);
    filtType.addItemList (StringArray{ "LP","BP","HP" }, 1);
    
    aW1 = std::make_unique<CAttach> (processor.apvts, ids::osc1Wave, w1);
    aW2 = std::make_unique<CAttach> (processor.apvts, ids::osc2Wave, w2);
    aW3 = std::make_unique<CAttach> (processor.apvts, ids::osc3Wave, w3);
    aFiltType = std::make_unique<CAttach> (processor.apvts, ids::filterType, filtType);
    aMix1 = std::make_unique<Attach> (processor.apvts, ids::mix1, mix1);
    aMix2 = std::make_unique<Attach> (processor.apvts, ids::mix2, mix2);
    aMix3 = std::make_unique<Attach> (processor.apvts, ids::mix3, mix3);
    aCut  = std::make_unique<Attach> (processor.apvts, ids::cutoff, cutoff);
    aQ    = std::make_unique<Attach> (processor.apvts, ids::resonance, resonance);
    aGain = std::make_unique<Attach> (processor.apvts, ids::gain, gain);

    // Advanced
    for (auto* s : { &det1,&det2,&det3,&uniDet,&uniWidth,&spread,
                      &pwm1,&pwm2,&pwm3,&pwmD1,&pwmD2,&pwmD3,&pwmR1,&pwmR2,&pwmR3,
                      &subLevel,&subDrive,
                      &noiseW,&noiseP,&noiseB,&noiseHPF,
                      &aA,&aD,&aS,&aR,
                      &fA,&fD,&fS,&fR,&fAmt,
                      &lfo1Rate,&lfo1Depth,&lfo2Rate,&lfo2Depth,
                      &fm31,&fm32 }) { addAndMakeVisible (*s); styleKnob (*s, this); }

    for (auto* b : { &uniOn, &subOn, &subAsym, &noiseHPFOn, &sync21, &sync31 }) addAndMakeVisible (*b);
    for (auto* cb : { &subWave, &subOct, &lfo1Target, &lfo2Target }) addAndMakeVisible (*cb);

    aDet1 = std::make_unique<Attach> (processor.apvts, ids::detune1, det1);
    aDet2 = std::make_unique<Attach> (processor.apvts, ids::detune2, det2);
    aDet3 = std::make_unique<Attach> (processor.apvts, ids::detune3, det3);
    aSpread = std::make_unique<Attach> (processor.apvts, ids::stereoSpread, spread);
    aUniOn  = std::make_unique<BAttach> (processor.apvts, ids::uniOn, uniOn);
    aUniDet = std::make_unique<Attach> (processor.apvts, ids::uniDetune, uniDet);
    aUniWidth = std::make_unique<Attach> (processor.apvts, ids::uniWidth, uniWidth);

    aPWM1 = std::make_unique<Attach> (processor.apvts, ids::pwm1, pwm1);
    aPWM2 = std::make_unique<Attach> (processor.apvts, ids::pwm2, pwm2);
    aPWM3 = std::make_unique<Attach> (processor.apvts, ids::pwm3, pwm3);
    aPWMD1= std::make_unique<Attach> (processor.apvts, ids::pwmDepth1, pwmD1);
    aPWMD2= std::make_unique<Attach> (processor.apvts, ids::pwmDepth2, pwmD2);
    aPWMD3= std::make_unique<Attach> (processor.apvts, ids::pwmDepth3, pwmD3);
    aPWMR1= std::make_unique<Attach> (processor.apvts, ids::pwmRate1,  pwmR1);
    aPWMR2= std::make_unique<Attach> (processor.apvts, ids::pwmRate2,  pwmR2);
    aPWMR3= std::make_unique<Attach> (processor.apvts, ids::pwmRate3,  pwmR3);

    aSubOn   = std::make_unique<BAttach> (processor.apvts, ids::subOn, subOn);
    aSubAsym = std::make_unique<BAttach> (processor.apvts, ids::subAsym, subAsym);
    aSubLevel= std::make_unique<Attach> (processor.apvts, ids::subLevel, subLevel);
    aSubDrive= std::make_unique<Attach> (processor.apvts, ids::subDrive, subDrive);
    aSubWave = std::make_unique<CAttach> (processor.apvts, ids::subWave, subWave);
    aSubOct  = std::make_unique<CAttach> (processor.apvts, ids::subOct,  subOct);
    subWave.addItemList (StringArray{ "Sine","Square","Tri" }, 1);
    subOct.addItemList  (StringArray{ "-1","-2" }, 1);

    aNoiseW  = std::make_unique<Attach> (processor.apvts, ids::mixNoiseW, noiseW);
    aNoiseP  = std::make_unique<Attach> (processor.apvts, ids::mixNoiseP, noiseP);
    aNoiseB  = std::make_unique<Attach> (processor.apvts, ids::mixNoiseB, noiseB);
    aNoiseHPF= std::make_unique<Attach> (processor.apvts, ids::noiseHPF,  noiseHPF);
    aNoiseHPFOn = std::make_unique<BAttach> (processor.apvts, ids::noiseHPFOn, noiseHPFOn);

    aAA = std::make_unique<Attach> (processor.apvts, ids::attack,  aA);
    aAD = std::make_unique<Attach> (processor.apvts, ids::decay,   aD);
    aAS = std::make_unique<Attach> (processor.apvts, ids::sustain, aS);
    aAR = std::make_unique<Attach> (processor.apvts, ids::release, aR);

    aFA = std::make_unique<Attach> (processor.apvts, ids::fA, fA);
    aFD = std::make_unique<Attach> (processor.apvts, ids::fD, fD);
    aFS = std::make_unique<Attach> (processor.apvts, ids::fS, fS);
    aFR = std::make_unique<Attach> (processor.apvts, ids::fR, fR);
    aFAmt = std::make_unique<Attach> (processor.apvts, ids::fAmt, fAmt);

    aLfo1Rate = std::make_unique<Attach> (processor.apvts, ids::lfoRate,  lfo1Rate);
    aLfo1Depth= std::make_unique<Attach> (processor.apvts, ids::lfoDepth, lfo1Depth);
    aLfo2Rate = std::make_unique<Attach> (processor.apvts, ids::lfo2Rate, lfo2Rate);
    aLfo2Depth= std::make_unique<Attach> (processor.apvts, ids::lfo2Depth, lfo2Depth);
    aLfo1T = std::make_unique<CAttach> (processor.apvts, ids::lfoTarget,  lfo1Target);
    aLfo2T = std::make_unique<CAttach> (processor.apvts, ids::lfo2Target, lfo2Target);
    lfo1Target.addItemList (StringArray{ "None","Pitch","Amp","Cutoff","PWM" }, 1);
    lfo2Target.addItemList (StringArray{ "None","Pitch","Amp","Cutoff","PWM" }, 1);

    aSync21 = std::make_unique<BAttach> (processor.apvts, ids::sync2to1, sync21);
    aSync31 = std::make_unique<BAttach> (processor.apvts, ids::sync3to1, sync31);
    aFM31   = std::make_unique<Attach> (processor.apvts, ids::fm31, fm31);
    aFM32   = std::make_unique<Attach> (processor.apvts, ids::fm32, fm32);

    // --- Labels for knobs ---
    addKnobLabel (mix1,        "Mix 1");
    addKnobLabel (mix2,        "Mix 2");
    addKnobLabel (mix3,        "Mix 3");
    addKnobLabel (cutoff,      "Cutoff");
    addKnobLabel (resonance,   "Reso");
    addKnobLabel (gain,        "Gain");

    addKnobLabel (det1,        "Det 1");
    addKnobLabel (det2,        "Det 2");
    addKnobLabel (det3,        "Det 3");
    addKnobLabel (spread,      "Spread");

    addKnobLabel (pwm1,        "PWM1");
    addKnobLabel (pwm2,        "PWM2");
    addKnobLabel (pwm3,        "PWM3");
    addKnobLabel (pwmD1,       "PWMD1");
    addKnobLabel (pwmD2,       "PWMD2");
    addKnobLabel (pwmD3,       "PWMD3");
    addKnobLabel (pwmR1,       "PWMR1");
    addKnobLabel (pwmR2,       "PWMR2");
    addKnobLabel (pwmR3,       "PWMR3");

    addKnobLabel (subLevel,    "SubLvl");
    addKnobLabel (subDrive,    "SubDrv");

    addKnobLabel (noiseW,      "NoiseW");
    addKnobLabel (noiseP,      "NoiseP");
    addKnobLabel (noiseB,      "NoiseB");
    addKnobLabel (noiseHPF,    "NoiseHPF");

    addKnobLabel (aA,          "A");
    addKnobLabel (aD,          "D");
    addKnobLabel (aS,          "S");
    addKnobLabel (aR,          "R");

    addKnobLabel (fA,          "fA");
    addKnobLabel (fD,          "fD");
    addKnobLabel (fS,          "fS");
    addKnobLabel (fR,          "fR");
    addKnobLabel (fAmt,        "fAmt");

    addKnobLabel (lfo1Rate,    "LFO1 Rt");
    addKnobLabel (lfo1Depth,   "LFO1 Dp");
    addKnobLabel (lfo2Rate,    "LFO2 Rt");
    addKnobLabel (lfo2Depth,   "LFO2 Dp");

    addKnobLabel (fm31,        "FM3-1");
    addKnobLabel (fm32,        "FM3-2");

    addKnobLabel (uniDet,      "Uni Det");
    addKnobLabel (uniWidth,    "Uni W");

    // --- Labels for non-knob controls (buttons & combo boxes) ---
    addControlLabel (w1,         "Osc 1");
    addControlLabel (w2,         "Osc 2");
    addControlLabel (w3,         "Osc 3");
    addControlLabel (filtType,   "Filter");

    addControlLabel (subWave,    "Sub Wave");
    addControlLabel (subOct,     "Sub Oct");

    addControlLabel (lfo1Target, "LFO1 Target");
    addControlLabel (lfo2Target, "LFO2 Target");

    addControlLabel (uniOn,      "Unison");
    addControlLabel (subOn,      "Sub");
    addControlLabel (subAsym,    "Asym");
    addControlLabel (noiseHPFOn, "Noise HPF");
    addControlLabel (sync21,     "Sync 2-1");
    addControlLabel (sync31,     "Sync 3-1");

    // Ensure labels and controls are laid out correctly on first show
    isCompact = compactToggle.getToggleState();
    
    resized();

    startTimerHz (20);
}

void MiniSynthAudioProcessorEditor::refreshPresetBox() {
    presetBox.clear (dontSendNotification);
    auto names = processor.getPresetNames();
    for (int i = 0; i < names.size(); ++i) presetBox.addItem (names[i], i + 1);
    presetBox.setSelectedItemIndex (0, dontSendNotification);
}

void MiniSynthAudioProcessorEditor::paint (Graphics& g) {
    g.fillAll (Colours::black);
    g.setColour (Colours::white);
    g.setFont (10.0f);
    g.drawText ("MiniSynth v1.0.0 — Compact:" + String (isCompact ? "On" : "Off"), getLocalBounds().removeFromTop(24), Justification::centred);
}

void MiniSynthAudioProcessorEditor::updateVisibility()
{
    // Tous les contrôles connus du layout "full"
    std::initializer_list<juce::Component*> all = {
        &w1,&w2,&w3,&filtType,
        &mix1,&mix2,&mix3,&cutoff,&resonance,&gain,
        &det1,&det2,&det3,&spread,
        &pwm1,&pwm2,&pwm3,&pwmD1,&pwmD2,&pwmD3,&pwmR1,&pwmR2,&pwmR3,
        &subOn,&subWave,&subOct,&subLevel,&subDrive,&subAsym,
        &noiseW,&noiseP,&noiseB,&noiseHPFOn,&noiseHPF,
        &aA,&aD,&aS,&aR,
        &fA,&fD,&fS,&fR,&fAmt,
        &lfo1Rate,&lfo1Depth,&lfo1Target,&lfo2Rate,&lfo2Depth,&lfo2Target,
        &uniOn,&uniDet,&uniWidth,
        &sync21,&sync31,&fm31,&fm32
    };

    // Sous-ensemble voulu en mode "compact" (Capture 1)
    std::initializer_list<juce::Component*> compact = {
        &w1,&w2,&w3,
        &mix1,&mix2,&mix3,
        &filtType,&cutoff,&resonance,&gain,
        &sync21,&sync31,
        &fm31,&fm32
    };

    if (isCompact)
    {
        for (auto* c : all)     if (c) c->setVisible(false);
        for (auto* c : compact) if (c) c->setVisible(true);
    }
    else
    {
        for (auto* c : all) if (c) c->setVisible(true);
    }
}

static void placeRow (std::initializer_list<Component*> comps, Rectangle<int> area, int w=110) {
    int pad = 6; int x = area.getX();
    for (auto* c : comps) { c->setBounds (Rectangle<int> (x, area.getY(), w, area.getHeight()).reduced (4)); x += w + pad; }
}

void MiniSynthAudioProcessorEditor::refreshBrandImage()
{
    // Load embedded logo from BinaryData (generated by juce_add_binary_data)
    juce::Image img;
    if (BinaryData::logo_pngSize > 0)
        img = juce::ImageFileFormat::loadFrom (BinaryData::logo_png, BinaryData::logo_pngSize);

    if (img.isValid())
        brandImage.setImage (img, juce::RectanglePlacement::centred);
    else
        brandImage.setImage (juce::Image(), juce::RectanglePlacement::centred);
}

void MiniSynthAudioProcessorEditor::resized() {
    auto r = getLocalBounds().reduced (8);

    // Presets bar
    auto bar = r.removeFromTop (26);
    presetBox.setBounds (bar.removeFromLeft (260).reduced (2));
    saveBtn.setBounds   (bar.removeFromLeft (70).reduced (2));
    deleteBtn.setBounds (bar.removeFromLeft (70).reduced (2));
    reloadBtn.setBounds (bar.removeFromLeft (70).reduced (2));
    compactToggle.setBounds (bar.removeFromLeft (100).reduced (2));

    updateVisibility();

    // Reserve a right column for the branding image, then lay out controls on the left
    auto working = r; // remaining area below the toolbar
    const int maxLogoW = 360;
    const int logoW = juce::jmin (maxLogoW, working.getWidth() / 3);
    auto right = working.removeFromRight (logoW).reduced (8);
    auto logoBox = right.removeFromTop (logoW); // keep it square
    brandImage.setBounds (logoBox);

    if (isCompact) layoutCompact (working); else layoutFull (working);
 }

void MiniSynthAudioProcessorEditor::layoutCompact (Rectangle<int> r) {
    auto row1 = r.removeFromTop (140);
    placeRow ({ &w1,&w2,&w3,&mix1,&mix2,&mix3 }, row1);

    auto row2 = r.removeFromTop (140);
    placeRow ({ &filtType,&cutoff,&resonance,&gain,&sync21,&sync31 }, row2);

    auto row3 = r.removeFromTop (140);
    placeRow ({ &fm31,&fm32 }, row3);

    positionKnobLabels();
    positionControlLabels();
    for (auto* lb : knobLabels)   if (lb) lb->toFront (false);
    for (auto* lb : controlLabels) if (lb) lb->toFront (false);
}

void MiniSynthAudioProcessorEditor::layoutFull (Rectangle<int> r) {
    auto row1 = r.removeFromTop (120);
    placeRow ({ &w1,&mix1,&det1,&pwm1,&pwmD1,&pwmR1 }, row1);

    auto row2 = r.removeFromTop (120);
    placeRow ({ &w2,&mix2,&det2,&pwm2,&pwmD2,&pwmR2 }, row2);

    auto row3 = r.removeFromTop (120);
    placeRow ({ &w3,&mix3,&det3,&pwm3,&pwmD3,&pwmR3 }, row3);

    auto row4 = r.removeFromTop (120);
    placeRow ({ &uniOn,&uniDet,&uniWidth,&spread,&sync21,&sync31,&fm31,&fm32 }, row4);

    auto row5 = r.removeFromTop (120);
    placeRow ({ &filtType,&cutoff,&resonance,&fAmt,&fA,&fD,&fS,&fR }, row5);

    auto row6 = r.removeFromTop (120);
    placeRow ({ &aA,&aD,&aS,&aR,&lfo1Rate,&lfo1Depth,&lfo1Target,&lfo2Rate,&lfo2Depth,&lfo2Target }, row6, 100);

    auto row7 = r.removeFromTop (120);
    placeRow ({ &subOn,&subWave,&subOct,&subLevel,&subDrive,&subAsym,&noiseW,&noiseP,&noiseB,&noiseHPFOn,&noiseHPF,&gain }, row7, 100);

    positionKnobLabels();
    positionControlLabels();
    for (auto* lb : knobLabels)   if (lb) lb->toFront (false);
    for (auto* lb : controlLabels) if (lb) lb->toFront (false);

}

void MiniSynthAudioProcessorEditor::timerCallback() {
    // placeholder for meters
}

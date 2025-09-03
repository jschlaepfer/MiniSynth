/*
    File: PluginEditor.h
    Project: MiniSynth
    Revision: 1.0.0
    Date: 2025-09-03
    Description:
        Implements the editor: builds controls, binds APVTS attachments,
        presets toolbar, and compact/full layouts. (Patched to remove deprecated
        AlertWindow::runModalLoop and old showOkCancelBox signature.)
*/

#pragma once
#include "JuceIncludes.h"
#include "PluginProcessor.h"

class MiniSynthAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit MiniSynthAudioProcessorEditor (MiniSynthAudioProcessor&);
    ~MiniSynthAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    juce::LookAndFeel* getKnobLookAndFeel() { return &knobLAF; }

private:
    using Attach  = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CAttach = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
  
    // --- Knob labels (name under rotary sliders)
    struct KnobLabelLink { juce::Slider* slider = nullptr; juce::Label* label = nullptr; };
    juce::OwnedArray<juce::Label> knobLabels;      // owns label instances
    juce::Array<KnobLabelLink>   knobLabelLinks;   // non-owning links

    void addKnobLabel(juce::Slider& s, const juce::String& text);
    void positionKnobLabels();
    
    // --- LookAndFeel to draw value inside rotary knobs
    struct KnobLookAndFeel : public juce::LookAndFeel_V4 {
        void drawRotarySlider (juce::Graphics& g,
                               int x, int y, int width, int height,
                               float sliderPosProportional,
                               float rotaryStartAngle, float rotaryEndAngle,
                               juce::Slider& slider) override;
    } knobLAF;
    
    // --- Generic control labels (buttons, combo boxes, etc.) ---
    struct CtrlLabelLink { juce::Component* comp = nullptr; juce::Label* label = nullptr; };
    juce::OwnedArray<juce::Label> controlLabels;     // owns label instances
    juce::Array<CtrlLabelLink>    controlLabelLinks; // non-owning links

    void addControlLabel(juce::Component& c, const juce::String& text);
    void positionControlLabels();
    
    void updateVisibility();
    // Branding (top-right logo)
    juce::ImageComponent brandImage;   // displays embedded logo
    void refreshBrandImage();          // loads from BinaryData
    MiniSynthAudioProcessor& processor;

    // Presets
    juce::ComboBox presetBox; juce::TextButton saveBtn {"Save"}, deleteBtn {"Delete"}, reloadBtn {"Reload"};

    // Compact toggle (disabled: always show full UI)
    juce::ToggleButton compactToggle {"Compact"}; bool isCompact = false;

    // Always visible
    juce::ComboBox w1, w2, w3, filtType;
    juce::Slider mix1, mix2, mix3, cutoff, resonance, gain;

    // Advanced
    juce::Slider det1, det2, det3, spread;
    juce::ToggleButton uniOn {"Unison"};
    juce::Slider uniDet, uniWidth;

    juce::Slider pwm1, pwm2, pwm3, pwmD1, pwmD2, pwmD3, pwmR1, pwmR2, pwmR3;

    juce::ToggleButton subOn {"Sub"}, subAsym {"Asym"};
    juce::ComboBox subWave, subOct;
    juce::Slider subLevel, subDrive;

    juce::Slider noiseW, noiseP, noiseB, noiseHPF; juce::ToggleButton noiseHPFOn {"Noise HPF"};

    juce::Slider aA, aD, aS, aR;

    juce::Slider fA, fD, fS, fR, fAmt;

    juce::Slider lfo1Rate, lfo1Depth; juce::ComboBox lfo1Target;
    juce::Slider lfo2Rate, lfo2Depth; juce::ComboBox lfo2Target;

    juce::ToggleButton sync21 {"Sync 2-1"}, sync31 {"Sync 3-1"};
    juce::Slider fm31, fm32;

    // Attachments
    std::unique_ptr<CAttach> aW1, aW2, aW3, aFiltType, aSubWave, aSubOct, aLfo1T, aLfo2T;
    std::unique_ptr<Attach> aMix1, aMix2, aMix3, aCut, aQ, aGain, aDet1, aDet2, aDet3, aSpread, aUniDet, aUniWidth;
    std::unique_ptr<Attach> aPWM1, aPWM2, aPWM3, aPWMD1, aPWMD2, aPWMD3, aPWMR1, aPWMR2, aPWMR3;
    std::unique_ptr<Attach> aSubLevel, aSubDrive, aNoiseW, aNoiseP, aNoiseB, aNoiseHPF, aAA, aAD, aAS, aAR, aFA, aFD, aFS, aFR, aFAmt;
    std::unique_ptr<Attach> aLfo1Rate, aLfo1Depth, aLfo2Rate, aLfo2Depth, aFM31, aFM32;
    std::unique_ptr<BAttach> aUniOn, aSubOn, aSubAsym, aNoiseHPFOn, aSync21, aSync31;

    // Helpers
    void layoutCompact (juce::Rectangle<int> r);
    void layoutFull    (juce::Rectangle<int> r);
    void refreshPresetBox();

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniSynthAudioProcessorEditor)
};

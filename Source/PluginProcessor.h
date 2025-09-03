/*
    File: PluginProcessor.h
    Project: MiniSynth
    Revision: 1.0.0
    Date: 2025-08-19
    Description:
        Main AudioProcessor declaration. Owns the Synthesiser, APVTS parameter
        tree, preset manager façade, and exposes plugin lifecycle methods.
*/

#pragma once
#include "JuceIncludes.h"
#include <atomic>

namespace presets { class PresetManager; }

namespace ids {
// Oscillateurs & Mix
static constexpr auto osc1Wave = "osc1Wave";
static constexpr auto osc2Wave = "osc2Wave";
static constexpr auto osc3Wave = "osc3Wave";
static constexpr auto mix1 = "mix1"; static constexpr auto mix2 = "mix2"; static constexpr auto mix3 = "mix3";
// Detune / Stereo / Unison
static constexpr auto detune1 = "detune1"; static constexpr auto detune2 = "detune2"; static constexpr auto detune3 = "detune3";
static constexpr auto stereoSpread = "stereoSpread";
static constexpr auto uniOn = "uniOn"; static constexpr auto uniDetune = "uniDetune"; static constexpr auto uniWidth = "uniWidth";
// PWM
static constexpr auto pwm1 = "pwm1"; static constexpr auto pwm2 = "pwm2"; static constexpr auto pwm3 = "pwm3";
static constexpr auto pwmDepth1 = "pwmDepth1"; static constexpr auto pwmDepth2 = "pwmDepth2"; static constexpr auto pwmDepth3 = "pwmDepth3";
static constexpr auto pwmRate1  = "pwmRate1";  static constexpr auto pwmRate2  = "pwmRate2";  static constexpr auto pwmRate3  = "pwmRate3";
// Sub & Noise
static constexpr auto subOn = "subOn"; static constexpr auto subWave = "subWave"; static constexpr auto subOct = "subOct";
static constexpr auto subLevel = "subLevel"; static constexpr auto subDrive = "subDrive"; static constexpr auto subAsym = "subAsym";
static constexpr auto mixNoiseW = "mixNoiseW"; static constexpr auto mixNoiseP = "mixNoiseP"; static constexpr auto mixNoiseB = "mixNoiseB";
static constexpr auto noiseHPFOn = "noiseHPFOn"; static constexpr auto noiseHPF = "noiseHPF";
// Amp ADSR
static constexpr auto attack = "attack"; static constexpr auto decay = "decay"; static constexpr auto sustain = "sustain"; static constexpr auto release = "release";
// Filter + Env
static constexpr auto filterType = "filterType"; static constexpr auto cutoff = "cutoff"; static constexpr auto resonance = "resonance";
static constexpr auto fA = "fAttack"; static constexpr auto fD = "fDecay"; static constexpr auto fS = "fSustain"; static constexpr auto fR = "fRelease"; static constexpr auto fAmt = "fAmount";
// LFOs
static constexpr auto lfoRate = "lfoRate"; static constexpr auto lfoDepth = "lfoDepth"; static constexpr auto lfoTarget = "lfoTarget";
static constexpr auto lfo2Rate = "lfo2Rate"; static constexpr auto lfo2Depth = "lfo2Depth"; static constexpr auto lfo2Target = "lfo2Target";
// Sync / FM (reserved)
static constexpr auto sync2to1 = "sync2to1"; static constexpr auto sync3to1 = "sync3to1"; static constexpr auto fm31 = "fm31"; static constexpr auto fm32 = "fm32";
// Global
static constexpr auto gain = "gain"; static constexpr auto mpeEnabled = "mpeEnabled"; static constexpr auto bendRange = "bendRange";
}

struct SynthSound : public juce::SynthesiserSound { bool appliesToNote (int) override { return true; } bool appliesToChannel (int) override { return true; } };
class SynthVoice; // fwd

class MiniSynthAudioProcessor : public juce::AudioProcessor {
public:
    MiniSynthAudioProcessor();
    ~MiniSynthAudioProcessor() override = default;

    // AudioProcessor
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MiniSynth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // APVTS
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "PARAMS", createLayout() };
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Presets API (via PresetManager)
    juce::StringArray getPresetNames() const;
    bool isFactoryPreset (int index) const;
    void applyPresetByIndex (int index);
    bool saveUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);
    juce::File getUserPresetDir() const;

    float getMeterLevel() const { return meterLevel.load(); }

private:
    std::unique_ptr<presets::PresetManager> presetMgr;
    juce::Synthesiser synth;
    std::atomic<float> meterLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniSynthAudioProcessor)
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

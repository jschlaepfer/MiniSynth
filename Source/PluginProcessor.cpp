/*
    File: PluginProcessor.cpp
    Project: MiniSynth
    Revision: 1.0.0
    Date: 2025-08-19
    Description:
        Implements the main processor logic: audio rendering via Synthesiser,
        parameter layout, state save/restore, and preset manager wiring.
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/SynthVoice.h"
#include "presets/PresetManager.h"

using namespace juce;

MiniSynthAudioProcessor::MiniSynthAudioProcessor()
: AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true))
{
    synth.clearVoices();
    for (int i = 0; i < 8; ++i)
        synth.addVoice (new SynthVoice (apvts));
    synth.clearSounds();
    synth.addSound (new SynthSound());

    presetMgr = std::make_unique<presets::PresetManager> (apvts, "YourName", "MiniSynth");
}

bool MiniSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void MiniSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            v->prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void MiniSynthAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi) {
    ScopedNoDenormals noDenormals;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, buffer.getNumSamples());

    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* d = buffer.getReadPointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            peak = jmax (peak, std::abs (d[i]));
    }
    meterLevel.store (0.9f * meterLevel.load() + 0.1f * peak);
}

void MiniSynthAudioProcessor::getStateInformation (MemoryBlock& dest) {
    auto tree = apvts.copyState();
    if (auto xml = tree.createXml()) copyXmlToBinary (*xml, dest);
}

void MiniSynthAudioProcessor::setStateInformation (const void* data, int size) {
    if (auto xml = getXmlFromBinary (data, size)) apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessorValueTreeState::ParameterLayout MiniSynthAudioProcessor::createLayout() {
    using AP = AudioProcessorValueTreeState;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    // Waves
    p.push_back (std::make_unique<AudioParameterChoice> (ids::osc1Wave, "OSC1", StringArray{ "Sine","Saw+","Pulse","Tri","NoiseW","Saw-","Fold","HalfS" }, 1));
    p.push_back (std::make_unique<AudioParameterChoice> (ids::osc2Wave, "OSC2", StringArray{ "Sine","Saw+","Pulse","Tri","NoiseW","Saw-","Fold","HalfS" }, 2));
    p.push_back (std::make_unique<AudioParameterChoice> (ids::osc3Wave, "OSC3", StringArray{ "Sine","Saw+","Pulse","Tri","NoiseW","Saw-","Fold","HalfS" }, 0));

    p.push_back (std::make_unique<AudioParameterFloat> (ids::mix1, "Mix1", NormalisableRange<float> (0,1), 0.7f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::mix2, "Mix2", NormalisableRange<float> (0,1), 0.6f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::mix3, "Mix3", NormalisableRange<float> (0,1), 0.2f));

    p.push_back (std::make_unique<AudioParameterFloat> (ids::detune1, "Det1 (st)", NormalisableRange<float> (-24, 24), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::detune2, "Det2 (st)", NormalisableRange<float> (-24, 24), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::detune3, "Det3 (st)", NormalisableRange<float> (-24, 24), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::stereoSpread, "Spread", NormalisableRange<float> (0, 1), 0.2f));

    p.push_back (std::make_unique<AudioParameterBool>  (ids::uniOn,  "Unison", true));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::uniDetune, "UniDet", NormalisableRange<float> (0, 50), 12.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::uniWidth,  "UniWidth", NormalisableRange<float> (0, 1), 0.5f));

    // PWM
    p.push_back (std::make_unique<AudioParameterFloat> (ids::pwm1, "PWM1", NormalisableRange<float> (0.05f, 0.95f), 0.5f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::pwm2, "PWM2", NormalisableRange<float> (0.05f, 0.95f), 0.5f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::pwm3, "PWM3", NormalisableRange<float> (0.05f, 0.95f), 0.5f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::pwmDepth1, "PWM1 Depth", NormalisableRange<float> (0,1), 0.3f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::pwmDepth2, "PWM2 Depth", NormalisableRange<float> (0,1), 0.3f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::pwmDepth3, "PWM3 Depth", NormalisableRange<float> (0,1), 0.3f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::pwmRate1,  "PWM1 Rate",  NormalisableRange<float> (0.05f, 10.0f, 0.f, 0.4f), 1.2f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::pwmRate2,  "PWM2 Rate",  NormalisableRange<float> (0.05f, 10.0f, 0.f, 0.4f), 0.8f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::pwmRate3,  "PWM3 Rate",  NormalisableRange<float> (0.05f, 10.0f, 0.f, 0.4f), 0.6f));

    // Sub & Noise
    p.push_back (std::make_unique<AudioParameterBool>  (ids::subOn, "Sub On", true));
    p.push_back (std::make_unique<AudioParameterChoice>(ids::subWave, "Sub Wave", StringArray{ "Sine","Square","Tri" }, 1));
    p.push_back (std::make_unique<AudioParameterChoice>(ids::subOct,  "Sub Oct",  StringArray{ "-1","-2" }, 1));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::subLevel,"Sub Level", NormalisableRange<float> (0,1), 0.35f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::subDrive,"Sub Drive", NormalisableRange<float> (0,24), 6.0f));
    p.push_back (std::make_unique<AudioParameterBool>  (ids::subAsym, "Sub Asym", false));

    p.push_back (std::make_unique<AudioParameterFloat> (ids::mixNoiseW, "White", NormalisableRange<float> (0,1), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::mixNoiseP, "Pink",  NormalisableRange<float> (0,1), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::mixNoiseB, "Brown", NormalisableRange<float> (0,1), 0.0f));
    p.push_back (std::make_unique<AudioParameterBool>  (ids::noiseHPFOn, "Noise HPF", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::noiseHPF,   "HPF Hz",     NormalisableRange<float> (20.0f, 2000.0f, 0.f, 0.4f), 120.0f));

    // Amp ADSR
    p.push_back (std::make_unique<AudioParameterFloat> (ids::attack,  "Attack",  NormalisableRange<float> (0.001f, 3.0f, 0.f, 0.5f), 0.01f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::decay,   "Decay",   NormalisableRange<float> (0.001f, 3.0f, 0.f, 0.5f), 0.12f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::sustain, "Sustain", NormalisableRange<float> (0.0f,   1.0f), 0.8f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::release, "Release", NormalisableRange<float> (0.001f, 4.0f, 0.f, 0.5f), 0.25f));

    // Filter + Env
    p.push_back (std::make_unique<AudioParameterChoice>(ids::filterType, "Filter", StringArray{ "LP","BP","HP" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::cutoff, "Cutoff", NormalisableRange<float> (20.0f, 20000.0f, 0.f, 0.3f), 12000.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::resonance, "Q", NormalisableRange<float> (0.1f, 10.0f, 0.f, 0.5f), 0.7f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::fA, "F-Attack",  NormalisableRange<float> (0.001f, 3.0f, 0.f, 0.5f), 0.01f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::fD, "F-Decay",   NormalisableRange<float> (0.001f, 3.0f, 0.f, 0.5f), 0.12f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::fS, "F-Sustain", NormalisableRange<float> (0.0f,   1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::fR, "F-Release", NormalisableRange<float> (0.001f, 4.0f, 0.f, 0.5f), 0.25f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::fAmt, "F-Amount", NormalisableRange<float> (0.0f,  1.0f), 0.0f));

    // LFOs
    p.push_back (std::make_unique<AudioParameterFloat> (ids::lfoRate,  "LFO1 Rate",  NormalisableRange<float> (0.05f, 20.0f, 0.f, 0.5f), 5.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::lfoDepth, "LFO1 Depth", NormalisableRange<float> (0.0f,   1.0f), 0.3f));
    p.push_back (std::make_unique<AudioParameterChoice>(ids::lfoTarget,"LFO1 Target", StringArray{ "None","Pitch","Amp","Cutoff","PWM" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::lfo2Rate,  "LFO2 Rate",  NormalisableRange<float> (0.05f, 20.0f, 0.f, 0.5f), 0.8f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::lfo2Depth, "LFO2 Depth", NormalisableRange<float> (0.0f,   1.0f), 0.2f));
    p.push_back (std::make_unique<AudioParameterChoice>(ids::lfo2Target,"LFO2 Target", StringArray{ "None","Pitch","Amp","Cutoff","PWM" }, 0));

    // Sync / FM (reserved)
    p.push_back (std::make_unique<AudioParameterBool>  (ids::sync2to1, "Sync 2-1", false));
    p.push_back (std::make_unique<AudioParameterBool>  (ids::sync3to1, "Sync 3-1", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::fm31, "FM 3-1 (st)", NormalisableRange<float> (0.0f, 24.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::fm32, "FM 3-2 (st)", NormalisableRange<float> (0.0f, 24.0f), 0.0f));

    // Global
    p.push_back (std::make_unique<AudioParameterFloat> (ids::gain, "Gain", NormalisableRange<float> (-24.0f, 6.0f), -6.0f));
    p.push_back (std::make_unique<AudioParameterBool>  (ids::mpeEnabled, "MPE", true));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::bendRange,  "Bend", NormalisableRange<float> (1.0f, 48.0f), 48.0f));

    return { p.begin(), p.end() };
}

// Presets pass-through
juce::StringArray MiniSynthAudioProcessor::getPresetNames() const { return presetMgr ? presetMgr->getAllPresetNames() : juce::StringArray(); }
bool MiniSynthAudioProcessor::isFactoryPreset (int index) const { return presetMgr && presetMgr->isFactoryIndex (index); }
void MiniSynthAudioProcessor::applyPresetByIndex (int index) { if (presetMgr) presetMgr->applyPresetByIndex (index); }
bool MiniSynthAudioProcessor::saveUserPreset (const juce::String& name) { return presetMgr && presetMgr->saveUserPreset (name); }
bool MiniSynthAudioProcessor::deleteUserPreset (const juce::String& name) { return presetMgr && presetMgr->deleteUserPreset (name); }
juce::File MiniSynthAudioProcessor::getUserPresetDir() const { return presetMgr ? presetMgr->getUserDir() : juce::File(); }

AudioProcessorEditor* MiniSynthAudioProcessor::createEditor() { return new MiniSynthAudioProcessorEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MiniSynthAudioProcessor(); }

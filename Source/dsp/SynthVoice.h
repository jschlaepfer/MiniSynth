/*
    File: SynthVoice.h
    Project: MiniSynth
    Revision: 1.0.0
    Date: 2025-08-19
    Description:
        Voice class for the polyphonic Synthesiser. Hosts oscillators, sub,
        noise, envelopes, filters, and LFOs. Renders audio per voice.
*/

#pragma once
#include "JuceIncludes.h"
#include "Noise.h"
#include "PulseOsc.h"
#include "PolyBLEPOsc.h"

class SynthVoice : public juce::SynthesiserVoice {
public:
    explicit SynthVoice (juce::AudioProcessorValueTreeState& stateRef);

    bool canPlaySound (juce::SynthesiserSound* sound) override;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote (float, bool allowTailOff) override;

    void pitchWheelMoved (int value) override;
    void channelPressureChanged (int value) override;
    void aftertouchChanged (int value) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override;

private:
    void updateStaticParams();
    void updateDynamicParams();

    juce::AudioProcessorValueTreeState& state;

    static constexpr int unisonVoices = 2;
    juce::dsp::Oscillator<float> osc1[unisonVoices], osc2[unisonVoices], osc3[unisonVoices];
    PulseOsc    pulse1[unisonVoices], pulse2[unisonVoices], pulse3[unisonVoices];
    PolyBLEPOsc blep1 [unisonVoices], blep2 [unisonVoices], blep3 [unisonVoices];

    juce::dsp::Oscillator<float> subSine { [](float x){ return std::sin (x); } };
    juce::dsp::Oscillator<float> subTri  { [](float x){ return (2.0f/juce::MathConstants<float>::pi) * std::asin (std::sin (x)); } };
    PulseOsc subPulse;
    NoiseBus noise;

    juce::dsp::StateVariableTPTFilter<float> filterL, filterR;

    juce::ADSR ampEnv, filtEnv;

    juce::dsp::Oscillator<float> lfo1 { [](float x){ return std::sin (x); } };
    juce::dsp::Oscillator<float> lfo2 { [](float x){ return std::sin (x); } };
    juce::dsp::Oscillator<float> pwmLfo1 { [](float x){ return std::sin (x); } };
    juce::dsp::Oscillator<float> pwmLfo2 { [](float x){ return std::sin (x); } };
    juce::dsp::Oscillator<float> pwmLfo3 { [](float x){ return std::sin (x); } };

    juce::AudioBuffer<float> temp;
    double sampleRate = 44100.0;
    float baseFreqHz = 440.0f, curVelocity = 1.0f;

    float pitchBendSemitones = 0.0f, aftertouch = 0.0f, channelPressure = 0.0f;
};

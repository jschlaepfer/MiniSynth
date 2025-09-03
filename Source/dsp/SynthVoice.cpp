/*
    File: SynthVoice.cpp
    Project: MiniSynth
    Revision: 1.0.0
    Date: 2025-08-19
    Description:
        Voice rendering implementation. Assembles oscillators, PWM, sub & noise,
        envelopes and filter modulation, then mixes into the output buffer.
*/

#include "SynthVoice.h"
#include "../PluginProcessor.h"

using namespace juce;
using namespace juce::dsp;

static inline float noteHz (int midi) { return (float) MidiMessage::getMidiNoteInHertz (midi); }

SynthVoice::SynthVoice (AudioProcessorValueTreeState& s) : state (s) {}

bool SynthVoice::canPlaySound (SynthesiserSound* snd) { return dynamic_cast<SynthSound*> (snd) != nullptr; }

void SynthVoice::prepare (double sr, int spb, int numCh) {
    sampleRate = sr;
    ProcessSpec spec{ sampleRate, (uint32) spb, (uint32) jmax (1, numCh) };

    for (int i = 0; i < unisonVoices; ++i) {
        osc1[i].initialise ([](float x){ return std::sin (x); }, 128);
        osc2[i].initialise ([](float x){ return std::sin (x); }, 128);
        osc3[i].initialise ([](float x){ return std::sin (x); }, 128);
        osc1[i].prepare (spec); osc2[i].prepare (spec); osc3[i].prepare (spec);
        pulse1[i].prepare (spec); pulse2[i].prepare (spec); pulse3[i].prepare (spec);
        blep1[i].prepare  (spec); blep2[i].prepare  (spec); blep3[i].prepare  (spec);
    }
    subSine.prepare (spec); subTri.prepare (spec); subPulse.prepare (spec); noise.prepare (spec);

    filterL.prepare (spec); filterR.prepare (spec);

    ampEnv.setSampleRate (sampleRate);
    filtEnv.setSampleRate (sampleRate);

    lfo1.prepare (spec); lfo2.prepare (spec);
    pwmLfo1.prepare (spec); pwmLfo2.prepare (spec); pwmLfo3.prepare (spec);

    updateStaticParams();
}

void SynthVoice::startNote (int midi, float vel, SynthesiserSound*, int) {
    baseFreqHz = noteHz (midi);
    curVelocity = jlimit (0.0f, 1.0f, vel);
    pitchBendSemitones = 0.0f; aftertouch = 0.0f; channelPressure = 0.0f;
    ampEnv.noteOn(); filtEnv.noteOn();
}

void SynthVoice::stopNote (float, bool tail) {
    ampEnv.noteOff(); filtEnv.noteOff();
    if (! tail || ! ampEnv.isActive()) clearCurrentNote();
}

void SynthVoice::pitchWheelMoved (int v) {
    const float range = state.getRawParameterValue (ids::bendRange)->load();
    const float norm = (v - 8192) / 8192.0f;
    pitchBendSemitones = norm * range;
}
void SynthVoice::channelPressureChanged (int v) { channelPressure = jlimit (0.0f, 1.0f, v / 127.0f); }
void SynthVoice::aftertouchChanged      (int v) { aftertouch      = jlimit (0.0f, 1.0f, v / 127.0f); }
void SynthVoice::controllerMoved (int /*controllerNumber*/, int /*newControllerValue*/)
{
    // No-op for now. You can map CC1 (mod wheel), CC11, etc. to parameters here.
}
void SynthVoice::updateStaticParams() {
    ampEnv.setParameters ({ state.getRawParameterValue (ids::attack )->load(),
                            state.getRawParameterValue (ids::decay  )->load(),
                            state.getRawParameterValue (ids::sustain)->load(),
                            state.getRawParameterValue (ids::release)->load() });

    filtEnv.setParameters ({ state.getRawParameterValue (ids::fA)->load(),
                             state.getRawParameterValue (ids::fD)->load(),
                             state.getRawParameterValue (ids::fS)->load(),
                             state.getRawParameterValue (ids::fR)->load() });
}

void SynthVoice::updateDynamicParams() {
    lfo1.setFrequency (state.getRawParameterValue (ids::lfoRate )->load());
    lfo2.setFrequency (state.getRawParameterValue (ids::lfo2Rate)->load());
    pwmLfo1.setFrequency (state.getRawParameterValue (ids::pwmRate1)->load());
    pwmLfo2.setFrequency (state.getRawParameterValue (ids::pwmRate2)->load());
    pwmLfo3.setFrequency (state.getRawParameterValue (ids::pwmRate3)->load());
}

void SynthVoice::renderNextBlock (AudioBuffer<float>& output, int start, int n) {
    if (! isVoiceActive()) return;

    updateDynamicParams();

    temp.setSize (2, n, false, false, true); temp.clear();
    float* L = temp.getWritePointer (0);
    float* R = temp.getWritePointer (1);

    const int  wave1i = (int) state.getRawParameterValue (ids::osc1Wave)->load();
    const int  wave2i = (int) state.getRawParameterValue (ids::osc2Wave)->load();
    const int  wave3i = (int) state.getRawParameterValue (ids::osc3Wave)->load();

    const float mix1v = state.getRawParameterValue (ids::mix1)->load();
    const float mix2v = state.getRawParameterValue (ids::mix2)->load();
    const float mix3v = state.getRawParameterValue (ids::mix3)->load();

    const float det1  = state.getRawParameterValue (ids::detune1)->load();
    const float det2  = state.getRawParameterValue (ids::detune2)->load();
    const float det3  = state.getRawParameterValue (ids::detune3)->load();

    const bool  uniOn = state.getRawParameterValue (ids::uniOn)->load() > 0.5f;
    const float uniDet= state.getRawParameterValue (ids::uniDetune)->load();
    const float spread= state.getRawParameterValue (ids::stereoSpread)->load();

    const float pwm1B = state.getRawParameterValue (ids::pwm1)->load();
    const float pwm2B = state.getRawParameterValue (ids::pwm2)->load();
    const float pwm3B = state.getRawParameterValue (ids::pwm3)->load();
    const float pwmD1 = state.getRawParameterValue (ids::pwmDepth1)->load();
    const float pwmD2 = state.getRawParameterValue (ids::pwmDepth2)->load();
    const float pwmD3 = state.getRawParameterValue (ids::pwmDepth3)->load();

    const int subWave = (int) state.getRawParameterValue (ids::subWave)->load();
    const int subOct  = (int) state.getRawParameterValue (ids::subOct )->load();
    const bool subOn  = state.getRawParameterValue (ids::subOn)->load() > 0.5f;
    const float subLvl= state.getRawParameterValue (ids::subLevel)->load();

    const float nW = state.getRawParameterValue (ids::mixNoiseW)->load();
    const float nP = state.getRawParameterValue (ids::mixNoiseP)->load();
    const float nB = state.getRawParameterValue (ids::mixNoiseB)->load();
    const bool nHPFon = state.getRawParameterValue (ids::noiseHPFOn)->load() > 0.5f;
    const float nHPFhz = state.getRawParameterValue (ids::noiseHPF)->load();

    const int fType = (int) state.getRawParameterValue (ids::filterType)->load();
    const float cutoff = state.getRawParameterValue (ids::cutoff)->load();
    const float q      = state.getRawParameterValue (ids::resonance)->load();
    const float fAmt   = state.getRawParameterValue (ids::fAmt)->load();

    const float lfo1Dp = state.getRawParameterValue (ids::lfoDepth)->load();
    const int   lfo1Tg = (int) state.getRawParameterValue (ids::lfoTarget)->load();
    const float lfo2Dp = state.getRawParameterValue (ids::lfo2Depth)->load();
    const int   lfo2Tg = (int) state.getRawParameterValue (ids::lfo2Target)->load();

    filterL.setType ((fType==0)? StateVariableTPTFilterType::lowpass
                   : (fType==1)? StateVariableTPTFilterType::bandpass
                               : StateVariableTPTFilterType::highpass);
    filterR.setType ((fType==0)? StateVariableTPTFilterType::lowpass
                   : (fType==1)? StateVariableTPTFilterType::bandpass
                               : StateVariableTPTFilterType::highpass);

    const float bendRatio = std::pow (2.0f, pitchBendSemitones / 12.0f);
    const float gLin = Decibels::decibelsToGain (state.getRawParameterValue (ids::gain)->load());

    for (int i = 0; i < n; ++i) {
        const float lfo1v = lfo1.processSample (0.0f); // -1..1
        const float lfo2v = lfo2.processSample (0.0f);

        float f1 = baseFreqHz * bendRatio * std::pow (2.0f, det1 / 12.0f);
        float f2 = baseFreqHz * bendRatio * std::pow (2.0f, det2 / 12.0f);
        float f3 = baseFreqHz * bendRatio * std::pow (2.0f, det3 / 12.0f);

        if (lfo1Tg == 1) { f1 *= std::pow (2.0f, 0.1f * lfo1Dp * lfo1v); f2 *= std::pow (2.0f, 0.1f * lfo1Dp * lfo1v); f3 *= std::pow (2.0f, 0.1f * lfo1Dp * lfo1v); }
        if (lfo2Tg == 1) { f1 *= std::pow (2.0f, 0.05f * lfo2Dp * lfo2v); f2 *= std::pow (2.0f, 0.05f * lfo2Dp * lfo2v); f3 *= std::pow (2.0f, 0.05f * lfo2Dp * lfo2v); }

        const float pw1 = juce::jlimit (0.05f, 0.95f, pwm1B + pwmD1 * pwmLfo1.processSample (0.0f));
        const float pw2 = juce::jlimit (0.05f, 0.95f, pwm2B + pwmD2 * pwmLfo2.processSample (0.0f));
        const float pw3 = juce::jlimit (0.05f, 0.95f, pwm3B + pwmD3 * pwmLfo3.processSample (0.0f));

        auto oscSample = [&](int wave, float freq, float pw, PulseOsc& pulse, PolyBLEPOsc& blep, Oscillator<float>& sinGen){
            float s = 0.0f;
            switch (wave) {
                case 0: sinGen.setFrequency (freq); s = sinGen.processSample (0.0f); break;                   // Sine
                case 1: blep.setMode (PolyBLEPOsc::SawUp);   blep.setFrequency (freq); s = blep.processSample(); break; // Saw+
                case 5: blep.setMode (PolyBLEPOsc::SawDown); blep.setFrequency (freq); s = blep.processSample(); break; // Saw-
                case 2: pulse.setFrequency (freq); pulse.setPulseWidth (pw); s = pulse.processSample(); break;          // Pulse
                case 3: sinGen.setFrequency (freq); s = (2.0f/MathConstants<float>::pi) * std::asin (sinGen.processSample (0.0f)); break; // Tri via arcsin(sin)
                case 4: s = noise.white(); break;                                                 // White noise as OSC
                case 6: sinGen.setFrequency (freq); s = std::tanh (2.0f * sinGen.processSample (0.0f)); break;          // Folded sine
                case 7: sinGen.setFrequency (freq); s = juce::jlimit (-1.0f, 1.0f, sinGen.processSample (0.0f) * 0.5f + 0.5f); break; // Half-sine
                default: sinGen.setFrequency (freq); s = sinGen.processSample (0.0f); break;
            }
            return s;
        };

        float s1 = oscSample (wave1i, f1, pw1, pulse1[0], blep1[0], osc1[0]);
        float s2 = oscSample (wave2i, f2, pw2, pulse2[0], blep2[0], osc2[0]);
        float s3 = oscSample (wave3i, f3, pw3, pulse3[0], blep3[0], osc3[0]);

        if (uniOn) {
            s1 = 0.5f * (s1 + oscSample (wave1i, f1 * std::pow (2.0f,  uniDet / 1200.0f), pw1, pulse1[1], blep1[1], osc1[1]));
            s2 = 0.5f * (s2 + oscSample (wave2i, f2 * std::pow (2.0f, -uniDet / 1200.0f), pw2, pulse2[1], blep2[1], osc2[1]));
            s3 = 0.5f * (s3 + oscSample (wave3i, f3 * std::pow (2.0f,  uniDet / 1200.0f), pw3, pulse3[1], blep3[1], osc3[1]));
        }

        float sub = 0.0f;
        if (subOn) {
            const float subF = baseFreqHz * (subOct == 0 ? 0.5f : 0.25f);
            if (subWave == 0) { subSine.setFrequency (subF); sub = subSine.processSample (0.0f); }
            else if (subWave == 1) { subPulse.setFrequency (subF); subPulse.setPulseWidth (0.5f); sub = subPulse.processSample(); }
            else { subTri.setFrequency (subF); sub = subTri.processSample (0.0f); }
        }

        float noi = nW * noise.white() + nP * noise.pink() + nB * noise.brown();
        if (nHPFon) {
            const float a = juce::jlimit (0.0f, 0.999f, nHPFhz / (nHPFhz + (float) sampleRate));
            noi = noise.highpass (noi, a);
        }

        float dry = mix1v * s1 + mix2v * s2 + mix3v * s3 + subLvl * sub + noi;

        float amp = ampEnv.getNextSample();
        if (lfo1Tg == 2) amp *= juce::jlimit (0.0f, 2.0f, 1.0f + lfo1Dp * 0.5f * lfo1v);
        if (lfo2Tg == 2) amp *= juce::jlimit (0.0f, 2.0f, 1.0f + lfo2Dp * 0.5f * lfo2v);

        const float envF = filtEnv.getNextSample();
        const float modCut = cutoff * std::pow (2.0f, fAmt * (envF - 0.5f));
        filterL.setCutoffFrequency (juce::jlimit (20.0f, 20000.0f, modCut));
        filterR.setCutoffFrequency (juce::jlimit (20.0f, 20000.0f, modCut));
        filterL.setResonance (q); filterR.setResonance (q);

        // simple pan from spread (0..1)
        const float panL = 0.5f - 0.5f * spread;
        const float panR = 0.5f + 0.5f * spread;

        float l = filterL.processSample (0, dry) * amp * panL * gLin;
        float r = filterR.processSample (1, dry) * amp * panR * gLin;

        L[i] += l; R[i] += r;
    }

    // Sum into output
    for (int ch = 0; ch < output.getNumChannels(); ++ch) {
        auto* dst = output.getWritePointer (ch, start);
        auto* src = temp.getReadPointer (jmin (ch, 1));
        for (int i = 0; i < n; ++i) dst[i] += src[i];
    }
}

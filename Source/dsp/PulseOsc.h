/*
    File: PulseOsc.h
    Project: MiniSynth
    Revision: 1.0.0
    Date: 2025-08-19
    Description:
        BLEP-based pulse oscillator with adjustable pulse width and optional
        softened edges to reduce harshness.
*/

#pragma once
#include "JuceIncludes.h"

class PulseOsc {
public:
    void prepare (const juce::dsp::ProcessSpec& spec) { sampleRate = spec.sampleRate; reset(); }
    void reset() { phase = 0.0; incr = 0.0; }
    void forceReset() { phase = 0.0; }

    void setFrequency (double f) {
        freq = juce::jlimit (0.0, sampleRate * 0.45, f);
        incr = freq / sampleRate;
    }
    void setPulseWidth (float pw01) { pw = juce::jlimit (0.01f, 0.99f, pw01); }
    void setRoundedEdges (bool b) { roundedEdges = b; }

    float processSample() {
        phase += incr; if (phase >= 1.0) phase -= 1.0;
        float x = (phase < pw ?  1.0f : -1.0f);
        const double dt = incr;
        x += polyBLEP (phase, dt);
        double t2 = phase - pw; if (t2 < 0.0) t2 += 1.0;
        x -= polyBLEP (t2, dt);
        if (roundedEdges) x = std::tanh (x * 1.5f);
        return x;
    }

private:
    static inline float polyBLEP (double t, double dt) {
        if (t < dt) { double x = t / dt; return (float) (x + x - x * x - 1.0); }
        if (t > 1.0 - dt) { double x = (t - 1.0) / dt; return (float) (x * x + x + x + 1.0); }
        return 0.0f;
    }

    double sampleRate = 44100.0, freq = 0.0, incr = 0.0, phase = 0.0;
    float pw = 0.5f; bool roundedEdges = true;
};

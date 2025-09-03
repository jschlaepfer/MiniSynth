/*
    File: PolyBLEPOsc.h
    Project: MiniSynth
    Revision: 1.0.0
    Date: 2025-08-19
    Description:
        Minimal polyBLEP oscillator for alias-reduced saw up/down. Provides
        phase handling and frequency control.
*/

#pragma once
#include "JuceIncludes.h"

class PolyBLEPOsc {
public:
    enum Mode { SawUp = 0, SawDown = 1 };

    void prepare (const juce::dsp::ProcessSpec& spec) { sampleRate = spec.sampleRate; reset(); }
    void reset() { phase = 0.0; incr = 0.0; }
    void forceReset() { phase = 0.0; }
    double getPhase() const { return phase; }

    void setFrequency (double f) {
        freq = juce::jlimit (0.0, sampleRate * 0.45, f);
        incr = freq / sampleRate;
    }
    void setMode (Mode m) { mode = m; }

    float processSample() {
        phase += incr; if (phase >= 1.0) phase -= 1.0;
        float x = (float) (mode == SawUp ? (2.0 * phase - 1.0) : (1.0 - 2.0 * phase));
        const double dt = incr;
        x -= polyBLEP (phase, dt);
        return x;
    }

private:
    static inline float polyBLEP (double t, double dt) {
        if (t < dt) { double x = t / dt; return (float) (x + x - x * x - 1.0); }
        if (t > 1.0 - dt) { double x = (t - 1.0) / dt; return (float) (x * x + x + x + 1.0); }
        return 0.0f;
    }

    double sampleRate = 44100.0;
    double freq = 0.0, incr = 0.0, phase = 0.0;
    Mode mode = SawUp;
};

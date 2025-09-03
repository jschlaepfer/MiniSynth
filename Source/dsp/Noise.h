/*
    File: Noise.h
    Project: MiniSynth
    Revision: 1.7.1
    Date: 2025-08-19
    Description:
        Simple noise generator bus (white, pink, brown) with a lightweight
        1-pole high-pass helper state.
*/

#pragma once
#include "JuceIncludes.h"

struct NoiseBus {
    juce::Random rng;
    float pinkState = 0.0f;
    float brownState = 0.0f;
    float hpX = 0.0f, hpY = 0.0f; // 1-pole HPF state

    void prepare (const juce::dsp::ProcessSpec&) {}

    inline float white() { return rng.nextFloat() * 2.0f - 1.0f; }
    inline float pink()  { pinkState  = 0.98f * pinkState  + 0.02f * white(); return pinkState; }
    inline float brown() { brownState = juce::jlimit (-1.0f, 1.0f, brownState + 0.02f * white()); return brownState; }

    inline float highpass (float x, float alpha) {
        float y = alpha * (hpY + x - hpX);
        hpX = x; hpY = y; return y;
    }
};

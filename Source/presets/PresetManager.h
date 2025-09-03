/*
    File: PresetManager.h
    Project: MiniSynth
    Revision: 1.0.0
    Date: 2025-08-19
    Description:
        Preset management interface. Lists factory and user presets, applies
        JSON/XML states, and exposes simple CRUD for user presets.
*/

#pragma once
#include "JuceIncludes.h"

namespace presets {

class PresetManager {
public:
    PresetManager (juce::AudioProcessorValueTreeState& s, juce::String org, juce::String app);

    juce::StringArray getAllPresetNames() const; // factory + user
    bool isFactoryIndex (int index) const;
    void applyPresetByIndex (int index);

    bool saveUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);
    juce::File getUserDir() const;

private:
    struct PresetEntry { juce::String name; bool factory = false; juce::String resName; juce::File file; };

    juce::AudioProcessorValueTreeState& apvts;
    juce::String organisation, application;

    juce::OwnedArray<PresetEntry> entries; // filled at construction

    void rebuildList();
    void applyJson (const juce::String& jsonText);
};

} // namespace presets

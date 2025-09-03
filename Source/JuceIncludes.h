/*
    JuceIncludes.h
    MiniSynth Project
    Revision: 1.0.0
    Date: 2025-08-19

    Résumé :
    Ce fichier sert d'en-tête central pour inclure les modules JUCE nécessaires.
    - Avec Projucer : inclut <JuceHeader.h>
    - Avec CMake    : inclut manuellement les modules utilisés.
*/

#pragma once

// Si on compile avec Projucer, "JuceHeader.h" existe
#if __has_include(<JuceHeader.h>)
    #include <JuceHeader.h>
#else
    // Inclusions directes pour un projet CMake
    #include <juce_core/juce_core.h>
    #include <juce_data_structures/juce_data_structures.h>
    #include <juce_events/juce_events.h>

    #include <juce_graphics/juce_graphics.h>
    #include <juce_gui_basics/juce_gui_basics.h>
    #include <juce_gui_extra/juce_gui_extra.h>

    #include <juce_audio_basics/juce_audio_basics.h>
    #include <juce_audio_processors/juce_audio_processors.h>
    #include <juce_audio_formats/juce_audio_formats.h>
    #include <juce_audio_utils/juce_audio_utils.h>
    #include <juce_dsp/juce_dsp.h>
#endif
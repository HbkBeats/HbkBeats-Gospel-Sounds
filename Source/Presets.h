#pragma once
#include <JuceHeader.h>
#include "Params.h"

// A preset lists only what differs from the defaults; everything else is reset first.
// Choice values are the index in the list (see Params.h), drawbars are 0-8.
struct Preset
{
    juce::String name;
    std::vector<std::pair<juce::String, float>> values;
    std::vector<int> drawbars;   // empty = keep default registration
};

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets = {

        { "Init - Gospel Mix", {}, {} },

        { "Sunday Morning Organ",
          { { ID::organ, 1.f }, { ID::keys, 0.f }, { ID::choir, 0.f }, { ID::strings, 0.f },
            { ID::perc, 1 }, { ID::rotor, 1 }, { ID::rotorDepth, 0.55f },
            { ID::revSize, 0.45f }, { ID::revMix, 0.22f }, { ID::ensemble, 0.1f } },
          { 8, 8, 8, 6, 0, 4, 0, 0, 0 } },

        { "Praise Break Shout",
          { { ID::organ, 1.f }, { ID::keys, 0.2f }, { ID::choir, 0.f }, { ID::strings, 0.f },
            { ID::perc, 2 }, { ID::rotor, 2 }, { ID::rotorDepth, 0.75f },
            { ID::tone, 0.9f }, { ID::revMix, 0.15f }, { ID::ensemble, 0.2f } },
          { 8, 8, 8, 8, 8, 6, 4, 3, 8 } },

        { "Hymn Organ",
          { { ID::organ, 1.f }, { ID::keys, 0.f }, { ID::choir, 0.f }, { ID::strings, 0.f },
            { ID::rotor, 1 }, { ID::rotorDepth, 0.3f },
            { ID::revSize, 0.8f }, { ID::revMix, 0.35f }, { ID::tone, 0.6f } },
          { 0, 0, 8, 6, 0, 3, 0, 0, 0 } },

        { "Altar Call Pad",
          { { ID::organ, 0.3f }, { ID::keys, 0.f }, { ID::choir, 0.8f }, { ID::strings, 0.6f },
            { ID::vowel, 0.5f }, { ID::attack, 1.2f }, { ID::release, 2.5f },
            { ID::revSize, 0.85f }, { ID::revMix, 0.4f }, { ID::ensemble, 0.5f }, { ID::tone, 0.6f } },
          { 8, 0, 8, 0, 4, 0, 0, 0, 0 } },

        { "Hallelujah Choir",
          { { ID::organ, 0.15f }, { ID::keys, 0.f }, { ID::choir, 1.f }, { ID::strings, 0.25f },
            { ID::vowel, 0.f }, { ID::choirVib, 0.5f }, { ID::attack, 0.6f }, { ID::release, 1.8f },
            { ID::revSize, 0.7f }, { ID::revMix, 0.35f }, { ID::ensemble, 0.45f } },
          {} },

        { "Worship Strings",
          { { ID::organ, 0.f }, { ID::keys, 0.f }, { ID::choir, 0.2f }, { ID::strings, 1.f },
            { ID::attack, 0.9f }, { ID::release, 2.2f },
            { ID::revSize, 0.75f }, { ID::revMix, 0.35f }, { ID::ensemble, 0.5f } },
          {} },

        { "Church Keys Warm",
          { { ID::organ, 0.35f }, { ID::keys, 1.f }, { ID::choir, 0.f }, { ID::strings, 0.15f },
            { ID::keysDecay, 2.6f }, { ID::keysTone, 0.35f },
            { ID::rotor, 1 }, { ID::rotorDepth, 0.4f },
            { ID::revSize, 0.5f }, { ID::revMix, 0.25f } },
          { 0, 0, 8, 0, 4, 0, 0, 0, 0 } },

        { "Glory Rhodes",
          { { ID::organ, 0.15f }, { ID::keys, 1.f }, { ID::choir, 0.2f }, { ID::strings, 0.f },
            { ID::keysDecay, 3.5f }, { ID::keysTone, 0.75f },
            { ID::revSize, 0.4f }, { ID::revMix, 0.25f }, { ID::ensemble, 0.4f } },
          {} },

        { "Gospel Shuffle Keys",
          { { ID::organ, 0.5f }, { ID::keys, 1.f }, { ID::choir, 0.f }, { ID::strings, 0.f },
            { ID::rhythm, 3 }, { ID::gateDepth, 0.85f }, { ID::gateLen, 0.55f },
            { ID::rotor, 1 }, { ID::ensemble, 0.3f }, { ID::revSize, 0.3f }, { ID::revMix, 0.2f } },
          {} },

        { "Ministry Swell",
          { { ID::organ, 0.6f }, { ID::keys, 0.f }, { ID::choir, 0.6f }, { ID::strings, 0.6f },
            { ID::rhythm, 1 }, { ID::gateDepth, 0.7f }, { ID::attack, 0.5f },
            { ID::revSize, 0.7f }, { ID::revMix, 0.35f } },
          {} },

        { "Shout Stabs",
          { { ID::organ, 1.f }, { ID::keys, 0.4f }, { ID::choir, 0.f }, { ID::strings, 0.f },
            { ID::rhythm, 4 }, { ID::gateDepth, 1.f }, { ID::gateLen, 0.35f },
            { ID::rotor, 2 }, { ID::tone, 0.9f }, { ID::revSize, 0.25f }, { ID::revMix, 0.15f } },
          { 8, 8, 8, 8, 4, 0, 0, 0, 4 } },

        { "One-Finger Worship (C)",
          { { ID::organ, 0.4f }, { ID::keys, 0.5f }, { ID::choir, 0.5f }, { ID::strings, 0.5f },
            { ID::chordMode, 2 }, { ID::key, 0 }, { ID::scale, 0 }, { ID::voicing, 1 }, { ID::bass, 1 },
            { ID::attack, 0.5f }, { ID::release, 1.5f },
            { ID::revSize, 0.7f }, { ID::revMix, 0.35f } },
          {} },

        { "One-Finger Praise Keys (C)",
          { { ID::organ, 0.7f }, { ID::keys, 0.7f }, { ID::choir, 0.f }, { ID::strings, 0.f },
            { ID::chordMode, 2 }, { ID::key, 0 }, { ID::scale, 0 }, { ID::voicing, 1 }, { ID::bass, 1 },
            { ID::perc, 1 }, { ID::rotor, 1 },
            { ID::rhythm, 2 }, { ID::gateDepth, 0.7f }, { ID::gateLen, 0.5f } },
          {} },

        { "Soaking Pad",
          { { ID::organ, 0.f }, { ID::keys, 0.2f }, { ID::choir, 0.7f }, { ID::strings, 0.8f },
            { ID::chordMode, 1 }, { ID::chordType, 3 }, { ID::voicing, 1 }, { ID::bass, 1 },
            { ID::vowel, 0.75f }, { ID::attack, 2.2f }, { ID::release, 3.5f },
            { ID::revSize, 0.95f }, { ID::revMix, 0.5f }, { ID::ensemble, 0.65f }, { ID::tone, 0.5f } },
          {} },
    };
    return presets;
}

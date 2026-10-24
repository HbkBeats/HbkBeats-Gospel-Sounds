#pragma once
#include <JuceHeader.h>

namespace ID
{
    constexpr const char *organ = "organ", *keys = "keys", *choir = "choir", *strings = "strings";
    constexpr const char *perc = "perc", *keysDecay = "keysDecay", *keysTone = "keysTone";
    constexpr const char *vowel = "vowel", *choirVib = "choirVib";
    constexpr const char *attack = "attack", *release = "release";
    constexpr const char *rotor = "rotor", *rotorDepth = "rotorDepth";
    constexpr const char *chordMode = "chordMode", *chordType = "chordType", *key = "key", *scale = "scale";
    constexpr const char *voicing = "voicing", *bass = "bass";
    constexpr const char *rhythm = "rhythm", *gateDepth = "gateDepth", *gateLen = "gateLen";
    constexpr const char *tone = "tone", *ensemble = "ensemble", *revSize = "revSize", *revMix = "revMix";
    constexpr const char *master = "master";

    inline constexpr const char* drawbarIds[9] = { "db1", "db2", "db3", "db4", "db5", "db6", "db7", "db8", "db9" };
    inline constexpr const char* drawbarNames[9] = { "16", "5 1/3", "8", "4", "2 2/3", "2", "1 3/5", "1 1/3", "1" };
    inline constexpr int drawbarDefaults[9] = { 8, 8, 8, 4, 0, 0, 0, 0, 2 };
}

inline const juce::StringArray chordTypeNames { "Maj9", "Maj7", "6/9", "Add9", "Min9", "Min7", "Dom9", "Dom13", "Sus4(9)", "m7b5" };

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto addF = [&](const char* id, const char* name, float lo, float hi, float def, float skew = 1.f, const char* unit = "")
    {
        p.push_back(std::make_unique<AudioParameterFloat>(ParameterID { id, 1 }, name,
                        NormalisableRange<float>(lo, hi, 0.0001f, skew), def,
                        AudioParameterFloatAttributes().withLabel(unit)));
    };
    auto addC = [&](const char* id, const char* name, StringArray choices, int def)
    {
        p.push_back(std::make_unique<AudioParameterChoice>(ParameterID { id, 1 }, name, choices, def));
    };
    auto addB = [&](const char* id, const char* name, bool def)
    {
        p.push_back(std::make_unique<AudioParameterBool>(ParameterID { id, 1 }, name, def));
    };
    auto addI = [&](const char* id, const String& name, int lo, int hi, int def)
    {
        p.push_back(std::make_unique<AudioParameterInt>(ParameterID { id, 1 }, name, lo, hi, def));
    };

    // Layers
    addF(ID::organ,   "Organ Level",   0, 1, 0.80f);
    addF(ID::keys,    "Keys Level",    0, 1, 0.50f);
    addF(ID::choir,   "Choir Level",   0, 1, 0.45f);
    addF(ID::strings, "Strings Level", 0, 1, 0.35f);

    // Organ
    for (int i = 0; i < 9; ++i)
        addI(ID::drawbarIds[i], String("Drawbar ") + ID::drawbarNames[i], 0, 8, ID::drawbarDefaults[i]);
    addC(ID::perc, "Percussion", { "Off", "2nd", "3rd" }, 0);
    addC(ID::rotor, "Rotary", { "Off", "Slow", "Fast", "Mod Wheel" }, 1);
    addF(ID::rotorDepth, "Rotary Depth", 0, 1, 0.5f);

    // Keys / choir / envelope
    addF(ID::keysDecay, "Keys Decay", 0.2f, 6.f, 1.8f, 0.5f, "s");
    addF(ID::keysTone,  "Keys Tone",  0, 1, 0.45f);
    addF(ID::vowel,     "Choir Vowel", 0, 1, 0.f);
    addF(ID::choirVib,  "Choir Vibrato", 0, 1, 0.35f);
    addF(ID::attack,    "Attack",  0.005f, 3.f, 0.35f, 0.4f, "s");
    addF(ID::release,   "Release", 0.02f, 5.f, 0.6f, 0.4f, "s");

    // Gospel chord engine
    addC(ID::chordMode, "Chord Mode", { "Off", "Fixed Chord", "Auto (Key)" }, 0);
    addC(ID::chordType, "Chord Type", chordTypeNames, 0);
    addC(ID::key, "Key", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0);
    addC(ID::scale, "Scale", { "Major", "Minor" }, 0);
    addC(ID::voicing, "Voicing", { "Close", "Open", "Rootless" }, 1);
    addB(ID::bass, "Add Bass", true);

    // Rhythm gate
    addC(ID::rhythm, "Rhythm", { "Off", "Swell", "Pulse 8ths", "Shuffle", "Stabs" }, 0);
    addF(ID::gateDepth, "Rhythm Depth", 0, 1, 0.8f);
    addF(ID::gateLen,   "Rhythm Length", 0.1f, 1, 0.6f);

    // FX
    addF(ID::tone,     "Tone",        0, 1, 0.75f);
    addF(ID::ensemble, "Ensemble",    0, 1, 0.30f);
    addF(ID::revSize,  "Reverb Size", 0, 1, 0.50f);
    addF(ID::revMix,   "Reverb Mix",  0, 1, 0.25f);
    addF(ID::master,   "Master",     -24, 6, -6.f, 1.f, "dB");

    AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& x : p)
        layout.add(std::move(x));
    return layout;
}

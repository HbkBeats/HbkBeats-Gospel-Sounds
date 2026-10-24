#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <array>
#include <map>
#include <vector>

struct ChordSettings
{
    int  mode    = 0;     // 0 off, 1 fixed chord, 2 auto (diatonic to key)
    int  type    = 0;     // index into chordTypeNames
    int  key     = 0;     // 0 = C ... 11 = B
    int  scale   = 0;     // 0 major, 1 minor
    int  voicing = 1;     // 0 close, 1 open (drop-2), 2 rootless
    bool bass    = true;
};

// Turns every incoming note into a gospel chord. Note-offs release exactly what the
// matching note-on produced, and shared notes are reference counted.
class ChordEngine
{
public:
    void reset()
    {
        active.clear();
        noteCount.fill(0);
    }

    void process(const juce::MidiBuffer& in, juce::MidiBuffer& out, const ChordSettings& s)
    {
        for (const auto meta : in)
        {
            const auto m = meta.getMessage();
            const int pos = meta.samplePosition;

            if (m.isNoteOn())
            {
                const int ch = m.getChannel();
                release(ch, m.getNoteNumber(), pos, out);   // retrigger safety

                const auto notes = buildChord(m.getNoteNumber(), s);
                active[keyFor(ch, m.getNoteNumber())] = notes;

                for (size_t i = 0; i < notes.size(); ++i)
                {
                    ++noteCount[(size_t) keyFor(ch, notes[i])];
                    const float taper = 1.0f - 0.04f * (float) i;
                    const float vel = juce::jlimit(0.05f, 1.0f, m.getFloatVelocity() * taper);
                    out.addEvent(juce::MidiMessage::noteOn(ch, notes[i], vel), pos);
                }
            }
            else if (m.isNoteOff())
            {
                if (! release(m.getChannel(), m.getNoteNumber(), pos, out))
                    out.addEvent(m, pos);
            }
            else if (m.isAllNotesOff() || m.isAllSoundOff())
            {
                out.addEvent(m, pos);
                reset();
            }
            else
            {
                out.addEvent(m, pos);
            }
        }
    }

private:
    static int keyFor(int channel, int note) { return (juce::jlimit(1, 16, channel) - 1) * 128 + note; }

    bool release(int ch, int note, int pos, juce::MidiBuffer& out)
    {
        auto it = active.find(keyFor(ch, note));
        if (it == active.end())
            return false;

        for (int nn : it->second)
        {
            auto& c = noteCount[(size_t) keyFor(ch, nn)];
            if (--c <= 0)
            {
                c = 0;
                out.addEvent(juce::MidiMessage::noteOff(ch, nn), pos);
            }
        }
        active.erase(it);
        return true;
    }

    static const std::vector<int>& intervalsFor(int type)
    {
        static const std::vector<std::vector<int>> table = {
            { 0, 4, 7, 11, 14 },   // Maj9
            { 0, 4, 7, 11 },       // Maj7
            { 0, 4, 7, 9, 14 },    // 6/9
            { 0, 4, 7, 14 },       // Add9
            { 0, 3, 7, 10, 14 },   // Min9
            { 0, 3, 7, 10 },       // Min7
            { 0, 4, 7, 10, 14 },   // Dom9
            { 0, 4, 10, 14, 21 },  // Dom13
            { 0, 5, 7, 10, 14 },   // Sus4(9)
            { 0, 3, 6, 10 }        // m7b5
        };
        return table[(size_t) juce::jlimit(0, (int) table.size() - 1, type)];
    }

    // Diatonic chord quality for the pressed note's position in the key.
    static int autoTypeFor(int semis, int scale)
    {
        if (scale == 0)   // major: I ii iii IV V vi vii
        {
            switch (semis)
            {
                case 0:  return 0;   // Maj9
                case 2:  return 4;   // Min9
                case 4:  return 5;   // Min7
                case 5:  return 0;   // Maj9
                case 7:  return 7;   // Dom13
                case 9:  return 4;   // Min9
                case 11: return 9;   // m7b5
                default: return 6;   // chromatic: Dom9
            }
        }
        switch (semis)           // natural minor: i ii III iv v VI VII
        {
            case 0:  return 4;   // Min9
            case 2:  return 9;   // m7b5
            case 3:  return 0;   // Maj9
            case 5:  return 4;   // Min9
            case 7:  return 5;   // Min7
            case 8:  return 0;   // Maj9
            case 10: return 6;   // Dom9
            default: return 6;
        }
    }

    std::vector<int> buildChord(int note, const ChordSettings& s) const
    {
        if (s.mode == 0)
            return { note };

        int type = s.type;
        if (s.mode == 2)
            type = autoTypeFor(((note - s.key) % 12 + 12) % 12, s.scale);

        std::vector<int> n;
        for (int i : intervalsFor(type))
            n.push_back(note + i);

        if (s.voicing == 1 && n.size() >= 4)        // open: drop the second-highest note an octave
            n[n.size() - 2] -= 12;
        else if (s.voicing == 2 && n.size() > 3)    // rootless: let the bass carry the root
            n.erase(n.begin());

        if (s.bass && note - 12 >= 0)
            n.push_back(note - 12);

        std::vector<int> clean;
        for (int v : n)
            if (v >= 0 && v <= 127 && std::find(clean.begin(), clean.end(), v) == clean.end())
                clean.push_back(v);

        if (clean.empty())
            clean.push_back(note);
        return clean;
    }

    std::map<int, std::vector<int>> active;
    std::array<int, 16 * 128> noteCount {};
};

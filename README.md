# HbkBeats Gospel Sounds

A gospel instrument plugin (VST3 + Standalone) built with JUCE 8. Four synthesized layers, a one-finger gospel chord engine and a tempo-synced rhythm swell. It is inspired by the gospel-keyboard plugin idea, but the sounds, engine and look are original.

## What's inside

- **Layers:** Organ, Keys, Choir, Strings, each with its own level.
- **Tonewheel organ:** nine drawbars (3 dB steps), 2nd/3rd percussion, key click, rotary speaker (Off / Slow / Fast / Mod Wheel).
- **Keys:** FM electric piano with decay and tone controls.
- **Choir:** formant "Ah / Oh / Ooh" with vibrato. **Strings:** detuned ensemble.
- **Gospel chords:** play one note, get a full gospel chord.
  - *Fixed Chord* uses the chord type you pick (Maj9, Maj7, 6/9, Add9, Min9, Min7, Dom9, Dom13, Sus4(9), m7b5).
  - *Auto (Key)* picks the right chord for each note in the chosen key and scale (Maj9 on I and IV, Min9 on ii and vi, Dom13 on V, and so on).
  - Voicing: Close, Open (drop-2) or Rootless, plus an optional bass note.
- **Rhythm Swell:** Swell, Pulse 8ths, Shuffle, Stabs. It follows the host tempo (120 BPM when free-running).
- **FX:** tone, ensemble, reverb and master.
- **15 factory presets** in the preset menu.

## Build

You need CMake 3.22+ and a C++17 compiler (Visual Studio 2022 on Windows, Xcode on macOS, GCC with the JUCE dependencies on Linux).

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target HbkBeatsGospelSounds_VST3
```

The first configure downloads JUCE 8.0.4. To use your own JUCE copy, add `-DJUCE_DIR=/path/to/JUCE`.

The plugin appears under `build/HbkBeatsGospelSounds_artefacts/Release/VST3/`. On Windows, copy the `.vst3` folder to `C:\Program Files\Common Files\VST3` and rescan in your DAW. Build the `HbkBeatsGospelSounds_Standalone` target for a stand-alone app with its own on-screen keyboard.

## Files

- `Source/GospelVoice.h`: the four-layer sound engine
- `Source/ChordEngine.h`: one-finger chord logic
- `Source/Presets.h`: factory presets
- `Source/PluginProcessor.*`: parameters, rhythm gate, effects
- `Source/PluginEditor.*` and `GospelLookAndFeel.h`: the interface

JUCE is dual-licensed (AGPLv3 or a commercial licence); check which applies before you sell the plugin.

## Build on GitHub (no local setup)

Push this folder to a GitHub repository. The workflow in `.github/workflows/build.yml` builds Windows and macOS versions automatically. Open the **Actions** tab, click the latest run, and download the zip under **Artifacts**. Inside is a `VST3` folder with your `.vst3` plugin and a `Standalone` folder with the app.

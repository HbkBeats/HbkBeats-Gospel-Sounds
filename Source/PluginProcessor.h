#pragma once
#include <JuceHeader.h>
#include "Params.h"
#include "GospelVoice.h"
#include "ChordEngine.h"
#include "Presets.h"

class GospelProcessor : public juce::AudioProcessor
{
public:
    GospelProcessor();
    ~GospelProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HbkBeats Gospel Sounds"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return (int) factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Always applies the preset (used by the editor's preset menu).
    void loadPreset(int index);

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    float param(const char* id) const { return apvts.getRawParameterValue(id)->load(); }
    void updateVoiceParams(int numSamples, double sr);
    void applyRhythmGate(juce::AudioBuffer<float>& buffer);
    void applyEffects(juce::AudioBuffer<float>& buffer);

    VoiceParams voiceParams;
    juce::Synthesiser synth;
    ChordEngine chord;

    // rotary speaker
    float  rotorHz = 0.8f;
    double rotorPhaseState = 0.0;
    float  modWheel = 0.f;

    // rhythm gate
    double internalPpq = 0.0;
    float  gateGain = 1.f;

    // effects
    juce::dsp::StateVariableTPTFilter<float> toneFilter;
    juce::dsp::Chorus<float> ensemble;
    juce::Reverb reverb;

    double sampleRateHz = 44100.0;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GospelProcessor)
};

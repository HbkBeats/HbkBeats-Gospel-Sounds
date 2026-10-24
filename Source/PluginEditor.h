#pragma once
#include "PluginProcessor.h"
#include "GospelLookAndFeel.h"

class GospelEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GospelEditor(GospelProcessor&);
    ~GospelEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<APVTS::SliderAttachment> att;

        void init(juce::Component& parent, APVTS& state, const char* id, const juce::String& name,
                  const juce::String& suffix = {}, int decimals = 2)
        {
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
            slider.setNumDecimalPlacesToDisplay(decimals);
            slider.setTextValueSuffix(suffix);
            slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                       juce::MathConstants<float>::pi * 2.75f, true);
            label.setText(name, juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centred);
            parent.addAndMakeVisible(slider);
            parent.addAndMakeVisible(label);
            att = std::make_unique<APVTS::SliderAttachment>(state, id, slider);
        }

        void place(juce::Rectangle<int> cell)
        {
            label.setBounds(cell.removeFromTop(16));
            slider.setBounds(cell);
        }
    };

    struct Fader
    {
        juce::Slider slider { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<APVTS::SliderAttachment> att;

        void init(juce::Component& parent, APVTS& state, const char* id, const juce::String& name,
                  juce::Colour cap, bool percent)
        {
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 44, 16);
            slider.setColour(juce::Slider::thumbColourId, cap);
            if (percent)
                slider.textFromValueFunction = [](double v) { return juce::String(juce::roundToInt(v * 100.0)) + "%"; };
            else
                slider.setNumDecimalPlacesToDisplay(0);
            label.setText(name, juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centred);
            parent.addAndMakeVisible(slider);
            parent.addAndMakeVisible(label);
            att = std::make_unique<APVTS::SliderAttachment>(state, id, slider);
        }

        void place(juce::Rectangle<int> col)
        {
            label.setBounds(col.removeFromTop(16));
            slider.setBounds(col);
        }
    };

    struct Choice
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<APVTS::ComboBoxAttachment> att;

        void init(juce::Component& parent, APVTS& state, const char* id, const juce::String& name)
        {
            if (auto* cp = dynamic_cast<juce::AudioParameterChoice*>(state.getParameter(id)))
                box.addItemList(cp->choices, 1);
            label.setText(name, juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centredLeft);
            parent.addAndMakeVisible(box);
            parent.addAndMakeVisible(label);
            att = std::make_unique<APVTS::ComboBoxAttachment>(state, id, box);
        }

        void place(juce::Rectangle<int> r)
        {
            label.setBounds(r.removeFromTop(14));
            box.setBounds(r.removeFromTop(26));
        }
    };

    void timerCallback() override;
    void stepPreset(int delta);

    GospelProcessor& proc;
    GospelLookAndFeel laf;

    // header
    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };

    // layers + organ
    Fader layerFaders[4];
    Fader drawbars[9];
    Choice percChoice, rotorChoice;
    Knob rotorDepthKnob;

    // sound
    Knob keysDecayKnob, keysToneKnob, attackKnob, vowelKnob, vibratoKnob, releaseKnob;

    // chords
    Choice modeChoice, typeChoice, keyChoice, scaleChoice, voicingChoice;
    juce::ToggleButton bassToggle { "Add bass note" };
    std::unique_ptr<APVTS::ButtonAttachment> bassAtt;

    // rhythm
    Choice rhythmChoice;
    Knob gateDepthKnob, gateLenKnob;

    // fx
    Knob toneKnob, ensembleKnob, masterKnob, revSizeKnob, revMixKnob;

    juce::MidiKeyboardComponent keyboard;

    std::vector<std::pair<juce::Rectangle<int>, juce::String>> panels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GospelEditor)
};

#include "PluginEditor.h"

GospelEditor::GospelEditor(GospelProcessor& p)
    : AudioProcessorEditor(&p), proc(p),
      keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&laf);
    auto& s = proc.apvts;

    // ---- header: preset browser ----
    for (const auto& pr : factoryPresets())
        presetBox.addItem(pr.name, presetBox.getNumItems() + 1);
    presetBox.setSelectedItemIndex(proc.getCurrentProgram(), juce::dontSendNotification);
    presetBox.onChange = [this] { proc.loadPreset(presetBox.getSelectedItemIndex()); };
    addAndMakeVisible(presetBox);
    prevButton.onClick = [this] { stepPreset(-1); };
    nextButton.onClick = [this] { stepPreset(+1); };
    addAndMakeVisible(prevButton);
    addAndMakeVisible(nextButton);

    // ---- layers ----
    const char* layerIds[4]   = { ID::organ, ID::keys, ID::choir, ID::strings };
    const char* layerNames[4] = { "Organ", "Keys", "Choir", "Strings" };
    for (int i = 0; i < 4; ++i)
        layerFaders[i].init(*this, s, layerIds[i], layerNames[i], Pal::gold, true);

    // ---- organ drawbars (brown, white, black caps like a real tonewheel organ) ----
    const juce::Colour brown(0xff8a5a3a), white(0xffeee4d0), black(0xff2b2326);
    const juce::Colour caps[9] = { brown, brown, white, white, black, white, black, white, white };
    for (int i = 0; i < 9; ++i)
        drawbars[i].init(*this, s, ID::drawbarIds[i], ID::drawbarNames[i], caps[i], false);

    percChoice.init(*this, s, ID::perc, "Percussion");
    rotorChoice.init(*this, s, ID::rotor, "Rotary speaker");
    rotorDepthKnob.init(*this, s, ID::rotorDepth, "Rotary depth", "", 2);

    // ---- sound ----
    keysDecayKnob.init(*this, s, ID::keysDecay, "Keys decay", " s", 1);
    keysToneKnob.init(*this, s, ID::keysTone, "Keys tone");
    attackKnob.init(*this, s, ID::attack, "Pad attack", " s", 2);
    vowelKnob.init(*this, s, ID::vowel, "Choir vowel");
    vibratoKnob.init(*this, s, ID::choirVib, "Choir vibrato");
    releaseKnob.init(*this, s, ID::release, "Release", " s", 2);

    // ---- gospel chord engine ----
    modeChoice.init(*this, s, ID::chordMode, "One-finger mode");
    typeChoice.init(*this, s, ID::chordType, "Chord (fixed mode)");
    keyChoice.init(*this, s, ID::key, "Key");
    scaleChoice.init(*this, s, ID::scale, "Scale");
    voicingChoice.init(*this, s, ID::voicing, "Voicing");
    addAndMakeVisible(bassToggle);
    bassAtt = std::make_unique<APVTS::ButtonAttachment>(s, ID::bass, bassToggle);

    // ---- rhythm ----
    rhythmChoice.init(*this, s, ID::rhythm, "Pattern");
    gateDepthKnob.init(*this, s, ID::gateDepth, "Depth");
    gateLenKnob.init(*this, s, ID::gateLen, "Length");

    // ---- fx ----
    toneKnob.init(*this, s, ID::tone, "Tone");
    ensembleKnob.init(*this, s, ID::ensemble, "Ensemble");
    masterKnob.init(*this, s, ID::master, "Master", " dB", 1);
    revSizeKnob.init(*this, s, ID::revSize, "Reverb size");
    revMixKnob.init(*this, s, ID::revMix, "Reverb mix");

    // ---- on-screen keyboard (handy in the standalone app) ----
    keyboard.setAvailableRange(36, 96);
    keyboard.setKeyWidth(952.f / 36.f);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, Pal::cream);
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, Pal::ink);
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, Pal::goldDim);
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, Pal::gold.withAlpha(0.7f));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Pal::gold.withAlpha(0.3f));
    addAndMakeVisible(keyboard);

    setSize(980, 680);
    startTimerHz(5);
}

GospelEditor::~GospelEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void GospelEditor::timerCallback()
{
    const int cur = proc.getCurrentProgram();
    if (presetBox.getSelectedItemIndex() != cur)
        presetBox.setSelectedItemIndex(cur, juce::dontSendNotification);
}

void GospelEditor::stepPreset(int delta)
{
    const int n = proc.getNumPrograms();
    const int next = (proc.getCurrentProgram() + delta + n) % n;
    presetBox.setSelectedItemIndex(next, juce::sendNotification);
}

void GospelEditor::paint(juce::Graphics& g)
{
    const int w = getWidth();
    g.fillAll(Pal::bg);

    // warm glow behind the header
    juce::ColourGradient glow(Pal::wine.withAlpha(0.6f), (float) w * 0.5f, 0.f,
                              Pal::bg.withAlpha(0.f), (float) w * 0.5f, 300.f, true);
    g.setGradientFill(glow);
    g.fillRect(0, 0, w, 320);

    // chapel-window arches across the header
    const int arches = 9;
    const float aw = (float) (w - 28) / (float) arches;
    g.setColour(Pal::gold.withAlpha(0.10f));
    for (int i = 0; i < arches; ++i)
        g.drawRoundedRectangle(14.f + aw * (float) i + 3.f, -40.f, aw - 6.f, 104.f, (aw - 6.f) * 0.5f, 1.2f);

    // cross with a little halo
    const float cx = 44.f, cy = 36.f;
    g.setColour(Pal::gold.withAlpha(0.25f));
    g.drawEllipse(cx - 24.f, cy - 24.f, 48.f, 48.f, 1.f);
    g.setColour(Pal::gold);
    g.fillRoundedRectangle(cx - 2.5f, cy - 15.f, 5.f, 30.f, 1.5f);
    g.fillRoundedRectangle(cx - 10.f, cy - 7.f, 20.f, 5.f, 1.5f);

    // title
    g.setColour(Pal::cream);
    g.setFont(gfont(30.f, true));
    g.drawText("HbkBeats", 84, 10, 220, 36, juce::Justification::centredLeft);
    g.setColour(Pal::gold);
    g.setFont(juce::Font(juce::FontOptions(13.f).withStyle("Bold").withKerningFactor(0.3f)));
    g.drawText("GOSPEL SOUNDS", 86, 44, 260, 18, juce::Justification::centredLeft);

    g.setColour(Pal::goldDim.withAlpha(0.6f));
    g.drawLine(14.f, 72.f, (float) w - 14.f, 72.f, 1.f);

    // panels
    for (const auto& pnl : panels)
    {
        auto r = pnl.first.toFloat();
        g.setColour(Pal::panel);
        g.fillRoundedRectangle(r, 10.f);
        g.setColour(Pal::edge);
        g.drawRoundedRectangle(r.reduced(0.5f), 10.f, 1.f);

        g.setColour(Pal::gold);
        g.setFont(juce::Font(juce::FontOptions(12.f).withStyle("Bold").withKerningFactor(0.2f)));
        g.drawText(pnl.second, (int) r.getX() + 14, (int) r.getY() + 7, (int) r.getWidth() - 28, 16,
                   juce::Justification::centredLeft);
        g.setColour(Pal::goldDim.withAlpha(0.35f));
        g.drawLine(r.getX() + 12.f, r.getY() + 26.f, r.getRight() - 12.f, r.getY() + 26.f, 1.f);
    }
}

void GospelEditor::resized()
{
    const int w = getWidth();
    const int M = 14, G = 12;

    // header
    const int px = w - M - 330;
    prevButton.setBounds(px, 20, 30, 30);
    presetBox.setBounds(px + 36, 20, 258, 30);
    nextButton.setBounds(px + 300, 20, 30, 30);

    // panels
    const int rowAY = 84, rowAH = 232;
    const int rowBY = rowAY + rowAH + G, rowBH = 256;

    const juce::Rectangle<int> layersR(M, rowAY, 236, rowAH);
    const juce::Rectangle<int> organR(M + 236 + G, rowAY, w - 2 * M - 236 - G, rowAH);
    const juce::Rectangle<int> soundR(M, rowBY, 252, rowBH);
    const juce::Rectangle<int> chordR(soundR.getRight() + G, rowBY, 236, rowBH);
    const juce::Rectangle<int> rhythmR(chordR.getRight() + G, rowBY, 168, rowBH);
    const juce::Rectangle<int> fxR(rhythmR.getRight() + G, rowBY, w - M - (rhythmR.getRight() + G), rowBH);

    panels.clear();
    panels.push_back({ layersR, "LAYERS" });
    panels.push_back({ organR, "TONEWHEEL ORGAN" });
    panels.push_back({ soundR, "KEYS  /  CHOIR  /  ENVELOPE" });
    panels.push_back({ chordR, "GOSPEL CHORDS" });
    panels.push_back({ rhythmR, "RHYTHM SWELL" });
    panels.push_back({ fxR, "SPACE  &  TONE" });

    auto inner = [](juce::Rectangle<int> r)
    {
        r.removeFromTop(30);
        return r.reduced(10, 4);
    };

    // layers
    {
        auto a = inner(layersR);
        for (auto& f : layerFaders)
            f.place(a.removeFromLeft(a.getWidth() / (4 - (int) (&f - layerFaders))));
    }

    // organ
    {
        auto a = inner(organR);
        auto bars = a.removeFromLeft(9 * 48);
        for (auto& d : drawbars)
            d.place(bars.removeFromLeft(48));

        a.removeFromLeft(20);
        percChoice.place(a.removeFromTop(44));
        a.removeFromTop(10);
        rotorChoice.place(a.removeFromTop(44));
        a.removeFromTop(10);
        rotorDepthKnob.place(a.removeFromTop(80).withSizeKeepingCentre(100, 80));
    }

    // keys / choir / envelope: 3 x 2 knobs
    {
        auto a = inner(soundR);
        auto row1 = a.removeFromTop(a.getHeight() / 2);
        const int cw = row1.getWidth() / 3;
        keysDecayKnob.place(row1.removeFromLeft(cw));
        keysToneKnob.place(row1.removeFromLeft(cw));
        attackKnob.place(row1);
        vowelKnob.place(a.removeFromLeft(cw));
        vibratoKnob.place(a.removeFromLeft(cw));
        releaseKnob.place(a);
    }

    // gospel chords
    {
        auto a = inner(chordR);
        modeChoice.place(a.removeFromTop(46));
        typeChoice.place(a.removeFromTop(46));
        auto ks = a.removeFromTop(46);
        keyChoice.place(ks.removeFromLeft(104));
        ks.removeFromLeft(8);
        scaleChoice.place(ks);
        voicingChoice.place(a.removeFromTop(46));
        bassToggle.setBounds(a.removeFromTop(28));
    }

    // rhythm
    {
        auto a = inner(rhythmR);
        rhythmChoice.place(a.removeFromTop(46));
        a.removeFromTop(10);
        gateDepthKnob.place(a.removeFromLeft(a.getWidth() / 2));
        gateLenKnob.place(a);
    }

    // fx: 3 x 2
    {
        auto a = inner(fxR);
        auto row1 = a.removeFromTop(a.getHeight() / 2);
        const int cw = row1.getWidth() / 3;
        toneKnob.place(row1.removeFromLeft(cw));
        ensembleKnob.place(row1.removeFromLeft(cw));
        masterKnob.place(row1);
        revSizeKnob.place(a.removeFromLeft(cw));
        revMixKnob.place(a.removeFromLeft(cw));
    }

    keyboard.setBounds(M, rowBY + rowBH + G, w - 2 * M, 70);
}

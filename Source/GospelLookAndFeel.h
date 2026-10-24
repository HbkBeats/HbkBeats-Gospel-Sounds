#pragma once
#include <JuceHeader.h>

// Original palette: candle-gold on deep burgundy, like light through a chapel window.
namespace Pal
{
    inline const juce::Colour bg      { 0xff120a0d };
    inline const juce::Colour panel   { 0xff1f1217 };
    inline const juce::Colour panelHi { 0xff2a1a20 };
    inline const juce::Colour edge    { 0xff4b2c30 };
    inline const juce::Colour gold    { 0xffe6bc5c };
    inline const juce::Colour goldDim { 0xff8f7236 };
    inline const juce::Colour cream   { 0xfff5e9cf };
    inline const juce::Colour wine    { 0xff7d2535 };
    inline const juce::Colour ink     { 0xff0a0506 };
}

inline juce::Font gfont(float height, bool bold = false)
{
    auto o = juce::FontOptions(height);
    if (bold)
        o = o.withStyle("Bold");
    return juce::Font(o);
}

class GospelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GospelLookAndFeel()
    {
        setColour(juce::ComboBox::textColourId, Pal::cream);
        setColour(juce::ComboBox::arrowColourId, Pal::gold);
        setColour(juce::PopupMenu::backgroundColourId, Pal::panel);
        setColour(juce::PopupMenu::textColourId, Pal::cream);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, Pal::wine);
        setColour(juce::PopupMenu::highlightedTextColourId, Pal::cream);
        setColour(juce::TextButton::buttonColourId, Pal::panelHi);
        setColour(juce::TextButton::textColourOffId, Pal::gold);
        setColour(juce::TextButton::textColourOnId, Pal::cream);
        setColour(juce::Slider::textBoxTextColourId, Pal::cream);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::textColourId, Pal::cream);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override { return gfont(13.f); }

    juce::Font getTextButtonFont(juce::TextButton&, int) override { return gfont(15.f, true); }

    juce::Font getLabelFont(juce::Label&) override { return gfont(11.5f); }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, juce::Slider&) override
    {
        auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(4.f);
        const float d = juce::jmin(b.getWidth(), b.getHeight());
        const auto c = b.getCentre();
        const float r = d * 0.5f;
        const float ang = startAngle + pos * (endAngle - startAngle);

        juce::Path track;
        track.addCentredArc(c.x, c.y, r - 3.f, r - 3.f, 0.f, startAngle, endAngle, true);
        g.setColour(Pal::ink);
        g.strokePath(track, juce::PathStrokeType(4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path val;
        val.addCentredArc(c.x, c.y, r - 3.f, r - 3.f, 0.f, startAngle, ang, true);
        g.setColour(Pal::gold);
        g.strokePath(val, juce::PathStrokeType(4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float kr = r - 10.f;
        juce::ColourGradient grad(juce::Colour(0xff55403a), c.x - kr * 0.5f, c.y - kr * 0.7f,
                                  juce::Colour(0xff160d0e), c.x + kr * 0.6f, c.y + kr, false);
        g.setGradientFill(grad);
        g.fillEllipse(c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);
        g.setColour(Pal::goldDim.withAlpha(0.8f));
        g.drawEllipse(c.x - kr, c.y - kr, kr * 2.f, kr * 2.f, 1.2f);

        juce::Path p;
        p.addRoundedRectangle(-1.6f, -kr + 2.f, 3.2f, kr * 0.5f, 1.5f);
        g.setColour(Pal::cream);
        g.fillPath(p, juce::AffineTransform::rotation(ang).translated(c.x, c.y));
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                          float minPos, float maxPos, juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        if (style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, sliderPos, minPos, maxPos, style, s);
            return;
        }

        const float capH = 20.f;
        const float cx = (float) x + (float) w * 0.5f;
        const float top = (float) y + capH * 0.5f;
        const float bottom = (float) (y + h) - capH * 0.5f;
        const float cy = juce::jlimit(top, bottom, sliderPos);

        juce::Rectangle<float> groove(cx - 3.f, top, 6.f, bottom - top);
        g.setColour(Pal::ink);
        g.fillRoundedRectangle(groove, 3.f);

        // lit portion below the cap
        g.setColour(Pal::gold.withAlpha(0.55f));
        g.fillRoundedRectangle(groove.withTop(cy), 3.f);

        const float capW = juce::jmin((float) w - 6.f, 34.f);
        juce::Rectangle<float> cap(cx - capW * 0.5f, cy - capH * 0.5f, capW, capH);
        g.setColour(s.findColour(juce::Slider::thumbColourId));
        g.fillRoundedRectangle(cap, 3.f);
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.drawRoundedRectangle(cap, 3.f, 1.f);
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.drawLine(cap.getX() + 4.f, cap.getCentreY(), cap.getRight() - 4.f, cap.getCentreY(), 1.5f);
    }

    int getSliderThumbRadius(juce::Slider&) override { return 10; }

    void drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float>(0.f, 0.f, (float) w, (float) h).reduced(0.5f);
        g.setColour(Pal::ink);
        g.fillRoundedRectangle(r, 5.f);
        g.setColour(box.hasKeyboardFocus(true) ? Pal::gold : Pal::goldDim.withAlpha(0.7f));
        g.drawRoundedRectangle(r, 5.f, 1.f);

        juce::Path arrow;
        const float ax = (float) w - 14.f, ay = (float) h * 0.5f;
        arrow.addTriangle(ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
        g.setColour(Pal::gold);
        g.fillPath(arrow);
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds(8, 1, box.getWidth() - 28, box.getHeight() - 2);
        label.setFont(getComboBoxFont(box));
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool, bool) override
    {
        auto r = b.getLocalBounds().toFloat();
        const float ph = 18.f, pw = 36.f;
        juce::Rectangle<float> pill(r.getX() + 2.f, r.getCentreY() - ph * 0.5f, pw, ph);

        g.setColour(Pal::ink);
        g.fillRoundedRectangle(pill, ph * 0.5f);
        g.setColour(Pal::goldDim);
        g.drawRoundedRectangle(pill, ph * 0.5f, 1.f);

        const float dot = ph - 6.f;
        const float dx = b.getToggleState() ? pill.getRight() - dot - 3.f : pill.getX() + 3.f;
        g.setColour(b.getToggleState() ? Pal::gold : Pal::edge);
        g.fillEllipse(dx, pill.getCentreY() - dot * 0.5f, dot, dot);

        g.setColour(Pal::cream);
        g.setFont(gfont(13.f));
        g.drawText(b.getButtonText(), pill.getRight() + 10.f, r.getY(), r.getWidth() - pw - 14.f, r.getHeight(),
                   juce::Justification::centredLeft);
    }
};

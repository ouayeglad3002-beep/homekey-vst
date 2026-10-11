#include "Editor.h"

static juce::Font ocFont (float s, bool b = false) { return juce::Font (juce::FontOptions (s, b ? juce::Font::bold : juce::Font::plain)); }

static juce::Path chamfer (juce::Rectangle<float> r, float c)
{
    c = juce::jmin (c, r.getWidth() * 0.5f, r.getHeight() * 0.5f);
    juce::Path p;
    p.startNewSubPath (r.getX() + c, r.getY());
    p.lineTo (r.getRight() - c, r.getY());  p.lineTo (r.getRight(), r.getY() + c);
    p.lineTo (r.getRight(), r.getBottom() - c); p.lineTo (r.getRight() - c, r.getBottom());
    p.lineTo (r.getX() + c, r.getBottom()); p.lineTo (r.getX(), r.getBottom() - c);
    p.lineTo (r.getX(), r.getY() + c);
    p.closeSubPath();
    return p;
}

static juce::Path gem (juce::Rectangle<float> r)
{
    juce::Path p; const float cx = r.getCentreX(), w = r.getWidth() * 0.5f;
    p.startNewSubPath (cx, r.getY());
    p.lineTo (cx + w, r.getY() + r.getHeight() * 0.28f); p.lineTo (cx + w, r.getY() + r.getHeight() * 0.72f);
    p.lineTo (cx, r.getBottom()); p.lineTo (cx - w, r.getY() + r.getHeight() * 0.72f); p.lineTo (cx - w, r.getY() + r.getHeight() * 0.28f);
    p.closeSubPath();
    return p;
}

static void metalPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour acc)
{
    auto path = chamfer (r, 12.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a232c), r.getX(), r.getY(), juce::Colour (0xff080b0e), r.getX(), r.getBottom(), false));
    g.fillPath (path);
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (path);
        for (float y = r.getY() + 1; y < r.getBottom(); y += 2.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (((int) y % 6 == 0) ? 0.02f : 0.008f));
            g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
        }
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4d5c6a), r.getX(), r.getY(), acc.withAlpha (0.45f), r.getRight(), r.getBottom(), false));
    g.strokePath (path, juce::PathStrokeType (1.2f));
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawLine (r.getX() + 14, r.getY() + 1.2f, r.getRight() - 14, r.getY() + 1.2f, 1.0f);
}

//==============================================================================
OneLookAndFeel::OneLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffdfeaf2));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff2a3642));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff06090c));
    setColour (juce::Label::textColourId, juce::Colour (0xffdfeaf2));
}

juce::Label* OneLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (ocFont (13.0f, true));
    return l;
}

void OneLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);

    for (int i = 0; i <= 30; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 30.0f;
        const bool lit = (float) i / 30.0f <= pos + 0.001f;
        g.setColour (lit ? accent : juce::Colour (0xff2a3642));
        const float len = (i % 5 == 0) ? 6.0f : 3.0f;
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (r - 1, a), c.getPointOnCircumference (r - 1 - len, a)), 1.5f);
    }
    const float ar = r - 11.0f;
    juce::Path arc; arc.addCentredArc (c.x, c.y, ar, ar, 0, a0, ang, true);
    g.setColour (accent.withAlpha (0.22f));
    g.strokePath (arc, juce::PathStrokeType (12.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float br = r - 18.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff050709), c.x, c.y + br, juce::Colour (0xff3a4652), c.x, c.y - br, false));
    g.fillEllipse (c.x - br - 2, c.y - br - 2, br * 2 + 4, br * 2 + 4);
    juce::ColourGradient chrome (juce::Colour (0xfff2f6f9), c.x - br * 0.6f, c.y - br * 0.8f, juce::Colour (0xff0b0f13), c.x + br * 0.5f, c.y + br, true);
    chrome.addColour (0.25, juce::Colour (0xff9aa8b5));
    chrome.addColour (0.55, juce::Colour (0xff2c3640));
    g.setGradientFill (chrome);
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    for (int k = 0; k < 10; ++k)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) k / 10.0f;
        juce::Path tri;
        tri.addTriangle (c, c.getPointOnCircumference (br, a), c.getPointOnCircumference (br, a + juce::MathConstants<float>::twoPi / 10.0f));
        g.setColour (juce::Colours::white.withAlpha (k % 2 == 0 ? 0.07f : 0.02f));
        g.fillPath (tri);
    }
    const float ir = br * 0.6f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1e2730), c.x, c.y - ir, juce::Colour (0xff06080b), c.x, c.y + ir, false));
    g.fillEllipse (c.x - ir, c.y - ir, ir * 2, ir * 2);
    const auto tip = c.getPointOnCircumference (br * 0.95f, ang), base = c.getPointOnCircumference (br * 0.2f, ang);
    juce::Path shard;
    shard.startNewSubPath (tip);
    shard.lineTo (c.getPointOnCircumference (br * 0.55f, ang + 0.2f));
    shard.lineTo (base);
    shard.lineTo (c.getPointOnCircumference (br * 0.55f, ang - 0.2f));
    shard.closeSubPath();
    g.setColour (accent.withAlpha (0.4f));
    g.strokePath (shard, juce::PathStrokeType (5.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, tip.x, tip.y, accent, base.x, base.y, false));
    g.fillPath (shard);
}

//==============================================================================
// VUMETRE A AIGUILLE (reduction de gain)
//==============================================================================
void GrMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto path = chamfer (r, 10.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b1015), r.getCentreX(), r.getBottom(), juce::Colour (0xff020304), r.getCentreX(), r.getY(), false));
    g.fillPath (path);

    const bool up = getStyle ((int) proc.apvts.getRawParameterValue ("style")->load()).upward;
    const auto pivot = juce::Point<float> (r.getCentreX(), r.getBottom() - 14.0f);
    const float rad = juce::jmin (r.getWidth() * 0.45f, r.getHeight() - 30.0f);
    const float aL = -1.15f, aR = 1.15f;
    // echelle : 0 a -20 dB (ou 0 a +16 dB pour BLOOM)
    const float range = up ? 16.0f : 20.0f;
    auto angFor = [&] (float db) { return aR - (aR - aL) * juce::jlimit (0.0f, 1.0f, std::abs (db) / range); };

    juce::Path scale; scale.addCentredArc (pivot.x, pivot.y, rad, rad, 0, aL, aR, true);
    g.setColour (juce::Colour (0xff2a3642));
    g.strokePath (scale, juce::PathStrokeType (2.0f));
    const int marks[] = { 0, 2, 4, 6, 8, 10, 14, 20 };
    for (int m : marks)
    {
        if (m > range) continue;
        const float a = angFor ((float) m);
        const auto p1 = pivot.getPointOnCircumference (rad, a), p2 = pivot.getPointOnCircumference (rad - 9, a);
        g.setColour (juce::Colour (0xff8d9aa7));
        g.drawLine ({ p1, p2 }, 1.5f);
        g.setFont (ocFont (10.0f, true));
        const auto tp = pivot.getPointOnCircumference (rad + 12, a);
        g.drawText (juce::String (up ? "+" : (m == 0 ? "" : "-")) + juce::String (m), juce::Rectangle<float> (tp.x - 14, tp.y - 7, 28, 14), juce::Justification::centred);
    }
    // zone lumineuse = reduction actuelle
    juce::Path lit; lit.addCentredArc (pivot.x, pivot.y, rad - 4, rad - 4, 0, angFor (shown), aR, true);
    g.setColour (accent.withAlpha (0.3f));
    g.strokePath (lit, juce::PathStrokeType (8.0f));

    const float a = angFor (shown);
    const auto tip = pivot.getPointOnCircumference (rad - 6, a);
    g.setColour (accent.withAlpha (0.35f));
    g.drawLine ({ pivot, tip }, 6.0f);
    g.setColour (juce::Colours::white);
    g.drawLine ({ pivot, tip }, 2.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, pivot.x - 8, pivot.y - 8, accent, pivot.x + 8, pivot.y + 8, false));
    g.fillPath (gem ({ pivot.x - 8, pivot.y - 12, 16, 24 }));

    g.setColour (juce::Colour (0xffdfeaf2));
    g.setFont (ocFont (22.0f, true));
    g.drawText ((shown > 0.05f ? "+" : "") + juce::String (shown, 1) + " dB",
                juce::Rectangle<float> (pivot.x - 70, pivot.y - rad * 0.55f - 14, 140, 28), juce::Justification::centred);
    g.setColour (juce::Colour (0xff6c7d8b));
    g.setFont (ocFont (10.5f, true));
    g.drawText (up ? "REMONTEE" : "COMPRESSION", r.reduced (10, 6), juce::Justification::topLeft);
    g.setColour (juce::Colour (0xff3a4652));
    g.strokePath (path, juce::PathStrokeType (1.2f));
}

//==============================================================================
// LES 5 STYLES (presets)
//==============================================================================
juce::Rectangle<float> StyleBar::cell (int i) const
{
    const float gap = 8.0f, w = ((float) getWidth() - gap * 4) / 5.0f;
    return { (float) i * (w + gap), 0.0f, w, (float) getHeight() };
}

void StyleBar::paint (juce::Graphics& g)
{
    const int cur = (int) proc.apvts.getRawParameterValue ("style")->load();
    for (int i = 0; i < numStyles; ++i)
    {
        auto r = cell (i).reduced (1.0f);
        auto path = chamfer (r, 9.0f);
        const auto col = OC::styleColour (i);
        const bool on = (i == cur);
        g.setGradientFill (juce::ColourGradient (on ? col.withAlpha (0.35f) : juce::Colour (0xff141b22), r.getX(), r.getY(),
                                                 juce::Colour (0xff05070a), r.getX(), r.getBottom(), false));
        g.fillPath (path);
        if (on)
        {
            g.setColour (col.withAlpha (0.25f));
            g.strokePath (chamfer (r.expanded (2.0f), 10.0f), juce::PathStrokeType (3.0f));
        }
        g.setColour (on ? col : (i == hover ? col.withAlpha (0.6f) : juce::Colour (0xff3a4652)));
        g.strokePath (path, juce::PathStrokeType (on ? 1.8f : 1.0f));
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawLine (r.getX() + 9, r.getY() + 1.2f, r.getRight() - 9, r.getY() + 1.2f, 1.0f);

        auto gr = juce::Rectangle<float> (r.getCentreX() - 7, r.getY() + 9, 14, 22);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (on ? 1.0f : 0.5f), gr.getX(), gr.getY(),
                                                 col.withAlpha (on ? 0.9f : 0.35f), gr.getRight(), gr.getBottom(), false));
        g.fillPath (gem (gr));
        g.setColour (on ? juce::Colours::white : juce::Colour (0xffdfeaf2).withAlpha (0.8f));
        g.setFont (ocFont (15.0f, true));
        g.drawText (getStyle (i).name, r.withTrimmedTop (34).withHeight (20), juce::Justification::centred);
        g.setColour (juce::Colour (0xff6c7d8b));
        g.setFont (ocFont (10.0f));
        g.drawFittedText (getStyle (i).hint, r.withTrimmedTop (54).reduced (6, 2).toNearestInt(), juce::Justification::centredTop, 2);
    }
}

void StyleBar::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < numStyles; ++i)
        if (cell (i).contains (e.position))
            proc.loadStylePreset (i);
    repaint();
}

void StyleBar::mouseMove (const juce::MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < numStyles; ++i) if (cell (i).contains (e.position)) h = i;
    if (h != hover) { hover = h; repaint(); }
}

//==============================================================================
OneCompressEditor::OneCompressEditor (OneCompressProcessor& p)
    : AudioProcessorEditor (&p), proc (p), meter (p), styleBar (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (meter);
    addAndMakeVisible (styleBar);
    const char* ids[3] = { "squeeze", "color", "mix" };
    const char* names[3] = { "SQUEEZE", "COLOR", "MIX" };
    const char* tips[3] = { "Quantite de compression (le volume se compense tout seul)",
                            "Couleur : saturation et caractere du style",
                            "Melange avec le son d'origine (compression parallele)" };
    for (int i = 0; i < 3; ++i)
    {
        auto& k = knobs[(size_t) i];
        k.s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
        k.s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        k.s.setTooltip (tips[i]);
        addAndMakeVisible (k.s);
        k.a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, ids[i], k.s);
        k.s.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
        k.s.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue() / 100.0; };
        k.s.updateText();
        k.l.setText (names[i], juce::dontSendNotification);
        k.l.setJustificationType (juce::Justification::centred);
        k.l.setFont (ocFont (14.0f, true));
        k.l.setColour (juce::Label::textColourId, juce::Colour (0xff8d9aa7));
        addAndMakeVisible (k.l);
    }
    setSize (720, 470);
    timerCallback();
    startTimerHz (30);
}

OneCompressEditor::~OneCompressEditor() { stopTimer(); setLookAndFeel (nullptr); }

void OneCompressEditor::timerCallback()
{
    const int s = (int) proc.apvts.getRawParameterValue ("style")->load();
    if (s != lastStyle)
    {
        lastStyle = s;
        lnf.accent = meter.accent = OC::styleColour (s);
        for (auto& k : knobs) k.s.repaint();
        repaint();
    }
    const float target = proc.grDb.load();
    meter.shown += 0.35f * (target - meter.shown);
    inShown  += 0.3f * (proc.inDb.load()  - inShown);
    outShown += 0.3f * (proc.outDb.load() - outShown);
    meter.repaint();
    styleBar.repaint();
    repaint (getLocalBounds().removeFromRight (40));
    repaint (getLocalBounds().removeFromLeft (40));
}

void OneCompressEditor::paint (juce::Graphics& g)
{
    const auto acc = OC::styleColour (lastStyle < 0 ? 0 : lastStyle);
    auto r = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b1118), r.getCentreX(), r.getHeight() * 0.3f, juce::Colour (0xff030508), 0, r.getHeight(), true));
    g.fillAll();
    g.setGradientFill (juce::ColourGradient (acc.withAlpha (0.12f), r.getCentreX(), 20, juce::Colours::transparentBlack, r.getCentreX(), 300, true));
    g.fillRect (r);
    g.setColour (acc.withAlpha (0.03f));
    for (float x = -r.getHeight(); x < r.getWidth(); x += 40.0f)
    {
        g.drawLine (x, 0, x + r.getHeight(), r.getHeight(), 0.7f);
        g.drawLine (x + r.getHeight(), 0, x, r.getHeight(), 0.7f);
    }

    // logo chrome
    const juce::String logo = "ONE COMPRESS";
    auto la = juce::Rectangle<int> (52, 14, 400, 40);
    g.setFont (ocFont (30.0f, true));
    for (int k = 3; k >= 1; --k) { g.setColour (acc.withAlpha (0.06f * k)); g.drawText (logo, la.expanded (k), juce::Justification::centredLeft); }
    juce::ColourGradient chrome (juce::Colours::white, 0, 20, juce::Colour (0xff5d6a77), 0, 50, false);
    chrome.addColour (0.48, juce::Colour (0xffc7d2dc)); chrome.addColour (0.52, juce::Colour (0xff6f7c88)); chrome.addColour (0.8, juce::Colour (0xffe9f0f5));
    g.setGradientFill (chrome);
    g.drawText (logo, la, juce::Justification::centredLeft);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, 28, 18, acc, 42, 48, false));
    g.fillPath (gem ({ 26, 18, 16, 32 }));
    g.setColour (juce::Colour (0xff6c7d8b));
    g.setFont (ocFont (11.0f));
    g.drawText ("by HomeKey Audio  //  3 boutons, 5 caracteres", 54, 50, 400, 14, juce::Justification::centredLeft);

    metalPanel (g, { 20, 80, r.getWidth() - 40, 108 }, acc);
    metalPanel (g, { 20, 198, r.getWidth() - 40, 252 }, acc);

    // vumetres entree / sortie
    auto bar = [&] (float x, float db, const char* label)
    {
        auto b = juce::Rectangle<float> (x, 222, 12, 196);
        g.setColour (juce::Colour (0xff070a0d)); g.fillRect (b);
        const float v = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, b.getX(), b.getY(), acc, b.getX(), b.getBottom(), false));
        g.fillRect (b.withTrimmedTop (b.getHeight() * (1.0f - v)));
        g.setColour (juce::Colour (0xff3a4652)); g.drawRect (b, 1.0f);
        g.setColour (juce::Colour (0xff6c7d8b)); g.setFont (ocFont (9.5f, true));
        g.drawText (label, juce::Rectangle<float> (x - 10, 422, 32, 14), juce::Justification::centred);
    };
    bar (34, inShown, "IN");
    bar (r.getWidth() - 46, outShown, "OUT");
}

void OneCompressEditor::resized()
{
    styleBar.setBounds (30, 88, getWidth() - 60, 92);
    meter.setBounds (60, 212, 250, 222);
    auto area = juce::Rectangle<int> (320, 214, getWidth() - 380, 220);
    const int w = area.getWidth() / 3;
    for (int i = 0; i < 3; ++i)
    {
        auto c = area.removeFromLeft (w);
        knobs[(size_t) i].l.setBounds (c.removeFromTop (22));
        knobs[(size_t) i].s.setBounds (c.reduced (4, 6));
    }
}

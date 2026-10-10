#include "PluginEditor.h"
#include "SoundDesigner.h"

using namespace AlienColours;

static juce::Font alienFont (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Path chamfer (juce::Rectangle<float> r, float c)
{
    c = juce::jmin (c, r.getWidth() * 0.5f, r.getHeight() * 0.5f);
    juce::Path p;
    p.startNewSubPath (r.getX() + c, r.getY());
    p.lineTo (r.getRight() - c, r.getY());
    p.lineTo (r.getRight(), r.getY() + c);
    p.lineTo (r.getRight(), r.getBottom() - c);
    p.lineTo (r.getRight() - c, r.getBottom());
    p.lineTo (r.getX() + c, r.getBottom());
    p.lineTo (r.getX(), r.getBottom() - c);
    p.lineTo (r.getX(), r.getY() + c);
    p.closeSubPath();
    return p;
}

// etoile cristal a 4 branches
static juce::Path sparkle (juce::Point<float> c, float r, float inner = 0.22f)
{
    juce::Path p;
    for (int k = 0; k < 8; ++k)
    {
        const float a = juce::MathConstants<float>::pi * 0.25f * (float) k;
        const float rr = (k % 2 == 0) ? r : r * inner;
        const auto pt = c.getPointOnCircumference (rr, a);
        if (k == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    p.closeSubPath();
    return p;
}

// gemme hexagonale allongee
static juce::Path gem (juce::Rectangle<float> r)
{
    juce::Path p;
    const float cx = r.getCentreX(), w = r.getWidth() * 0.5f;
    p.startNewSubPath (cx, r.getY());
    p.lineTo (cx + w, r.getY() + r.getHeight() * 0.28f);
    p.lineTo (cx + w, r.getY() + r.getHeight() * 0.72f);
    p.lineTo (cx, r.getBottom());
    p.lineTo (cx - w, r.getY() + r.getHeight() * 0.72f);
    p.lineTo (cx - w, r.getY() + r.getHeight() * 0.28f);
    p.closeSubPath();
    return p;
}

static void metalPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour acc, const juce::String& title = {}, const juce::String& sub = {})
{
    auto path = chamfer (r, 14.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff18212a), r.getX(), r.getY(), juce::Colour (0xff090d11), r.getX(), r.getBottom(), false));
    g.fillPath (path);
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (path);
        for (float y = r.getY() + 1.0f; y < r.getBottom(); y += 2.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (((int) y % 6 == 0) ? 0.018f : 0.008f));
            g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
        }
        g.setGradientFill (juce::ColourGradient (acc.withAlpha (0.08f), r.getX(), r.getY(), juce::Colours::transparentBlack,
                                                 r.getX() + r.getWidth() * 0.5f, r.getY() + 80, false));
        g.fillRect (r);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4d5c6a), r.getX(), r.getY(), acc.withAlpha (0.4f), r.getRight(), r.getBottom(), false));
    g.strokePath (path, juce::PathStrokeType (1.2f));
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawLine (r.getX() + 16, r.getY() + 1.2f, r.getRight() - 16, r.getY() + 1.2f, 1.0f);
    for (auto pt : { juce::Point<float> (r.getX() + 7, r.getY() + 7), juce::Point<float> (r.getRight() - 7, r.getY() + 7),
                     juce::Point<float> (r.getX() + 7, r.getBottom() - 7), juce::Point<float> (r.getRight() - 7, r.getBottom() - 7) })
    {
        g.setColour (acc.withAlpha (0.6f));
        g.fillPath (sparkle (pt, 3.2f, 0.3f));
    }
    if (title.isNotEmpty())
    {
        const auto txt = "<> " + title;
        g.setFont (alienFont (10.5f, true));
        g.setColour (acc.withAlpha (0.95f));
        g.drawText (txt, (int) r.getX() + 18, (int) r.getY() + 6, 400, 14, juce::Justification::left);
        if (sub.isNotEmpty())
        {
            const int tw = (int) juce::GlyphArrangement::getStringWidth (alienFont (10.5f, true), txt);
            g.setColour (dim);
            g.setFont (alienFont (11.0f));
            g.drawText (sub, (int) r.getX() + 32 + tw, (int) r.getY() + 6, 700, 14, juce::Justification::left);
        }
    }
}

//==============================================================================
// LOOK AND FEEL : chrome + cristal
//==============================================================================
AlienLookAndFeel::AlienLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, edge);
    setColour (juce::ComboBox::arrowColourId, acid);
    setColour (juce::PopupMenu::backgroundColourId, bg1);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, acid.withAlpha (0.18f));
    setColour (juce::PopupMenu::highlightedTextColourId, acid);
    setColour (juce::PopupMenu::headerTextColourId, steel);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::Label::textColourId, text);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff2a3642));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff06090c));
    setColour (juce::Slider::textBoxHighlightColourId, juce::Colour (0x553ff6ff));
    setColour (juce::AlertWindow::backgroundColourId, bg1);
    setColour (juce::AlertWindow::textColourId, text);
    setColour (juce::AlertWindow::outlineColourId, acid);
    setColour (juce::TextEditor::backgroundColourId, bg0);
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::outlineColourId, edge);
    setColour (juce::TextEditor::focusedOutlineColourId, acid);
    setColour (juce::CaretComponent::caretColourId, acid);
}

juce::Label* AlienLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (alienFont (11.5f, true));
    return l;
}

void AlienLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float a0, float a1, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = a0 + pos * (a1 - a0);

    // graduations
    for (int i = 0; i <= 20; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 20.0f;
        const bool lit = (float) i / 20.0f <= pos + 0.001f;
        g.setColour (lit ? accent.withAlpha (0.9f) : juce::Colour (0xff2a3642));
        const float len = (i % 5 == 0) ? 4.0f : 2.0f;
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (r - 0.5f, a), c.getPointOnCircumference (r - 0.5f - len, a)), 1.2f);
    }

    // anneau lumineux
    const float ar = r - 7.0f;
    juce::Path track; track.addCentredArc (c.x, c.y, ar, ar, 0, a0, a1, true);
    g.setColour (juce::Colour (0xff141b22));
    g.strokePath (track, juce::PathStrokeType (3.0f));
    juce::Path arc; arc.addCentredArc (c.x, c.y, ar, ar, 0, a0, angle, true);
    g.setColour (accent.withAlpha (0.22f));
    g.strokePath (arc, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // corps chrome
    const float br = r - 11.0f;
    if (br < 4.0f) return;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff050709), c.x, c.y + br, juce::Colour (0xff3a4652), c.x, c.y - br, false));
    g.fillEllipse (c.x - br - 1.5f, c.y - br - 1.5f, br * 2 + 3, br * 2 + 3);
    juce::ColourGradient chrome (juce::Colour (0xffeef3f7), c.x - br * 0.6f, c.y - br * 0.8f, juce::Colour (0xff0b0f13), c.x + br * 0.5f, c.y + br, true);
    chrome.addColour (0.25, juce::Colour (0xff9aa8b5));
    chrome.addColour (0.55, juce::Colour (0xff2c3640));
    g.setGradientFill (chrome);
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    for (int k = 0; k < 8; ++k)   // facettes cristal
    {
        const float a = juce::MathConstants<float>::twoPi * (float) k / 8.0f + 0.2f;
        g.setColour (juce::Colours::white.withAlpha (k % 2 == 0 ? 0.07f : 0.02f));
        juce::Path tri;
        tri.addTriangle (c, c.getPointOnCircumference (br, a), c.getPointOnCircumference (br, a + juce::MathConstants<float>::twoPi / 8.0f));
        g.fillPath (tri);
    }
    const float ir = br * 0.62f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1e2730), c.x, c.y - ir, juce::Colour (0xff07090c), c.x, c.y + ir, false));
    g.fillEllipse (c.x - ir, c.y - ir, ir * 2, ir * 2);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawEllipse (c.x - ir, c.y - ir, ir * 2, ir * 2, 0.8f);

    // index : eclat de cristal
    const auto tip = c.getPointOnCircumference (br * 0.95f, angle);
    const auto base = c.getPointOnCircumference (br * 0.25f, angle);
    juce::Path shard;
    shard.startNewSubPath (tip);
    shard.lineTo (c.getPointOnCircumference (br * 0.55f, angle + 0.24f));
    shard.lineTo (base);
    shard.lineTo (c.getPointOnCircumference (br * 0.55f, angle - 0.24f));
    shard.closeSubPath();
    g.setColour (accent.withAlpha (0.35f));
    g.strokePath (shard, juce::PathStrokeType (4.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, tip.x, tip.y, accent, base.x, base.y, false));
    g.fillPath (shard);
}

void AlienLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    auto path = chamfer (r, 6.0f);
    const bool on = b.getToggleState();
    if (on || down)
        g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.55f), r.getX(), r.getY(), accent.withAlpha (0.12f), r.getX(), r.getBottom(), false));
    else
        g.setGradientFill (juce::ColourGradient (over ? juce::Colour (0xff26323d) : juce::Colour (0xff1a222a), r.getX(), r.getY(),
                                                 juce::Colour (0xff0a0e12), r.getX(), r.getBottom(), false));
    g.fillPath (path);
    g.setColour (juce::Colours::white.withAlpha (0.13f));
    g.drawLine (r.getX() + 6, r.getY() + 1, r.getRight() - 6, r.getY() + 1, 1.0f);
    g.setColour (over || on ? accent : juce::Colour (0xff3a4652));
    g.strokePath (path, juce::PathStrokeType (1.1f));
}

void AlienLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    auto path = chamfer (r, 7.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0d1318), 0, 0, juce::Colour (0xff040608), 0, (float) h, false));
    g.fillPath (path);
    g.setColour (accent.withAlpha (0.7f));
    g.strokePath (path, juce::PathStrokeType (1.2f));
    g.fillPath (gem ({ (float) w - 20.0f, (float) h * 0.5f - 6.0f, 8.0f, 12.0f }));
}

juce::Font AlienLookAndFeel::getComboBoxFont (juce::ComboBox&)          { return alienFont (14.0f, true); }
juce::Font AlienLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return alienFont (12.5f, true); }
void AlienLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (bg1);
    g.setColour (accent.withAlpha (0.5f));
    g.drawRect (0, 0, w, h, 1);
}

//==============================================================================
// SELECTEUR DE MONDE (5 cristaux)
//==============================================================================
juce::Rectangle<float> TypeSelector::podBounds (int i) const
{
    const float gap = 10.0f;
    const float w = ((float) getWidth() - gap * 4.0f) / 5.0f;
    return { (float) i * (w + gap), 0.0f, w, (float) getHeight() };
}

void TypeSelector::paint (juce::Graphics& g)
{
    const int current = (int) apvts.getRawParameterValue ("type")->load();
    const juce::StringArray names { "DREAMING", "NEBULA", "CINEMA", "ABYSS", "BACKROOMS" };
    const juce::StringArray subs  { "reve / shimmer", "espace alien", "lourd / epique", "dark fantasy / metal", "liminal / neon" };

    for (int i = 0; i < 5; ++i)
    {
        auto r = podBounds (i).reduced (1.5f);
        auto path = chamfer (r, 12.0f);
        const auto col = forType (i);
        const bool on = (i == current);
        const float pulse = on ? 0.5f + 0.5f * std::sin (phase * 2.0f) : 0.0f;

        g.setGradientFill (juce::ColourGradient (on ? col.withAlpha (0.24f + 0.08f * pulse) : juce::Colour (0xff111820), r.getX(), r.getY(),
                                                 juce::Colour (0xff04070a), r.getX(), r.getBottom(), false));
        g.fillPath (path);
        {
            juce::Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (path);
            g.setColour (juce::Colours::white.withAlpha (on ? 0.07f : 0.03f));
            juce::Path f;
            f.addTriangle (r.getX(), r.getY(), r.getX() + r.getWidth() * 0.55f, r.getY(), r.getX(), r.getBottom());
            g.fillPath (f);
        }
        if (on)
            for (int k = 3; k >= 1; --k)
            {
                g.setColour (col.withAlpha (0.08f * (float) k * (0.6f + 0.4f * pulse)));
                g.strokePath (chamfer (r.expanded ((float) k * 1.5f), 13.0f), juce::PathStrokeType (2.0f));
            }
        g.setColour (on ? col : (i == hover ? col.withAlpha (0.6f) : juce::Colour (0xff3a4652)));
        g.strokePath (path, juce::PathStrokeType (on ? 1.8f : 1.0f));
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawLine (r.getX() + 14, r.getY() + 1.5f, r.getRight() - 14, r.getY() + 1.5f, 1.0f);

        auto gr = juce::Rectangle<float> (r.getX() + 16.0f, r.getCentreY() - 19.0f, 24.0f, 38.0f);
        auto gp = gem (gr);
        if (on)
        {
            g.setColour (col.withAlpha (0.25f + 0.2f * pulse));
            g.strokePath (gp, juce::PathStrokeType (7.0f));
        }
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (on ? 0.95f : 0.4f), gr.getX(), gr.getY(),
                                                 col.withAlpha (on ? 0.9f : 0.3f), gr.getRight(), gr.getBottom(), false));
        g.fillPath (gp);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawLine (gr.getCentreX(), gr.getY(), gr.getCentreX(), gr.getBottom(), 0.8f);
        g.drawLine (gr.getX(), gr.getY() + gr.getHeight() * 0.28f, gr.getCentreX(), gr.getCentreY(), 0.6f);
        g.drawLine (gr.getRight(), gr.getY() + gr.getHeight() * 0.28f, gr.getCentreX(), gr.getCentreY(), 0.6f);

        auto tr = r.withTrimmedLeft (52.0f).reduced (4.0f, 10.0f);
        g.setColour (on ? col : text.withAlpha (0.8f));
        g.setFont (alienFont (15.0f, true));
        g.drawFittedText (names[i], tr.removeFromTop (tr.getHeight() * 0.55f).toNearestInt(), juce::Justification::bottomLeft, 1);
        g.setColour (dim);
        g.setFont (alienFont (11.0f));
        g.drawFittedText (subs[i], tr.toNearestInt(), juce::Justification::topLeft, 1);
    }
}

void TypeSelector::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < 5; ++i)
        if (podBounds (i).contains (e.position))
            if (auto* p = apvts.getParameter ("type"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (p->convertTo0to1 ((float) i));
                p->endChangeGesture();
            }
    repaint();
}

void TypeSelector::mouseMove (const juce::MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < 5; ++i)
        if (podBounds (i).contains (e.position)) h = i;
    if (h != hover) { hover = h; repaint(); }
}

//==============================================================================
// OSCILLOSCOPE
//==============================================================================
void AlienScope::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto path = chamfer (r, 10.0f);
    g.setColour (juce::Colour (0xff030508));
    g.fillPath (path);
    g.setColour (accent.withAlpha (0.05f));
    for (float x = 12; x < r.getWidth(); x += 24)
        g.drawVerticalLine ((int) x, 4.0f, r.getHeight() - 4.0f);
    g.drawHorizontalLine ((int) r.getCentreY(), 6.0f, r.getWidth() - 6.0f);

    const int N = HomeKeysProcessor::scopeSize;
    const int wp = proc.scopeWritePos.load();
    const int shown = 512;
    juce::Path wave;
    const float mid = r.getCentreY(), amp = r.getHeight() * 0.42f;
    for (int i = 0; i < shown; ++i)
    {
        const float v = proc.scope[(size_t) ((wp - shown + i + N) % N)];
        const float x = r.getX() + 8.0f + (r.getWidth() - 26.0f) * (float) i / (float) (shown - 1);
        const float y = mid - juce::jlimit (-1.0f, 1.0f, v * 2.5f) * amp;
        if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
    }
    g.setColour (accent.withAlpha (0.12f));
    g.strokePath (wave, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved));
    g.setColour (accent.withAlpha (0.45f));
    g.strokePath (wave, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved));
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.strokePath (wave, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));

    const float lvl = juce::jlimit (0.0f, 1.0f, proc.outputLevel.load() * 1.6f);
    auto meter = r.removeFromRight (12.0f).reduced (3.0f, 10.0f);
    g.setColour (juce::Colour (0xff10181e));
    g.fillRect (meter);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, meter.getX(), meter.getY(), accent, meter.getX(), meter.getBottom(), false));
    g.fillRect (meter.withTrimmedTop (meter.getHeight() * (1.0f - lvl)));
    g.setColour (juce::Colour (0xff3a4652));
    g.strokePath (path, juce::PathStrokeType (1.0f));
}

//==============================================================================
// DISQUE DE GRAIN
//==============================================================================
void GrainDisc::paint (juce::Graphics& g)
{
    const float drive = apvts.getRawParameterValue ("drive")->load();
    const float wow   = apvts.getRawParameterValue ("wow")->load();
    const float crush = apvts.getRawParameterValue ("crush")->load();
    const float vinyl = apvts.getRawParameterValue ("vinyl")->load();
    const float amount = juce::jlimit (0.0f, 1.0f, (drive + wow + crush + vinyl) * 0.5f);

    auto r = getLocalBounds().toFloat();
    const float rad = juce::jmin (r.getHeight(), r.getWidth() * 0.5f) * 0.5f - 2.0f;
    const auto c = juce::Point<float> (r.getX() + rad + 6.0f, r.getCentreY());

    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.10f + 0.25f * amount), c, juce::Colours::transparentBlack, c.translated (rad * 1.4f, 0), true));
    g.fillEllipse (c.x - rad * 1.4f, c.y - rad * 1.4f, rad * 2.8f, rad * 2.8f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a4652), c.x - rad, c.y - rad, juce::Colour (0xff020304), c.x + rad, c.y + rad, false));
    g.fillEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2);
    for (float gr = rad - 4.0f; gr > rad * 0.38f; gr -= 3.0f)
    {
        g.setColour (juce::Colour (0xff0b1015).withAlpha (0.9f));
        g.drawEllipse (c.x - gr, c.y - gr, gr * 2, gr * 2, 0.8f);
    }
    juce::Path shine;
    shine.addPieSegment (c.x - rad, c.y - rad, rad * 2, rad * 2, angle, angle + 0.5f, 0.38f);
    g.setColour (juce::Colours::white.withAlpha (0.08f + 0.15f * amount));
    g.fillPath (shine);
    shine.applyTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::pi, c.x, c.y));
    g.fillPath (shine);

    const float lr = rad * 0.34f;
    auto gp = gem ({ c.x - lr * 0.6f, c.y - lr, lr * 1.2f, lr * 2.0f });
    gp.applyTransform (juce::AffineTransform::rotation (angle * 0.5f, c.x, c.y));
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, c.x - lr, c.y - lr, accent, c.x + lr, c.y + lr, false));
    g.fillPath (gp);

    juce::Random rnd (12345);
    for (int i = 0; i < (int) (vinyl * 40.0f); ++i)
    {
        const float a = rnd.nextFloat() * juce::MathConstants<float>::twoPi + angle;
        const float d = rad * (0.42f + 0.55f * rnd.nextFloat());
        const auto p = c.getPointOnCircumference (d, a);
        g.setColour (accent.withAlpha (0.35f + 0.4f * rnd.nextFloat()));
        g.fillEllipse (p.x - 0.8f, p.y - 0.8f, 1.6f, 1.6f);
    }

    auto meters = r.withTrimmedLeft (rad * 2.0f + 24.0f).reduced (0.0f, 6.0f);
    const char* names[4] = { "DRIVE", "WOW", "CRUSH", "VINYL" };
    const float vals[4]  = { drive, wow, crush, vinyl };
    const float rowH = meters.getHeight() / 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        auto row = meters.removeFromTop (rowH).reduced (0.0f, 4.0f);
        g.setColour (dim);
        g.setFont (alienFont (10.0f, true));
        g.drawText (names[i], row.removeFromLeft (46.0f), juce::Justification::centredLeft);
        auto bar = row.reduced (0.0f, row.getHeight() * 0.3f);
        g.setColour (juce::Colour (0xff10181e));
        g.fillRect (bar);
        g.setGradientFill (juce::ColourGradient (accent, bar.getX(), 0, juce::Colours::white, bar.getRight(), 0, false));
        g.fillRect (bar.withWidth (bar.getWidth() * vals[i]));
    }
}

//==============================================================================
// CONSTELLATION 2.0
//==============================================================================
juce::Point<float> ConstellationView::starPos (int i) const
{
    auto f = field();
    const float x = proc.apvts.getRawParameterValue ("s" + juce::String (i) + "x")->load();
    const float y = proc.apvts.getRawParameterValue ("s" + juce::String (i) + "y")->load();
    return { f.getX() + x * f.getWidth(), f.getY() + y * f.getHeight() };
}

void ConstellationView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto path = chamfer (r, 16.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff070b12), r.getCentreX(), r.getCentreY(), juce::Colour (0xff010203), r.getX(), r.getY(), true));
    g.fillPath (path);

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (path);

        g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.11f), r.getX() + r.getWidth() * 0.3f, r.getY() + r.getHeight() * 0.35f,
                                                 juce::Colours::transparentBlack, r.getX() + r.getWidth() * 0.8f, r.getY() + r.getHeight() * 0.95f, true));
        g.fillRect (r);

        juce::Random rnd (777);
        for (int i = 0; i < 180; ++i)
        {
            const float x = r.getX() + rnd.nextFloat() * r.getWidth();
            const float y = r.getY() + rnd.nextFloat() * r.getHeight();
            const float tw = 0.3f + 0.3f * std::sin (phase * (0.5f + rnd.nextFloat()) + (float) i);
            g.setColour (juce::Colours::white.withAlpha (juce::jlimit (0.03f, 0.55f, tw * rnd.nextFloat())));
            const float s = rnd.nextFloat() < 0.08f ? 2.2f : 1.2f;
            g.fillEllipse (x, y, s, s);
        }

        const auto c = field().getCentre();
        const float R = field().getHeight() * 0.5f;
        for (int k = 1; k <= 3; ++k)
        {
            const float rr = R * (float) k / 3.0f;
            g.setColour (accent.withAlpha (0.07f));
            g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 1.0f);
        }
        const float oa = proc.orbitAngle.load();
        for (int k = 0; k < 6; ++k)
        {
            juce::Path seg;
            const float a = oa + juce::MathConstants<float>::twoPi * (float) k / 6.0f;
            seg.addCentredArc (c.x, c.y, R * 1.02f, R * 1.02f, 0, a, a + 0.35f, true);
            g.setColour (accent.withAlpha (0.35f));
            g.strokePath (seg, juce::PathStrokeType (1.5f));
        }
        g.setColour (accent.withAlpha (0.45f));
        g.fillPath (sparkle (c, 6.0f));

        const auto targets = Constellation::targetNames();
        const auto shapes = Constellation::shapeNames();
        for (int i = 0; i < HomeKeysProcessor::numStars; ++i)
        {
            const auto a = starPos (i), b = starPos ((i + 1) % HomeKeysProcessor::numStars);
            g.setColour (accent.withAlpha (0.08f));
            g.drawLine ({ a, b }, 6.0f);
            g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.65f), a.x, a.y, juce::Colours::white.withAlpha (0.3f), b.x, b.y, false));
            g.drawLine ({ a, b }, 1.2f);
            g.setColour (accent.withAlpha (0.05f));
            g.drawLine ({ a, c }, 1.0f);
        }

        for (int i = 0; i < HomeKeysProcessor::numStars; ++i)
        {
            const auto p = starPos (i);
            const float m = proc.starMod[(size_t) i].load();
            const float size = 9.0f + 10.0f * std::abs (m);
            g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.5f + 0.4f * std::abs (m)), p, accent.withAlpha (0.0f), p.translated (size * 2.4f, 0), true));
            g.fillEllipse (p.x - size * 2.4f, p.y - size * 2.4f, size * 4.8f, size * 4.8f);
            auto sp = sparkle (p, size, 0.2f);
            sp.applyTransform (juce::AffineTransform::rotation (m * 0.8f, p.x, p.y));
            g.setColour (juce::Colours::white);
            g.fillPath (sp);
            g.setColour (accent);
            g.strokePath (sp, juce::PathStrokeType (0.8f));
            if (i == selected)
            {
                g.setColour (juce::Colours::white.withAlpha (0.85f));
                g.strokePath (chamfer ({ p.x - 15, p.y - 15, 30, 30 }, 7.0f), juce::PathStrokeType (1.2f));
            }
            juce::Path arc;
            arc.addCentredArc (p.x, p.y, 13.0f, 13.0f, 0, 0.0f, m * juce::MathConstants<float>::pi, true);
            g.setColour (accent);
            g.strokePath (arc, juce::PathStrokeType (2.0f));

            const int t = (int) proc.apvts.getRawParameterValue ("s" + juce::String (i) + "t")->load();
            const int w = (int) proc.apvts.getRawParameterValue ("s" + juce::String (i) + "w")->load();
            g.setColour (text.withAlpha (i == selected ? 1.0f : 0.75f));
            g.setFont (alienFont (10.5f, true));
            g.drawText (targets[t], (int) p.x - 45, (int) p.y + 16, 90, 13, juce::Justification::centred);
            g.setColour (dim);
            g.setFont (alienFont (9.5f));
            g.drawText (shapes[w], (int) p.x - 45, (int) p.y + 28, 90, 12, juce::Justification::centred);
        }

        g.setColour (dim);
        g.setFont (alienFont (11.0f));
        g.drawText ("glisse les etoiles  -  clic droit : cible / forme  -  loin du centre = plus fort, autour = plus rapide",
                    r.reduced (18.0f, 8.0f), juce::Justification::bottomLeft);
    }
    g.setColour (juce::Colour (0xff3a4652));
    g.strokePath (path, juce::PathStrokeType (1.2f));
}

void ConstellationView::showStarMenu (int star)
{
    juce::PopupMenu tm, sm;
    const auto targets = Constellation::targetNames();
    const auto shapes = Constellation::shapeNames();
    const int curT = (int) proc.apvts.getRawParameterValue ("s" + juce::String (star) + "t")->load();
    const int curW = (int) proc.apvts.getRawParameterValue ("s" + juce::String (star) + "w")->load();
    for (int i = 0; i < targets.size(); ++i) tm.addItem (100 + i, targets[i], true, i == curT);
    for (int i = 0; i < shapes.size(); ++i) sm.addItem (200 + i, shapes[i], true, i == curW);
    juce::PopupMenu m;
    m.addSectionHeader ("ETOILE " + juce::String (star + 1));
    m.addSubMenu ("Cible", tm);
    m.addSubMenu ("Forme", sm);
    m.showMenuAsync (juce::PopupMenu::Options(), [this, star] (int r)
    {
        auto set = [this, star] (const juce::String& suffix, int v)
        {
            if (auto* p = proc.apvts.getParameter ("s" + juce::String (star) + suffix))
                p->setValueNotifyingHost (p->convertTo0to1 ((float) v));
        };
        if (r >= 200) set ("w", r - 200);
        else if (r >= 100) set ("t", r - 100);
        repaint();
    });
}

void ConstellationView::mouseDown (const juce::MouseEvent& e)
{
    dragging = -1;
    float best = 24.0f;
    for (int i = 0; i < HomeKeysProcessor::numStars; ++i)
    {
        const float d = starPos (i).getDistanceFrom (e.position);
        if (d < best) { best = d; dragging = i; }
    }
    if (dragging < 0) return;
    selected = dragging;
    if (onSelect) onSelect (selected);
    if (e.mods.isPopupMenu()) { showStarMenu (dragging); dragging = -1; return; }
    for (auto axis : { "x", "y" })
        if (auto* prm = proc.apvts.getParameter ("s" + juce::String (dragging) + axis))
            prm->beginChangeGesture();
}

void ConstellationView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0) return;
    auto f = field();
    const float x = juce::jlimit (0.0f, 1.0f, (e.position.x - f.getX()) / f.getWidth());
    const float y = juce::jlimit (0.0f, 1.0f, (e.position.y - f.getY()) / f.getHeight());
    proc.apvts.getParameter ("s" + juce::String (dragging) + "x")->setValueNotifyingHost (x);
    proc.apvts.getParameter ("s" + juce::String (dragging) + "y")->setValueNotifyingHost (y);
    repaint();
}

void ConstellationView::mouseUp (const juce::MouseEvent&)
{
    if (dragging >= 0)
        for (auto axis : { "x", "y" })
            if (auto* prm = proc.apvts.getParameter ("s" + juce::String (dragging) + axis))
                prm->endChangeGesture();
    dragging = -1;
    repaint();
}

//==============================================================================
// PADS DE CHOP
//==============================================================================
juce::Rectangle<float> ChopPads::pad (int i) const
{
    auto r = getLocalBounds().toFloat();
    const float w = (r.getWidth() - 3 * 10.0f) / 4.0f, h = (r.getHeight() - 10.0f) / 2.0f;
    return { (float) (i % 4) * (w + 10.0f), (float) (i / 4) * (h + 10.0f), w, h };
}

void ChopPads::paint (juce::Graphics& g)
{
    const auto names = HomeKeysProcessor::chopNames();
    const char* keys[8] = { "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1" };
    const char* desc[8] = { "coupe le son en rythme", "motif de gate 16 pas", "repete un morceau",
                            "joue a l'envers", "la bande ralentit", "une octave plus bas",
                            "une octave plus haut", "melange aleatoire" };
    const int active = proc.chopActive.load();
    const int selected = (int) proc.apvts.getRawParameterValue ("choptype")->load();
    for (int i = 0; i < 8; ++i)
    {
        auto r = pad (i).reduced (1.0f);
        auto path = chamfer (r, 10.0f);
        const bool on = (i == active);
        g.setGradientFill (juce::ColourGradient (on ? accent.withAlpha (0.4f) : juce::Colour (0xff141c24), r.getX(), r.getY(),
                                                 juce::Colour (0xff06090c), r.getX(), r.getBottom(), false));
        g.fillPath (path);
        g.setColour (on ? accent : (i == selected ? accent.withAlpha (0.7f) : juce::Colour (0xff3a4652)));
        g.strokePath (path, juce::PathStrokeType (on ? 2.2f : (i == selected ? 1.6f : 1.0f)));
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawLine (r.getX() + 10, r.getY() + 1.2f, r.getRight() - 10, r.getY() + 1.2f, 1.0f);
        g.setColour (on ? juce::Colours::white : text);
        g.setFont (alienFont (15.0f, true));
        g.drawText (names[i], r.reduced (12.0f, 9.0f), juce::Justification::topLeft);
        g.setColour (dim);
        g.setFont (alienFont (10.5f));
        g.drawText (desc[i], r.reduced (12.0f, 9.0f), juce::Justification::bottomLeft);
        g.setColour (accent);
        g.setFont (alienFont (12.0f, true));
        g.drawText (juce::String ("touche ") + keys[i], r.reduced (12.0f, 9.0f), juce::Justification::topRight);
    }
}

void ChopPads::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < 8; ++i)
        if (pad (i).contains (e.position))
            if (auto* prm = proc.apvts.getParameter ("choptype"))
            {
                prm->beginChangeGesture();
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i));
                prm->endChangeGesture();
            }
    repaint();
}

//==============================================================================
// COURBE DU FILTRE
//==============================================================================
void FilterView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto path = chamfer (r, 10.0f);
    g.setColour (juce::Colour (0xff030508));
    g.fillPath (path);
    const float fc = apvts.getRawParameterValue ("fcut")->load();
    const float q = 0.5f + apvts.getRawParameterValue ("fres")->load() * 6.0f;
    const int type = (int) apvts.getRawParameterValue ("ftype")->load();
    juce::Path curve;
    auto inner = r.reduced (10.0f, 16.0f);
    for (int i = 0; i <= 200; ++i)
    {
        const float f = 20.0f * std::pow (1000.0f, (float) i / 200.0f);
        const float w = f / fc;
        const float re = 1.0f - w * w, im = w / q;
        const float den = std::sqrt (re * re + im * im);
        const float mag = type == 0 ? 1.0f / den : type == 1 ? (w / q) / den : (w * w) / den;
        const float db = juce::jlimit (-36.0f, 18.0f, juce::Decibels::gainToDecibels (mag));
        const float x = inner.getX() + inner.getWidth() * (float) i / 200.0f;
        const float y = inner.getY() + inner.getHeight() * (1.0f - (db + 36.0f) / 54.0f);
        if (i == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
    }
    juce::Path fill (curve);
    fill.lineTo (inner.getRight(), inner.getBottom()); fill.lineTo (inner.getX(), inner.getBottom()); fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.25f), 0, inner.getY(), accent.withAlpha (0.0f), 0, inner.getBottom(), false));
    g.fillPath (fill);
    g.setColour (accent.withAlpha (0.4f));
    g.strokePath (curve, juce::PathStrokeType (4.0f));
    g.setColour (juce::Colours::white);
    g.strokePath (curve, juce::PathStrokeType (1.3f));
    g.setColour (dim);
    g.setFont (alienFont (10.5f, true));
    g.drawText (juce::String ("FILTRE ") + (type == 0 ? "LP" : type == 1 ? "BP" : "HP") + "  " + juce::String ((int) fc) + " Hz",
                r.reduced (12.0f, 4.0f), juce::Justification::topLeft);
    g.setColour (juce::Colour (0xff3a4652));
    g.strokePath (path, juce::PathStrokeType (1.0f));
}

//==============================================================================
// EDITEUR
//==============================================================================
HomeKeysEditor::Knob& HomeKeysEditor::addKnob (const juce::String& id, const juce::String& title, int pg, const juce::String& suffix)
{
    auto k = std::make_unique<Knob>();
    k->page = pg;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 17);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    addChildComponent (k->slider);
    k->attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
    if (suffix.isNotEmpty()) k->slider.setTextValueSuffix (suffix);
    k->slider.setNumDecimalPlacesToDisplay (id == "fcut" || id == "octave" ? 0 : (id == "volume" || id == "fine") ? 1 : 2);
    k->label.setText (title, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (alienFont (11.5f, true));
    k->label.setColour (juce::Label::textColourId, steel);
    addChildComponent (k->label);
    auto& ref = *k;
    knobById[id] = k.get();
    knobs.push_back (std::move (k));
    return ref;
}

void HomeKeysEditor::placeRow (juce::Rectangle<int> area, const juce::StringArray& ids, int maxW)
{
    const int kw = juce::jmin (maxW, area.getWidth() / juce::jmax (1, ids.size()));
    for (auto& id : ids)
    {
        auto cell = area.removeFromLeft (kw);
        if (id.isEmpty()) continue;
        auto* k = knobById[id];
        k->label.setBounds (cell.removeFromTop (15));
        k->slider.setBounds (cell.reduced (6, 0));
    }
}

HomeKeysEditor::HomeKeysEditor (HomeKeysProcessor& p)
    : AudioProcessorEditor (&p), proc (p), typeSelector (p.apvts), scope (p), grainDisc (p.apvts),
      constellation (p), chopPads (p), filterView (p.apvts),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (typeSelector);
    for (juce::Component* c : { (juce::Component*) &scope, (juce::Component*) &grainDisc, (juce::Component*) &constellation,
                                (juce::Component*) &chopPads, (juce::Component*) &filterView })
        addChildComponent (c);

    // PAGE 0 : SON
    addKnob ("tone", "TONE", 0);             addKnob ("velocity", "VELOCITY", 0);
    addKnob ("release", "RELEASE", 0, " s"); addKnob ("layer", "NAPPE", 0);
    addKnob ("width", "WIDTH", 0);           addKnob ("chorus", "CHORUS", 0);
    addKnob ("reverb", "REVERB", 0);         addKnob ("size", "SIZE", 0);
    addKnob ("volume", "VOLUME", 0, " dB");
    addKnob ("octave", "OCTAVE", 0, " oct"); addKnob ("drive", "DRIVE", 0);
    addKnob ("wow", "WOW", 0);               addKnob ("crush", "CRUSH", 0);
    addKnob ("vinyl", "VINYL", 0);
    // PAGE 1 : SYNTH
    addKnob ("piano", "PIANO", 1);           addKnob ("harm", "HARMONIC", 1);
    addKnob ("metal", "METAL", 1);           addKnob ("ring", "ANNEAU", 1);
    addKnob ("tex", "TEXTURE", 1);           addKnob ("texlvl", "TEX LVL", 1);
    addKnob ("sub", "SUB", 1);               addKnob ("unison", "UNISON", 1);
    addKnob ("fine", "FINE", 1);
    addKnob ("fcut", "CUTOFF", 1);           addKnob ("fres", "RESO", 1);
    addKnob ("ftype", "TYPE", 1);            addKnob ("attack", "ATTACK", 1);
    addKnob ("decay", "DECAY", 1);           addKnob ("hammer", "HAMMER", 1);
    addKnob ("shimmer", "SHIMMER", 1);       addKnob ("air", "AIR", 1);
    addKnob ("hum", "HUM", 1);
    addKnob ("dtime", "DELAY", 1);           addKnob ("dfb", "FEEDBACK", 1);
    addKnob ("dmix", "DLY MIX", 1);
    // PAGE 2 : CONSTELLATION
    addKnob ("cdepth", "DEPTH", 2);          addKnob ("crate", "RATE", 2);
    addKnob ("cchaos", "CHAOS", 2);          addKnob ("corbit", "ORBIT", 2);
    addKnob ("cvar", "VARIATION", 2);        addKnob ("cmut", "MUTATION", 2);
    // PAGE 3 : CHOP
    addKnob ("choprate", "RATE", 3);         addKnob ("chopmix", "MIX", 3);

    knobById["tex"]->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 96, 17);

    // RANDOM : la cle de HomeKey I
    addAndMakeVisible (randomBtn);
    randomBtn.onClick = [this] { randomSound(); };
    randomBtn.setTooltip ("Cree un son nouveau a chaque clic (MUTATION regle la distance)");
    addAndMakeVisible (lockBtn);
    lockBtn.setClickingTogglesState (true);
    lockBtn.setTooltip ("Garder le monde actuel pendant le RANDOM");

    addChildComponent (starsBtn);
    starsBtn.onClick = [this] { SoundDesigner::randomizeStars (proc.apvts, proc.apvts.getRawParameterValue ("cmut")->load()); constellation.repaint(); };
    addChildComponent (soundBtn);
    soundBtn.onClick = [this] { randomSound(); };

    addChildComponent (starLabel);
    starLabel.setFont (alienFont (12.0f, true));
    starLabel.setColour (juce::Label::textColourId, text);
    addChildComponent (starTarget);
    addChildComponent (starShape);
    starTarget.addItemList (Constellation::targetNames(), 1);
    starShape.addItemList (Constellation::shapeNames(), 1);
    constellation.onSelect = [this] (int i) { selectStar (i); };
    selectStar (0);

    addChildComponent (chopOn);
    chopOn.setClickingTogglesState (true);
    chopAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "chop", chopOn);

    const char* tabNames[4] = { "SON", "SYNTH", "CONSTELLATION", "CHOP" };
    for (int i = 0; i < 4; ++i)
    {
        tabs[(size_t) i].setButtonText (tabNames[i]);
        tabs[(size_t) i].onClick = [this, i] { setPage (i); };
        addAndMakeVisible (tabs[(size_t) i]);
    }

    addAndMakeVisible (presetBox);
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && (idx != proc.presets.getCurrentIndex() || proc.presets.getCurrentName() != proc.presets.getPresetName (idx)))
            proc.presets.loadPreset (idx);
    };
    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &delBtn, &importBtn, &folderBtn, &creditBtn })
        addAndMakeVisible (b);
    prevBtn.onClick = [this] { proc.presets.previous(); refreshPresetBox(); };
    nextBtn.onClick = [this] { proc.presets.next();     refreshPresetBox(); };
    saveBtn.onClick = [this] { showSaveDialog(); };
    importBtn.onClick = [this] { showImportMenu(); };
    creditBtn.onClick = [this] { showCredits(); };
    creditBtn.setTooltip ("Credits des banques de sons");
    delBtn.onClick  = [this]
    {
        const int idx = proc.presets.getCurrentIndex();
        if (proc.presets.isFactory (idx)) return;
        proc.presets.deletePreset (idx);
        refreshPresetBox();
    };
    folderBtn.onClick = [] { PresetManager::getUserFolder().startAsProcess(); };
    importBtn.setTooltip ("Ajouter un dossier, un .zip ou des .hkpreset");

    keyboard.setAvailableRange (21, 108);
    keyboard.setOctaveForMiddleC (5);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xff1c242c));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff030406));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff070a0d));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colour (0x00000000));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, dim);
    addAndMakeVisible (keyboard);

    setSize (1000, 790);
    setPage (0);
    refreshPresetBox();
    timerCallback();
    startTimerHz (30);
}

HomeKeysEditor::~HomeKeysEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void HomeKeysEditor::selectStar (int i)
{
    constellation.selected = i;
    targetAttach.reset();
    shapeAttach.reset();
    targetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "s" + juce::String (i) + "t", starTarget);
    shapeAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "s" + juce::String (i) + "w", starShape);
    starLabel.setText ("ETOILE " + juce::String (i + 1), juce::dontSendNotification);
    constellation.repaint();
}

void HomeKeysEditor::randomSound()
{
    const auto name = SoundDesigner::randomizeSound (proc.apvts, proc.apvts.getRawParameterValue ("cmut")->load(), lockBtn.getToggleState());
    proc.presets.setCurrentName ("~ " + name);
    refreshPresetBox();
    constellation.repaint();
}

void HomeKeysEditor::setPage (int pg)
{
    page = pg;
    for (auto& k : knobs) { k->slider.setVisible (k->page == pg); k->label.setVisible (k->page == pg); }
    scope.setVisible (pg == 0);
    grainDisc.setVisible (pg == 0);
    filterView.setVisible (pg == 1);
    constellation.setVisible (pg == 2);
    for (juce::Component* c : { (juce::Component*) &starsBtn, (juce::Component*) &soundBtn, (juce::Component*) &starLabel,
                                (juce::Component*) &starTarget, (juce::Component*) &starShape })
        c->setVisible (pg == 2);
    chopPads.setVisible (pg == 3);
    chopOn.setVisible (pg == 3);
    for (int i = 0; i < 4; ++i)
        tabs[(size_t) i].setToggleState (i == pg, juce::dontSendNotification);
    resized();
    repaint();
}

void HomeKeysEditor::showCredits()
{
    auto opts = juce::MessageBoxOptions()
        .withIconType (juce::MessageBoxIconType::NoIcon)
        .withTitle ("HOMEKEY I - CREDITS")
        .withMessage ("HomeKey I  v3.0  -  Xeno Piano Engine\n\n"
                      "Piano : Salamander Grand Piano V3\n"
                      "par Alexander Holm - licence CC-BY 3.0\n"
                      "(echantillons raccourcis, 4 couches, convertis en Ogg)\n\n"
                      "Textures metal : Versilian Community Sample Library (VCSL)\n"
                      "Versilian Studios - licence CC0 (domaine public)\n\n"
                      "Moteur, effets, Constellation et interface : HomeKey I")
        .withButton ("OK")
        .withAssociatedComponent (this);
    juce::AlertWindow::showAsync (opts, nullptr);
}

void HomeKeysEditor::showImportMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Importer un DOSSIER de presets...");
    m.addItem (2, "Importer un fichier .zip ou .hkpreset...");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&importBtn), [this] (int r)
    {
        if (r == 0) return;
        const bool folder = (r == 1);
        chooser = std::make_unique<juce::FileChooser> (folder ? "Choisis le dossier de presets" : "Choisis un .zip ou des .hkpreset",
                                                       juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Downloads"),
                                                       folder ? juce::String() : juce::String ("*.zip;*.hkpreset"));
        const int flags = juce::FileBrowserComponent::openMode
                        | (folder ? juce::FileBrowserComponent::canSelectDirectories
                                  : (juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems));
        chooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
        {
            if (fc.getResults().isEmpty()) return;
            int added = 0;
            for (auto& f : fc.getResults())
                added += PresetManager::importFrom (f);
            refreshPresetBox();
            auto opts = juce::MessageBoxOptions()
                            .withIconType (added > 0 ? juce::MessageBoxIconType::InfoIcon : juce::MessageBoxIconType::WarningIcon)
                            .withTitle (added > 0 ? "IMPORT REUSSI" : "AUCUN PRESET TROUVE")
                            .withMessage (added > 0 ? juce::String (added) + " preset(s) ajoute(s) a HomeKey I.\nIls sont dans le menu des presets."
                                                    : "Ce dossier ne contient aucun fichier .hkpreset.")
                            .withButton ("OK")
                            .withAssociatedComponent (this);
            juce::AlertWindow::showAsync (opts, nullptr);
        });
    });
}

void HomeKeysEditor::refreshPresetBox()
{
    auto& pm = proc.presets;
    pm.refresh();
    presetBox.clear (juce::dontSendNotification);

    juce::String lastCat;
    bool firstUser = true;
    for (int i = 0; i < pm.getNumPresets(); ++i)
    {
        if (pm.isFactory (i))
        {
            if (i == 0) presetBox.addSectionHeading ("INIT");
        }
        else
        {
            auto cat = pm.getCategory (i);
            if (cat.isEmpty()) cat = "MES PRESETS";
            if (firstUser || cat != lastCat)
            {
                presetBox.addSeparator();
                presetBox.addSectionHeading (cat);
                lastCat = cat;
                firstUser = false;
            }
        }
        presetBox.addItem (pm.getPresetName (i), i + 1);
    }
    presetBox.setSelectedId (pm.getCurrentIndex() + 1, juce::dontSendNotification);
    if (pm.getCurrentName() != pm.getPresetName (pm.getCurrentIndex()))
        presetBox.setText (pm.getCurrentName(), juce::dontSendNotification);   // son RANDOM pas encore sauve
    lastPresetCount = pm.getNumPresets();
    lastPresetName = pm.getCurrentName();
    delBtn.setEnabled (! pm.isFactory (pm.getCurrentIndex()));
}

void HomeKeysEditor::showSaveDialog()
{
    dialog = std::make_unique<juce::AlertWindow> ("NOUVEAU PRESET HOMEKEY I", "Donne un nom a ton son :", juce::MessageBoxIconType::NoIcon, this);
    dialog->setLookAndFeel (&lnf);
    auto suggested = proc.presets.getCurrentName().trimCharactersAtStart ("~ ").trim();
    dialog->addTextEditor ("name", suggested.isEmpty() ? "Mon Preset" : suggested);
    dialog->addButton ("SAUVER", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("ANNULER", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    dialog->enterModalState (true, juce::ModalCallbackFunction::create ([this] (int result)
    {
        if (result == 1 && dialog != nullptr)
        {
            proc.presets.savePreset (dialog->getTextEditorContents ("name"));
            refreshPresetBox();
        }
        dialog.reset();
    }), false);
}

void HomeKeysEditor::timerCallback()
{
    t += 0.033f;
    const int type = (int) proc.apvts.getRawParameterValue ("type")->load();
    if (type != lastType)
    {
        lastType = type;
        const auto c = forType (type);
        lnf.accent = c;
        scope.accent = grainDisc.accent = constellation.accent = chopPads.accent = filterView.accent = c;
        keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, c.withAlpha (0.8f));
        keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, c.withAlpha (0.25f));
        repaint();
    }

    if (++refreshTick >= 60)
    {
        refreshTick = 0;
        proc.presets.refresh();
        if (proc.presets.getNumPresets() != lastPresetCount) refreshPresetBox();
    }
    if (proc.presets.getCurrentName() != lastPresetName)
        refreshPresetBox();

    chopOn.setButtonText (chopOn.getToggleState() ? "CHOP ON" : "CHOP OFF");

    typeSelector.phase = t;
    typeSelector.repaint();
    if (page == 0) { scope.repaint(); grainDisc.angle += 0.06f + 0.10f * proc.apvts.getRawParameterValue ("wow")->load(); grainDisc.repaint(); }
    if (page == 1) filterView.repaint();
    if (page == 2) { constellation.phase = t; constellation.repaint(); }
    if (page == 3) chopPads.repaint();
}

void HomeKeysEditor::paint (juce::Graphics& g)
{
    const auto acc = forType (lastType < 0 ? 0 : lastType);
    auto r = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (bg1, r.getCentreX(), r.getHeight() * 0.3f, bg0, 0.0f, r.getHeight(), true));
    g.fillAll();
    g.setGradientFill (juce::ColourGradient (acc.withAlpha (0.13f), r.getCentreX(), 30.0f, juce::Colours::transparentBlack, r.getCentreX(), 380.0f, true));
    g.fillRect (r);

    // treillis cristallin
    g.setColour (acc.withAlpha (0.03f));
    for (float x = -r.getHeight(); x < r.getWidth(); x += 46.0f)
    {
        g.drawLine (x, 0, x + r.getHeight(), r.getHeight(), 0.7f);
        g.drawLine (x + r.getHeight(), 0, x, r.getHeight(), 0.7f);
    }

    // eclats de cristal
    auto shard = [&] (std::initializer_list<juce::Point<float>> pts, float alpha)
    {
        juce::Path p; bool first = true;
        for (auto pt : pts) { if (first) p.startNewSubPath (pt); else p.lineTo (pt); first = false; }
        p.closeSubPath();
        const auto b = p.getBounds();
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (alpha), b.getX(), b.getY(), acc.withAlpha (alpha * 0.2f), b.getRight(), b.getBottom(), false));
        g.fillPath (p);
        g.setColour (juce::Colours::white.withAlpha (alpha * 1.6f));
        g.strokePath (p, juce::PathStrokeType (0.7f));
    };
    shard ({ { 0, 0 }, { 120, 0 }, { 40, 70 } }, 0.05f);
    shard ({ { 0, 30 }, { 40, 70 }, { 0, 150 } }, 0.035f);
    shard ({ { r.getWidth(), 0 }, { r.getWidth() - 70, 0 }, { r.getWidth(), 60 } }, 0.04f);

    // logo chrome + cristal "I"
    auto logoArea = juce::Rectangle<float> (28, 12, 330, 48);
    const juce::String logo = "HOMEKEY";
    g.setFont (alienFont (36.0f, true));
    for (int k = 4; k >= 1; --k)
    {
        g.setColour (acc.withAlpha (0.05f * (float) k));
        g.drawText (logo, logoArea.expanded ((float) k).toNearestInt(), juce::Justification::centredLeft);
    }
    juce::ColourGradient chrome (juce::Colours::white, 0, logoArea.getY() + 8, juce::Colour (0xff5d6a77), 0, logoArea.getBottom() - 6, false);
    chrome.addColour (0.47, juce::Colour (0xffc7d2dc));
    chrome.addColour (0.53, juce::Colour (0xff6f7c88));
    chrome.addColour (0.8, juce::Colour (0xffe9f0f5));
    g.setGradientFill (chrome);
    g.drawText (logo, logoArea.toNearestInt(), juce::Justification::centredLeft);
    const float lw = juce::GlyphArrangement::getStringWidth (alienFont (36.0f, true), logo);
    const auto gr = juce::Rectangle<float> (logoArea.getX() + lw + 12.0f, logoArea.getY() + 7.0f, 15.0f, 34.0f);
    auto ig = gem (gr);
    g.setColour (acc.withAlpha (0.4f));
    g.strokePath (ig, juce::PathStrokeType (5.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, gr.getX(), gr.getY(), acc, gr.getRight(), gr.getBottom(), false));
    g.fillPath (ig);
    g.setColour (dim);
    g.setFont (alienFont (11.0f));
    g.drawText ("XENO PIANO ENGINE  //  v3.0  //  DREAM - ALIEN - DARK", 32, 60, 380, 14, juce::Justification::centredLeft);

    const float W = r.getWidth() - 40.0f;
    if (page == 0)
    {
        metalPanel (g, { 20, 308, W, 158 }, acc, "CORPS");
        const float sx = 20.0f + W * 5.0f / 9.0f;
        g.setColour (acc.withAlpha (0.25f));
        g.drawLine (sx, 326, sx, 448, 1.0f);
        g.setFont (alienFont (10.5f, true));
        g.setColour (acc.withAlpha (0.95f));
        g.drawText ("<> ESPACE", (int) sx + 14, 314, 120, 14, juce::Justification::left);
        metalPanel (g, { 20, 474, W, 152 }, acc, "GRAIN", "bande  /  cassette  /  lo-fi  /  vinyle");
    }
    else if (page == 1)
    {
        metalPanel (g, { 20, 214, W, 132 }, acc, "SOURCES", "piano Salamander + couche harmonique + metal + textures");
        metalPanel (g, { 20, 352, W, 132 }, acc, "FILTRE / AMPLI");
        metalPanel (g, { 20, 490, W, 136 }, acc, "ATMOSPHERE / DELAY", "shimmer de reve, vent spatial, neon des Backrooms");
    }
    else if (page == 2)
    {
        metalPanel (g, { 632, 214, W - 612, 412 }, acc, "CONSTELLATION");
        g.setColour (dim);
        g.setFont (alienFont (10.5f));
        g.drawFittedText ("MUTATION : 0 = variation legere, 1 = son totalement neuf.\nLa molette de modulation augmente le DEPTH.",
                          juce::Rectangle<int> (648, 584, (int) W - 644, 34), juce::Justification::topLeft, 2);
    }
    else
    {
        metalPanel (g, { 20, 214, W, 412 }, acc, "CHOP EN TEMPS REEL",
                    "CHOP ON = chop permanent  //  ou joue les notes C1 a G1 dans le piano roll pour chopper en direct");
    }

    g.setColour (acc.withAlpha (0.4f));
    g.strokePath (chamfer (keyboard.getBounds().toFloat().expanded (4.0f), 8.0f), juce::PathStrokeType (1.2f));
}

void HomeKeysEditor::resized()
{
    auto r = getLocalBounds();

    auto top = juce::Rectangle<int> (420, 22, r.getWidth() - 440, 34);
    creditBtn.setBounds (top.removeFromRight (30));  top.removeFromRight (5);
    folderBtn.setBounds (top.removeFromRight (72));  top.removeFromRight (5);
    importBtn.setBounds (top.removeFromRight (62));  top.removeFromRight (5);
    delBtn.setBounds (top.removeFromRight (40));     top.removeFromRight (5);
    saveBtn.setBounds (top.removeFromRight (50));    top.removeFromRight (8);
    prevBtn.setBounds (top.removeFromLeft (28));
    nextBtn.setBounds (top.removeFromRight (28));
    top.reduce (5, 0);
    presetBox.setBounds (top);

    typeSelector.setBounds (20, 88, r.getWidth() - 40, 78);

    auto tb = juce::Rectangle<int> (20, 174, r.getWidth() - 40, 32);
    const int tw[4] = { 100, 100, 160, 100 };
    for (int i = 0; i < 4; ++i) { tabs[(size_t) i].setBounds (tb.removeFromLeft (tw[i])); tb.removeFromLeft (6); }
    lockBtn.setBounds (tb.removeFromRight (64));
    tb.removeFromRight (6);
    randomBtn.setBounds (tb.removeFromRight (190));

    const int W = r.getWidth() - 40;
    if (page == 0)
    {
        scope.setBounds (20, 214, W, 86);
        placeRow ({ 28, 330, W - 16, 122 }, { "tone", "velocity", "release", "layer", "width", "chorus", "reverb", "size", "volume" });
        auto gp = juce::Rectangle<int> (28, 496, W - 16, 122);
        grainDisc.setBounds (gp.removeFromRight (300).reduced (10, 0));
        placeRow (gp, { "octave", "drive", "wow", "crush", "vinyl" });
    }
    else if (page == 1)
    {
        placeRow ({ 28, 232, W - 16, 108 }, { "piano", "harm", "metal", "ring", "tex", "texlvl", "sub", "unison", "fine" });
        auto fr = juce::Rectangle<int> (28, 370, W - 16, 108);
        filterView.setBounds (fr.removeFromRight (300).reduced (6, 0));
        placeRow (fr, { "fcut", "fres", "ftype", "attack", "decay", "hammer" });
        placeRow ({ 28, 510, W - 16, 108 }, { "shimmer", "air", "hum", "", "dtime", "dfb", "dmix" });
    }
    else if (page == 2)
    {
        constellation.setBounds (20, 214, 604, 412);
        const int x0 = 644;
        placeRow ({ x0, 236, 330, 104 }, { "cdepth", "crate", "cchaos" }, 110);
        placeRow ({ x0, 346, 330, 104 }, { "corbit", "cvar", "cmut" }, 110);
        starLabel.setBounds (x0 + 4, 458, 96, 24);
        starTarget.setBounds (x0 + 98, 456, 114, 28);
        starShape.setBounds (x0 + 218, 456, 112, 28);
        starsBtn.setBounds (x0 + 4, 496, 160, 40);
        soundBtn.setBounds (x0 + 170, 496, 160, 40);
    }
    else
    {
        chopOn.setBounds (40, 244, 150, 40);
        placeRow ({ 210, 236, 240, 130 }, { "choprate", "chopmix" });
        chopPads.setBounds (40, 380, W - 40, 232);
    }

    keyboard.setBounds (24, 646, r.getWidth() - 48, 128);
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 52.0f);
    keyboard.setLowestVisibleKey (21);
}

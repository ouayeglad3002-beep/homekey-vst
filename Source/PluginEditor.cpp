#include "PluginEditor.h"

using namespace AlienColours;

static juce::Font alienFont (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
// LOOK AND FEEL
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
    setColour (juce::PopupMenu::headerTextColourId, dim);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, acid);
    setColour (juce::Label::textColourId, text);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::AlertWindow::backgroundColourId, bg1);
    setColour (juce::AlertWindow::textColourId, text);
    setColour (juce::AlertWindow::outlineColourId, acid);
    setColour (juce::TextEditor::backgroundColourId, bg0);
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::outlineColourId, edge);
    setColour (juce::TextEditor::focusedOutlineColourId, acid);
    setColour (juce::CaretComponent::caretColourId, acid);
}

void AlienLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float a0, float a1, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = a0 + pos * (a1 - a0);

    // halo
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.18f * (0.3f + pos)), c, juce::Colours::transparentBlack,
                                             c.translated (r * 1.25f, 0), true));
    g.fillEllipse (b.expanded (6.0f));

    // piste
    juce::Path track;
    track.addCentredArc (c.x, c.y, r - 3, r - 3, 0, a0, a1, true);
    g.setColour (juce::Colour (0xff17222a));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // arc neon (lueur + trait)
    juce::Path arc;
    arc.addCentredArc (c.x, c.y, r - 3, r - 3, 0, a0, angle, true);
    g.setColour (accent.withAlpha (0.25f));
    g.strokePath (arc, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // corps du bouton (metal organique sombre)
    const float br = r - 10.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff222d36), c.x, c.y - br,
                                             juce::Colour (0xff06090c), c.x, c.y + br, false));
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    g.setColour (juce::Colour (0xff2b3a44));
    g.drawEllipse (c.x - br, c.y - br, br * 2, br * 2, 1.2f);

    // petits "pores" alien autour
    for (int i = 0; i < 12; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 12.0f;
        const auto p = c.getPointOnCircumference (br - 4.0f, a);
        g.setColour (juce::Colour (0xff0a0f13));
        g.fillEllipse (p.x - 1.2f, p.y - 1.2f, 2.4f, 2.4f);
    }

    // indicateur
    const auto p1 = c.getPointOnCircumference (br * 0.25f, angle);
    const auto p2 = c.getPointOnCircumference (br * 0.85f, angle);
    g.setColour (accent.withAlpha (0.35f));
    g.drawLine ({ p1, p2 }, 6.0f);
    g.setColour (accent);
    g.drawLine ({ p1, p2 }, 2.5f);
    g.fillEllipse (p2.x - 2.5f, p2.y - 2.5f, 5.0f, 5.0f);
}

void AlienLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (down || on ? accent.withAlpha (on ? 0.22f : 0.25f) : (over ? juce::Colour (0xff13202a) : panel));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (over || on ? accent : edge.brighter (0.2f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
}

void AlienLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    g.setColour (bg0);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (accent.withAlpha (0.6f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
    juce::Path arrow;
    const float ax = (float) w - 18.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 5, ay - 3, ax + 5, ay - 3, ax, ay + 4);
    g.setColour (accent);
    g.fillPath (arrow);
}

juce::Font AlienLookAndFeel::getComboBoxFont (juce::ComboBox&)          { return alienFont (15.0f, true); }
juce::Font AlienLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return alienFont (12.5f, true); }
void AlienLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (bg1);
    g.setColour (accent.withAlpha (0.5f));
    g.drawRect (0, 0, w, h, 1);
}

//==============================================================================
// SELECTEUR DE PIANO (5 capsules)
//==============================================================================
juce::Rectangle<float> TypeSelector::podBounds (int i) const
{
    const float gap = 12.0f;
    const float w = ((float) getWidth() - gap * 4.0f) / 5.0f;
    return { (float) i * (w + gap), 0.0f, w, (float) getHeight() };
}

void TypeSelector::paint (juce::Graphics& g)
{
    const int current = (int) apvts.getRawParameterValue ("type")->load();
    const auto names = juce::StringArray { "DOUX", "GRAVE", "ORCHESTRE", "CINEMATIQUE", "DREAMING" };
    const auto subs  = juce::StringArray { "felt / intime", "profond / sub", "grand + cordes", "nappe / epique", "shimmer / reve" };

    for (int i = 0; i < 5; ++i)
    {
        auto r = podBounds (i).reduced (2.0f);
        const auto col = forType (i);
        const bool on = (i == current);
        const float pulse = on ? 0.5f + 0.5f * std::sin (phase * 2.0f) : 0.0f;

        // fond capsule
        g.setGradientFill (juce::ColourGradient (on ? col.withAlpha (0.20f + 0.08f * pulse) : juce::Colour (0xff0d141a),
                                                 r.getCentreX(), r.getY(),
                                                 juce::Colour (0xff05080b), r.getCentreX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 14.0f);

        if (on)
        {
            for (int k = 3; k >= 1; --k)
            {
                g.setColour (col.withAlpha (0.07f * (float) k * (0.6f + 0.4f * pulse)));
                g.drawRoundedRectangle (r.expanded ((float) k * 1.5f), 14.0f, 2.0f);
            }
        }
        g.setColour (on ? col : (i == hover ? col.withAlpha (0.6f) : edge.brighter (0.3f)));
        g.drawRoundedRectangle (r, 14.0f, on ? 1.8f : 1.0f);

        // "oeil" / noyau alien
        const auto c = juce::Point<float> (r.getX() + 30.0f, r.getCentreY());
        const float er = 13.0f;
        g.setColour (juce::Colour (0xff04070a));
        g.fillEllipse (c.x - er, c.y - er, er * 2, er * 2);
        g.setGradientFill (juce::ColourGradient (col.withAlpha (on ? 1.0f : 0.35f), c,
                                                 col.withAlpha (0.0f), c.translated (er, 0), true));
        g.fillEllipse (c.x - er + 2, c.y - er + 2, er * 2 - 4, er * 2 - 4);
        g.setColour (juce::Colour (0xff04070a));
        g.fillEllipse (c.x - 2.0f, c.y - 6.5f, 4.0f, 13.0f); // pupille fendue
        g.setColour (col.withAlpha (on ? 0.9f : 0.3f));
        g.drawEllipse (c.x - er, c.y - er, er * 2, er * 2, 1.0f);

        // textes
        auto tr = r.withTrimmedLeft (52.0f).reduced (4.0f, 10.0f);
        g.setColour (on ? col : text.withAlpha (0.75f));
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
    g.setColour (juce::Colour (0xff04070a));
    g.fillRoundedRectangle (r, 12.0f);

    // grille hexagonale fine
    g.setColour (accent.withAlpha (0.05f));
    for (float x = 10; x < r.getWidth(); x += 24)
        g.drawVerticalLine ((int) x, 4.0f, r.getHeight() - 4.0f);
    g.drawHorizontalLine ((int) r.getCentreY(), 6.0f, r.getWidth() - 6.0f);

    // onde
    const int N = HomeKeysProcessor::scopeSize;
    const int wp = proc.scopeWritePos.load();
    const int shown = 512;
    juce::Path wave;
    const float mid = r.getCentreY();
    const float amp = r.getHeight() * 0.42f;
    for (int i = 0; i < shown; ++i)
    {
        const float v = proc.scope[(size_t) ((wp - shown + i + N) % N)];
        const float x = r.getX() + 8.0f + (r.getWidth() - 16.0f) * (float) i / (float) (shown - 1);
        const float y = mid - juce::jlimit (-1.0f, 1.0f, v * 2.5f) * amp;
        if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
    }
    g.setColour (accent.withAlpha (0.12f));
    g.strokePath (wave, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved));
    g.setColour (accent.withAlpha (0.35f));
    g.strokePath (wave, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved));
    g.setColour (accent);
    g.strokePath (wave, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved));

    // vu-metre
    const float lvl = juce::jlimit (0.0f, 1.0f, proc.outputLevel.load() * 1.6f);
    auto meter = r.removeFromRight (10.0f).reduced (3.0f, 10.0f);
    g.setColour (juce::Colour (0xff10181e));
    g.fillRoundedRectangle (meter, 2.0f);
    g.setColour (accent);
    g.fillRoundedRectangle (meter.withTrimmedTop (meter.getHeight() * (1.0f - lvl)), 2.0f);

    g.setColour (edge);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 12.0f, 1.0f);
}

//==============================================================================
// DISQUE DE GRAIN (vinyle alien qui tourne)
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

    // halo
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.10f + 0.25f * amount), c,
                                             juce::Colours::transparentBlack, c.translated (rad * 1.4f, 0), true));
    g.fillEllipse (c.x - rad * 1.4f, c.y - rad * 1.4f, rad * 2.8f, rad * 2.8f);

    // disque
    g.setColour (juce::Colour (0xff030506));
    g.fillEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2);
    for (float gr = rad - 4.0f; gr > rad * 0.38f; gr -= 3.0f)
    {
        g.setColour (juce::Colour (0xff11181e).withAlpha (0.9f));
        g.drawEllipse (c.x - gr, c.y - gr, gr * 2, gr * 2, 0.8f);
    }
    // reflet qui tourne
    juce::Path shine;
    shine.addPieSegment (c.x - rad, c.y - rad, rad * 2, rad * 2, angle, angle + 0.5f, 0.38f);
    g.setColour (accent.withAlpha (0.10f + 0.20f * amount));
    g.fillPath (shine);
    shine.applyTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::pi, c.x, c.y));
    g.fillPath (shine);

    // etiquette centrale (oeil alien)
    const float lr = rad * 0.34f;
    g.setGradientFill (juce::ColourGradient (accent, c, accent.withAlpha (0.15f), c.translated (lr, 0), true));
    g.fillEllipse (c.x - lr, c.y - lr, lr * 2, lr * 2);
    g.setColour (juce::Colour (0xff04070a));
    g.fillEllipse (c.x - 2.5f, c.y - lr * 0.7f, 5.0f, lr * 1.4f);
    const auto dot = c.getPointOnCircumference (lr * 0.75f, angle * 1.0f);
    g.fillEllipse (dot.x - 2.0f, dot.y - 2.0f, 4.0f, 4.0f);

    // grains de poussiere (proportionnels au vinyle)
    juce::Random rnd (12345);
    for (int i = 0; i < (int) (vinyl * 40.0f); ++i)
    {
        const float a = rnd.nextFloat() * juce::MathConstants<float>::twoPi + angle;
        const float d = rad * (0.42f + 0.55f * rnd.nextFloat());
        const auto p = c.getPointOnCircumference (d, a);
        g.setColour (accent.withAlpha (0.35f + 0.4f * rnd.nextFloat()));
        g.fillEllipse (p.x - 0.8f, p.y - 0.8f, 1.6f, 1.6f);
    }

    // jauges a droite
    auto meters = r.withTrimmedLeft (rad * 2.0f + 24.0f).reduced (0.0f, 6.0f);
    const char* names[4] = { "DRIVE", "WOW", "CRUSH", "VINYL" };
    const float vals[4]  = { drive, wow, crush, vinyl };
    const float rowH = meters.getHeight() / 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        auto row = meters.removeFromTop (rowH).reduced (0.0f, 4.0f);
        g.setColour (AlienColours::dim);
        g.setFont (alienFont (10.0f, true));
        g.drawText (names[i], row.removeFromLeft (46.0f), juce::Justification::centredLeft);
        auto bar = row.reduced (0.0f, row.getHeight() * 0.3f);
        g.setColour (juce::Colour (0xff10181e));
        g.fillRoundedRectangle (bar, 2.0f);
        g.setColour (accent.withAlpha (0.85f));
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * vals[i]), 2.0f);
    }
}

//==============================================================================
// CONSTELLATION (LFO aleatoire coherent)
//==============================================================================
static const char* starTargets[8] = { "TONE", "FILTRE", "DRIVE", "WOW", "CRUSH", "CHORUS", "REVERB", "ESPACE" };

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
    g.setColour (juce::Colour (0xff03050a));
    g.fillRoundedRectangle (r, 14.0f);

    // champ d'etoiles
    juce::Random rnd (777);
    for (int i = 0; i < 140; ++i)
    {
        const float x = r.getX() + rnd.nextFloat() * r.getWidth();
        const float y = r.getY() + rnd.nextFloat() * r.getHeight();
        const float tw = 0.3f + 0.3f * std::sin (phase * (0.5f + rnd.nextFloat()) + (float) i);
        g.setColour (juce::Colours::white.withAlpha (juce::jlimit (0.03f, 0.5f, tw * rnd.nextFloat())));
        g.fillEllipse (x, y, 1.4f, 1.4f);
    }

    // centre = aucune modulation ; plus une etoile est loin, plus elle module
    const auto c = field().getCentre();
    for (int k = 1; k <= 3; ++k)
    {
        const float rr = field().getHeight() * 0.5f * (float) k / 3.0f;
        g.setColour (accent.withAlpha (0.06f));
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 1.0f);
    }
    g.setColour (accent.withAlpha (0.25f));
    g.fillEllipse (c.x - 3, c.y - 3, 6, 6);

    // lignes de la constellation
    juce::Path lines;
    for (int i = 0; i < 8; ++i)
    {
        const auto p = starPos (i);
        if (i == 0) lines.startNewSubPath (p); else lines.lineTo (p);
    }
    lines.closeSubPath();
    g.setColour (accent.withAlpha (0.10f));
    g.strokePath (lines, juce::PathStrokeType (6.0f));
    g.setColour (accent.withAlpha (0.45f));
    g.strokePath (lines, juce::PathStrokeType (1.2f));

    // etoiles
    for (int i = 0; i < 8; ++i)
    {
        const auto p = starPos (i);
        const float m = proc.starMod[(size_t) i].load();
        const float size = 7.0f + 9.0f * std::abs (m);
        g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.55f + 0.45f * std::abs (m)), p,
                                                 accent.withAlpha (0.0f), p.translated (size * 2.6f, 0), true));
        g.fillEllipse (p.x - size * 2.6f, p.y - size * 2.6f, size * 5.2f, size * 5.2f);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillEllipse (p.x - size * 0.35f, p.y - size * 0.35f, size * 0.7f, size * 0.7f);
        // rayons
        g.setColour (accent.withAlpha (0.6f));
        g.drawLine (p.x - size, p.y, p.x + size, p.y, 1.0f);
        g.drawLine (p.x, p.y - size, p.x, p.y + size, 1.0f);
        g.setColour (AlienColours::text.withAlpha (i == dragging ? 1.0f : 0.6f));
        g.setFont (alienFont (10.5f, true));
        g.drawText (starTargets[i], (int) p.x - 40, (int) p.y + 12, 80, 14, juce::Justification::centred);
    }

    g.setColour (AlienColours::dim);
    g.setFont (alienFont (11.0f));
    g.drawText ("glisse les etoiles : loin du centre = plus de mouvement, autour du centre = vitesse",
                r.reduced (16.0f, 8.0f), juce::Justification::bottomLeft);
    g.setColour (AlienColours::edge);
    g.drawRoundedRectangle (r.reduced (0.5f), 14.0f, 1.0f);
}

void ConstellationView::mouseDown (const juce::MouseEvent& e)
{
    dragging = -1;
    float best = 22.0f;
    for (int i = 0; i < 8; ++i)
    {
        const float d = starPos (i).getDistanceFrom (e.position);
        if (d < best) { best = d; dragging = i; }
    }
    if (dragging >= 0)
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

void ConstellationView::randomize()
{
    // nouvelle forme aleatoire mais harmonieuse : angles tries, rayons varies
    auto& rnd = juce::Random::getSystemRandom();
    const float start = rnd.nextFloat() * juce::MathConstants<float>::twoPi;
    for (int i = 0; i < 8; ++i)
    {
        const float a = start + juce::MathConstants<float>::twoPi * ((float) i + 0.6f * rnd.nextFloat()) / 8.0f;
        const float rad = 0.10f + 0.36f * rnd.nextFloat();
        const float x = juce::jlimit (0.02f, 0.98f, 0.5f + rad * std::cos (a));
        const float y = juce::jlimit (0.02f, 0.98f, 0.5f + rad * std::sin (a));
        proc.apvts.getParameter ("s" + juce::String (i) + "x")->setValueNotifyingHost (x);
        proc.apvts.getParameter ("s" + juce::String (i) + "y")->setValueNotifyingHost (y);
    }
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
        const bool on = (i == active);
        g.setColour (on ? accent.withAlpha (0.35f) : juce::Colour (0xff0b1116));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (on ? accent : (i == selected ? accent.withAlpha (0.7f) : AlienColours::edge.brighter (0.3f)));
        g.drawRoundedRectangle (r, 10.0f, on ? 2.2f : (i == selected ? 1.6f : 1.0f));
        g.setColour (on ? juce::Colours::white : AlienColours::text);
        g.setFont (alienFont (15.0f, true));
        g.drawText (names[i], r.reduced (10.0f, 8.0f), juce::Justification::topLeft);
        g.setColour (AlienColours::dim);
        g.setFont (alienFont (10.5f));
        g.drawText (desc[i], r.reduced (10.0f, 8.0f), juce::Justification::bottomLeft);
        g.setColour (accent);
        g.setFont (alienFont (12.0f, true));
        g.drawText (juce::String ("touche ") + keys[i], r.reduced (10.0f, 8.0f), juce::Justification::topRight);
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
    g.setColour (juce::Colour (0xff04070a));
    g.fillRoundedRectangle (r, 12.0f);
    const float fc = apvts.getRawParameterValue ("fcut")->load();
    const float q = 0.5f + apvts.getRawParameterValue ("fres")->load() * 6.0f;
    const int type = (int) apvts.getRawParameterValue ("ftype")->load();
    juce::Path curve;
    auto inner = r.reduced (10.0f, 14.0f);
    for (int i = 0; i <= 200; ++i)
    {
        const float f = 20.0f * std::pow (1000.0f, (float) i / 200.0f);
        const float w = f / fc;
        // reponse d'un filtre 2 poles
        const float re = 1.0f - w * w, im = w / q;
        const float den = std::sqrt (re * re + im * im);
        float mag = type == 0 ? 1.0f / den : type == 1 ? (w / q) / den : (w * w) / den;
        const float db = juce::jlimit (-36.0f, 18.0f, juce::Decibels::gainToDecibels (mag));
        const float x = inner.getX() + inner.getWidth() * (float) i / 200.0f;
        const float y = inner.getY() + inner.getHeight() * (1.0f - (db + 36.0f) / 54.0f);
        if (i == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
    }
    juce::Path fill (curve);
    fill.lineTo (inner.getRight(), inner.getBottom()); fill.lineTo (inner.getX(), inner.getBottom()); fill.closeSubPath();
    g.setColour (accent.withAlpha (0.12f));
    g.fillPath (fill);
    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (2.0f));
    g.setColour (AlienColours::dim);
    g.setFont (alienFont (10.5f, true));
    g.drawText (juce::String ("FILTRE ") + (type == 0 ? "LP" : type == 1 ? "BP" : "HP") + "  " + juce::String ((int) fc) + " Hz",
                r.reduced (10.0f, 4.0f), juce::Justification::topLeft);
    g.setColour (AlienColours::edge);
    g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.0f);
}

//==============================================================================
// EDITEUR
//==============================================================================
HomeKeysEditor::Knob& HomeKeysEditor::addKnob (const juce::String& id, const juce::String& title, int pg, const juce::String& suffix)
{
    auto k = std::make_unique<Knob>();
    k->page = pg;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 18);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff1c2a33));
    k->slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff070b0f));
    k->slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffcfe9de));
    k->slider.setColour (juce::Slider::textBoxHighlightColourId, juce::Colour (0x5539ff8f));
    addChildComponent (k->slider);
    k->attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
    if (suffix.isNotEmpty()) k->slider.setTextValueSuffix (suffix);
    k->label.setText (title, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (alienFont (12.0f, true));
    k->label.setColour (juce::Label::textColourId, dim.brighter (0.4f));
    addChildComponent (k->label);
    auto& ref = *k;
    knobById[id] = k.get();
    knobs.push_back (std::move (k));
    return ref;
}

void HomeKeysEditor::placeRow (juce::Rectangle<int> area, const juce::StringArray& ids)
{
    const int kw = juce::jmin (104, area.getWidth() / juce::jmax (1, ids.size()));
    for (auto& id : ids)
    {
        auto cell = area.removeFromLeft (kw);
        if (id.isEmpty()) continue;
        auto* k = knobById[id];
        k->label.setBounds (cell.removeFromTop (16));
        k->slider.setBounds (cell.reduced (8, 0));
    }
}

HomeKeysEditor::HomeKeysEditor (HomeKeysProcessor& p)
    : AudioProcessorEditor (&p), proc (p), typeSelector (p.apvts), scope (p), grainDisc (p.apvts),
      constellation (p), chopPads (p), filterView (p.apvts),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (typeSelector);
    addChildComponent (scope);
    addChildComponent (grainDisc);
    addChildComponent (constellation);
    addChildComponent (chopPads);
    addChildComponent (filterView);

    // PAGE 0 : PIANO
    addKnob ("tone", "TONE", 0);         addKnob ("velocity", "VELOCITY", 0);
    addKnob ("release", "RELEASE", 0, " s"); addKnob ("layer", "LAYER", 0);
    addKnob ("width", "WIDTH", 0);       addKnob ("chorus", "CHORUS", 0);
    addKnob ("reverb", "REVERB", 0);     addKnob ("size", "SIZE", 0);
    addKnob ("volume", "VOLUME", 0, " dB");
    addKnob ("octave", "OCTAVE", 0, " oct"); addKnob ("drive", "DRIVE", 0);
    addKnob ("wow", "WOW", 0);           addKnob ("crush", "CRUSH", 0);
    addKnob ("vinyl", "VINYL", 0);
    // PAGE 1 : SYNTH
    addKnob ("fcut", "CUTOFF", 1);       addKnob ("fres", "RESO", 1);
    addKnob ("ftype", "TYPE", 1);        addKnob ("attack", "ATTACK", 1);
    addKnob ("decay", "DECAY", 1);       addKnob ("hammer", "HAMMER", 1);
    addKnob ("sub", "SUB", 1);           addKnob ("unison", "UNISON", 1);
    addKnob ("fine", "FINE", 1);
    addKnob ("dtime", "DELAY", 1);       addKnob ("dfb", "FEEDBACK", 1);
    addKnob ("dmix", "DLY MIX", 1);
    // PAGE 2 : CONSTELLATION
    addKnob ("cdepth", "DEPTH", 2);      addKnob ("crate", "RATE", 2);
    // PAGE 3 : CHOP
    addKnob ("choprate", "RATE", 3);     addKnob ("chopmix", "MIX", 3);

    addChildComponent (randomBtn);
    randomBtn.onClick = [this] { constellation.randomize(); };
    randomBtn.setTooltip ("Nouvelle constellation aleatoire");

    addChildComponent (chopOn);
    chopOn.setClickingTogglesState (true);
    chopAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "chop", chopOn);

    const char* tabNames[4] = { "PIANO", "SYNTH", "CONSTELLATION", "CHOP" };
    for (int i = 0; i < 4; ++i)
    {
        tabs[(size_t) i].setButtonText (tabNames[i]);
        tabs[(size_t) i].setClickingTogglesState (false);
        tabs[(size_t) i].onClick = [this, i] { setPage (i); };
        addAndMakeVisible (tabs[(size_t) i]);
    }

    // presets
    addAndMakeVisible (presetBox);
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != proc.presets.getCurrentIndex())
            proc.presets.loadPreset (idx);
    };
    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &delBtn, &importBtn, &folderBtn })
        addAndMakeVisible (b);
    prevBtn.onClick = [this] { proc.presets.previous(); refreshPresetBox(); };
    nextBtn.onClick = [this] { proc.presets.next();     refreshPresetBox(); };
    saveBtn.onClick = [this] { showSaveDialog(); };
    importBtn.onClick = [this] { showImportMenu(); };
    delBtn.onClick  = [this]
    {
        const int idx = proc.presets.getCurrentIndex();
        if (proc.presets.isFactory (idx)) return;
        proc.presets.deletePreset (idx);
        refreshPresetBox();
    };
    folderBtn.onClick = [] { PresetManager::getUserFolder().startAsProcess(); };
    importBtn.setTooltip ("Ajouter un dossier, un .zip ou des .hkpreset");

    // clavier
    keyboard.setAvailableRange (21, 108);
    keyboard.setOctaveForMiddleC (5); // comme FL Studio
    keyboard.setScrollButtonsVisible (false);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xff1a222a));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff030506));
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

void HomeKeysEditor::setPage (int pg)
{
    page = pg;
    for (auto& k : knobs) { k->slider.setVisible (k->page == pg); k->label.setVisible (k->page == pg); }
    scope.setVisible (pg == 0);
    grainDisc.setVisible (pg == 0);
    filterView.setVisible (pg == 1);
    constellation.setVisible (pg == 2);
    randomBtn.setVisible (pg == 2);
    chopPads.setVisible (pg == 3);
    chopOn.setVisible (pg == 3);
    for (int i = 0; i < 4; ++i)
        tabs[(size_t) i].setToggleState (i == pg, juce::dontSendNotification);
    resized();
    repaint();
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
        chooser = std::make_unique<juce::FileChooser> (folder ? "Choisis le dossier de presets HomeKeys" : "Choisis un .zip ou des .hkpreset",
                                                       juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Downloads"),
                                                       folder ? juce::String() : juce::String ("*.zip;*.hkpreset"));
        const int flags = juce::FileBrowserComponent::openMode
                        | (folder ? juce::FileBrowserComponent::canSelectDirectories
                                  : (juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems));
        chooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
        {
            int added = 0;
            for (auto& f : fc.getResults())
                added += PresetManager::importFrom (f);
            if (fc.getResults().isEmpty()) return;
            refreshPresetBox();
            auto opts = juce::MessageBoxOptions()
                            .withIconType (added > 0 ? juce::MessageBoxIconType::InfoIcon : juce::MessageBoxIconType::WarningIcon)
                            .withTitle (added > 0 ? "IMPORT REUSSI" : "AUCUN PRESET TROUVE")
                            .withMessage (added > 0 ? juce::String (added) + " preset(s) ajoute(s) a HomeKeys.\nIls sont dans le menu des presets."
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

    const juce::StringArray groups { "DOUX", "GRAVE", "ORCHESTRE", "CINEMATIQUE", "DREAMING" };
    int lastGroup = -1;
    juce::String lastCat;
    bool firstUser = true;
    for (int i = 0; i < pm.getNumPresets(); ++i)
    {
        if (pm.isFactory (i))
        {
            const int grp = i / 3;
            if (grp != lastGroup) { presetBox.addSectionHeading ("USINE - " + groups[grp]); lastGroup = grp; }
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
    lastPresetCount = pm.getNumPresets();
    lastPresetName = pm.getCurrentName();
    delBtn.setEnabled (! pm.isFactory (pm.getCurrentIndex()));
}

void HomeKeysEditor::showSaveDialog()
{
    dialog = std::make_unique<juce::AlertWindow> ("NOUVEAU PRESET HOMEKEYS",
                                                  "Donne un nom a ton son :", juce::MessageBoxIconType::NoIcon, this);
    dialog->setLookAndFeel (&lnf);
    auto suggested = proc.presets.getCurrentName().trim();
    dialog->addTextEditor ("name", suggested.isEmpty() ? "Mon Preset" : suggested + " (mod)");
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
        keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, c.withAlpha (0.75f));
        keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, c.withAlpha (0.25f));
        repaint();
    }

    // le dossier de presets est relu toutes les 2 s : un dossier ajoute apparait tout seul
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

static void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour acc, const juce::String& title, const juce::String& sub = {})
{
    g.setColour (AlienColours::panel.withAlpha (0.85f));
    g.fillRoundedRectangle (r, 14.0f);
    g.setColour (AlienColours::edge);
    g.drawRoundedRectangle (r, 14.0f, 1.0f);
    g.setFont (alienFont (10.5f, true));
    g.setColour (acc.withAlpha (0.85f));
    g.drawText ("// " + title, (int) r.getX() + 14, (int) r.getY() + 6, 200, 14, juce::Justification::left);
    if (sub.isNotEmpty())
    {
        g.setColour (AlienColours::dim);
        g.setFont (alienFont (11.0f));
        const int tw = (int) juce::GlyphArrangement::getStringWidth (alienFont (10.5f, true), "// " + title);
        g.drawText (sub, (int) r.getX() + 30 + tw, (int) r.getY() + 6, 700, 14, juce::Justification::left);
    }
}

void HomeKeysEditor::paint (juce::Graphics& g)
{
    const auto acc = forType (lastType < 0 ? 0 : lastType);
    auto r = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (bg1, r.getCentreX(), r.getHeight() * 0.35f, bg0, 0.0f, r.getHeight(), true));
    g.fillAll();
    g.setGradientFill (juce::ColourGradient (acc.withAlpha (0.10f), r.getCentreX(), 40.0f,
                                             juce::Colours::transparentBlack, r.getCentreX(), 360.0f, true));
    g.fillRect (r);

    // grille hexagonale
    g.setColour (acc.withAlpha (0.035f));
    const float hs = 18.0f, hw = std::sqrt (3.0f) * hs;
    for (int row = 0; row * hs * 1.5f < r.getHeight() + hs; ++row)
        for (int col = 0; col * hw < r.getWidth() + hw; ++col)
        {
            const float cx = (float) col * hw + ((row & 1) ? hw * 0.5f : 0.0f), cy = (float) row * hs * 1.5f;
            juce::Path hex;
            for (int k = 0; k < 6; ++k)
            {
                const float a = juce::MathConstants<float>::pi / 3.0f * (float) k + juce::MathConstants<float>::pi / 6.0f;
                if (k == 0) hex.startNewSubPath (cx + hs * std::cos (a), cy + hs * std::sin (a));
                else hex.lineTo (cx + hs * std::cos (a), cy + hs * std::sin (a));
            }
            hex.closeSubPath();
            g.strokePath (hex, juce::PathStrokeType (0.8f));
        }

    // logo
    const juce::String logo = "H O M E K E Y S";
    auto logoArea = juce::Rectangle<int> (28, 16, 420, 44);
    g.setFont (alienFont (34.0f, true));
    for (int k = 4; k >= 1; --k)
    {
        g.setColour (acc.withAlpha (0.06f * (float) k));
        g.drawText (logo, logoArea.expanded (k, k), juce::Justification::centredLeft);
    }
    g.setColour (acc);
    g.drawText (logo, logoArea, juce::Justification::centredLeft);
    g.setColour (dim);
    g.setFont (alienFont (11.5f));
    g.drawText ("XENO PIANO ENGINE  //  v2.0", 32, 58, 300, 16, juce::Justification::centredLeft);

    // panneaux de la page
    const float W = r.getWidth() - 40.0f;
    if (page == 0)
    {
        drawPanel (g, { 20, 308, W, 158 }, acc, "CORPS");
        const float sx = 20.0f + W * 5.0f / 9.0f;
        g.setColour (acc.withAlpha (0.25f));
        g.drawLine (sx, 326, sx, 448, 1.0f);
        g.setFont (alienFont (10.5f, true));
        g.setColour (acc.withAlpha (0.85f));
        g.drawText ("// ESPACE", (int) sx + 14, 314, 120, 14, juce::Justification::left);
        drawPanel (g, { 20, 474, W, 152 }, acc, "GRAIN", "bande  /  cassette  /  lo-fi  /  vinyle");
    }
    else if (page == 1)
    {
        drawPanel (g, { 20, 214, W, 168 }, acc, "OSCILLATEUR / FILTRE / AMPLI", "filtre LP-BP-HP, attaque, declin, marteau, sub, unison");
        drawPanel (g, { 20, 392, W, 234 }, acc, "DELAY", "ping-pong synchronise au tempo de FL Studio");
    }
    else if (page == 2)
    {
        drawPanel (g, { 680, 214, W - 660, 412 }, acc, "LFO CONSTELLATION");
        g.setColour (AlienColours::dim);
        g.setFont (alienFont (11.5f));
        g.drawFittedText ("Chaque etoile module une partie du son\n(TONE, FILTRE, DRIVE, WOW, CRUSH,\nCHORUS, REVERB, ESPACE).\n\n"
                          "Loin du centre = plus intense.\nTourner autour du centre = vitesse.\n\nDEPTH = quantite totale (0 = off).\nRATE = vitesse generale.",
                          juce::Rectangle<int> (696, 410, (int) W - 692, 200), juce::Justification::topLeft, 12);
    }
    else
    {
        drawPanel (g, { 20, 214, W, 412 }, acc, "CHOP EN TEMPS REEL",
                   "CHOP ON = chop permanent  //  ou joue les notes C1 a G1 dans le piano roll pour chopper en direct");
    }

    g.setColour (acc.withAlpha (0.35f));
    g.drawRoundedRectangle (keyboard.getBounds().toFloat().expanded (4.0f), 8.0f, 1.2f);
}

void HomeKeysEditor::resized()
{
    auto r = getLocalBounds();

    // barre de presets
    auto top = juce::Rectangle<int> (440, 24, r.getWidth() - 460, 34);
    folderBtn.setBounds (top.removeFromRight (74)); top.removeFromRight (5);
    importBtn.setBounds (top.removeFromRight (64)); top.removeFromRight (5);
    delBtn.setBounds (top.removeFromRight (42));    top.removeFromRight (5);
    saveBtn.setBounds (top.removeFromRight (52));   top.removeFromRight (8);
    prevBtn.setBounds (top.removeFromLeft (30));
    nextBtn.setBounds (top.removeFromRight (30));
    top.reduce (5, 0);
    presetBox.setBounds (top);

    typeSelector.setBounds (20, 92, r.getWidth() - 40, 76);

    // onglets
    auto tb = juce::Rectangle<int> (20, 176, r.getWidth() - 40, 30);
    const int tw[4] = { 110, 110, 170, 110 };
    for (int i = 0; i < 4; ++i) { tabs[(size_t) i].setBounds (tb.removeFromLeft (tw[i])); tb.removeFromLeft (6); }

    const int W = r.getWidth() - 40;
    if (page == 0)
    {
        scope.setBounds (20, 214, W, 86);
        placeRow (juce::Rectangle<int> (28, 330, W - 16, 120), { "tone", "velocity", "release", "layer", "width", "chorus", "reverb", "size", "volume" });
        auto gp = juce::Rectangle<int> (28, 498, W - 16, 120);
        grainDisc.setBounds (gp.removeFromRight (300).reduced (10, 0));
        placeRow (gp, { "octave", "drive", "wow", "crush", "vinyl" });
    }
    else if (page == 1)
    {
        placeRow (juce::Rectangle<int> (28, 238, W - 16, 130), { "fcut", "fres", "ftype", "attack", "decay", "hammer", "sub", "unison", "fine" });
        auto dp = juce::Rectangle<int> (28, 418, W - 16, 196);
        filterView.setBounds (dp.removeFromRight (520).reduced (10, 6));
        placeRow (dp.withTrimmedTop (30).withHeight (130), { "dtime", "dfb", "dmix" });
    }
    else if (page == 2)
    {
        constellation.setBounds (20, 214, 650, 412);
        placeRow (juce::Rectangle<int> (700, 244, 240, 130), { "cdepth", "crate" });
        randomBtn.setBounds (712, 376, 216, 30);
    }
    else
    {
        chopOn.setBounds (40, 244, 150, 40);
        placeRow (juce::Rectangle<int> (210, 236, 240, 130), { "choprate", "chopmix" });
        chopPads.setBounds (40, 380, W - 40, 232);
    }

    keyboard.setBounds (24, 646, r.getWidth() - 48, 128);
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 52.0f);
    keyboard.setLowestVisibleKey (21);
}

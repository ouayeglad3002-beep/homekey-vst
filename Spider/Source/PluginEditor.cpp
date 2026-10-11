#include "PluginEditor.h"

namespace
{
    const juce::Colour paper  { 0xffffffff };
    const juce::Colour paper2 { 0xfff6f6f4 };
    const juce::Colour ink    { 0xff111111 };
    const juce::Colour ink2   { 0xff555555 };
    const juce::Colour silk   { 0xffd9d9d9 };
    const juce::Colour faint  { 0xffececec };

    juce::Font sf (float s, bool bold = false) { return juce::Font (juce::FontOptions (s, bold ? juce::Font::bold : juce::Font::plain)); }

    juce::String spaced (const juce::String& s)
    {
        juce::String r;
        for (int i = 0; i < s.length(); ++i) { r << s[i]; if (i < s.length() - 1) r << ' '; }
        return r;
    }
}

//==============================================================================
// LA TOILE
//==============================================================================
float WebView::radiusFor (float midi, float centreMidi) const
{
    const float R = juce::jmin (getWidth(), getHeight()) * 0.5f - 18.0f;
    const float x = juce::jlimit (0.0f, 1.0f, (midi - (centreMidi - 14.0f)) / 28.0f);
    return R * (0.16f + 0.84f * x);
}

void WebView::tick (float dt)
{
    time += dt;
    int s1, n1, s2, n2;
    auto& f = proc.engine.vizFifo;
    const int ready = f.getNumReady();
    f.prepareToRead (ready, s1, n1, s2, n2);
    for (int i = 0; i < n1; ++i) notes.push_back ({ proc.engine.vizBuf[(size_t) (s1 + i)], 0.0f });
    for (int i = 0; i < n2; ++i) notes.push_back ({ proc.engine.vizBuf[(size_t) (s2 + i)], 0.0f });
    f.finishedRead (n1 + n2);
    for (auto& n : notes) n.age += dt;
    while (! notes.empty() && notes.front().age > 9.0f) notes.pop_front();
    while (notes.size() > 96) notes.pop_front();
}

void WebView::drawSpider (juce::Graphics& g, juce::Point<float> at, float angle, float size)
{
    juce::Graphics::ScopedSaveState s (g);
    g.addTransform (juce::AffineTransform::rotation (angle).translated (at.x, at.y));
    g.setColour (ink);
    for (int side = -1; side <= 1; side += 2)
        for (int k = 0; k < 4; ++k)
        {
            const float a = (-0.9f + 0.6f * (float) k) * (float) side;
            const float wig = 0.15f * std::sin (time * 12.0f + (float) k * 1.7f + (float) side);
            juce::Path leg;
            const auto knee = juce::Point<float> (std::sin (a + wig) * size * 1.0f, -std::cos (a + wig) * size * 0.2f + (float) (k - 1.5f) * size * 0.35f);
            leg.startNewSubPath (0, 0);
            leg.quadraticTo (knee.x * 0.9f, knee.y - size * 0.5f, knee.x * 1.6f, knee.y + size * 0.3f);
            g.strokePath (leg, juce::PathStrokeType (1.1f));
        }
    g.fillEllipse (-size * 0.32f, -size * 0.55f, size * 0.64f, size * 0.7f);
    g.fillEllipse (-size * 0.45f, size * 0.05f, size * 0.9f, size * 1.0f);
}

void WebView::paint (juce::Graphics& g)
{
    auto& e = proc.engine;
    auto b = getLocalBounds().toFloat();
    const auto c = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 18.0f;
    const float morph = e.morph.load();
    const float shake = 1.0f + 5.0f * (1.0f - morph);
    const Theory::Key key { e.keyTonic.load(), e.keyMinor.load() != 0 };
    float centreMidi = 0.0f; int cnt = 0;
    for (auto& n : notes) if (n.v.voiced) { centreMidi += n.v.origMidi; ++cnt; }
    centreMidi = cnt > 0 ? centreMidi / (float) cnt : 64.0f;

    // --- les 15 fils de fond, qui se melangent a l'infini ---
    for (int i = 0; i < Web::NumParams; ++i)
    {
        const float v = e.shown[(size_t) i].load();
        const float dir = (i % 2 == 0) ? 1.0f : -1.0f;
        const float a0 = juce::MathConstants<float>::twoPi * (float) i / Web::NumParams + time * 0.025f * dir;
        const float a1 = a0 + juce::MathConstants<float>::pi * (0.55f + 0.4f * v) + 0.2f * std::sin (time * 0.11f + (float) i);
        const auto p0 = c.getPointOnCircumference (R * 1.15f, a0), p1 = c.getPointOnCircumference (R * 1.15f, a1);
        const float bend = R * (0.15f + 0.45f * v) * std::sin (time * (0.17f + 0.02f * (float) i) + (float) i * 1.3f) * shake * 0.6f;
        const auto mid = c.getPointOnCircumference (bend, (a0 + a1) * 0.5f + time * 0.05f);
        juce::Path th; th.startNewSubPath (p0); th.quadraticTo (mid, p1);
        g.setColour (ink.withAlpha (0.05f + 0.16f * v));
        g.strokePath (th, juce::PathStrokeType (0.6f + 1.2f * v));
    }

    // --- rayons : 2 mesures en doubles-croches ---
    for (int s = 0; s < 32; ++s)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) s / 32.0f;
        const bool bar = s % 16 == 0, beat = s % 4 == 0;
        g.setColour (bar ? ink.withAlpha (0.55f) : beat ? silk.darker (0.15f) : faint);
        g.drawLine ({ c.getPointOnCircumference (R * 0.12f, a), c.getPointOnCircumference (R, a) }, bar ? 1.2f : 0.8f);
    }

    // --- cercles = notes de la gamme detectee (spirale de soie) ---
    for (int deg = -20; deg <= 40; ++deg)
    {
        const float m = Theory::midiOfDegree (deg, key);
        if (m < centreMidi - 14.0f || m > centreMidi + 14.0f) continue;
        const float r = radiusFor (m, centreMidi);
        const bool tonic = (((int) m - key.tonic) % 12 + 12) % 12 == 0;
        juce::Path ring;
        for (int s = 0; s <= 32; ++s)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) s / 32.0f;
            const float wob = std::sin (time * 0.9f + (float) s * 0.7f + (float) deg) * 1.2f * shake;
            const auto pt = c.getPointOnCircumference (r + wob, a);
            if (s == 0) ring.startNewSubPath (pt); else ring.lineTo (pt);
        }
        g.setColour (tonic ? ink.withAlpha (0.45f) : silk);
        g.strokePath (ring, juce::PathStrokeType (tonic ? 1.1f : 0.7f));
    }

    // --- notes : d'origine (gris) et recomposees (noir) ---
    auto posFor = [&] (float step, float midi) { return c.getPointOnCircumference (radiusFor (midi, centreMidi), juce::MathConstants<float>::twoPi * step / 32.0f); };
    const Note* prev = nullptr;
    for (auto& n : notes)
    {
        const float fadeA = juce::jlimit (0.0f, 1.0f, 1.0f - n.age / 9.0f);
        const auto po = posFor (n.v.step, n.v.origMidi);
        const auto pn = posFor (n.v.step, n.v.newMidi);
        if (! n.v.voiced)
        {
            g.setColour (ink2.withAlpha (0.35f * fadeA));
            g.drawEllipse (pn.x - 3, pn.y - 3, 6, 6, 1.0f);
            continue;
        }
        if (prev != nullptr && prev->v.voiced && n.age - prev->age < 2.0f)
        {
            // fil de soie entre deux notes recomposees
            const auto pp = posFor (prev->v.step, prev->v.newMidi);
            const auto ctrl = juce::Point<float> ((pp.x + pn.x) * 0.5f, (pp.y + pn.y) * 0.5f);
            const auto pull = ctrl + (c - ctrl) * 0.18f;
            juce::Path silkP; silkP.startNewSubPath (pp); silkP.quadraticTo (pull, pn);
            g.setColour (ink.withAlpha (0.55f * fadeA));
            g.strokePath (silkP, juce::PathStrokeType (1.3f));
        }
        g.setColour (ink2.withAlpha (0.5f * fadeA));
        g.drawEllipse (po.x - 4, po.y - 4, 8, 8, 1.0f);
        if (std::abs (n.v.newMidi - n.v.origMidi) > 0.3f)
        {
            juce::Path link; link.startNewSubPath (po); link.lineTo (pn);
            const float dash[] = { 2.0f, 3.0f };
            juce::Path dashed; juce::PathStrokeType (0.8f).createDashedStroke (dashed, link, dash, 2);
            g.setColour (ink2.withAlpha (0.6f * fadeA));
            g.fillPath (dashed);
        }
        for (float hm : { n.v.h1Midi, n.v.h2Midi })
            if (hm > 0.0f)
            {
                const auto ph = posFor (n.v.step + 0.35f, hm);
                g.setColour (ink.withAlpha (0.25f * fadeA));
                g.drawLine ({ pn, ph }, 0.7f);
                g.setColour (ink2.withAlpha (0.7f * fadeA));
                g.fillEllipse (ph.x - 2.5f, ph.y - 2.5f, 5, 5);
            }
        const float sz = 7.0f + (n.v.stutter ? 3.0f : 0.0f);
        g.setColour (ink.withAlpha (fadeA));
        g.fillEllipse (pn.x - sz * 0.5f, pn.y - sz * 0.5f, sz, sz);
        if (n.v.grace)
        {
            const auto pg = posFor (n.v.step - 0.4f, n.v.newMidi + 1.0f);
            g.fillEllipse (pg.x - 2, pg.y - 2, 4, 4);
        }
        if (n.age < 0.6f)
        {
            g.setColour (ink.withAlpha (0.3f * (1.0f - n.age / 0.6f)));
            const float rr = 6.0f + n.age * 40.0f;
            g.drawEllipse (pn.x - rr, pn.y - rr, rr * 2, rr * 2, 1.0f);
        }
        prev = &n;
    }

    // --- l'araignee qui parcourt la toile ---
    const float ang = juce::MathConstants<float>::twoPi * e.cyclePos.load();
    const auto sp = c.getPointOnCircumference (R + 6.0f, ang);
    g.setColour (ink.withAlpha (0.18f));
    g.drawLine ({ c, sp }, 0.8f);
    drawSpider (g, sp, ang, 9.0f);

    // centre : la gamme
    g.setColour (paper);
    g.fillEllipse (c.x - R * 0.12f, c.y - R * 0.12f, R * 0.24f, R * 0.24f);
    g.setColour (ink);
    g.drawEllipse (c.x - R * 0.12f, c.y - R * 0.12f, R * 0.24f, R * 0.24f, 1.0f);
    g.setFont (sf (13.0f, true));
    const bool sure = e.keyConf.load() > 0.12f;
    g.drawText (sure ? juce::String (Theory::noteName (key.tonic)) : juce::String ("..."), juce::Rectangle<float> (c.x - 40, c.y - 14, 80, 16), juce::Justification::centred);
    g.setFont (sf (9.5f));
    g.setColour (ink2);
    g.drawText (sure ? (key.minor ? "mineur" : "majeur") : "ecoute", juce::Rectangle<float> (c.x - 40, c.y + 1, 80, 13), juce::Justification::centred);
}

//==============================================================================
SpiderEditor::SpiderEditor (SpiderProcessor& p) : AudioProcessorEditor (&p), proc (p), web (p)
{
    addAndMakeVisible (web);
    web.setInterceptsMouseClicks (false, false);
    setSize (980, 640);
    startTimerHz (40);
}

juce::Rectangle<float> SpiderEditor::presetRow (int i) const
{
    return { 28.0f, 214.0f + (float) i * 38.0f, 214.0f, 32.0f };
}

void SpiderEditor::timerCallback()
{
    t += 0.025f;
    web.tick (0.025f);
    for (int i = 0; i < Web::NumParams; ++i)
        shownVals[(size_t) i] += 0.15f * (proc.engine.shown[(size_t) i].load() - shownVals[(size_t) i]);
    repaint();
}

void SpiderEditor::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < Web::numPresets; ++i)
        if (presetRow (i).contains (e.position)) proc.selectPreset (i);
}

void SpiderEditor::mouseMove (const juce::MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < Web::numPresets; ++i) if (presetRow (i).contains (e.position)) h = i;
    if (h != hover) { hover = h; repaint(); }
}

void SpiderEditor::paint (juce::Graphics& g)
{
    auto& e = proc.engine;
    g.fillAll (paper);
    // grain de papier tres leger
    g.setColour (paper2);
    g.fillRect (getLocalBounds().removeFromLeft (262));
    g.fillRect (getLocalBounds().removeFromRight (232));

    // logo
    g.setColour (ink);
    g.setFont (sf (34.0f, false));
    g.drawText (spaced ("SPIDER"), 28, 26, 230, 44, juce::Justification::centredLeft);
    g.setFont (sf (10.5f));
    g.setColour (ink2);
    g.drawText ("tisse une nouvelle melodie dans ta gamme", 30, 70, 230, 14, juce::Justification::centredLeft);
    g.drawText ("by HomeKey Audio", 30, 84, 230, 14, juce::Justification::centredLeft);

    // carte tonalite
    const Theory::Key key { e.keyTonic.load(), e.keyMinor.load() != 0 };
    const float conf = e.keyConf.load();
    auto card = juce::Rectangle<float> (28, 110, 214, 76);
    g.setColour (paper);
    g.fillRect (card);
    g.setColour (ink);
    g.drawRect (card, 1.0f);
    g.setFont (sf (9.5f, true));
    g.setColour (ink2);
    g.drawText ("TONALITE DETECTEE", card.reduced (12, 8).removeFromTop (12), juce::Justification::topLeft);
    g.setColour (ink);
    g.setFont (sf (20.0f, true));
    if (conf > 0.12f)
        g.drawText (juce::String (Theory::noteName (key.tonic)) + (key.minor ? " MINEUR" : " MAJEUR"), card.reduced (12, 0).withTrimmedTop (22).withHeight (26), juce::Justification::centredLeft);
    else
        g.drawText ("ecoute...", card.reduced (12, 0).withTrimmedTop (22).withHeight (26), juce::Justification::centredLeft);
    g.setFont (sf (10.0f));
    g.setColour (ink2);
    const juce::String mood = conf <= 0.12f ? "joue ta melodie" : key.minor ? "humeur : sombre, profonde" : "humeur : lumineuse, joyeuse";
    g.drawText (mood, card.reduced (12, 0).withTrimmedTop (48).withHeight (14), juce::Justification::centredLeft);
    g.setColour (faint);
    g.fillRect (card.getX() + 12, card.getBottom() - 9, card.getWidth() - 24, 2.0f);
    g.setColour (ink);
    g.fillRect (card.getX() + 12, card.getBottom() - 9, (card.getWidth() - 24) * juce::jlimit (0.0f, 1.0f, conf), 2.0f);

    // presets
    g.setFont (sf (9.5f, true));
    g.setColour (ink2);
    g.drawText ("PRESETS", 30, 196, 200, 14, juce::Justification::centredLeft);
    const int cur = (int) proc.apvts.getRawParameterValue ("preset")->load();
    for (int i = 0; i < Web::numPresets; ++i)
    {
        auto r = presetRow (i);
        const bool on = i == cur;
        if (on) { g.setColour (ink); g.fillRect (r); }
        else
        {
            g.setColour (paper); g.fillRect (r);
            g.setColour (i == hover ? ink : silk);
            g.drawRect (r, 1.0f);
        }
        g.setColour (on ? paper : ink);
        g.setFont (sf (12.5f, true));
        g.drawText (juce::String (i + 1).paddedLeft ('0', 2) + "   " + Web::preset (i).name, r.reduced (12, 0), juce::Justification::centredLeft);
        if (on && e.morph.load() < 1.0f)
        {
            g.setColour (paper.withAlpha (0.6f));
            g.fillRect (r.getX(), r.getBottom() - 3, r.getWidth() * e.morph.load(), 3.0f);
        }
    }

    // les 15 fils (lecture seule)
    const float x0 = (float) getWidth() - 214.0f;
    g.setFont (sf (9.5f, true));
    g.setColour (ink2);
    g.drawText ("LES 15 FILS", (int) x0, 28, 190, 14, juce::Justification::centredLeft);
    for (int i = 0; i < Web::NumParams; ++i)
    {
        const float y = 52.0f + (float) i * 34.0f;
        const float v = shownVals[(size_t) i];
        g.setColour (ink);
        g.setFont (sf (10.5f, true));
        g.drawText (Web::paramName (i), (int) x0, (int) y, 140, 13, juce::Justification::centredLeft);
        g.setFont (sf (10.0f));
        g.setColour (ink2);
        g.drawText (juce::String (juce::roundToInt (v * 100.0f)), (int) x0 + 150, (int) y, 40, 13, juce::Justification::centredRight);
        // fil tendu : vibre pendant le morphing
        juce::Path th;
        const float wy = y + 21.0f, len = 190.0f;
        const float amp = (1.0f - e.morph.load()) * 3.0f + 0.4f;
        th.startNewSubPath (x0, wy);
        for (int k = 1; k <= 24; ++k)
        {
            const float fx = (float) k / 24.0f;
            th.lineTo (x0 + fx * len, wy + std::sin (fx * juce::MathConstants<float>::pi) * std::sin (t * 9.0f + (float) i) * amp);
        }
        g.setColour (silk);
        g.strokePath (th, juce::PathStrokeType (1.0f));
        g.setColour (ink);
        g.fillRect (x0, wy - 1.0f, len * v, 2.0f);
        g.fillEllipse (x0 + len * v - 3.0f, wy - 3.0f, 6.0f, 6.0f);
    }

    // pied
    const auto& pr = Web::preset (cur);
    g.setColour (ink);
    g.setFont (sf (12.0f, true));
    g.drawText (pr.name, 278, getHeight() - 52, 420, 16, juce::Justification::centredLeft);
    g.setFont (sf (11.0f));
    g.setColour (ink2);
    g.drawText (pr.desc, 278, getHeight() - 34, 460, 14, juce::Justification::centredLeft);
    g.setFont (sf (10.0f));
    const int seen = e.notesSeen.load(), changed = e.notesChanged.load();
    g.drawText (juce::String (changed) + " / " + juce::String (seen) + " notes recomposees",
                getWidth() - 232 - 220, getHeight() - 34, 210, 14, juce::Justification::centredRight);
    g.setColour (silk);
    g.drawLine (262, 0, 262, (float) getHeight(), 1.0f);
    g.drawLine ((float) getWidth() - 232, 0, (float) getWidth() - 232, (float) getHeight(), 1.0f);
}

void SpiderEditor::resized()
{
    web.setBounds (272, 14, getWidth() - 262 - 232 - 20, getHeight() - 76);
}

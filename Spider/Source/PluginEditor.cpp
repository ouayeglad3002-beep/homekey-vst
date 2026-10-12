#include "PluginEditor.h"

namespace
{
    const juce::Colour paper  { 0xffffffff };
    const juce::Colour paper2 { 0xfff7f7f5 };
    const juce::Colour ink    { 0xff070707 };
    const juce::Colour ink2   { 0xff6a6a6a };
    const juce::Colour silk   { 0xffdedede };

    constexpr float pi = juce::MathConstants<float>::pi;
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    juce::Font sf (float s, bool bold = false) { return juce::Font (juce::FontOptions (s, bold ? juce::Font::bold : juce::Font::plain)); }

    // bruit lisse (somme de sinus) : le mouvement "vivant" de la matiere
    inline float wave (float x, float y, float t)
    {
        return std::sin (x * 0.031f + t * 0.71f) * std::cos (y * 0.027f - t * 0.53f)
             + 0.55f * std::sin ((x + y) * 0.047f + t * 1.13f)
             + 0.30f * std::cos ((x - 1.7f * y) * 0.071f - t * 1.61f);
    }

    juce::Point<float> quadAt (juce::Point<float> a, juce::Point<float> c, juce::Point<float> b, float u)
    {
        const float v = 1.0f - u;
        return a * (v * v) + c * (2.0f * u * v) + b * (u * u);
    }
}

//==============================================================================
// ENCRE
//==============================================================================
Ink::Pts Ink::flatten (const juce::Path& p, const juce::AffineTransform& t)
{
    Pts out;
    juce::PathFlatteningIterator it (p, t, 0.25f);
    bool first = true;
    while (it.next())
    {
        if (first) { out.push_back ({ it.x1, it.y1 }); first = false; }
        out.push_back ({ it.x2, it.y2 });
    }
    return out;
}

juce::Path Ink::taper (const Pts& p, float wMax, float tin, float tout, float wMin)
{
    juce::Path out;
    const int n = (int) p.size();
    if (n < 2) return out;
    std::vector<float> L ((size_t) n, 0.0f);
    for (int i = 1; i < n; ++i) L[(size_t) i] = L[(size_t) i - 1] + p[(size_t) i].getDistanceFrom (p[(size_t) i - 1]);
    const float total = juce::jmax (1.0e-3f, L.back());
    tin = juce::jmax (1.0e-3f, tin); tout = juce::jmax (1.0e-3f, tout);
    Pts left, right; left.reserve ((size_t) n); right.reserve ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        auto d = p[(size_t) juce::jmin (i + 1, n - 1)] - p[(size_t) juce::jmax (i - 1, 0)];
        const float len = juce::jmax (1.0e-4f, std::hypot (d.x, d.y));
        const juce::Point<float> nrm (-d.y / len, d.x / len);
        const float u = L[(size_t) i] / total;
        const float k = juce::jmin (1.0f, u / tin, (1.0f - u) / tout);
        const float w = wMin + (wMax - wMin) * std::sin (0.5f * pi * juce::jlimit (0.0f, 1.0f, k));
        left.push_back (p[(size_t) i] + nrm * (0.5f * w));
        right.push_back (p[(size_t) i] - nrm * (0.5f * w));
    }
    out.startNewSubPath (left[0]);
    for (int i = 1; i < n; ++i) out.lineTo (left[(size_t) i]);
    for (int i = n - 1; i >= 0; --i) out.lineTo (right[(size_t) i]);
    out.closeSubPath();
    return out;
}

void Ink::strand (juce::Graphics& g, const Pts& p, float wMax, float tin, float tout, float wMin, float alpha, float gloss)
{
    if (p.size() < 2 || alpha <= 0.003f) return;
    g.setColour (ink.withAlpha (alpha));
    g.fillPath (taper (p, wMax, tin, tout, wMin));
    // reflet de vernis : une ligne claire decalee vers la lumiere (haut-gauche), au coeur du fil
    if (gloss > 0.0f && wMax >= 1.6f)
    {
        const int n = (int) p.size();
        juce::Path hl;
        bool started = false;
        for (int i = n / 6; i < n - n / 5; ++i)
        {
            auto d = p[(size_t) juce::jmin (i + 1, n - 1)] - p[(size_t) juce::jmax (i - 1, 0)];
            const float len = juce::jmax (1.0e-4f, std::hypot (d.x, d.y));
            juce::Point<float> nrm (-d.y / len, d.x / len);
            if (nrm.x + nrm.y > 0.0f) nrm = -nrm;   // toujours vers la lumiere
            const auto q = p[(size_t) i] + nrm * (wMax * 0.2f);
            if (! started) { hl.startNewSubPath (q); started = true; } else hl.lineTo (q);
        }
        g.setColour (juce::Colours::white.withAlpha (gloss * alpha));
        g.strokePath (hl, juce::PathStrokeType (juce::jmax (0.5f, wMax * 0.16f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void Ink::droplet (juce::Graphics& g, juce::Point<float> c, float r, float alpha)
{
    g.setColour (ink.withAlpha (alpha));
    g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
    g.setColour (juce::Colours::white.withAlpha (0.75f * alpha));
    g.fillEllipse (c.x - r * 0.55f, c.y - r * 0.6f, r * 0.55f, r * 0.42f);
    g.setColour (juce::Colours::white.withAlpha (0.25f * alpha));
    g.drawEllipse (c.x - r * 0.75f, c.y - r * 0.75f, r * 1.5f, r * 1.5f, 0.6f);
}

//==============================================================================
// LA TOILE
//==============================================================================
float WebView::radiusFor (float midi, float centreMidi) const
{
    const float R = juce::jmin (getWidth(), getHeight()) * 0.5f - 22.0f;
    const float x = juce::jlimit (0.0f, 1.0f, (midi - (centreMidi - 14.0f)) / 28.0f);
    return R * (0.2f + 0.8f * x);
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
    while (! notes.empty() && notes.front().age > 13.0f) notes.pop_front();
    while (notes.size() > 96) notes.pop_front();
}

// deplacement organique d'un point : courant lent + attraction des notes qui arrivent
juce::Point<float> WebView::flow (juce::Point<float> p, float amp) const
{
    juce::Point<float> d (wave (p.x, p.y, time), wave (p.y + 91.0f, p.x - 37.0f, time * 1.13f));
    d = d * amp;
    for (auto& a : attractors)
    {
        const auto v = a.first - p;
        const float dist2 = v.x * v.x + v.y * v.y;
        d += v * (a.second * std::exp (-dist2 / 2600.0f));
    }
    return p + d;
}

void WebView::drawSpider (juce::Graphics& g, juce::Point<float> at, float angle, float size)
{
    juce::Graphics::ScopedSaveState s (g);
    g.addTransform (juce::AffineTransform::rotation (angle).translated (at.x, at.y));
    for (int side = -1; side <= 1; side += 2)
        for (int k = 0; k < 4; ++k)
        {
            const float a = (-1.0f + 0.62f * (float) k) * (float) side;
            const float wig = 0.18f * std::sin (time * 11.0f + (float) k * 1.9f + (float) side);
            const juce::Point<float> base (0.0f, size * 0.15f);
            const juce::Point<float> knee (std::sin (a + wig) * size * 1.25f * (float) side * (float) side,
                                           -std::cos (a + wig) * size * 0.9f + (float) (k - 1.5f) * size * 0.25f);
            const juce::Point<float> foot (knee.x * 1.7f, knee.y + size * 0.9f);
            Ink::Pts leg;
            for (int q = 0; q <= 10; ++q) leg.push_back (quadAt (base, knee + juce::Point<float> (0, -size * 0.5f), foot, (float) q / 10.0f));
            Ink::strand (g, leg, size * 0.22f, 0.05f, 0.9f, 0.3f, 1.0f, 0.0f);
        }
    g.setColour (ink);
    g.fillEllipse (-size * 0.34f, -size * 0.62f, size * 0.68f, size * 0.66f);
    Ink::droplet (g, { 0.0f, size * 0.55f }, size * 0.55f, 1.0f);
}

void WebView::paint (juce::Graphics& g)
{
    auto& e = proc.engine;
    const auto b = getLocalBounds().toFloat();
    const auto c = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 22.0f;
    const float morph = e.morph.load();
    const float agit = 1.0f + 3.5f * (1.0f - morph);          // la matiere s'agite pendant le changement de preset
    const Theory::Key key { e.keyTonic.load(), e.keyMinor.load() != 0 };
    float centreMidi = 0.0f; int cnt = 0;
    for (auto& n : notes) if (n.v.voiced) { centreMidi += n.v.origMidi; ++cnt; }
    centreMidi = cnt > 0 ? centreMidi / (float) cnt : 64.0f;

    auto posFor = [&] (float step, float midi) { return c.getPointOnCircumference (radiusFor (midi, centreMidi), twoPi * step / 64.0f); };

    // les notes qui arrivent tirent la toile vers elles
    attractors.clear();
    for (auto& n : notes)
        if (n.v.voiced && n.age < 1.4f)
        {
            const float k = 1.0f - n.age / 1.4f;
            attractors.push_back ({ posFor (n.v.step, n.v.newMidi), 0.22f * k * k * std::sin (juce::jmin (1.0f, n.age * 6.0f) * pi * 0.5f) });
        }

    const float amp = 2.2f * agit;

    // --- 1. les tentacules du symbiote (pilotes en interne par les 15 fils) ---
    for (int k = 0; k < 7; ++k)
    {
        const float v = 0.5f * (e.shown[(size_t) (k * 2) % Web::NumParams].load() + e.shown[(size_t) (k * 2 + 1) % Web::NumParams].load());
        const float th = twoPi * (float) k / 7.0f + 0.35f + 0.12f * std::sin (time * 0.13f + (float) k);
        const float reach = R * (0.62f - 0.3f * v) + R * 0.06f * std::sin (time * (0.4f + 0.05f * (float) k) + (float) k * 2.1f) * agit;
        Ink::Pts pts;
        for (int q = 0; q <= 36; ++q)
        {
            const float u = (float) q / 36.0f;
            const float r = R * 1.3f + (reach - R * 1.3f) * u;
            const float lat = std::sin (u * 7.0f - time * (1.3f + 0.4f * v) * agit + (float) k * 1.7f) * (4.0f + 16.0f * u) * (0.6f + v);
            const float a = th + lat / juce::jmax (20.0f, r);
            pts.push_back (flow (c.getPointOnCircumference (r, a), amp * 0.6f));
        }
        Ink::strand (g, pts, 3.2f + 4.0f * v, 0.02f, 0.85f, 0.25f, 0.92f);
    }

    // --- 2. les rayons : 16 temps sur 4 mesures ---
    std::array<Ink::Pts, 16> spokes;
    for (int s = 0; s < 16; ++s)
    {
        const float a0 = twoPi * (float) s / 16.0f;
        for (int q = 0; q <= 22; ++q)
        {
            const float u = (float) q / 22.0f;
            const float r = R * (0.14f + 0.9f * u);
            const float swirl = 0.05f * std::sin (time * 0.37f + (float) s * 0.8f) * u * agit;
            spokes[(size_t) s].push_back (flow (c.getPointOnCircumference (r, a0 + swirl), amp * u));
        }
        const bool bar = s % 4 == 0;
        Ink::strand (g, spokes[(size_t) s], bar ? 3.6f : 2.0f, 0.08f, 0.45f, 0.45f, bar ? 1.0f : 0.85f);
    }

    // --- 3. la spirale : chaque note de la gamme = un anneau de fil qui pend entre les rayons ---
    for (int deg = -20; deg <= 40; ++deg)
    {
        const float m = Theory::midiOfDegree (deg, key);
        if (m < centreMidi - 14.0f || m > centreMidi + 14.0f) continue;
        const float r = radiusFor (m, centreMidi);
        const bool tonic = (((int) m - key.tonic) % 12 + 12) % 12 == 0;
        const int qi = juce::jlimit (0, 22, (int) std::round (((r / R) - 0.14f) / 0.9f * 22.0f));
        for (int s = 0; s < 16; ++s)
        {
            const auto& A = spokes[(size_t) s];
            const auto& B = spokes[(size_t) ((s + 1) % 16)];
            const auto pa = A[(size_t) qi], pb = B[(size_t) qi];
            const auto mid = (pa + pb) * 0.5f;
            const float sag = 0.1f + 0.05f * std::sin (time * 0.9f + (float) (s + deg));
            const auto ctrl = mid + (c - mid) * sag;
            Ink::Pts seg;
            for (int q = 0; q <= 8; ++q) seg.push_back (quadAt (pa, ctrl, pb, (float) q / 8.0f));
            Ink::strand (g, seg, tonic ? 2.4f : 1.3f, 0.45f, 0.45f, 0.5f, tonic ? 0.95f : 0.55f, 0.35f);
        }
    }

    // --- 4. les notes : perles grises (origine) et gouttes noires vernies (recomposees) ---
    const Note* prev = nullptr;
    for (auto& n : notes)
    {
        const float fadeA = juce::jlimit (0.0f, 1.0f, 1.0f - n.age / 13.0f);
        const auto po = flow (posFor (n.v.step, n.v.origMidi), amp);
        const auto pn = flow (posFor (n.v.step, n.v.newMidi), amp);
        if (! n.v.voiced)
        {
            g.setColour (ink2.withAlpha (0.4f * fadeA));
            g.drawEllipse (pn.x - 3, pn.y - 3, 6, 6, 1.0f);
            continue;
        }
        if (prev != nullptr && prev->v.voiced && n.age - prev->age < 2.0f)
        {
            // le fil qui relie la melodie recomposee : il ondule comme une matiere vivante
            const auto pp = flow (posFor (prev->v.step, prev->v.newMidi), amp);
            const auto mid = (pp + pn) * 0.5f;
            const auto ctrl = mid + (c - mid) * 0.22f + juce::Point<float> (std::sin (time * 1.7f + n.v.step), std::cos (time * 1.3f + n.v.step)) * 6.0f * agit;
            Ink::Pts seg;
            for (int q = 0; q <= 16; ++q) seg.push_back (quadAt (pp, ctrl, pn, (float) q / 16.0f));
            Ink::strand (g, seg, 3.0f, 0.25f, 0.25f, 1.0f, fadeA);
        }
        g.setColour (ink2.withAlpha (0.55f * fadeA));
        g.drawEllipse (po.x - 3.5f, po.y - 3.5f, 7, 7, 1.0f);
        if (std::abs (n.v.newMidi - n.v.origMidi) > 0.3f)
        {
            g.setColour (ink2.withAlpha (0.35f * fadeA));
            g.drawLine ({ po, pn }, 0.7f);
        }
        for (float hm : { n.v.h1Midi, n.v.h2Midi })
            if (hm > 0.0f)
            {
                const auto ph = flow (posFor (n.v.step + 0.35f, hm), amp);
                Ink::strand (g, { pn, (pn + ph) * 0.5f + juce::Point<float> (0, 2), ph }, 1.4f, 0.3f, 0.3f, 0.4f, 0.6f * fadeA, 0.0f);
                Ink::droplet (g, ph, 2.6f, 0.85f * fadeA);
            }
        // une goutte qui gonfle a l'arrivee puis se pose
        const float pop = n.age < 0.35f ? 1.0f + 0.6f * std::sin (n.age / 0.35f * pi) : 1.0f;
        const float sz = (4.2f + (n.v.stutter ? 1.8f : 0.0f)) * pop;
        Ink::droplet (g, pn, sz, fadeA);
        if (n.v.grace) Ink::droplet (g, flow (posFor (n.v.step - 0.4f, n.v.newMidi + 1.0f), amp), 2.0f, fadeA);
        if (n.age < 0.7f)
        {
            g.setColour (ink.withAlpha (0.35f * (1.0f - n.age / 0.7f)));
            const float rr = 6.0f + n.age * 46.0f;
            g.drawEllipse (pn.x - rr, pn.y - rr, rr * 2, rr * 2, 1.2f);
        }
        prev = &n;
    }

    // --- 5. l'araignee qui parcourt la toile ---
    const float ang = twoPi * e.cyclePos.load();
    const auto sp = flow (c.getPointOnCircumference (R * 1.02f, ang), amp);
    {
        Ink::Pts thread { c.getPointOnCircumference (R * 0.14f, ang), (c + sp) * 0.5f + juce::Point<float> (std::sin (time * 2.0f), 0) * 3.0f, sp };
        Ink::strand (g, thread, 1.0f, 0.2f, 0.2f, 0.5f, 0.35f, 0.0f);
    }
    drawSpider (g, sp, ang, 8.0f);

    // --- 6. le coeur : une goutte noire avec la gamme ---
    const float cr = R * 0.13f * (1.0f + 0.03f * std::sin (time * 2.2f) * agit);
    g.setColour (ink);
    g.fillEllipse (c.x - cr, c.y - cr, 2 * cr, 2 * cr);
    {
        juce::Path arc; arc.addCentredArc (c.x, c.y, cr * 0.82f, cr * 0.82f, 0.0f, -1.45f, -0.35f, true);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.strokePath (arc, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    const int learnt = e.learnCount.load();
    const bool sure = learnt >= Engine::learnNotes;
    g.setColour (paper);
    g.setFont (sf (14.0f, true));
    g.drawText (sure ? juce::String (Theory::noteName (key.tonic)) : juce::String (learnt) + "/" + juce::String (Engine::learnNotes),
                juce::Rectangle<float> (c.x - 40, c.y - 13, 80, 16), juce::Justification::centred);
    g.setFont (sf (9.5f));
    g.setColour (paper.withAlpha (0.75f));
    g.drawText (sure ? (key.minor ? "mineur" : "majeur") : "ecoute", juce::Rectangle<float> (c.x - 40, c.y + 2, 80, 12), juce::Justification::centred);
}

//==============================================================================
// L'EDITEUR
//==============================================================================
SpiderEditor::SpiderEditor (SpiderProcessor& p) : AudioProcessorEditor (&p), web (p), proc (p)
{
    addAndMakeVisible (web);
    web.setInterceptsMouseClicks (false, false);
    setSize (660, 520);
    startTimerHz (40);
}

juce::Rectangle<float> SpiderEditor::presetRow (int i) const
{
    return { 16.0f, 102.0f + (float) i * 24.0f, 168.0f, 21.0f };
}

void SpiderEditor::timerCallback()
{
    t += 0.025f;
    web.tick (0.025f);
    repaint();
}

void SpiderEditor::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < Web::numPresets; ++i)
        if (presetRow (i).contains (e.position)) proc.selectPreset (i);
}

void SpiderEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    // double-clic sur le coeur de la toile : Spider reapprend la melodie
    const auto wb = web.getBounds().toFloat();
    const float R = juce::jmin (wb.getWidth(), wb.getHeight()) * 0.5f - 22.0f;
    if (e.position.getDistanceFrom (wb.getCentre()) < R * 0.16f) proc.engine.requestRelearn();
}

void SpiderEditor::mouseMove (const juce::MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < Web::numPresets; ++i) if (presetRow (i).contains (e.position)) h = i;
    hover = h;
}

// Le logo : lettres tracees a l'encre, effilees comme des crocs, qui coulent
void SpiderEditor::drawLogo (juce::Graphics& g, juce::Rectangle<float> area)
{
    const float h = area.getHeight();
    struct Letter { float w; std::vector<juce::Path> strokes; };
    auto P = [] { return juce::Path(); };
    std::vector<Letter> letters;

    {   // S
        auto s = P(); s.startNewSubPath (0.82f, 0.12f);
        s.cubicTo (0.55f, -0.06f, 0.06f, 0.0f, 0.16f, 0.3f);
        s.cubicTo (0.24f, 0.52f, 0.86f, 0.45f, 0.86f, 0.74f);
        s.cubicTo (0.86f, 1.02f, 0.3f, 1.06f, 0.0f, 0.86f);
        letters.push_back ({ 0.86f, { s } });
    }
    {   // P : la hampe descend en croc sous la ligne
        auto a = P(); a.startNewSubPath (0.14f, -0.02f); a.cubicTo (0.16f, 0.4f, 0.12f, 0.8f, 0.16f, 1.22f);
        auto b = P(); b.startNewSubPath (0.12f, 0.06f); b.cubicTo (0.9f, -0.12f, 1.0f, 0.62f, 0.2f, 0.56f);
        letters.push_back ({ 0.8f, { a, b } });
    }
    {   // I
        auto a = P(); a.startNewSubPath (0.2f, -0.08f); a.cubicTo (0.24f, 0.3f, 0.16f, 0.7f, 0.22f, 1.04f);
        letters.push_back ({ 0.42f, { a } });
    }
    {   // D
        auto a = P(); a.startNewSubPath (0.14f, -0.02f); a.lineTo (0.13f, 1.02f);
        auto b = P(); b.startNewSubPath (0.1f, 0.04f); b.cubicTo (1.08f, -0.1f, 1.1f, 1.08f, 0.1f, 0.98f);
        letters.push_back ({ 0.92f, { a, b } });
    }
    {   // E
        auto a = P(); a.startNewSubPath (0.86f, 0.02f); a.cubicTo (0.5f, -0.04f, 0.14f, 0.02f, 0.14f, 0.2f);
        a.cubicTo (0.12f, 0.5f, 0.16f, 0.8f, 0.14f, 0.98f); a.cubicTo (0.4f, 1.02f, 0.7f, 0.96f, 0.92f, 1.02f);
        auto b = P(); b.startNewSubPath (0.16f, 0.5f); b.cubicTo (0.36f, 0.46f, 0.56f, 0.52f, 0.7f, 0.46f);
        letters.push_back ({ 0.86f, { a, b } });
    }
    {   // R : la jambe file en pointe
        auto a = P(); a.startNewSubPath (0.14f, -0.02f); a.cubicTo (0.16f, 0.4f, 0.12f, 0.8f, 0.14f, 1.02f);
        auto b = P(); b.startNewSubPath (0.12f, 0.06f); b.cubicTo (0.92f, -0.12f, 0.98f, 0.6f, 0.18f, 0.52f);
        auto c = P(); c.startNewSubPath (0.36f, 0.52f); c.cubicTo (0.62f, 0.62f, 0.7f, 0.95f, 1.02f, 1.16f);
        letters.push_back ({ 0.92f, { a, b, c } });
    }
    letters.push_back ({ 0.12f, {} });   // espace
    for (int k = 0; k < 2; ++k)
    {   // II : deux crocs
        auto a = P(); a.startNewSubPath (0.2f, -0.02f); a.cubicTo (0.25f, 0.35f, 0.15f, 0.75f, 0.2f, 1.1f);
        letters.push_back ({ 0.34f, { a } });
    }

    float x = area.getX();
    const float y = area.getY();
    const float gap = 0.05f * h;
    int li = 0;
    std::vector<juce::Point<float>> drips;
    juce::Point<float> iFoot;
    for (auto& L : letters)
    {
        const auto tr = juce::AffineTransform::scale (h, h).translated (x, y);
        int si = 0;
        for (auto& s : L.strokes)
        {
            auto pts = Ink::flatten (s, tr);
            // l'encre respire tres legerement
            for (auto& p : pts) p.y += 0.4f * std::sin (t * 1.6f + p.x * 0.09f + (float) li);
            const float wMax = h * (si == 0 ? 0.21f : 0.15f);
            Ink::strand (g, pts, wMax, 0.18f, 0.55f, 0.6f, 1.0f, 0.5f);
            ++si;
        }
        if (li == 2) iFoot = { x + 0.22f * h, y + 1.04f * h };
        if (li == 0) drips.push_back ({ x + 0.38f * h, y + 1.0f * h });
        if (li == 4) drips.push_back ({ x + 0.58f * h, y + 1.0f * h });
        x += L.w * h + gap;
        ++li;
    }
    // gouttes qui coulent des lettres
    for (size_t k = 0; k < drips.size(); ++k)
    {
        const float len = 4.0f + 5.0f * (0.5f + 0.5f * std::sin (t * (0.7f + 0.2f * (float) k) + (float) k * 2.0f));
        const auto a = drips[k];
        Ink::strand (g, { a + juce::Point<float> (0, -2), a + juce::Point<float> (0.3f, len * 0.5f), a + juce::Point<float> (0, len) }, 2.2f, 0.1f, 0.9f, 0.6f, 1.0f, 0.0f);
        Ink::droplet (g, a + juce::Point<float> (0, len + 1.6f), 2.0f, 1.0f);
    }
    // une petite araignee descend du I au bout de son fil
    {
        const float drop = 12.0f + 5.0f * std::sin (t * 0.9f);
        const auto sp = iFoot + juce::Point<float> (1.5f * std::sin (t * 1.7f), drop);
        g.setColour (ink.withAlpha (0.7f));
        g.drawLine ({ iFoot, sp }, 0.7f);
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::scale (0.5f).translated (sp.x, sp.y + 2.0f));
        g.setColour (ink);
        for (int side = -1; side <= 1; side += 2)
            for (int k = 0; k < 4; ++k)
            {
                juce::Path leg;
                const float a = (float) k * 0.5f - 0.75f;
                leg.startNewSubPath (0, 0);
                leg.quadraticTo ((float) side * 8.0f, a * 10.0f - 5.0f, (float) side * 12.0f, a * 12.0f + 4.0f + std::sin (t * 4.0f + (float) k));
                g.strokePath (leg, juce::PathStrokeType (1.5f));
            }
        Ink::droplet (g, { 0.0f, 0.0f }, 5.5f, 1.0f);
    }
}

void SpiderEditor::paint (juce::Graphics& g)
{
    auto& e = proc.engine;
    g.fillAll (paper);
    g.setColour (paper2);
    g.fillRect (0, 0, 196, getHeight());
    g.setColour (silk);
    g.drawLine (196.0f, 0.0f, 196.0f, (float) getHeight(), 1.0f);

    // logo
    drawLogo (g, { 16.0f, 24.0f, 172.0f, 27.0f });

    g.setFont (sf (9.5f));
    g.setColour (ink2);
    g.drawText ("tisse ta melodie dans ta gamme", 18, 84, 170, 12, juce::Justification::centredLeft);

    // presets
    const int cur = (int) proc.apvts.getRawParameterValue ("preset")->load();
    for (int i = 0; i < Web::numPresets; ++i)
    {
        auto r = presetRow (i);
        const bool on = i == cur;
        if (on)
        {
            g.setColour (ink);
            g.fillRoundedRectangle (r, 3.0f);
            g.setColour (juce::Colours::white.withAlpha (0.18f));     // vernis
            g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.42f).reduced (2.0f, 1.5f), 2.0f);
            if (e.morph.load() < 1.0f)
            {
                g.setColour (paper.withAlpha (0.7f));
                g.fillRect (r.getX() + 3.0f, r.getBottom() - 3.0f, (r.getWidth() - 6.0f) * e.morph.load(), 1.5f);
            }
        }
        else if (i == hover)
        {
            g.setColour (ink);
            g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
        }
        g.setColour (on ? paper : ink);
        g.setFont (sf (11.0f, true));
        g.drawText (juce::String (i + 1).paddedLeft ('0', 2), r.withTrimmedLeft (10.0f).withWidth (22.0f), juce::Justification::centredLeft);
        g.drawText (Web::preset (i).name, r.withTrimmedLeft (34.0f), juce::Justification::centredLeft);
        if (i >= 10)
        {
            g.setColour (on ? paper.withAlpha (0.6f) : ink2);
            g.setFont (sf (8.5f, true));
            g.drawText ("II", r.withTrimmedRight (8.0f), juce::Justification::centredRight);
        }
    }

    // description du preset choisi
    g.setColour (ink2);
    g.setFont (sf (9.5f));
    g.drawFittedText (Web::preset (cur).desc, 18, getHeight() - 50, 166, 44, juce::Justification::topLeft, 3, 1.0f);
}

void SpiderEditor::resized()
{
    web.setBounds (200, 0, getWidth() - 200, getHeight());
}

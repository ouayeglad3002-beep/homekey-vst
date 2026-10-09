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
    g.setColour (down ? accent.withAlpha (0.25f) : (over ? juce::Colour (0xff13202a) : panel));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (over ? accent : edge.brighter (0.2f));
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
// EDITEUR
//==============================================================================
HomeKeysEditor::HomeKeysEditor (HomeKeysProcessor& p)
    : AudioProcessorEditor (&p), proc (p), typeSelector (p.apvts), scope (p), grainDisc (p.apvts),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (typeSelector);
    addAndMakeVisible (scope);

    addAndMakeVisible (grainDisc);

    const char* ids[14]    = { "tone", "velocity", "release", "layer", "width", "chorus", "reverb", "size", "volume",
                               "octave", "drive", "wow", "crush", "vinyl" };
    const char* titles[14] = { "TONE", "VELOCITY", "RELEASE", "LAYER", "WIDTH", "CHORUS", "REVERB", "SIZE", "VOLUME",
                               "OCTAVE", "DRIVE", "WOW", "CRUSH", "VINYL" };
    for (int i = 0; i < 14; ++i)
    {
        auto& k = knobs[(size_t) i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        k.slider.setPopupDisplayEnabled (false, false, this);
        k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff1c2a33));
        k.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff070b0f));
        k.slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffcfe9de));
        k.slider.setColour (juce::Slider::textBoxHighlightColourId, juce::Colour (0x5539ff8f));
        addAndMakeVisible (k.slider);
        k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, ids[i], k.slider);

        k.label.setText (titles[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (alienFont (12.0f, true));
        k.label.setColour (juce::Label::textColourId, dim.brighter (0.4f));
        addAndMakeVisible (k.label);
    }
    knobs[2].slider.setTextValueSuffix (" s");
    knobs[8].slider.setTextValueSuffix (" dB");
    knobs[9].slider.setTextValueSuffix (" oct");
    knobs[10].slider.setTooltip ("Saturation chaude de bande magnetique");
    knobs[11].slider.setTooltip ("Ondulation de cassette (pleurage / scintillement)");
    knobs[12].slider.setTooltip ("Grain numerique lo-fi (bits / echantillonnage)");
    knobs[13].slider.setTooltip ("Souffle et craquements de vinyle");

    // presets
    addAndMakeVisible (presetBox);
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != proc.presets.getCurrentIndex())
            proc.presets.loadPreset (idx);
    };
    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &delBtn, &folderBtn })
        addAndMakeVisible (b);
    prevBtn.onClick = [this] { proc.presets.previous(); refreshPresetBox(); };
    nextBtn.onClick = [this] { proc.presets.next();     refreshPresetBox(); };
    saveBtn.onClick = [this] { showSaveDialog(); };
    delBtn.onClick  = [this]
    {
        const int idx = proc.presets.getCurrentIndex();
        if (proc.presets.isFactory (idx)) return;
        proc.presets.deletePreset (idx);
        refreshPresetBox();
    };
    folderBtn.onClick = [] { PresetManager::getUserFolder().startAsProcess(); };
    saveBtn.setTooltip ("Sauvegarder le son actuel comme preset HomeKeys");
    folderBtn.setTooltip ("Ouvrir le dossier des presets (ajoute tes .hkpreset ici)");

    // clavier
    keyboard.setAvailableRange (21, 108);
    keyboard.setOctaveForMiddleC (5); // comme FL Studio
    keyboard.setScrollButtonsVisible (false);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xff1a222a));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff030506));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff070a0d));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colour (0x00000000));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, dim);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, bg1);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, acid);
    addAndMakeVisible (keyboard);

    setSize (1000, 790);
    refreshPresetBox();
    timerCallback();
    startTimerHz (30);
}

HomeKeysEditor::~HomeKeysEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
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
            if (grp != lastGroup) { presetBox.addSectionHeading (groups[grp]); lastGroup = grp; }
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
    auto suggested = proc.presets.getCurrentName().fromLastOccurrenceOf ("- ", false, false).trim();
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
        lnf.accent = forType (type);
        scope.accent = forType (type);
        grainDisc.accent = forType (type);
        keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, forType (type).withAlpha (0.75f));
        keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, forType (type).withAlpha (0.25f));
        repaint();
    }

    if (proc.presets.getCurrentName() != lastPresetName || proc.presets.getNumPresets() != lastPresetCount)
        refreshPresetBox();

    grainDisc.angle += 0.06f + 0.10f * proc.apvts.getRawParameterValue ("wow")->load();
    grainDisc.repaint();
    typeSelector.phase = t;
    typeSelector.repaint();
    scope.repaint();
}

void HomeKeysEditor::paint (juce::Graphics& g)
{
    const auto acc = forType (lastType < 0 ? 0 : lastType);
    auto r = getLocalBounds().toFloat();

    // fond : vide spatial + lueur organique
    g.setGradientFill (juce::ColourGradient (bg1, r.getCentreX(), r.getHeight() * 0.35f,
                                             bg0, 0.0f, r.getHeight(), true));
    g.fillAll();
    g.setGradientFill (juce::ColourGradient (acc.withAlpha (0.10f), r.getCentreX(), 40.0f,
                                             juce::Colours::transparentBlack, r.getCentreX(), 360.0f, true));
    g.fillRect (r);

    // grille hexagonale
    g.setColour (acc.withAlpha (0.035f));
    const float hs = 18.0f;
    const float hw = std::sqrt (3.0f) * hs;
    for (int row = 0; row * hs * 1.5f < r.getHeight() + hs; ++row)
        for (int col = 0; col * hw < r.getWidth() + hw; ++col)
        {
            const float cx = (float) col * hw + ((row & 1) ? hw * 0.5f : 0.0f);
            const float cy = (float) row * hs * 1.5f;
            juce::Path hex;
            for (int k = 0; k < 6; ++k)
            {
                const float a = juce::MathConstants<float>::pi / 3.0f * (float) k + juce::MathConstants<float>::pi / 6.0f;
                const float px = cx + hs * std::cos (a), py = cy + hs * std::sin (a);
                if (k == 0) hex.startNewSubPath (px, py); else hex.lineTo (px, py);
            }
            hex.closeSubPath();
            g.strokePath (hex, juce::PathStrokeType (0.8f));
        }

    // logo HOMEKEYS avec lueur
    const juce::String logo = "H O M E K E Y S";
    auto logoArea = juce::Rectangle<int> (28, 16, 420, 44);
    g.setFont (alienFont (34.0f, true));
    for (int k = 4; k >= 1; --k)
    {
        g.setColour (acc.withAlpha (0.06f * (float) k));
        g.drawText (logo, logoArea.translated (0, 0).expanded (k, k), juce::Justification::centredLeft);
    }
    g.setColour (acc);
    g.drawText (logo, logoArea, juce::Justification::centredLeft);
    g.setColour (dim);
    g.setFont (alienFont (11.5f));
    g.drawText ("XENO PIANO ENGINE  //  v1.0", 32, 58, 300, 16, juce::Justification::centredLeft);

    // panneau des boutons
    auto knobPanel = juce::Rectangle<float> (20.0f, 318.0f, r.getWidth() - 40.0f, 158.0f);
    g.setColour (panel.withAlpha (0.85f));
    g.fillRoundedRectangle (knobPanel, 14.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (knobPanel, 14.0f, 1.0f);

    // separateurs de sections
    const float sx = knobPanel.getX() + knobPanel.getWidth() * 5.0f / 9.0f;
    g.setColour (acc.withAlpha (0.25f));
    g.drawLine (sx, knobPanel.getY() + 18, sx, knobPanel.getBottom() - 18, 1.0f);
    g.setFont (alienFont (10.5f, true));
    g.setColour (acc.withAlpha (0.8f));
    g.drawText ("// CORPS", (int) knobPanel.getX() + 14, (int) knobPanel.getY() + 6, 120, 14, juce::Justification::left);
    g.drawText ("// ESPACE", (int) sx + 14, (int) knobPanel.getY() + 6, 120, 14, juce::Justification::left);

    // panneau GRAIN
    auto grainPanel = juce::Rectangle<float> (20.0f, 488.0f, r.getWidth() - 40.0f, 140.0f);
    g.setColour (panel.withAlpha (0.85f));
    g.fillRoundedRectangle (grainPanel, 14.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (grainPanel, 14.0f, 1.0f);
    g.setFont (alienFont (10.5f, true));
    g.setColour (acc.withAlpha (0.8f));
    g.drawText ("// GRAIN", (int) grainPanel.getX() + 14, (int) grainPanel.getY() + 6, 120, 14, juce::Justification::left);
    g.setColour (dim);
    g.setFont (alienFont (11.0f));
    g.drawText ("bande  /  cassette  /  lo-fi  /  vinyle", (int) grainPanel.getX() + 90, (int) grainPanel.getY() + 6, 300, 14,
                juce::Justification::left);

    // cadre clavier
    g.setColour (acc.withAlpha (0.35f));
    g.drawRoundedRectangle (keyboard.getBounds().toFloat().expanded (4.0f), 8.0f, 1.2f);
}

void HomeKeysEditor::resized()
{
    auto r = getLocalBounds();

    // barre de presets (haut droite)
    auto top = juce::Rectangle<int> (460, 24, r.getWidth() - 480, 34);
    folderBtn.setBounds (top.removeFromRight (78));
    top.removeFromRight (6);
    delBtn.setBounds (top.removeFromRight (46));
    top.removeFromRight (6);
    saveBtn.setBounds (top.removeFromRight (58));
    top.removeFromRight (10);
    prevBtn.setBounds (top.removeFromLeft (34));
    nextBtn.setBounds (top.removeFromRight (34));
    top.reduce (6, 0);
    presetBox.setBounds (top);

    typeSelector.setBounds (20, 92, r.getWidth() - 40, 76);
    scope.setBounds (20, 182, r.getWidth() - 40, 122);

    auto kp = juce::Rectangle<int> (20, 318, r.getWidth() - 40, 158).reduced (8, 22);
    const int kw = kp.getWidth() / 9;
    for (int i = 0; i < 9; ++i)
    {
        auto cell = kp.removeFromLeft (kw);
        knobs[(size_t) i].label.setBounds (cell.removeFromTop (16));
        knobs[(size_t) i].slider.setBounds (cell.reduced (6, 0));
    }

    // rangee GRAIN : 5 boutons + disque vinyle
    auto gp = juce::Rectangle<int> (20, 488, r.getWidth() - 40, 140).reduced (8, 0);
    gp.removeFromTop (24);
    gp.removeFromBottom (8);
    grainDisc.setBounds (gp.removeFromRight (300).reduced (10, 0));
    for (int i = 9; i < 14; ++i)
    {
        auto cell = gp.removeFromLeft (kw + 8);
        knobs[(size_t) i].label.setBounds (cell.removeFromTop (16));
        knobs[(size_t) i].slider.setBounds (cell.reduced (10, 0));
    }

    keyboard.setBounds (24, 646, r.getWidth() - 48, 128);
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 52.0f); // 52 touches blanches = 88 touches
    keyboard.setLowestVisibleKey (21);
}

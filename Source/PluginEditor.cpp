// Tobo Exciter (TE-02): the panel. Copyright (C) 2026 Tobokegao. Licensed under the GNU AGPL v3 or later.
#include "PluginEditor.h"
#include "BinaryData.h"

#ifndef JucePlugin_VersionString          // the test programs are not built as plug-ins
 #define JucePlugin_VersionString "test"
#endif

namespace
{
// Everything is laid out in units; one unit is Z pixels. The window is 448 x 672 units with the pedal's body at
// (PX, PY), 300 wide and 512 long: the same frame the pictures were rendered in.
const float Z100 = 1.15f;                    // pixels to a unit at 100 %
const float PX = 74.0f, PY = 132.0f;
const int UW = 448, UH = 672;

const juce::Colour INK   (0xff141416);
const juce::Colour WHITE (0xfff4f4f2);
const juce::Colour LAMP  (0xffe8281e);

const float KX[4] = { 73.5f, 226.5f, 73.5f, 226.5f };
const float KY[4] = { 57.5f, 57.5f, 161.5f, 161.5f };
const float KHALF = 35.0f;                   // half the side of a knob's picture (the knob itself is 29 in radius)
const float TRAVEL = 150.0f;                 // the knobs turn 150 degrees each way
const float TX[2] = { 78.0f, 370.0f };       // the level knobs above the pedal: input, output
const float TY = 62.0f, THALF = 23.0f;
const float MX[2] = { 26.0f, 414.0f };       // their meters
const float MY = 26.0f, MW = 8.0f, MH = 74.0f;
const juce::Rectangle<float> PRESET (122.0f, 50.0f, 204.0f, 24.0f), OSBOX (284.0f, 82.0f, 42.0f, 16.0f);   // as far from the level knobs as those are from their meters      // the oversampling, under the presets: its name at the left edge, its box at the right
// the four round buttons over the preset box: the size of the panel, a preset from a file, a preset to a file, about
const float BX[4] = { 122.0f, 174.0f, 226.0f, 278.0f };       // their left edges: four boxes across the preset box's width       // each with its name to the right; the four fill the preset box's width
const float BW = 48.0f, BH = 16.0f, BCORNER = 8.0f;
const float BY = 27.0f;
// The lit lamp's patch was cut from the render on whole pixels: 157 of them from (438, 841), at 1548 / 672 pixels to a
// unit. Drawn on exactly that place it lies on the picture under it to the pixel; rounded to 68 units from (190, 365)
// it was a third of a unit off, which showed at the sharp upper edge of the name plate.
const float PPU = 1548.0f / 672.0f;
const juce::Rectangle<float> LAMPBOX (438.0f / PPU, 841.0f / PPU, 157.0f / PPU, 157.0f / PPU);

// A line of capitals as a shape, its capitals `capH` high, its middle at (cx, cy). Narrowed to `maxW` if longer.
juce::Path lettering (const juce::String& s, float cx, float cy, float capH, bool bold, float track = 0.0f, float maxW = 0.0f)
{
    const juce::Font f = juce::Font (juce::FontOptions ("Bahnschrift", 100.0f, bold ? juce::Font::bold : juce::Font::plain))
                             .withExtraKerningFactor (track);
    juce::GlyphArrangement cap, ga;
    cap.addLineOfText (f, "H", 0.0f, 0.0f);
    juce::Path hp; cap.createPath (hp);
    const float k = capH / juce::jmax (1.0f, hp.getBounds().getHeight());
    ga.addLineOfText (f, s, 0.0f, 0.0f);
    juce::Path p; ga.createPath (p);
    const auto b = p.getBounds();
    float sx = k;
    if (maxW > 0.0f && b.getWidth() * k > maxW) sx = maxW / b.getWidth();
    p.applyTransform (juce::AffineTransform::translation (-b.getCentreX(), 0.0f).scaled (sx, k).translated (cx, cy + capH * 0.5f));
    return p;
}

void letter (juce::Graphics& g, const juce::String& s, float cx, float cy, float capH, juce::Colour c, bool bold = true,
             float track = 0.0f, float maxW = 0.0f)
{
    g.setColour (c);
    g.fillPath (lettering (s, cx, cy, capH, bold, track, maxW));
}

// a dark panel let into the ground: darker at the top (in the shade of its own rim), a light line under it
void inset (juce::Graphics& g, juce::Rectangle<float> r, float corner)
{
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.fillRoundedRectangle (r.translated (0.0f, 0.8f), corner);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0c0c0e), 0.0f, r.getY(), juce::Colour (0xff242428), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (0.25f), corner, 0.5f);
}

// a button standing on the ground: lighter than the ground, a soft shadow under it, a fine dark edge
void raised (juce::Graphics& g, juce::Rectangle<float> r, float corner)
{
    g.setColour (juce::Colours::black.withAlpha (0.10f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.6f).expanded (0.4f), corner + 0.4f);
    g.setColour (juce::Colours::black.withAlpha (0.16f));
    g.fillRoundedRectangle (r.translated (0.0f, 0.8f), corner);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffbfbfb), 0.0f, r.getY(), juce::Colour (0xffe6e6e8), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    g.setColour (INK.withAlpha (0.38f));
    g.drawRoundedRectangle (r.reduced (0.25f), corner, 0.5f);
}

float dbOf (float lin) { return lin > 1.0e-5f ? 20.0f * std::log10 (lin) : -100.0f; }
}

ExciterLook::ExciterLook (bool flatKnob) : flat (flatKnob)
{
    strip = juce::ImageCache::getFromMemory (BinaryData::knobs_png, BinaryData::knobs_pngSize);
    if (strip.isValid() && strip.getWidth() > 0) frames = juce::jmax (1, strip.getHeight() / strip.getWidth());
}

void ExciterLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&)
{
    if (flat)
    {
        // a flat knob: one dark disc and its white line, no light on it and no shadow
        const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
        const float r = (float) juce::jmin (w, h) * 0.5f * 0.76f, a = a0 + pos * (a1 - a0);
        g.setColour (INK);
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        juce::Path p;
        p.startNewSubPath (cx + r * 0.38f * std::sin (a), cy - r * 0.38f * std::cos (a));
        p.lineTo (cx + r * 0.80f * std::sin (a), cy - r * 0.80f * std::cos (a));
        g.setColour (WHITE);
        g.strokePath (p, juce::PathStrokeType (juce::jmax (1.6f, r * 0.13f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }
    if (! strip.isValid()) return;
    const int side = strip.getWidth();
    // The knob was rendered every 5 degrees. Shown as they are, a slow turn would jump from picture to picture (ten a
    // second). So the nearest picture is turned by what is left, at most 2.5 degrees: too little for its light to look wrong.
    const float at = juce::jlimit (0.0f, 1.0f, std::isfinite (pos) ? pos : 0.5f) * (float) (frames - 1);
    const int i = juce::jlimit (0, frames - 1, (int) std::lround (at));
    const float rest = (at - (float) i) * juce::degreesToRadians (2.0f * TRAVEL / (float) (frames - 1));
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    const float k = (float) w / (float) side;
    g.drawImageTransformed (strip.getClippedImage ({ 0, i * side, side, side }),
                            juce::AffineTransform::scale (k, (float) h / (float) side)
                                .rotated (rest, (float) w * 0.5f, (float) h * 0.5f).translated ((float) x, (float) y));
}

void ExciterSpot::paintButton (juce::Graphics& g, bool, bool down)
{
    if (down) { g.setColour (juce::Colours::black.withAlpha (0.14f)); g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f); }
}

ExciterEditor::ExciterEditor (ExciterProcessor& p) : juce::AudioProcessorEditor (p), proc (p)
{
    base = juce::ImageCache::getFromMemory (BinaryData::base_png, BinaryData::base_pngSize);
    lampOnImage = juce::ImageCache::getFromMemory (BinaryData::lamp_on_png, BinaryData::lamp_on_pngSize);
    static const char* ids[6] = { "frequency", "color", "balance", "multipeak", "input", "output" };
    for (int k = 0; k < 6; ++k)
    {
        auto& s = knob[k];
        s.setLookAndFeel (k < 4 ? &look : &flatLook);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::degreesToRadians (-TRAVEL), juce::degreesToRadians (TRAVEL), true);
        s.setMouseDragSensitivity (k < 4 ? 220 : 300);
        addAndMakeVisible (s);
        att[k] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ids[k], s);
        auto* q = proc.apvts.getParameter (ids[k]);
        s.setDoubleClickReturnValue (true, q->convertFrom0to1 (q->getDefaultValue()));
    }
    pedal.setClickingTogglesState (true);
    addAndMakeVisible (pedal);
    pedalAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "on", pedal);
    for (auto* b : { &prev, &next, &over, &settings, &importer, &exporter, &help }) addAndMakeVisible (*b);
    settings.setTooltip ("Panel size"); importer.setTooltip ("Load a preset from a file");
    exporter.setTooltip ("Save settings as a preset file"); help.setTooltip ("About");
    settings.onClick = [this]
    {
        ++dialogAsked; if (! dialogs) return;
        juce::PopupMenu m;
        m.addSectionHeader ("Panel size");
        static const int pct[] = { 75, 100, 125, 150, 200 };
        const float most = largestScale();      // sizes that would not fit on this screen are greyed out
        for (int v : pct) m.addItem (v, juce::String (v) + "%", (float) v / 100.0f <= most + 0.001f, std::abs (proc.uiScale * 100.0f - (float) v) < 1.0f);
        juce::Component::SafePointer<ExciterEditor> self (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settings),
                         [self] (int v) { if (self != nullptr && v > 0) self->setZoom ((float) v / 100.0f); });
    };
    const auto folder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Tobo Exciter Presets");
    importer.onClick = [this, folder]
    {
        ++dialogAsked; if (! dialogs) return;
        chooser = std::make_unique<juce::FileChooser> ("Load a preset", folder, "*.tepreset");
        juce::Component::SafePointer<ExciterEditor> self (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [self] (const juce::FileChooser& c)
                              {
                                  const auto f = c.getResult();
                                  if (self != nullptr && f.existsAsFile()) { self->proc.loadPreset (f); self->repaint(); }
                              });
    };
    exporter.onClick = [this, folder]
    {
        ++dialogAsked; if (! dialogs) return;
        folder.createDirectory();
        chooser = std::make_unique<juce::FileChooser> ("Save a preset", folder.getChildFile ("My preset.tepreset"), "*.tepreset");
        juce::Component::SafePointer<ExciterEditor> self (this);
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [self] (const juce::FileChooser& c)
                              {
                                  auto f = c.getResult();
                                  if (self == nullptr || f == juce::File()) return;
                                  f = f.withFileExtension ("tepreset");
                                  if (self->proc.savePreset (f)) { self->proc.filePresetName = f.getFileNameWithoutExtension(); self->repaint(); }
                              });
    };
    help.onClick = [this] { showAbout(); };
    addChildComponent (about);
    Z = Z100 * proc.uiScale;
    prev.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + ExciterProcessor::numPresets - 1) % ExciterProcessor::numPresets); };
    next.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % ExciterProcessor::numPresets); };
    over.onClick = [this]
    {
        if (auto* q = proc.apvts.getParameter ("oversampling"))
        {
            const int n = (int) std::lround (q->convertFrom0to1 (q->getValue()));
            q->beginChangeGesture();
            q->setValueNotifyingHost (q->convertTo0to1 ((float) ((n + 1) % 3)));
            q->endChangeGesture();
        }
    };
    // The window can be pulled larger or smaller by its corner, always in the same proportions.
    setResizable (true, true);
    if (auto* c = getConstrainer())
    {
        c->setFixedAspectRatio ((double) UW / (double) UH);
        c->setSizeLimits ((int) std::lround (UW * Z100 * 0.6f), (int) std::lround (UH * Z100 * 0.6f),
                          (int) std::lround (UW * Z100 * 2.0f), (int) std::lround (UH * Z100 * 2.0f));
    }
    setSize ((int) std::lround (UW * Z), (int) std::lround (UH * Z));
    lampOn = pedal.getToggleState();
    startTimerHz (30);
}

ExciterEditor::~ExciterEditor()
{
    stopTimer();
    for (auto& s : knob) s.setLookAndFeel (nullptr);
}

void ExciterEditor::timerCallback()
{
    const float in = dbOf (proc.peakIn.exchange (0.0f)), out = dbOf (proc.peakOut.exchange (0.0f));
    const float mi = juce::jmax (in, meterIn - 1.5f), mo = juce::jmax (out, meterOut - 1.5f);
    const bool meters = std::abs (mi - meterIn) > 0.05f || std::abs (mo - meterOut) > 0.05f;
    meterIn = juce::jmax (-100.0f, mi); meterOut = juce::jmax (-100.0f, mo);
    const bool on = pedal.getToggleState();
    const float i = (float) knob[4].getValue(), o = (float) knob[5].getValue();
    const int pg = proc.getCurrentProgram();
    const bool renamed = proc.filePresetName != shownName;
    shownName = proc.filePresetName;
    const int osn = (int) std::lround (proc.apvts.getRawParameterValue ("oversampling")->load());
    if (on != lampOn || i != shownIn || o != shownOut || pg != shownProgram || osn != shownOs || renamed)
    {
        lampOn = on; shownIn = i; shownOut = o; shownProgram = pg; shownOs = osn;
        repaint();
    }
    else if (meters)
    {
        for (int k = 0; k < 2; ++k)
            repaint (juce::Rectangle<float> ((MX[k] - 2.0f) * Z, (MY - 2.0f) * Z, (MW + 4.0f) * Z, (MH + 4.0f) * Z).toNearestInt());
    }
}

void ExciterEditor::resized()
{
    // everything follows the window's width; the size is kept with the session
    if (getWidth() <= 0 || getHeight() <= 0) return;
    Z = (float) getWidth() / (float) UW;
    proc.uiScale = juce::jlimit (0.6f, 2.0f, Z / Z100);
    auto place = [this] (juce::Component& c, float cx, float cy, float half)
    {
        c.setBounds (juce::Rectangle<float> ((cx - half) * Z, (cy - half) * Z, half * 2.0f * Z, half * 2.0f * Z).toNearestInt());
    };
    auto box = [this] (juce::Component& c, juce::Rectangle<float> r) { c.setBounds ((r * Z).toNearestInt()); };
    for (int k = 0; k < 4; ++k) place (knob[k], PX + KX[k], PY + KY[k], KHALF);
    place (knob[4], TX[0], TY, THALF);
    place (knob[5], TX[1], TY, THALF);
    box (pedal, { PX + 111.0f, PY + 407.0f, 78.0f, 115.0f });
    box (prev, PRESET.withWidth (24.0f));
    box (next, PRESET.withLeft (PRESET.getRight() - 24.0f));
    box (over, OSBOX);
    ExciterSpot* round[4] = { &settings, &importer, &exporter, &help };
    for (int k = 0; k < 4; ++k) box (*round[k], { BX[k], BY - BH * 0.5f, BW, BH });
    about.zoom = Z;
    about.setBounds (getLocalBounds());
}

void ExciterEditor::paint (juce::Graphics& g)
{
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (base, getLocalBounds().toFloat());
    g.addTransform (juce::AffineTransform::scale (Z));
    if (lampOn && lampOnImage.isValid())
    {
        g.drawImage (lampOnImage, LAMPBOX);
        // the lamp's light on what lies round it
        const float lx = LAMPBOX.getCentreX(), ly = LAMPBOX.getCentreY();
        juce::ColourGradient glow (LAMP.withAlpha (0.55f), lx, ly, LAMP.withAlpha (0.0f), lx + 34.0f, ly, true);
        glow.addColour (0.25, LAMP.withAlpha (0.30f));
        g.setGradientFill (glow);
        g.fillEllipse (lx - 34.0f, ly - 34.0f, 68.0f, 68.0f);
    }
    // above the pedal: the level knobs' names, values and meters, the presets, the oversampling
    letter (g, "INPUT", TX[0], 22.0f, 7.4f, INK);
    letter (g, "OUTPUT", TX[1], 22.0f, 7.4f, INK);
    auto db = [] (float v) { return juce::String (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB"; };
    letter (g, db ((float) knob[4].getValue()), TX[0], 96.0f, 6.4f, INK);
    letter (g, db ((float) knob[5].getValue()), TX[1], 96.0f, 6.4f, INK);

    // the meters: -60 dB at the foot of the slot, 0 dBFS at nine tenths of it, red above
    const float lv[2] = { meterIn, meterOut };
    for (int k = 0; k < 2; ++k)
    {
        inset (g, { MX[k], MY, MW, MH }, 2.5f);
        const float t = juce::jlimit (0.0f, 1.0f, (lv[k] + 60.0f) / 66.0f);
        if (t <= 0.0f) continue;
        const float h = (MH - 3.0f) * t, zero = (MH - 3.0f) * (60.0f / 66.0f);
        g.setColour (juce::Colour (0xffe9e9eb));
        g.fillRoundedRectangle (MX[k] + 1.5f, MY + MH - 1.5f - juce::jmin (h, zero), MW - 3.0f, juce::jmin (h, zero), 1.2f);
        if (h > zero)
        {
            g.setColour (LAMP);
            g.fillRoundedRectangle (MX[k] + 1.5f, MY + MH - 1.5f - h, MW - 3.0f, h - zero, 1.2f);
        }
    }

    inset (g, PRESET, 5.0f);
    inset (g, OSBOX, 5.0f);
    {
        // its first letter stands under the left edge of the preset box; a dashed line leads from it to its box
        auto name = lettering ("OVERSAMPLING", 0.0f, OSBOX.getCentreY(), 6.6f, true);
        name.applyTransform (juce::AffineTransform::translation (PRESET.getX() - name.getBounds().getX(), 0.0f));
        g.setColour (INK);
        g.fillPath (name);
        juce::Path lead, dashed;
        lead.startNewSubPath (name.getBounds().getRight() + 6.0f, OSBOX.getCentreY());
        lead.lineTo (OSBOX.getX() - 6.0f, OSBOX.getCentreY());
        const float dash[2] = { 3.0f, 3.0f };
        juce::PathStrokeType (1.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded).createDashedStroke (dashed, lead, dash, 2);
        g.setColour (INK.withAlpha (0.5f));
        g.fillPath (dashed);
    }
    // The four buttons do something; the dark boxes show a value. So they must not look alike: the boxes are let
    // into the ground (dark, shaded at the top), the buttons stand on it (light, a shadow under them, dark signs).
    for (int k = 0; k < 4; ++k)
    {
        const juce::Rectangle<float> r (BX[k], BY - BH * 0.5f, BW, BH);
        raised (g, r, BCORNER);
        static const char* const title[4] = { "SIZE", "LOAD", "SAVE", "INFO" };
        // the sign (a square of 8) and the name, 3.5 apart, stand as one in the middle of the button
        const float cy = BY, sign = 8.0f, gap = 3.5f;
        g.setColour (INK);
        auto word = lettering (title[k], 0.0f, cy, 5.2f, true, 0.02f);
        const float x0 = r.getCentreX() - (sign + gap + word.getBounds().getWidth()) * 0.5f;
        const float cx = x0 + sign * 0.5f;
        word.applyTransform (juce::AffineTransform::translation (x0 + sign + gap - word.getBounds().getX(), 0.0f));
        g.fillPath (word);
        const juce::PathStrokeType pen (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        juce::Path s;
        if (k == 0)             // resize: an arrow to the upper left corner and one to the lower right (the corner the window is pulled by)
        {
            const float q = 4.0f, h = 2.6f;
            s.startNewSubPath (cx - q, cy - q); s.lineTo (cx + q, cy + q);
            s.startNewSubPath (cx - q + h, cy - q); s.lineTo (cx - q, cy - q); s.lineTo (cx - q, cy - q + h);
            s.startNewSubPath (cx + q - h, cy + q); s.lineTo (cx + q, cy + q); s.lineTo (cx + q, cy + q - h);
            g.strokePath (s, pen);
        }
        else if (k == 1 || k == 2)      // an arrow down into a tray (from a file), an arrow up out of it (to a file)
        {
            const float d = k == 1 ? 1.0f : -1.0f;
            const float from = k == 1 ? cy - 4.4f : cy + 1.2f, to = k == 1 ? cy + 1.2f : cy - 4.4f;
            s.startNewSubPath (cx, from); s.lineTo (cx, to);
            s.startNewSubPath (cx - 2.2f, to - 2.2f * d); s.lineTo (cx, to); s.lineTo (cx + 2.2f, to - 2.2f * d);
            s.startNewSubPath (cx - 4.0f, cy + 1.8f); s.lineTo (cx - 4.0f, cy + 4.2f); s.lineTo (cx + 4.0f, cy + 4.2f); s.lineTo (cx + 4.0f, cy + 1.8f);
            g.strokePath (s, pen);
        }
        else                    // a question mark in a ring, as wide as it is high like the other signs
        {
            g.drawEllipse (cx - 3.9f, cy - 3.9f, 7.8f, 7.8f, 1.1f);
            g.fillPath (lettering ("?", cx, cy, 4.4f, true));
        }
    }
    for (int k = 0; k < 2; ++k)
    {
        const float cx = k == 0 ? PRESET.getX() + 12.0f : PRESET.getRight() - 12.0f, cy = PRESET.getCentreY(), d = k == 0 ? -1.0f : 1.0f;
        juce::Path a;
        a.startNewSubPath (cx - 2.5f * d, cy - 5.0f); a.lineTo (cx + 2.5f * d, cy); a.lineTo (cx - 2.5f * d, cy + 5.0f);
        g.setColour (WHITE);
        g.strokePath (a, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    letter (g, proc.filePresetName.isNotEmpty() ? proc.filePresetName : proc.getProgramName (proc.getCurrentProgram()), PRESET.getCentreX(), PRESET.getCentreY(), 7.6f, WHITE, false, 0.03f, 108.0f);
    static const char* osn[3] = { "2x", "4x", "8x" };
    letter (g, osn[juce::jlimit (0, 2, (int) std::lround (proc.apvts.getRawParameterValue ("oversampling")->load()))],
            OSBOX.getCentreX(), OSBOX.getCentreY(), 7.0f, WHITE);
}

// A window taller than the screen is cut short by the host; the panel then shrinks to keep its proportions and
// leaves an empty strip beside it. So no size is offered that does not fit: the screen's height less the host's
// title and the bar it puts above the panel (about 60 pixels).
float ExciterEditor::largestScale() const
{
    const auto* d = juce::Desktop::getInstance().getDisplays().getDisplayForRect (getScreenBounds());
    if (d == nullptr) return 2.0f;
    return juce::jlimit (0.6f, 2.0f, (float) (d->userArea.getHeight() - 60) / ((float) UH * Z100));
}

void ExciterEditor::setZoom (float scale)
{
    proc.uiScale = std::isfinite (scale) ? juce::jlimit (0.6f, largestScale(), scale) : 1.0f;
    Z = Z100 * proc.uiScale;
    setSize ((int) std::lround (UW * Z), (int) std::lround (UH * Z));
    resized();
    repaint();
}

void ExciterAbout::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.55f));
    g.addTransform (juce::AffineTransform::scale (zoom));
    const juce::Rectangle<float> card (44.0f, 112.0f, 360.0f, 420.0f);
    g.setColour (juce::Colour (0xff17171a));
    g.fillRoundedRectangle (card, 10.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (card.reduced (0.5f), 10.0f, 1.0f);
    const juce::Colour orange (0xffe9a070);
    letter (g, "TOBO EXCITER", card.getCentreX(), card.getY() + 30.0f, 12.0f, WHITE, true, 0.06f);
    letter (g, juce::String ("TE-02    VERSION ") + JucePlugin_VersionString, card.getCentreX(), card.getY() + 52.0f, 5.6f, orange, true, 0.08f);
    static const char* const lines[][2] = {
        { "", "A 1980s exciter pedal as a plug-in, modeled on hardware measurements. It adds a bright, biting edge above the note. Turned up, it sings at a single pitch of its own." },
        { "FREQUENCY", "Where the filter begins, from 600 Hz to 7 kHz." },
        { "COLOR", "How sharp the filter's peak is. All the way up, it rings." },
        { "BALANCE", "Mixes the direct and filtered sound." },
        { "MULTIPEAK", "Adds six fixed peaks between 1 and 5 kHz." },
        { "INPUT / OUTPUT", "Levels in and out. INPUT sets how hard it is driven." },
        { "FOOT SWITCH", "Click to turn the effect on and off. Fades with no pop." },
        { "OVERSAMPLING", "2x, 4x, or 8x. Higher is cleaner, but heavier on the processor." },
        { "BUTTONS", "Resize panel, load a preset, save a preset, show this page." },
        { "", "Double-click a knob to reset it. Click anywhere to close." } };
    float y = card.getY() + 78.0f;
    const juce::Font f (juce::FontOptions ("Bahnschrift", 10.6f, juce::Font::plain));
    for (auto& ln : lines)
    {
        const bool head = ln[0][0] != 0;
        if (head)
        {
            auto p = lettering (ln[0], 0.0f, y + 6.0f, 5.6f, true, 0.05f);
            p.applyTransform (juce::AffineTransform::translation (card.getX() + 22.0f - p.getBounds().getX(), 0.0f));
            g.setColour (orange);
            g.fillPath (p);
        }
        const juce::Rectangle<float> r (card.getX() + (head ? 112.0f : 22.0f), y, card.getWidth() - (head ? 134.0f : 44.0f), head ? 26.0f : 40.0f);
        g.setColour (WHITE.withAlpha (0.92f));
        g.setFont (f);
        g.drawFittedText (ln[1], r.toNearestInt(), juce::Justification::topLeft, 3, 1.0f);
        y += head ? 30.0f : 44.0f;
    }
    g.setColour (WHITE.withAlpha (0.6f));
    g.setFont (juce::Font (juce::FontOptions ("Bahnschrift", 8.4f, juce::Font::plain)));
    g.drawFittedText ("An independent work, not affiliated with or endorsed by any hardware maker.",
                      juce::Rectangle<float> (card.getX() + 10.0f, card.getBottom() - 38.0f, card.getWidth() - 20.0f, 12.0f).toNearestInt(), juce::Justification::centred, 1, 1.0f);
    letter (g, "(C) 2026 TOBOKEGAO", card.getCentreX(), card.getBottom() - 15.0f, 5.0f, WHITE.withAlpha (0.6f), true, 0.1f);
}

void ExciterEditor::poke (float x, float y, int phase, float wheel)
{
    const juce::Point<float> at (x, y);
    juce::Component* c = phase == 1 || phase == 2 ? poked.getComponent() : getComponentAt (at.toInt());
    if (c == nullptr) return;
    if (phase == 0 || phase == 3) poked = c;
    const auto local = c->getLocalPoint (this, at);
    const auto now = juce::Time::getCurrentTime();
    const juce::ModifierKeys mods (phase == 2 || phase == 4 ? 0 : juce::ModifierKeys::leftButtonModifier);
    const juce::MouseEvent e (juce::Desktop::getInstance().getMainMouseSource(), local, mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                              c, c, now, local, now, phase == 3 ? 2 : 1, phase == 1);
    if (phase == 0) c->mouseDown (e);
    else if (phase == 1) c->mouseDrag (e);
    else if (phase == 2) { c->mouseUp (e); poked = nullptr; }
    else if (phase == 3) { c->mouseDown (e); c->mouseDoubleClick (e); c->mouseUp (e); poked = nullptr; }
    else { juce::MouseWheelDetails w; w.deltaX = 0.0f; w.deltaY = wheel; w.isReversed = false; w.isSmooth = false; w.isInertial = false; c->mouseWheelMove (e, w); }
}

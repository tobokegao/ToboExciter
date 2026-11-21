// Tobo Exciter (TE-02): tests without a host or a screen. Copyright (C) 2026 Tobokegao. Licensed under the GNU AGPL v3 or later.
// First what a user does in the ordinary way (every control does what it says), then what a user or a host is not
// supposed to do, done on purpose: odd sample rates and block sizes, numbers that are not numbers, broken sessions
// and preset files, a mouse that presses, drags and lets go anywhere, the panel opened and closed while the sound runs.
// Every check prints a line; the last line says how many failed.   usage: StressTest [folder for its files]
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <iostream>
#include <thread>

static int failed = 0, checks = 0;
static void check (bool ok, const juce::String& what)
{
    ++checks; if (! ok) ++failed;
    std::cout << (ok ? "ok    " : "FAIL  ") << what << std::endl;
}

static const char* const ids[] = { "frequency", "color", "balance", "multipeak", "input", "output", "on", "bypass", "oversampling" };
static const int numIds = 9;
static const double kPi = 3.141592653589793;

static float getp (ExciterProcessor& p, const char* id) { return p.apvts.getRawParameterValue (id)->load(); }
static void setp (ExciterProcessor& p, const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); }
static void defaults (ExciterProcessor& p) { for (auto* id : ids) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->getDefaultValue()); } p.applyPendingNow(); }
static void start (ExciterProcessor& p, double sr, int size, int ch = 2)
{
    p.setPlayConfigDetails (ch, ch, sr, size); p.prepareToPlay (sr, size); defaults (p);
}

struct Stat { bool finite = true; double peak = 0, energy = 0; long n = 0; double rms() const { return std::sqrt (energy / (double) juce::jmax (1L, n)); } };

// noise through the plug-in
static Stat noise (ExciterProcessor& p, juce::Random& rng, int blocks, int size, double level = 0.25, int ch = 2)
{
    juce::AudioBuffer<float> b (ch, juce::jmax (1, size)); juce::MidiBuffer m; Stat s;
    for (int n = 0; n < blocks; ++n)
    {
        b.setSize (ch, size, false, false, true);
        for (int c = 0; c < ch; ++c) for (int i = 0; i < size; ++i) b.setSample (c, i, (float) (level * (rng.nextDouble() * 2 - 1)));
        p.processBlock (b, m);
        for (int c = 0; c < ch; ++c) for (int i = 0; i < size; ++i)
        {
            const double v = b.getSample (c, i);
            if (! std::isfinite (v)) s.finite = false;
            s.peak = juce::jmax (s.peak, std::abs (v)); s.energy += v * v; ++s.n;
        }
    }
    return s;
}

// a sine through the plug-in for `seconds`; measures what comes out after the first half (level 0 = silence in)
static Stat tone (ExciterProcessor& p, double sr, double freq, double level, double seconds = 1.0, double* inRms = nullptr)
{
    juce::AudioBuffer<float> b (2, 256); juce::MidiBuffer m; Stat s; double ph = 0, ein = 0; long nin = 0;
    const int blocks = (int) (sr * seconds / 256);
    for (int n = 0; n < blocks; ++n)
    {
        for (int i = 0; i < 256; ++i) { ph += 2 * kPi * freq / sr; const float v = (float) (level * std::sin (ph)); b.setSample (0, i, v); b.setSample (1, i, v); }
        const bool count = n >= blocks / 2;
        if (count) for (int i = 0; i < 256; ++i) { ein += b.getSample (0, i) * b.getSample (0, i); ++nin; }
        p.processBlock (b, m);
        for (int i = 0; i < 256; ++i)
        {
            const double v = b.getSample (0, i);
            if (! std::isfinite (v)) s.finite = false;
            if (count) { s.peak = juce::jmax (s.peak, std::abs (v)); s.energy += v * v; ++s.n; }
        }
    }
    if (inRms != nullptr) *inRms = std::sqrt (ein / (double) juce::jmax (1L, nin));
    return s;
}
static double dB (double a, double b) { return 20 * std::log10 ((a + 1e-30) / (b + 1e-30)); }

static bool paramsSane (ExciterProcessor& p, juce::String& why)
{
    static const float lo[] = { 0, 0, 0, 0, -30, -30, 0, 0, 0 }, hi[] = { 10, 10, 10, 10, 30, 30, 1, 1, 2 };
    for (int k = 0; k < numIds; ++k)
    {
        const float v = getp (p, ids[k]);
        if (! std::isfinite (v) || v < lo[k] - 1e-3f || v > hi[k] + 1e-3f) { why = juce::String (ids[k]) + " = " + juce::String (v); return false; }
    }
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// the panel, in units (448 x 672); a pixel is a unit times 1.15 times the panel's size
namespace at
{
const float knobX[6] = { 147.5f, 300.5f, 147.5f, 300.5f, 78.0f, 370.0f }, knobY[6] = { 189.5f, 189.5f, 293.5f, 293.5f, 62.0f, 62.0f };
const float pedalX = 224.0f, pedalY = 596.0f, prevX = 134.0f, nextX = 314.0f, presetY = 62.0f, osX = 305.0f, osY = 90.0f;
const float buttonX[4] = { 146.0f, 198.0f, 250.0f, 302.0f }, buttonY = 27.0f;
}

struct Panel
{
    ExciterProcessor& p; std::unique_ptr<ExciterEditor> ed; float z;
    explicit Panel (ExciterProcessor& proc) : p (proc), ed (dynamic_cast<ExciterEditor*> (proc.createEditor())) { ed->dialogs = false; ed->setVisible (true); z = (float) ed->getWidth() / 448.0f; }
    void press (float ux, float uy) { ed->poke (ux * z, uy * z, 0); }
    void move (float ux, float uy) { ed->poke (ux * z, uy * z, 1); }
    void up (float ux, float uy) { ed->poke (ux * z, uy * z, 2); }
    void click (float ux, float uy) { press (ux, uy); up (ux, uy); }
    void twice (float ux, float uy) { ed->poke (ux * z, uy * z, 3); }
    void drag (float ux, float uy, float dy, int steps = 8) { press (ux, uy); for (int i = 1; i <= steps; ++i) move (ux, uy + dy * (float) i / (float) steps); up (ux, uy + dy); }
    void draw() { ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f); }
};

// ---------------------------------------------------------------------------------------------------------------------
static void ordinary (const juce::File& dir)
{
    juce::Random rng (3);
    std::cout << "\n== ordinary use: the sound ==\n";
    {
        ExciterProcessor p; start (p, 48000, 256);
        double in = 0;
        const Stat on = tone (p, 48000, 440, 0.1, 1.0, &in);
        setp (p, "on", 0); tone (p, 48000, 440, 0.1, 0.3);
        const Stat off = tone (p, 48000, 440, 0.1, 1.0, &in);
        check (on.finite && std::abs (dB (on.rms(), off.rms())) > 0.5, "the effect changes the sound (on against off: " + juce::String (dB (on.rms(), off.rms()), 2) + " dB at 440 Hz)");
        juce::String levels; bool flat = true;
        for (double f : { 50.0, 440.0, 3000.0, 10000.0, 16000.0 })
        {
            const Stat s = tone (p, 48000, f, 0.1, 0.6, &in); const double d = dB (s.rms(), in);
            levels += " " + juce::String (f, 0) + " Hz " + juce::String (d, 2) + " dB;"; if (std::abs (d) > 0.2) flat = false;
        }
        check (flat, "switched off, the sound passes at its own level:" + levels);
        setp (p, "on", 1);
        const Stat quiet = tone (p, 48000, 440, 0.0, 2.0);
        check (quiet.finite && quiet.peak < 1e-6, "silence in gives silence out (peak " + juce::String (quiet.peak, 9) + ")");
        for (auto* id : { "frequency", "color", "balance", "multipeak" }) setp (p, id, 10);
        tone (p, 48000, 440, 0.25, 0.5);
        const Stat ring = tone (p, 48000, 440, 0.0, 3.0);
        check (ring.finite && ring.peak < 1e-5, "all knobs fully right, then silence: it dies away, it does not sing by itself (peak " + juce::String (ring.peak, 9) + ")");
        setp (p, "input", 30); setp (p, "output", 0);
        const Stat hot = noise (p, rng, 200, 256, 1.0);
        check (hot.finite && hot.peak < 4.0, "full-scale noise driven 30 dB too hard: the output stays bounded (peak " + juce::String (hot.peak, 3) + ")");
        check (p.getLatencySamples() >= 0 && p.getLatencySamples() < 64, "the delay told to the host: " + juce::String (p.getLatencySamples()) + " samples");
    }
    {
        ExciterProcessor p; start (p, 48000, 256);
        bool each = true; juce::String why;
        for (auto* id : { "frequency", "color", "balance", "multipeak" })
        {
            defaults (p); const Stat a = tone (p, 48000, 1500, 0.1, 0.6);
            setp (p, id, 9.5f); const Stat b = tone (p, 48000, 1500, 0.1, 0.6);
            setp (p, id, 0.5f); const Stat c = tone (p, 48000, 1500, 0.1, 0.6);
            const double d = juce::jmax (std::abs (dB (b.rms(), a.rms())), std::abs (dB (c.rms(), a.rms())));
            why += " " + juce::String (id) + " " + juce::String (d, 2) + " dB;"; if (d < 0.3) each = false;
        }
        check (each, "each of the four knobs changes the sound:" + why);
        defaults (p); const Stat a = tone (p, 48000, 440, 0.05, 0.5);
        setp (p, "output", 6.0f + 4.9f); const Stat b = tone (p, 48000, 440, 0.05, 0.5);
        check (std::abs (dB (b.rms(), a.rms()) - 6.0) < 0.1, "OUTPUT 6 dB up is 6 dB louder (" + juce::String (dB (b.rms(), a.rms()), 2) + " dB)");
        defaults (p);
        bool osOk = true; juce::String osWhy;
        for (int o = 0; o < 3; ++o)
        {
            setp (p, "oversampling", (float) o); p.applyPendingNow();
            const Stat s = tone (p, 48000, 440, 0.1, 0.5);
            osWhy += " " + juce::String (p.oversamplingFactor()) + "x rms " + juce::String (s.rms(), 4) + ";";
            if (! s.finite || p.oversamplingFactor() != (2 << o) || s.rms() < 0.01) osOk = false;
        }
        check (osOk, "2x, 4x and 8x all run and sound:" + osWhy);
        ExciterProcessor mono; start (mono, 44100, 128, 1);
        check (noise (mono, rng, 200, 128, 0.25, 1).finite, "one channel in, one out: runs");
    }

    std::cout << "\n== ordinary use: the panel ==\n";
    for (float scale : { 1.0f, 0.6f, 1.5f, 2.0f })
    {
        ExciterProcessor p; start (p, 48000, 256); p.uiScale = scale;
        Panel g (p); const juce::String zs = juce::String ((int) (scale * 100)) + " %: ";
        check (std::abs (g.ed->getWidth() - 448 * 1.15f * scale) < 1.5f && std::abs (g.ed->getHeight() - 672 * 1.15f * scale) < 1.5f,
               zs + "the window is " + juce::String (g.ed->getWidth()) + " x " + juce::String (g.ed->getHeight()));
        g.draw();
        bool knobs = true; juce::String why;
        for (int k = 0; k < 6; ++k)
        {
            const float v0 = getp (p, ids[k]);
            g.drag (at::knobX[k], at::knobY[k], -30.0f / g.z); const float up = getp (p, ids[k]);
            g.drag (at::knobX[k], at::knobY[k], 60.0f / g.z); const float down = getp (p, ids[k]);
            g.twice (at::knobX[k], at::knobY[k]); const float back = getp (p, ids[k]);
            if (! (up > v0 && down < up && std::abs (back - v0) < 0.011f)) { knobs = false; why += " " + juce::String (ids[k]) + ": " + juce::String (v0) + " up " + juce::String (up) + " down " + juce::String (down) + " double click " + juce::String (back) + ";"; }
            for (int i = 0; i < 6; ++i) if (i != k && std::abs (getp (p, ids[i]) - p.apvts.getParameter (ids[i])->convertFrom0to1 (p.apvts.getParameter (ids[i])->getDefaultValue())) > 0.011f) { knobs = false; why += " dragging " + juce::String (ids[k]) + " moved " + ids[i] + ";"; }
        }
        check (knobs, zs + "each knob: dragged up it rises, dragged down it falls, a double click puts it back, no other moves" + why);
        g.click (at::pedalX, at::pedalY); const bool off = getp (p, "on") < 0.5f;
        g.click (at::pedalX, at::pedalY); const bool on = getp (p, "on") > 0.5f;
        check (off && on, zs + "a click on the foot switch turns the effect off, the next one on");
        bool pres = true; juce::String pw;
        for (int n = 1; n <= ExciterProcessor::numPresets; ++n)
        {
            g.click (at::nextX, at::presetY);
            const int want = n % ExciterProcessor::numPresets; const auto& q = ExciterProcessor::presets[want];
            if (p.getCurrentProgram() != want || std::abs (getp (p, "frequency") - q.frequency) > 0.011f || std::abs (getp (p, "color") - q.color) > 0.011f
                || std::abs (getp (p, "balance") - q.balance) > 0.011f || std::abs (getp (p, "multipeak") - q.multipeak) > 0.011f) { pres = false; pw += " after " + juce::String (n) + " clicks: " + juce::String (p.getCurrentProgram()) + ";"; }
        }
        g.click (at::prevX, at::presetY);
        check (pres && p.getCurrentProgram() == ExciterProcessor::numPresets - 1, zs + "the right arrow goes through the eight presets and round again, the left one goes back; each sets its four knobs" + pw);
        const float in0 = getp (p, "input");
        bool os = true;
        for (int n = 0; n < 3; ++n) { const int was = (int) getp (p, "oversampling"); g.click (at::osX, at::osY); p.applyPendingNow(); if ((int) getp (p, "oversampling") != (was + 1) % 3) os = false; }
        check (os && getp (p, "input") == in0, zs + "a click on the oversampling box goes 2x, 4x, 8x and round");
        int asked = g.ed->dialogAsked;
        for (int b = 0; b < 3; ++b) g.click (at::buttonX[b], at::buttonY);
        check (g.ed->dialogAsked == asked + 3, zs + "SIZE, LOAD and SAVE each answer a click");
        g.click (at::buttonX[3], at::buttonY); g.draw();
        juce::Component* over = g.ed->getComponentAt ((int) (224 * g.z), (int) (300 * g.z));
        const bool shown = dynamic_cast<ExciterAbout*> (over) != nullptr;
        g.click (224, 300); g.draw();
        check (shown && dynamic_cast<ExciterAbout*> (g.ed->getComponentAt ((int) (224 * g.z), (int) (300 * g.z))) == nullptr, zs + "INFO opens the page; a click on it puts it away");
        noise (p, rng, 20, 256);
        check (p.peakIn.load() > 0.0f && p.peakOut.load() > 0.0f, zs + "the meters get their levels");
    }
    {
        ExciterProcessor p; start (p, 48000, 256); Panel g (p);
        const float want = juce::jmin (1.5f, g.ed->largestScale());
        g.ed->setZoom (1.5f); const int w = g.ed->getWidth();
        g.ed->setSize (600, 900);
        check (std::abs (w - 448 * 1.15f * want) < 1.5f && std::abs (p.uiScale - 600.0f / 448.0f / 1.15f) < 0.01f, "the size from the menu (150 %, or the largest that fits this screen = " + juce::String (w) + " wide) and a pull on the corner (600 wide = " + juce::String (p.uiScale * 100, 0) + " %) are both kept");
    }

    std::cout << "\n== ordinary use: sessions and preset files ==\n";
    {
        ExciterProcessor a; start (a, 48000, 256);
        static const float vals[] = { 1.23f, 8.76f, 3.3f, 6.6f, -12.5f, 7.5f, 0, 0, 2 };
        for (int k = 0; k < numIds; ++k) setp (a, ids[k], vals[k]);
        a.setCurrentProgram (5); for (int k = 0; k < 4; ++k) setp (a, ids[k], vals[k]);
        a.uiScale = 1.25f;
        juce::MemoryBlock mb; a.getStateInformation (mb);
        ExciterProcessor b; start (b, 48000, 256); b.setStateInformation (mb.getData(), (int) mb.getSize());
        bool same = b.getCurrentProgram() == 5 && std::abs (b.uiScale - 1.25f) < 1e-4f; juce::String why;
        for (int k = 0; k < numIds; ++k) if (std::abs (getp (b, ids[k]) - vals[k]) > 0.011f) { same = false; why += " " + juce::String (ids[k]) + " " + juce::String (getp (b, ids[k])) + ";"; }
        check (same, "a saved session comes back: every control, the preset, the panel's size" + why);

        const juce::File f = dir.getChildFile ("My test preset.tepreset"); f.deleteFile();
        const bool saved = a.savePreset (f);
        ExciterProcessor c; start (c, 48000, 256);
        const bool loaded = c.loadPreset (f);
        bool back = saved && loaded && c.filePresetName == "My test preset"; why = {};
        for (auto* id : { "frequency", "color", "balance", "multipeak", "input", "output", "oversampling" })
            if (std::abs (getp (c, id) - getp (a, id)) > 0.011f) { back = false; why += " " + juce::String (id) + ";"; }
        check (back && getp (c, "on") > 0.5f, "a preset saved to a file comes back from it: four knobs, both levels, oversampling; the foot switch is left alone" + why);
        c.setCurrentProgram (2);
        check (c.filePresetName.isEmpty(), "choosing a built-in preset afterwards drops the file's name");
    }
}

// ---------------------------------------------------------------------------------------------------------------------
static void abuse (const juce::File& dir)
{
    juce::Random rng (11);
    std::cout << "\n== sample rates and block sizes, the controls changed at random every block ==\n";
    for (double sr : { 8000.0, 11025.0, 22050.0, 44100.0, 48000.0, 88200.0, 96000.0, 192000.0, 384000.0 })
    {
        bool fin = true; double peak = 0;
        for (int size : { 1, 2, 7, 64, 441, 512, 4096 })
        {
            ExciterProcessor p; start (p, sr, size);
            for (int n = 0; n < juce::jmax (40, 20000 / size); ++n)
            {
                for (auto* id : ids) if (rng.nextInt (3) == 0) p.apvts.getParameter (id)->setValueNotifyingHost (rng.nextFloat());
                setp (p, "output", juce::jmin (0.0f, getp (p, "output")));
                p.applyPendingNow();
                const Stat s = noise (p, rng, 1, size, 0.5);
                fin = fin && s.finite; peak = juce::jmax (peak, s.peak);
            }
        }
        check (fin && peak < 4.0, juce::String (sr, 0) + " Hz, blocks of 1 to 4096: finite, peak " + juce::String (peak, 3));
    }
    {
        ExciterProcessor p; start (p, 48000, 64);
        bool fin = true;
        for (int n = 0; n < 400; ++n) fin = fin && noise (p, rng, 1, 1 + rng.nextInt (3000)).finite;
        juce::AudioBuffer<float> none (2, 0); juce::MidiBuffer m; p.processBlock (none, m);
        check (fin, "blocks far longer than the host announced (up to 3000 for 64), and an empty block: no crash, finite");
        ExciterProcessor q; juce::AudioBuffer<float> b (2, 256); b.clear(); q.processBlock (b, m);
        check (true, "asked for sound before it was set up: no crash");
        for (int n = 0; n < 50; ++n) { q.setPlayConfigDetails (2, 2, n % 2 ? 44100 : 96000, 32 << (n % 5)); q.prepareToPlay (n % 2 ? 44100 : 96000, 32 << (n % 5)); noise (q, rng, 2, 32 << (n % 5)); if (n % 3 == 0) q.releaseResources(); }
        check (noise (q, rng, 5, 32).finite, "set up 50 times over at changing rates and block sizes: runs");
    }

    std::cout << "\n== numbers that are not numbers ==\n";
    {
        ExciterProcessor p; start (p, 48000, 256);
        for (auto* id : { "frequency", "color", "balance", "multipeak" }) setp (p, id, 10);
        juce::AudioBuffer<float> b (2, 256); juce::MidiBuffer m; bool fin = true;
        const float bad[] = { std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), 1e30f, -1e30f, 3.0e38f };
        for (float v : bad)
        {
            for (int n = 0; n < 4; ++n) { for (int i = 0; i < 256; ++i) { b.setSample (0, i, i % 3 == 0 ? v : 0.1f); b.setSample (1, i, v); } p.processBlock (b, m); for (int i = 0; i < 256; ++i) if (! std::isfinite (b.getSample (0, i)) || ! std::isfinite (b.getSample (1, i))) { if (fin) std::cout << "      not finite out for " << v << std::endl; fin = false; } }
            const Stat s = tone (p, 48000, 3000, 0.1, 0.5);
            if (! s.finite || s.rms() < 0.01) { fin = false; std::cout << "      after " << v << ": finite " << s.finite << ", rms " << s.rms() << std::endl; }
        }
        check (fin, "NaN, infinity and 1e30 in the input: what comes out is finite, and afterwards it plays again");
        bool pf = true;
        for (auto* id : ids)
        {
            defaults (p);
            for (float v : { std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -5.0f, 7.0f })
            {
                p.apvts.getParameter (id)->setValueNotifyingHost (v); p.applyPendingNow();
                pf = pf && noise (p, rng, 4, 256).finite;
            }
            defaults (p); pf = pf && tone (p, 48000, 440, 0.1, 0.4).rms() > 0.01;
        }
        check (pf, "NaN, infinity and values outside 0..1 sent to each control by the host: finite, and it plays again");
    }

    std::cout << "\n== broken sessions and preset files ==\n";
    {
        ExciterProcessor p; start (p, 48000, 256); juce::String why; bool ok = true;
        juce::MemoryBlock good; p.getStateInformation (good);
        for (int n = 0; n < 300; ++n)
        {
            juce::MemoryBlock mb (good);
            if (n % 3 == 0) mb.setSize ((size_t) rng.nextInt ((int) good.getSize()));
            else if (n % 3 == 1) for (int i = 0; i < 12; ++i) static_cast<char*> (mb.getData())[rng.nextInt ((int) mb.getSize())] = (char) rng.nextInt (256);
            else { mb.setSize ((size_t) rng.nextInt (400)); for (size_t i = 0; i < mb.getSize(); ++i) static_cast<char*> (mb.getData())[i] = (char) rng.nextInt (256); }
            p.setStateInformation (mb.getData(), (int) mb.getSize()); p.applyPendingNow();
            ok = ok && paramsSane (p, why) && p.uiScale >= 0.6f && p.uiScale <= 2.0f && p.getCurrentProgram() >= 0 && p.getCurrentProgram() < ExciterProcessor::numPresets && noise (p, rng, 1, 256).finite;
        }
        p.setStateInformation (nullptr, 0);
        check (ok, "300 cut short, scribbled-on and random sessions: no crash, every control stays in its range " + why);
        auto withState = [&p] (const juce::String& xmlText)
        {
            if (auto xml = juce::parseXML (xmlText)) { juce::MemoryBlock mb; juce::AudioProcessor::copyXmlToBinary (*xml, mb); p.setStateInformation (mb.getData(), (int) mb.getSize()); p.applyPendingNow(); }
        };
        withState ("<ToboExciter program=\"999\" uiScale=\"NaN\" filePreset=\"x\"><PARAM id=\"frequency\" value=\"1e30\"/><PARAM id=\"color\" value=\"nan\"/><PARAM id=\"oversampling\" value=\"-7\"/><PARAM id=\"nosuch\" value=\"3\"/></ToboExciter>");
        std::cout << "      after the absurd session: frequency " << getp (p, "frequency") << ", color " << getp (p, "color") << ", oversampling " << getp (p, "oversampling") << ", size " << p.uiScale << std::endl;
        const bool a = paramsSane (p, why) && std::isfinite (p.uiScale) && p.uiScale >= 0.6f && p.uiScale <= 2.0f && noise (p, rng, 4, 256).finite;
        withState ("<ToboExciter program=\"-4\" uiScale=\"1e9\"/>");
        const bool b = paramsSane (p, why) && p.uiScale <= 2.0f && p.getCurrentProgram() == 0;
        withState ("<SomethingElse/>");
        { Panel g (p); g.draw(); check (a && b && g.ed->getWidth() >= 300 && g.ed->getWidth() <= 1040, "sessions with absurd values (1e30, NaN, a preset number 999, a size of a thousand million): all brought back into range; the panel opens " + juce::String (g.ed->getWidth()) + " wide " + why); }

        defaults (p); setp (p, "frequency", 3.3f);
        struct Bad { const char* name; const char* text; bool takes; };
        const Bad files[] = {
            { "empty", "", false }, { "not xml", "hello, this is no preset", false }, { "another tag", "<Other frequency=\"1\"/>", false },
            { "no values", "<TE02Preset name=\"x\"/>", false },
            { "absurd values", "<TE02Preset frequency=\"1e30\" color=\"-1e30\" balance=\"nan\" multipeak=\"inf\" input=\"abc\" output=\"9999\" oversampling=\"42\"/>", true },
            { "one value only", "<TE02Preset color=\"2.5\"/>", true },
            { "cut short", "<TE02Preset frequency=\"1\" color=", true } };       // the reader takes what is whole in it: one good value
        bool pf = true; juce::String pw;
        for (auto& bf : files)
        {
            const juce::File f = dir.getChildFile ("bad.tepreset"); f.replaceWithText (bf.text);
            const bool took = p.loadPreset (f);
            if (took != bf.takes || ! paramsSane (p, why) || ! noise (p, rng, 2, 256).finite) { pf = false; pw += " " + juce::String (bf.name) + (took ? " taken" : " refused") + " " + why + ";"; }
        }
        { const juce::File f = dir.getChildFile ("bad.tepreset"); juce::MemoryBlock junk (5000); for (size_t i = 0; i < junk.getSize(); ++i) static_cast<char*> (junk.getData())[i] = (char) rng.nextInt (256); f.replaceWithData (junk.getData(), junk.getSize()); pf = pf && ! p.loadPreset (f); }
        pf = pf && ! p.loadPreset (dir.getChildFile ("there is no such file.tepreset")) && ! p.loadPreset (dir) && ! p.loadPreset (juce::File());
        pf = pf && ! p.savePreset (dir) && paramsSane (p, why);
        check (pf, "preset files that are empty, not presets, cut short, random, missing, a folder, or full of absurd values: refused or brought into range, no crash" + pw);
        const juce::File odd = dir.getChildFile (juce::String::fromUTF8 ("\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86 preset & more (2).tepreset"));
        check (p.savePreset (odd) && p.loadPreset (odd) && p.filePresetName.isNotEmpty(), "a preset file with Japanese letters, spaces and signs in its name: saved and read back");
        const juce::File longName = dir.getChildFile (juce::String::repeatedString ("long name ", 15) + ".tepreset");
        if (p.savePreset (longName) && p.loadPreset (longName)) { Panel g (p); g.draw(); }
        check (p.filePresetName.length() <= 60, "a preset file with a very long name: its name on the panel is cut to " + juce::String (p.filePresetName.length()) + " letters");
    }

    std::cout << "\n== the panel, misused ==\n";
    for (float scale : { 0.6f, 1.0f, 2.0f })
    {
        ExciterProcessor p; start (p, 48000, 256); p.uiScale = scale; Panel g (p);
        juce::String why; bool ok = true; int moved = 0; float last[numIds]; for (int k = 0; k < numIds; ++k) last[k] = getp (p, ids[k]);
        for (int n = 0; n < 30000; ++n)
        {
            const float x = (rng.nextFloat() * 1.2f - 0.1f) * 448, y = (rng.nextFloat() * 1.2f - 0.1f) * 672;
            const int what = rng.nextInt (10);
            if (what < 4) g.press (x, y); else if (what < 7) g.move (x, y); else if (what < 9) g.up (x, y); else if (rng.nextBool()) g.twice (x, y); else g.ed->poke (x * g.z, y * g.z, 4, rng.nextFloat() * 4 - 2);
            if (n % 50 == 0) { p.applyPendingNow(); ok = ok && noise (p, rng, 1, 256).finite && paramsSane (p, why); g.draw(); }
            for (int k = 0; k < numIds; ++k) if (getp (p, ids[k]) != last[k]) { ++moved; last[k] = getp (p, ids[k]); }
        }
        check (ok && moved > 200, juce::String ((int) (scale * 100)) + " %: 30000 presses, drags, releases, double clicks and wheel turns anywhere, also outside the window, releases without a press: no crash, controls in range (" + juce::String (moved) + " changes) " + why);
    }
    {
        ExciterProcessor p; start (p, 48000, 256); Panel g (p); bool ok = true;
        for (float v : { 0.0f, -3.0f, 1000.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.6f, 2.0f })
        {
            g.ed->setZoom (v); g.draw();
            ok = ok && std::isfinite (p.uiScale) && p.uiScale >= 0.6f && p.uiScale <= 2.0f && g.ed->getWidth() >= 300 && g.ed->getWidth() <= 1040;
        }
        const int sizes[][2] = { { 0, 0 }, { 1, 1 }, { 3, 4000 }, { 4000, 3 }, { 5000, 7500 }, { 515, 773 } };
        for (auto& s : sizes) { g.ed->setSize (s[0], s[1]); g.draw(); g.z = juce::jmax (0.01f, (float) g.ed->getWidth() / 448.0f); g.click (at::pedalX, at::pedalY); ok = ok && std::isfinite (p.uiScale) && p.uiScale >= 0.6f && p.uiScale <= 2.0f; }
        check (ok, "sizes of zero, below zero, a thousand, NaN; windows of 0 x 0, 1 x 1, 3 x 4000, 5000 x 7500: no crash, the size kept for the session stays between 60 and 200 %");
    }
    {
        ExciterProcessor p; start (p, 48000, 256);
        for (auto* id : { "frequency", "color", "balance", "multipeak" }) setp (p, id, 10);
        std::atomic<bool> stop { false }, fin { true };
        // as a host does it: the sound is asked for under the processor's own lock, and not while it is suspended
        std::thread audio ([&]
        {
            juce::Random r2 (3); juce::AudioBuffer<float> b (2, 256); juce::MidiBuffer m;
            while (! stop)
            {
                for (int i = 0; i < 256; ++i) { b.setSample (0, i, r2.nextFloat() * 0.5f - 0.25f); b.setSample (1, i, r2.nextFloat() * 0.5f - 0.25f); }
                { const juce::ScopedLock sl (p.getCallbackLock()); if (! p.isSuspended()) p.processBlock (b, m); }
                for (int i = 0; i < 256; ++i) if (! std::isfinite (b.getSample (0, i))) fin = false;
            }
        });
        for (int n = 0; n < 200; ++n)
        {
            Panel g (p);
            for (int f = 0; f < 6; ++f) { const float x = rng.nextFloat() * 448, y = rng.nextFloat() * 672; g.press (x, y); g.move (x, y - 20); g.up (x, y - 20); }
            g.draw();
            setp (p, "oversampling", (float) (n % 3)); p.applyPendingNow();
            if (n % 5 == 0) p.setCurrentProgram (n % ExciterProcessor::numPresets);
            if (n % 9 == 0) { juce::MemoryBlock mb; p.getStateInformation (mb); p.setStateInformation (mb.getData(), (int) mb.getSize()); }
            g.ed->setZoom (0.6f + 0.1f * (float) (n % 14));
        }
        stop = true; audio.join();
        check (fin.load(), "200 times opened, used, resized and closed, the oversampling changed each time, with the sound running on another thread: no crash, sound finite");
    }
    {
        ExciterProcessor p; start (p, 48000, 256);
        std::unique_ptr<juce::AudioProcessorEditor> one (p.createEditor()), two (p.createEditor());
        one->createComponentSnapshot (one->getLocalBounds(), true, 1.0f); two.reset(); one->createComponentSnapshot (one->getLocalBounds(), true, 1.0f);
        check (true, "two panels of one plug-in at once, one closed: no crash");
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File dir (argc > 1 ? juce::String (argv[1]) : juce::String ("stress_files"));
    dir.createDirectory();
    ordinary (dir);
    abuse (dir);
    std::cout << "\n" << (failed == 0 ? "ALL " + std::to_string (checks) + " CHECKS PASSED" : std::to_string (failed) + " OF " + std::to_string (checks) + " CHECKS FAILED") << "\n";
    return failed == 0 ? 0 : 1;
}

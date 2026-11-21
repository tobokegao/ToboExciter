// Tobo Exciter (TE-02): draws the panel to PNG files without a host or a screen. Copyright (C) 2026 Tobokegao. Licensed under the GNU AGPL v3 or later.
// usage: PanelShot <output folder>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <iostream>

static void shot (ExciterProcessor& p, const juce::File& f)
{
    std::unique_ptr<juce::AudioProcessorEditor> e (p.createEditor());
    auto img = e->createComponentSnapshot (e->getLocalBounds(), true, 1.0f);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File out (argc > 1 ? juce::String (argv[1]) : juce::String ("panel_shots"));
    out.createDirectory();
    ExciterProcessor p;
    p.prepareToPlay (48000.0, 512);
    shot (p, out.getChildFile ("panel_default.png"));
    p.uiScale = 1.5f; shot (p, out.getChildFile ("panel_default_150.png")); p.uiScale = 1.0f;      // for the manual
    auto set = [&p] (const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
    set ("frequency", 1.6f); set ("color", 10.0f); set ("balance", 10.0f); set ("multipeak", 7.5f); set ("on", 0.0f);
    shot (p, out.getChildFile ("panel_off.png"));
    {
        // the page about the plug-in, and the panel at 150 %
        std::unique_ptr<juce::AudioProcessorEditor> e (p.createEditor());
        static_cast<ExciterEditor*> (e.get())->showAbout();
        auto img = e->createComponentSnapshot (e->getLocalBounds(), true, 1.0f);
        auto f = out.getChildFile ("panel_about.png"); f.deleteFile();
        juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        p.uiScale = 1.5f;
        shot (p, out.getChildFile ("panel_150.png"));
        p.uiScale = 1.0f;
        // a preset written to a file and read back gives the same knobs
        auto pf = out.getChildFile ("roundtrip.tepreset");
        const bool saved = p.savePreset (pf);
        const float before = p.apvts.getRawParameterValue ("multipeak")->load();
        set ("multipeak", 2.0f);
        const bool loaded = p.loadPreset (pf);
        std::cout << "preset file: saved " << saved << ", loaded " << loaded << ", multipeak " << before << " -> "
                  << p.apvts.getRawParameterValue ("multipeak")->load() << ", name '" << p.filePresetName << "'" << std::endl;
    }

    {
        // how long one drawing of the whole panel takes (the host asks for one whenever something on it changes)
        for (float sc : { 1.0f, 1.5f, 2.0f })
        {
            p.uiScale = sc;
            std::unique_ptr<juce::AudioProcessorEditor> e (p.createEditor());
            juce::Image img (juce::Image::ARGB, e->getWidth(), e->getHeight(), true);
            const double t0 = juce::Time::getMillisecondCounterHiRes();
            for (int n = 0; n < 20; ++n) { juce::Graphics g (img); e->paintEntireComponent (g, true); }
            std::cout << "one drawing of the panel at " << (int) (sc * 100) << " %: " << (juce::Time::getMillisecondCounterHiRes() - t0) / 20.0 << " ms" << std::endl;
        }
        p.uiScale = 1.0f;
    }
    {
        // the delay told to the host, for the manual's table
        for (double sr : { 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 })
        {
            std::cout << "delay at " << sr << " Hz:";
            for (float o : { 0.0f, 1.0f, 2.0f })
            {
                ExciterProcessor q; q.setPlayConfigDetails (2, 2, sr, 512);
                auto* par = q.apvts.getParameter ("oversampling"); par->setValueNotifyingHost (par->convertTo0to1 (o));
                q.prepareToPlay (sr, 512);
                std::cout << "  " << q.getLatencySamples() << " samples (" << q.oversamplingFactor() << "x)";
            }
            std::cout << std::endl;
        }
    }
    // The switch must not click: a steady low tone through a loud setting, switched every 0.25 s, with the pedal and
    // with the host's bypass. A click is a jump from one sample to the next far above what the tone itself makes.
    for (const char* id : { "on", "bypass" })
    {
        set ("on", 1.0f); set ("bypass", 0.0f); set ("frequency", 1.6f); set ("color", 10.0f); set ("balance", 10.0f); set ("multipeak", 10.0f);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m;
        double ph = 0.0; float last = 0.0f, steady = 0.0f, switched = 0.0f; bool state = juce::String (id) == "on";
        for (int blk = 0; blk < 375; ++blk)          // 4 s at 48 kHz
        {
            const bool toggle = blk > 90 && blk % 23 == 0;
            if (toggle) { state = ! state; set (id, state ? 1.0f : 0.0f); }
            for (int i = 0; i < 512; ++i) { ph += 2.0 * 3.141592653589793 * 110.0 / 48000.0; const float v = (float) (0.25 * std::sin (ph)); b.setSample (0, i, v); b.setSample (1, i, v); }
            p.processBlock (b, m);
            const bool near = blk > 90 && blk % 23 < 3;      // the blocks in which a fade runs
            for (int i = 0; i < 512; ++i)
            {
                const float v = b.getSample (0, i), d = std::abs (v - last); last = v;
                if (blk > 40) { if (near) switched = juce::jmax (switched, d); else steady = juce::jmax (steady, d); }
            }
        }
        std::cout << "switching with '" << id << "': largest step between samples " << switched << " while fading, " << steady << " otherwise" << std::endl;
    }
    return 0;
}

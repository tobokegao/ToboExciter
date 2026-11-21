// Tobo Exciter (TE-02): the panel. Copyright (C) 2026 Tobokegao. Licensed under the GNU AGPL v3 or later.
// The pedal, its leads and the ground are pictures rendered from a 3D model (Assets/): one still
// picture, the knob turned in 61 steps, and the lit lamp. Above the pedal, drawn here: the level knobs with
// their meters, the presets and the oversampling.
#pragma once
#include "PluginProcessor.h"

class ExciterLook : public juce::LookAndFeel_V4
{
public:
    explicit ExciterLook (bool flatKnob = false);
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;

private:
    juce::Image strip;      // the knob's turns, one under the other
    int frames = 61;
    bool flat = false;      // a flat drawn knob (the level knobs above the pedal) instead of the rendered one
};

// a part of the picture that answers a click (the picture itself is drawn by the panel)
class ExciterSpot : public juce::Button
{
public:
    ExciterSpot() : juce::Button ("spot") {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// the page about the plug-in, laid over the panel; a click anywhere puts it away
class ExciterAbout : public juce::Component
{
public:
    float zoom = 1.15f;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { setVisible (false); }
};

class ExciterEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ExciterEditor (ExciterProcessor&);
    ~ExciterEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void showAbout() { about.setVisible (true); about.toFront (false); }
    void setZoom (float scale);
    // for tests: a mouse at (x, y) in pixels. phase 0 = press, 1 = drag, 2 = let go, 3 = double click, 4 = wheel
    void poke (float x, float y, int phase, float wheel = 0.0f);
    bool dialogs = true;        // false in tests: SIZE, LOAD and SAVE then open nothing, only count
    int dialogAsked = 0;

private:
    void timerCallback() override;
    ExciterProcessor& proc;
    ExciterLook look, flatLook { true };
    juce::Slider knob[6];       // frequency, color, balance, multipeak, input, output
    ExciterSpot pedal, prev, next, over, settings, importer, exporter, help;
    ExciterAbout about;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Component::SafePointer<juce::Component> poked;
    float Z = 1.15f;            // pixels to a unit: 1.15 at 100 %
    juce::String shownName;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att[6];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pedalAtt;
    juce::Image base, lampOnImage;
    bool lampOn = true;
    float meterIn = -100.0f, meterOut = -100.0f;    // dB, falling
    int shownProgram = -1, shownOs = -1;
    float shownIn = 0.0f, shownOut = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExciterEditor)
};

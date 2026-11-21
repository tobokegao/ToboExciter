// Tobo Exciter (TE-02): the processor. Copyright (C) 2026 Tobokegao. Licensed under the GNU AGPL v3 or later.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Engine.h"

class ExciterProcessor : public juce::AudioProcessor,
                      private juce::AudioProcessorValueTreeState::Listener,
                      private juce::AsyncUpdater
{
public:
    ExciterProcessor();
    ~ExciterProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    const juce::String getName() const override { return "Tobo Exciter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }

    // the presets: knob positions only (the levels and the oversampling stay as they are)
    struct Preset { const char* name; float frequency, color, balance, multipeak; };
    static const Preset presets[];
    static const int numPresets;
    int getNumPrograms() override { return numPresets; }
    int getCurrentProgram() override { return program; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int i) override { return juce::isPositiveAndBelow (i, numPresets) ? presets[i].name : ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    // a preset in a file: the four knobs, the levels and the oversampling
    bool savePreset (const juce::File&) const;
    bool loadPreset (const juce::File&);
    juce::String filePresetName;       // the name of the preset last read from a file; empty while a built-in one is chosen
    float uiScale = 1.0f;              // how large the panel is shown (kept with the session)
    void applyPendingNow() { handleUpdateNowIfNeeded(); }      // for tests without a message loop: a changed oversampling takes effect
    int oversamplingFactor() const { return 1 << osFactorLog2; }
    // for the meters: the loudest sample since the panel last looked, going into the model and coming out
    std::atomic<float> peakIn { 0.0f }, peakOut { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void parameterChanged (const juce::String&, float) override;
    void handleAsyncUpdate() override;
    void setUp (double sampleRate, int samplesPerBlock);

    std::atomic<float>* pFreq = nullptr; std::atomic<float>* pColor = nullptr; std::atomic<float>* pBalance = nullptr;
    std::atomic<float>* pMulti = nullptr; std::atomic<float>* pIn = nullptr; std::atomic<float>* pOut = nullptr;
    std::atomic<float>* pOn = nullptr; std::atomic<float>* pOs = nullptr;
    double mix[2] = { 1.0, 1.0 };
    double rate = 48000.0;
    te02::Engine engine[2];
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    int osFactorLog2 = 2, blockSize = 512, program = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExciterProcessor)
};

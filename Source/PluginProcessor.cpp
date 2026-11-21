// Tobo Exciter (TE-02): the processor. Copyright (C) 2026 Tobokegao. Licensed under the GNU AGPL v3 or later.
#include "PluginProcessor.h"
#include "PluginEditor.h"

const ExciterProcessor::Preset ExciterProcessor::presets[] = {
    { "Default",      5.0f,  5.0f,  5.0f,  5.0f },
    { "Gentle Air",   6.5f,  3.0f,  3.5f,  4.0f },
    { "Bite",         5.0f,  7.5f,  5.0f,  5.0f },
    { "Six Peaks",    5.0f,  5.0f,  5.0f, 10.0f },
    { "Filter Only",  5.0f,  5.0f, 10.0f,  5.0f },
    { "Low Honk",     0.0f,  7.5f,  5.0f,  5.0f },
    { "One Note",     1.6f, 10.0f, 10.0f, 10.0f },
    { "Thin Scream",  4.4f, 10.0f, 10.0f, 10.0f },
};
const int ExciterProcessor::numPresets = (int) (sizeof (ExciterProcessor::presets) / sizeof (ExciterProcessor::presets[0]));

juce::AudioProcessorValueTreeState::ParameterLayout ExciterProcessor::layout()
{
    using F = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    // the four knobs of the unit: 0 = fully left, 5 = 12 o'clock, 10 = fully right
    l.add (std::make_unique<F> (juce::ParameterID { "frequency", 1 }, "Frequency", R (0.0f, 10.0f, 0.01f), 5.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "color", 1 }, "Color", R (0.0f, 10.0f, 0.01f), 5.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "balance", 1 }, "Balance", R (0.0f, 10.0f, 0.01f), 5.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "multipeak", 1 }, "Multipeak", R (0.0f, 10.0f, 0.01f), 5.0f));
    // how loud the track is compared with the guitar at the unit's input. The model's 0 dB is the level of the
    // measurement; a DI track recorded with 8 dB of gain on the instrument input sits 4.9 dB above it.
    l.add (std::make_unique<F> (juce::ParameterID { "input", 1 }, "Input", R (-30.0f, 30.0f, 0.1f), -4.9f,
                                juce::AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<F> (juce::ParameterID { "output", 1 }, "Output", R (-30.0f, 30.0f, 0.1f), 4.9f,
                                juce::AudioParameterFloatAttributes().withLabel ("dB")));
    // the pedal: off passes the sound through untouched
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "on", 1 }, "Effect", true));
    // the host's own bypass switch: handed to the host (getBypassParameter), so that it too fades instead of cutting
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false));
    // how many times the sample rate the model runs at (it clips hard, so more is cleaner and heavier)
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "oversampling", 1 }, "Oversampling",
                                                         juce::StringArray { "2x", "4x", "8x" }, 1));
    return l;
}

ExciterProcessor::ExciterProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ToboExciter", layout())
{
    pFreq = apvts.getRawParameterValue ("frequency");
    pColor = apvts.getRawParameterValue ("color");
    pBalance = apvts.getRawParameterValue ("balance");
    pMulti = apvts.getRawParameterValue ("multipeak");
    pIn = apvts.getRawParameterValue ("input");
    pOut = apvts.getRawParameterValue ("output");
    pOn = apvts.getRawParameterValue ("on");
    pOs = apvts.getRawParameterValue ("oversampling");
    apvts.addParameterListener ("oversampling", this);
}

ExciterProcessor::~ExciterProcessor()
{
    apvts.removeParameterListener ("oversampling", this);
    cancelPendingUpdate();
}

bool ExciterProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void ExciterProcessor::setUp (double sampleRate, int samplesPerBlock)
{
    // 2x / 4x / 8x at 44.1 and 48 kHz; one step less at 88.2 and 96 kHz, two less above
    rate = sampleRate;        // kept here: getSampleRate() is only right when a host has set the processor up
    int f = 1 + juce::jlimit (0, 2, (int) std::lround (pOs->load()));
    if (sampleRate > 80000.0) --f;
    if (sampleRate > 160000.0) --f;
    osFactorLog2 = juce::jmax (0, f);
    const int ch = juce::jmax (1, getTotalNumInputChannels());
    os = std::make_unique<juce::dsp::Oversampling<float>> ((size_t) ch, (size_t) osFactorLog2,
                                                           juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    os->initProcessing ((size_t) samplesPerBlock);
    setLatencySamples ((int) std::lround (os->getLatencyInSamples()));
    te02::Knobs k { *pFreq / 10.0, *pColor / 10.0, *pBalance / 10.0, *pMulti / 10.0 };
    for (auto& e : engine) { e.setKnobs (k); e.prepare (sampleRate * (1 << osFactorLog2)); e.snap(); }
}

void ExciterProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    blockSize = juce::jmax (1, samplesPerBlock);
    setUp (sampleRate, samplesPerBlock);
}

// the oversampling was changed: build the filters again with the sound held
void ExciterProcessor::parameterChanged (const juce::String&, float) { triggerAsyncUpdate(); }

void ExciterProcessor::handleAsyncUpdate()
{
    suspendProcessing (true);
    setUp (rate, blockSize);
    suspendProcessing (false);
}

void ExciterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int ch = juce::jmin (2, buffer.getNumChannels());
    if (os == nullptr || ch == 0) return;

    // a block longer than the host said it would send: in pieces (the oversampling was set up for the shorter one)
    if (buffer.getNumSamples() > blockSize)
    {
        juce::MidiBuffer none;
        for (int at = 0; at < buffer.getNumSamples(); at += blockSize)
        {
            juce::AudioBuffer<float> part (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), at,
                                           juce::jmin (blockSize, buffer.getNumSamples() - at));
            processBlock (part, none);
        }
        return;
    }
    if (buffer.getNumSamples() == 0) return;
    // numbers that are not numbers, from the host or from what stands before: a knob falls back to its middle, a
    // sample to silence (one such sample would otherwise stay in the filters for good)
    auto num = [] (float v, float fallback) { return std::isfinite (v) ? v : fallback; };
    for (int c = 0; c < ch; ++c)
    {
        float* d = buffer.getWritePointer (c);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (! (std::abs (d[i]) < 1.0e6f)) d[i] = 0.0f;
    }
    te02::Knobs k { num (*pFreq, 5.0f) / 10.0, num (*pColor, 5.0f) / 10.0, num (*pBalance, 5.0f) / 10.0, num (*pMulti, 5.0f) / 10.0 };
    const double gin = std::pow (10.0, num (*pIn, 0.0f) / 20.0), gout = std::pow (10.0, num (*pOut, 0.0f) / 20.0);
    const bool on = *pOn > 0.5f && apvts.getRawParameterValue ("bypass")->load() < 0.5f;
    // how far the switch moves per sample: a straight ramp over 12 ms, shaped below into an S (no corner at either end)
    const double step = 1.0 / (0.012 * rate * (1 << osFactorLog2));

    float pk = 0.0f;
    for (int c = 0; c < ch; ++c) pk = juce::jmax (pk, buffer.getMagnitude (c, 0, buffer.getNumSamples()));
    pk *= (float) gin;
    if (pk > peakIn.load()) peakIn.store (pk);

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) ch, (size_t) buffer.getNumSamples());
    auto up = os->processSamplesUp (block);
    const int n = (int) up.getNumSamples();
    for (int c = 0; c < ch; ++c)
    {
        engine[c].setKnobs (k);
        float* d = up.getChannelPointer ((size_t) c);
        for (int i = 0; i < n; ++i)
        {
            // The switch cross-fades between the sound as it came in and the effect, so it does not click. Both run
            // through the same oversampling, so they stay in step; the effect keeps running while it is off.
            mix[c] = on ? juce::jmin (1.0, mix[c] + step) : juce::jmax (0.0, mix[c] - step);
            const double m = mix[c] * mix[c] * (3.0 - 2.0 * mix[c]);
            const double dry = (double) d[i];
            double wet = gout * engine[c].tick (gin * dry);
            if (! (std::abs (wet) < 1.0e6))        // the model has run away: start it afresh
            {
                engine[c].prepare (rate * (1 << osFactorLog2));
                wet = 0.0;
            }
            d[i] = (float) (dry + m * (wet - dry));
        }
    }
    os->processSamplesDown (block);

    pk = 0.0f;
    for (int c = 0; c < ch; ++c) pk = juce::jmax (pk, buffer.getMagnitude (c, 0, buffer.getNumSamples()));
    if (pk > peakOut.load()) peakOut.store (pk);
}

void ExciterProcessor::setCurrentProgram (int i)
{
    if (! juce::isPositiveAndBelow (i, numPresets)) return;
    program = i;
    filePresetName.clear();
    const float v[4] = { presets[i].frequency, presets[i].color, presets[i].balance, presets[i].multipeak };
    static const char* ids[4] = { "frequency", "color", "balance", "multipeak" };
    for (int k = 0; k < 4; ++k)
        if (auto* q = apvts.getParameter (ids[k])) q->setValueNotifyingHost (q->convertTo0to1 (v[k]));
}

void ExciterProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", program, nullptr);
    state.setProperty ("filePreset", filePresetName, nullptr);
    state.setProperty ("uiScale", uiScale, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void ExciterProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            program = juce::jlimit (0, numPresets - 1, (int) state.getProperty ("program", 0));
            filePresetName = state.getProperty ("filePreset", "").toString();
            const float sc = (float) state.getProperty ("uiScale", 1.0f);
            uiScale = std::isfinite (sc) ? juce::jlimit (0.6f, 2.0f, sc) : 1.0f;
            apvts.replaceState (state);
            // a damaged session can hold a value that is not a number, which the parameter would keep: back to its default
            for (auto* q : getParameters())
                if (! std::isfinite (q->getValue())) q->setValueNotifyingHost (q->getDefaultValue());
        }
}

namespace { const char* const presetIds[] = { "frequency", "color", "balance", "multipeak", "input", "output", "oversampling" }; }

bool ExciterProcessor::savePreset (const juce::File& f) const
{
    juce::XmlElement xml ("TE02Preset");
    xml.setAttribute ("name", f.getFileNameWithoutExtension());
    for (auto* id : presetIds)
        if (auto* q = apvts.getParameter (id)) xml.setAttribute (id, (double) q->convertFrom0to1 (q->getValue()));
    return xml.writeTo (f);
}

bool ExciterProcessor::loadPreset (const juce::File& f)
{
    auto xml = juce::parseXML (f);
    if (xml == nullptr || ! xml->hasTagName ("TE02Preset")) return false;
    bool any = false;
    for (auto* id : presetIds) any = any || xml->hasAttribute (id);
    if (! any) return false;
    for (auto* id : presetIds)
        if (auto* q = apvts.getParameter (id))
            if (xml->hasAttribute (id))
            {
                q->beginChangeGesture();
                const auto& range = q->getNormalisableRange();
                const double v = xml->getDoubleAttribute (id, (double) q->convertFrom0to1 (q->getDefaultValue()));
                const float vv = std::isfinite (v) ? juce::jlimit (range.start, range.end, (float) v) : q->convertFrom0to1 (q->getDefaultValue());
                q->setValueNotifyingHost (q->convertTo0to1 (vv));
                q->endChangeGesture();
            }
    filePresetName = f.getFileNameWithoutExtension().substring (0, 60);
    return true;
}

juce::AudioProcessorEditor* ExciterProcessor::createEditor() { return new ExciterEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ExciterProcessor(); }

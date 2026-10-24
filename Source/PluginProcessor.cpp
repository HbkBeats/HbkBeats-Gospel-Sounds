#include "PluginProcessor.h"
#include "PluginEditor.h"

GospelProcessor::GospelProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < 28; ++i)
        synth.addVoice(new GospelVoice(voiceParams));
    synth.addSound(new GospelSound());
    synth.setNoteStealingEnabled(true);
}

bool GospelProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void GospelProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleRateHz = sampleRate;
    synth.setCurrentPlaybackSampleRate(sampleRate);
    synth.allNotesOff(0, false);
    chord.reset();
    keyboardState.reset();

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    toneFilter.prepare(spec);
    toneFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    ensemble.prepare(spec);
    ensemble.setRate(0.45f);
    ensemble.setDepth(0.35f);
    ensemble.setCentreDelay(14.f);
    ensemble.setFeedback(0.f);
    reverb.setSampleRate(sampleRate);
    reverb.reset();

    gateGain = 1.f;
}

void GospelProcessor::updateVoiceParams(int numSamples, double sr)
{
    voiceParams.organLevel   = param(ID::organ);
    voiceParams.keysLevel    = param(ID::keys);
    voiceParams.choirLevel   = param(ID::choir);
    voiceParams.stringsLevel = param(ID::strings);

    // drawbars: 3 dB per step, like the real thing
    for (int i = 0; i < 9; ++i)
    {
        const float v = param(ID::drawbarIds[i]);
        voiceParams.drawbar[i] = v < 0.5f ? 0.f : std::pow(10.f, (v - 8.f) * 0.15f);
    }
    voiceParams.perc      = (int) param(ID::perc);
    voiceParams.keysDecay = param(ID::keysDecay);
    voiceParams.keysTone  = param(ID::keysTone);
    voiceParams.vowel     = param(ID::vowel);
    voiceParams.choirVib  = param(ID::choirVib);
    voiceParams.attack    = param(ID::attack);
    voiceParams.release   = param(ID::release);

    // rotary speaker: spins up and down smoothly between slow and fast
    const int rm = (int) param(ID::rotor);
    const float slowHz = 0.8f, fastHz = 6.6f;
    const float target = rm == 2 ? fastHz : (rm == 3 && modWheel > 0.5f ? fastHz : slowHz);
    rotorHz += (target - rotorHz) * (1.f - std::exp(-(float) numSamples / (float) (sr * 1.2)));
    voiceParams.rotorDepth = rm == 0 ? 0.f : param(ID::rotorDepth);
    voiceParams.rotorInc   = rotorHz / (float) sr;
    voiceParams.rotorPhase = rotorPhaseState;
    rotorPhaseState = std::fmod(rotorPhaseState + (double) numSamples * (double) rotorHz / sr, 1.0);
}

namespace
{
    float gateValue(int pattern, double ppq, float depth, float len)
    {
        double bar = std::fmod(ppq, 4.0);
        if (bar < 0.0) bar += 4.0;
        float v = 1.f;

        auto stepInfo = [&](double perBar, int& idx, double& frac)
        {
            const double step = bar * perBar / 4.0 * 4.0;   // bar is in beats (0..4)
            idx = (int) std::floor(step);
            frac = step - std::floor(step);
        };

        switch (pattern)
        {
            case 1: // Swell: one slow breath per bar
                v = 0.5f - 0.5f * (float) std::cos(juce::MathConstants<double>::twoPi * bar / 4.0);
                break;

            case 2: // Pulse 8ths
            {
                int idx; double frac;
                stepInfo(2.0, idx, frac);
                v = frac < len ? 1.f : 0.f;
                break;
            }
            case 3: // Gospel shuffle: triplet grid, hit on 1 and the "let"
            {
                static const float pat[3] = { 1.f, 0.f, 0.7f };
                int idx; double frac;
                stepInfo(3.0, idx, frac);
                v = frac < len ? pat[idx % 3] : 0.f;
                break;
            }
            case 4: // Shout stabs on a 16th grid
            {
                static const float pat[16] = { 1.f, 0.f, 0.f, 0.7f,  0.f, 0.f, 1.f, 0.f,
                                               0.8f, 0.f, 0.f, 0.7f, 0.f, 0.f, 1.f, 0.f };
                int idx; double frac;
                stepInfo(4.0, idx, frac);
                v = frac < len ? pat[idx % 16] : 0.f;
                break;
            }
            default: break;
        }
        return 1.f - depth * (1.f - v);
    }
}

void GospelProcessor::applyRhythmGate(juce::AudioBuffer<float>& buffer)
{
    const int pattern = (int) param(ID::rhythm);
    if (pattern == 0)
    {
        gateGain = 1.f;
        return;
    }

    const int n = buffer.getNumSamples();
    const float depth = param(ID::gateDepth), len = param(ID::gateLen);

    double bpm = 120.0, ppqStart = internalPpq;
    bool hostRunning = false;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (pos->getIsPlaying())
                if (auto p = pos->getPpqPosition()) { ppqStart = *p; hostRunning = true; }
        }
    }

    const double ppqPerSample = bpm / (60.0 * sampleRateHz);
    const float smooth = 1.f - std::exp(-1.f / (0.003f * (float) sampleRateHz));
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);

    for (int i = 0; i < n; ++i)
    {
        const float target = gateValue(pattern, ppqStart + (double) i * ppqPerSample, depth, len);
        gateGain += (target - gateGain) * smooth;
        L[i] *= gateGain;
        R[i] *= gateGain;
    }
    internalPpq = ppqStart + (double) n * ppqPerSample;
    juce::ignoreUnused(hostRunning);
}

void GospelProcessor::applyEffects(juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);

    // tone: low-pass sweep from dark to bright
    toneFilter.setCutoffFrequency(1200.f * std::pow(15.f, param(ID::tone)));
    toneFilter.process(ctx);

    // ensemble: gentle chorus that widens the pads
    const float ens = param(ID::ensemble);
    if (ens > 0.01f)
    {
        ensemble.setMix(ens * 0.7f);
        ensemble.process(ctx);
    }

    // reverb
    juce::Reverb::Parameters rp;
    rp.roomSize = param(ID::revSize);
    rp.damping = 0.45f;
    rp.wetLevel = param(ID::revMix) * 0.6f;
    rp.dryLevel = 1.f;
    rp.width = 1.f;
    rp.freezeMode = 0.f;
    reverb.setParameters(rp);
    reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), n);

    // master + gentle safety saturation
    const float gain = juce::Decibels::decibelsToGain(param(ID::master));
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        for (int i = 0; i < n; ++i)
            d[i] = std::tanh(d[i] * gain);
    }
}

void GospelProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (buffer.getNumChannels() < 2)
        return;

    keyboardState.processNextMidiBuffer(midi, 0, n, true);

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isController() && m.getControllerNumber() == 1)
            modWheel = (float) m.getControllerValue() / 127.f;
    }

    updateVoiceParams(n, sampleRateHz);

    ChordSettings cs;
    cs.mode    = (int) param(ID::chordMode);
    cs.type    = (int) param(ID::chordType);
    cs.key     = (int) param(ID::key);
    cs.scale   = (int) param(ID::scale);
    cs.voicing = (int) param(ID::voicing);
    cs.bass    = param(ID::bass) > 0.5f;

    juce::MidiBuffer processed;
    chord.process(midi, processed, cs);

    synth.renderNextBlock(buffer, processed, 0, n);

    applyRhythmGate(buffer);
    applyEffects(buffer);
}

juce::AudioProcessorEditor* GospelProcessor::createEditor()
{
    return new GospelEditor(*this);
}

const juce::String GospelProcessor::getProgramName(int index)
{
    const auto& p = factoryPresets();
    return juce::isPositiveAndBelow(index, (int) p.size()) ? p[(size_t) index].name : juce::String();
}

void GospelProcessor::setCurrentProgram(int index)
{
    if (index != currentProgram)
        loadPreset(index);
}

void GospelProcessor::loadPreset(int index)
{
    const auto& presets = factoryPresets();
    if (! juce::isPositiveAndBelow(index, (int) presets.size()))
        return;

    currentProgram = index;

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            rp->setValueNotifyingHost(rp->getDefaultValue());

    const auto& preset = presets[(size_t) index];

    for (size_t i = 0; i < preset.drawbars.size() && i < 9; ++i)
        if (auto* rp = apvts.getParameter(ID::drawbarIds[i]))
            rp->setValueNotifyingHost(rp->convertTo0to1((float) preset.drawbars[i]));

    for (const auto& kv : preset.values)
        if (auto* rp = apvts.getParameter(kv.first))
            rp->setValueNotifyingHost(rp->convertTo0to1(kv.second));
}

void GospelProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void GospelProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml(*xml);
            currentProgram = juce::jlimit(0, getNumPrograms() - 1, (int) vt.getProperty("program", 0));
            apvts.replaceState(vt);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GospelProcessor();
}

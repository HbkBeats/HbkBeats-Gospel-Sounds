#pragma once
#include <JuceHeader.h>
#include <cmath>

// Snapshot of the sound parameters, refreshed by the processor once per block.
struct VoiceParams
{
    float organLevel = 0.8f, keysLevel = 0.5f, choirLevel = 0.45f, stringsLevel = 0.35f;
    float drawbar[9] = { 1, 1, 1, 0, 0, 0, 0, 0, 0 };
    int   perc = 0;
    float keysDecay = 1.8f, keysTone = 0.45f;
    float vowel = 0.f, choirVib = 0.35f;
    float attack = 0.35f, release = 0.6f;
    float rotorDepth = 0.5f, rotorInc = 0.f;   // rotorInc = cycles per sample
    double rotorPhase = 0.0;                   // rotor phase at the start of the block
};

struct GospelSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class GospelVoice : public juce::SynthesiserVoice
{
public:
    explicit GospelVoice(const VoiceParams& p) : vp(p) {}

    bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<GospelSound*>(s) != nullptr; }

    void startNote(int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override
    {
        noteNumber = midiNote;
        vel        = velocity;
        baseFreq   = juce::MidiMessage::getMidiNoteInHertz(midiNote);
        pitchWheelMoved(pitchWheel);
        gate = true;

        fastEnv = slowEnv = 0.f;
        keysEnv = keysIdxEnv = percEnv = clickEnv = 1.f;

        orgPhase = keysCar = keysMod = keysBell = 0.f;
        vibPhase = random.nextFloat();
        for (auto& ph : chPh) ph = random.nextFloat();
        for (auto& ph : stPh) ph = random.nextFloat();
        for (auto& f : formant) f.reset();
        lp1 = lp2 = 0.f;

        const float p = juce::jlimit(-0.35f, 0.35f, (float) (midiNote - 60) / 80.f);
        panL = 1.f - p * 0.7f;
        panR = 1.f + p * 0.7f;
    }

    void stopNote(float, bool allowTailOff) override
    {
        gate = false;
        if (! allowTailOff)
        {
            fastEnv = slowEnv = 0.f;
            clearCurrentNote();
        }
    }

    void pitchWheelMoved(int v) override
    {
        bend = std::pow(2.0, ((double) v - 8192.0) / 8192.0 * 2.0 / 12.0);
    }

    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! isVoiceActive())
            return;

        const float sr = (float) getSampleRate();
        const float twoPi = juce::MathConstants<float>::twoPi;
        const float f = (float) (baseFreq * bend);

        // Envelope coefficients
        const float cAtkFast = std::exp(-1.f / (0.003f * sr));
        const float cAtkSlow = std::exp(-1.f / (juce::jmax(0.002f, vp.attack * 0.3f) * sr));
        const float cRel     = std::exp(-1.f / (juce::jmax(0.01f, vp.release * 0.25f) * sr));
        const float keysTau  = juce::jmax(0.05f, 0.5f * vp.keysDecay
                                  * juce::jlimit(0.4f, 1.6f, 2.f - (float) noteNumber / 60.f));
        const float cKeys    = std::exp(-1.f / (keysTau * sr));
        const float cKeysIdx = std::exp(-1.f / (0.35f * sr));
        const float cPerc    = std::exp(-1.f / (0.20f * sr));
        const float cClick   = std::exp(-1.f / (0.004f * sr));

        // Choir formants (Ah -> Oh -> Ooh), set once per block
        static constexpr float fA[3] = { 800.f, 1150.f, 2900.f };
        static constexpr float fO[3] = { 450.f, 800.f, 2830.f };
        static constexpr float fU[3] = { 325.f, 700.f, 2700.f };
        static constexpr float formQ[3] = { 5.f, 7.f, 9.f };
        static constexpr float formW[3] = { 1.f, 0.55f, 0.3f };
        for (int k = 0; k < 3; ++k)
        {
            const float fm = vp.vowel < 0.5f ? juce::jmap(vp.vowel * 2.f, fA[k], fO[k])
                                             : juce::jmap((vp.vowel - 0.5f) * 2.f, fO[k], fU[k]);
            formant[k].set(juce::jmin(fm, sr * 0.45f), formQ[k], sr);
        }

        static constexpr float choirDet[3]  = { 1.f, 1.0035f, 0.9965f };
        static constexpr float stringDet[4] = { 1.f, 1.0045f, 0.9955f, 1.0095f };
        static constexpr int   harm[9]      = { 1, 3, 2, 4, 6, 8, 10, 12, 16 };   // drawbars in half-fundamental units

        const float stringCut = juce::jmin(8000.f, 1200.f + f * 2.5f);
        const float stringA   = 1.f - std::exp(-twoPi * stringCut / sr);

        auto* L = out.getWritePointer(0, startSample);
        auto* R = out.getNumChannels() > 1 ? out.getWritePointer(1, startSample) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            const float tgt = gate ? 1.f : 0.f;
            fastEnv = tgt + (fastEnv - tgt) * (gate ? cAtkFast : cRel);
            slowEnv = tgt + (slowEnv - tgt) * (gate ? cAtkSlow : cRel);
            keysEnv *= cKeys;
            keysIdxEnv *= cKeysIdx;
            percEnv *= cPerc;
            clickEnv *= cClick;

            const double rp = vp.rotorPhase + (double) (startSample + i) * (double) vp.rotorInc;
            const float th = twoPi * (float) (rp - std::floor(rp));

            float l = 0.f, r = 0.f;

            // ---------------- Organ ----------------
            if (vp.organLevel > 0.0005f)
            {
                const float rd = vp.rotorDepth;
                const float inc = (f * 0.5f / sr) * (1.f + rd * 0.0025f * std::sin(th + 0.8f));
                orgPhase += inc;
                if (orgPhase >= 1.f) orgPhase -= 1.f;

                float o = 0.f;
                for (int k = 0; k < 9; ++k)
                {
                    const float a = vp.drawbar[k];
                    if (a > 0.f && (float) harm[k] * inc < 0.45f)
                        o += a * std::sin(twoPi * (float) harm[k] * orgPhase);
                }

                if (vp.perc > 0)
                {
                    const int h = vp.perc == 1 ? 4 : 6;
                    if ((float) h * inc < 0.45f)
                        o += 0.7f * percEnv * std::sin(twoPi * (float) h * orgPhase);
                }
                o += clickEnv * (random.nextFloat() * 2.f - 1.f) * 1.2f;
                o *= 0.16f * fastEnv * (0.7f + 0.3f * vel) * vp.organLevel;

                const float gL = 1.f + rd * 0.35f * std::sin(th);
                const float gR = 1.f + rd * 0.35f * std::sin(th + 1.5708f);
                l += o * gL;
                r += o * gR;
            }

            // ---------------- Keys (FM electric piano) ----------------
            if (vp.keysLevel > 0.0005f && keysEnv > 1.0e-4f)
            {
                const float kinc = f / sr;
                keysCar += kinc; if (keysCar >= 1.f) keysCar -= 1.f;
                keysMod += kinc; if (keysMod >= 1.f) keysMod -= 1.f;

                const float idx = (0.3f + 3.2f * vp.keysTone * (0.4f + 0.6f * vel)) * keysIdxEnv + 0.15f * vp.keysTone;
                float k = std::sin(twoPi * keysCar + idx * std::sin(twoPi * keysMod));

                if (kinc * 14.f < 0.45f)
                {
                    keysBell += kinc * 14.f;
                    if (keysBell >= 1.f) keysBell -= 1.f;
                    k += 0.12f * vp.keysTone * keysIdxEnv * std::sin(twoPi * keysBell);
                }
                const float kk = k * keysEnv * fastEnv * (0.35f + 0.65f * vel) * 0.45f * vp.keysLevel;
                l += kk * panL;
                r += kk * panR;
            }

            // ---------------- Choir ----------------
            if (vp.choirLevel > 0.0005f || vp.stringsLevel > 0.0005f)
            {
                vibPhase += 5.3f / sr;
                if (vibPhase >= 1.f) vibPhase -= 1.f;
                const float vib = 1.f + std::sin(twoPi * vibPhase) * vp.choirVib * 0.006f * slowEnv;

                if (vp.choirLevel > 0.0005f)
                {
                    const float fc = f * vib;
                    float src = 0.f;
                    for (int j = 0; j < 3; ++j)
                        src += sawBlep(chPh[j], juce::jmin(0.45f, fc * choirDet[j] / sr));
                    src = src * 0.33f + (random.nextFloat() * 2.f - 1.f) * 0.04f;

                    float y = 0.f;
                    for (int k = 0; k < 3; ++k)
                        y += formW[k] * formant[k].bandpass(src);

                    const float c = y * 1.8f * slowEnv * (0.5f + 0.5f * vel) * vp.choirLevel * 0.6f;
                    l += c * panL;
                    r += c * panR;
                }

                // ---------------- Strings ----------------
                if (vp.stringsLevel > 0.0005f)
                {
                    float src = 0.f;
                    for (int j = 0; j < 4; ++j)
                        src += sawBlep(stPh[j], juce::jmin(0.45f, f * vib * stringDet[j] / sr));
                    src *= 0.25f;

                    lp1 += stringA * (src - lp1);
                    lp2 += stringA * (lp1 - lp2);
                    const float s = lp2 * slowEnv * (0.5f + 0.5f * vel) * vp.stringsLevel * 0.5f;
                    l += s * panL;
                    r += s * panR;
                }
            }

            if (R != nullptr)
            {
                L[i] += l;
                R[i] += r;
            }
            else
            {
                L[i] += 0.5f * (l + r);
            }

            if (! gate && fastEnv < 0.0005f && slowEnv < 0.0005f)
            {
                clearCurrentNote();
                break;
            }
        }
    }

private:
    // Zavalishin/Cytomic TPT state-variable filter, band-pass output normalised to unity gain at centre
    struct SVF
    {
        float ic1 = 0.f, ic2 = 0.f, k = 1.f, a1 = 0.f, a2 = 0.f, a3 = 0.f;

        void reset() { ic1 = ic2 = 0.f; }

        void set(float fc, float q, float sr)
        {
            const float g = std::tan(juce::MathConstants<float>::pi * fc / sr);
            k = 1.f / q;
            a1 = 1.f / (1.f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }

        float bandpass(float v0)
        {
            const float v3 = v0 - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.f * v1 - ic1;
            ic2 = 2.f * v2 - ic2;
            return v1 * k;
        }
    };

    static float polyBlep(float t, float dt)
    {
        if (t < dt)         { t /= dt; return t + t - t * t - 1.f; }
        if (t > 1.f - dt)   { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
        return 0.f;
    }

    static float sawBlep(float& ph, float dt)
    {
        float v = 2.f * ph - 1.f;
        v -= polyBlep(ph, dt);
        ph += dt;
        if (ph >= 1.f) ph -= 1.f;
        return v;
    }

    const VoiceParams& vp;
    juce::Random random;

    int    noteNumber = 60;
    float  vel = 0.8f;
    double baseFreq = 261.63, bend = 1.0;
    bool   gate = false;

    float fastEnv = 0.f, slowEnv = 0.f, keysEnv = 1.f, keysIdxEnv = 1.f, percEnv = 1.f, clickEnv = 1.f;
    float orgPhase = 0.f, keysCar = 0.f, keysMod = 0.f, keysBell = 0.f, vibPhase = 0.f;
    float chPh[3] = { 0, 0, 0 }, stPh[4] = { 0, 0, 0, 0 };
    float lp1 = 0.f, lp2 = 0.f;
    float panL = 1.f, panR = 1.f;
    SVF   formant[3];
};

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr int WT_SIZE = 2048;
constexpr int WT_FRAMES = 64;

struct TableBank
{
    std::array<std::array<float, WT_SIZE>, WT_FRAMES> frames{};

    TableBank()
    {
        for (int f = 0; f < WT_FRAMES; ++f)
        {
            const float morph = (float) f / (float) (WT_FRAMES - 1);
            for (int i = 0; i < WT_SIZE; ++i)
            {
                const float p = juce::MathConstants<float>::twoPi * (float)i / (float)WT_SIZE;
                const float saw = 2.0f * ((float)i / WT_SIZE) - 1.0f;
                const float sine = std::sin(p);
                const float square = i < WT_SIZE / 2 ? 1.0f : -1.0f;
                const float tri = 1.0f - 4.0f * std::abs(std::round((float)i / WT_SIZE) - (float)i / WT_SIZE);
                const float custom = std::sin(p) + 0.38f * std::sin(3.0f*p) + 0.22f * std::sin(7.0f*p);
                frames[f][i] = juce::jmap(morph, sine, saw) * (1.0f-morph)
                            + juce::jmap(morph, square, custom) * morph * 0.35f
                            + tri * 0.18f;
            }
        }
    }

    float read(float phase, float frame, float warp) const
    {
        float ph = phase - std::floor(phase);
        // Phase distortion / wavefold: deliberately bounded for stable realtime DSP.
        ph = std::fmod(ph + warp * std::sin(juce::MathConstants<float>::twoPi * ph), 1.0f);
        if (ph < 0) ph += 1.0f;
        const float fp = juce::jlimit(0.0f, (float)WT_FRAMES - 1.001f, frame);
        const int a = (int)fp;
        const int b = juce::jmin(a + 1, WT_FRAMES - 1);
        const float ft = fp - (float)a;
        const float pos = ph * WT_SIZE;
        const int i0 = ((int)pos) & (WT_SIZE-1);
        const int i1 = (i0 + 1) & (WT_SIZE-1);
        const float it = pos - std::floor(pos);
        const float va = juce::jmap(it, frames[a][i0], frames[a][i1]);
        const float vb = juce::jmap(it, frames[b][i0], frames[b][i1]);
        return juce::jmap(ft, va, vb);
    }
};

static const TableBank& bank()
{
    static TableBank b;
    return b;
}

class Voice : public juce::SynthesiserVoice
{
public:
    Voice(SalekHightechAudioProcessor& p) : proc(p)
    {
        amp.setParameters({ 0.005f, 0.15f, 0.8f, 0.12f });
        filtEnv.setParameters({ 0.002f, 0.12f, 0.0f, 0.16f });
    }

    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<juce::SynthesiserSound*>(s) != nullptr;
    }

    void startNote(int midi, float velocity, juce::SynthesiserSound*, int) override
    {
        note = midi;
        vel = velocity;
        phase1 = phase2 = phase3 = 0.0;
        lfoPhase = 0.0;
        freq = juce::MidiMessage::getMidiNoteInHertz(midi);
        amp.noteOn();
        filtEnv.noteOn();
    }

    void stopNote(float, bool allowTailOff) override
    {
        amp.noteOff();
        filtEnv.noteOff();
        if (!allowTailOff) clearCurrentNote();
    }

    void pitchWheelMoved(int value) override { pitchBend = (value - 8192.0f) / 8192.0f; }
    void controllerMoved(int cc, int value) override
    {
        if (cc == 1) modWheel = value / 127.0f;
    }

    void renderNextBlock(juce::AudioBuffer<float>& out, int start, int num) override
    {
        auto* o = out.getArrayOfWritePointers();
        const double sr = proc.getSampleRateF();
        const auto& a = proc.apvts;

        const float level1 = a.getRawParameterValue("OSC1_LEVEL")->load();
        const float level2 = a.getRawParameterValue("OSC2_LEVEL")->load();
        const float level3 = a.getRawParameterValue("OSC3_LEVEL")->load();
        const float wt1 = a.getRawParameterValue("OSC1_WT")->load();
        const float wt2 = a.getRawParameterValue("OSC2_WT")->load();
        const float wt3 = a.getRawParameterValue("OSC3_WT")->load();
        const float warp1 = a.getRawParameterValue("OSC1_WARP")->load();
        const float warp2 = a.getRawParameterValue("OSC2_WARP")->load();
        const float warp3 = a.getRawParameterValue("OSC3_WARP")->load();
        const float fm = a.getRawParameterValue("FM")->load();
        const float ring = a.getRawParameterValue("RING")->load();
        const float cutoffBase = a.getRawParameterValue("CUTOFF")->load();
        const float res = a.getRawParameterValue("RESONANCE")->load();
        const float filterEnvAmt = a.getRawParameterValue("FILTER_ENV")->load();
        const float lfoRate = a.getRawParameterValue("LFO_RATE")->load();
        const float lfoAmt = a.getRawParameterValue("LFO_AMOUNT")->load();
        const int sync = (int)a.getRawParameterValue("LFO_SYNC")->load();

        const float macroCut = a.getRawParameterValue("MACRO1")->load();
        const float macroWarp = a.getRawParameterValue("MACRO2")->load();

        juce::dsp::StateVariableTPTFilter<float> filter;
        filter.setType(juce::dsp::StateVariableTPTFilter<float>::StateVariableFilterType::lowpass);
        filter.setResonance(juce::jmap(res, 0.1f, 0.95f));
        filter.prepare({ sr, (juce::uint32)num, 1 });

        float lfoHz = lfoRate;
        if (sync > 0)
        {
            static const float divisions[] = { 4.f, 2.f, 1.f, .5f, .25f, .125f };
            const int idx = juce::jlimit(0, 5, sync - 1);
            lfoHz = (float)(proc.getBpm() / 60.0) / divisions[idx];
        }

        for (int i = 0; i < num; ++i)
        {
            const float lfo = std::sin(juce::MathConstants<float>::twoPi * (float)lfoPhase);
            lfoPhase += lfoHz / sr;
            if (lfoPhase >= 1.0) lfoPhase -= 1.0;

            const float env = amp.getNextSample();
            const float fe = filtEnv.getNextSample();

            const float macroFM = 1.0f + macroCut * 3.0f;
            const float fmod = fm * macroFM * (std::sin(juce::MathConstants<float>::twoPi * phase2)
                            + 0.5f * std::sin(juce::MathConstants<float>::twoPi * phase3));

            const float s1 = bank().read((float)phase1 + fmod * 0.08f, wt1 * (WT_FRAMES-1), warp1 + macroWarp*0.7f);
            const float s2 = bank().read((float)phase2, wt2 * (WT_FRAMES-1), warp2 + macroWarp*0.4f);
            const float s3 = bank().read((float)phase3, wt3 * (WT_FRAMES-1), warp3 + macroWarp*0.3f);

            const float ringMix = s1 * (1.0f-ring) + (s1*s2) * ring;
            float sig = level1 * ringMix + level2 * s2 + level3 * s3;

            sig += a.getRawParameterValue("SUB")->load() * std::sin(juce::MathConstants<float>::twoPi * phase1 * 0.5f);
            sig += a.getRawParameterValue("NOISE")->load() * juce::Random::getSystemRandom().nextFloat() * 2.0f - a.getRawParameterValue("NOISE")->load();

            sig *= (1.0f + lfo * lfoAmt);
            const float hz1 = freq * std::pow(2.0f, pitchBend / 12.0f);
            const float hz2 = hz1 * std::pow(2.0f, a.getRawParameterValue("OSC2_DETUNE")->load() / 1200.0f);
            const float hz3 = hz1 * std::pow(2.0f, a.getRawParameterValue("OSC3_DETUNE")->load() / 1200.0f);
            phase1 += hz1 / sr;
            phase2 += hz2 / sr;
            phase3 += hz3 / sr;
            phase1 -= std::floor(phase1); phase2 -= std::floor(phase2); phase3 -= std::floor(phase3);

            const float fc = juce::jlimit(30.0f, 20000.0f,
                cutoffBase * std::pow(2.0f, fe * filterEnvAmt * 4.0f + lfo * lfoAmt * 2.0f + macroCut * 2.0f));
            filter.setCutoffFrequency(fc);
            sig = filter.processSample(sig);

            const float gain = env * vel * 0.22f;
            o[0][start+i] += sig * gain;
            if (out.getNumChannels() > 1) o[1][start+i] += sig * gain;
        }

        if (!amp.isActive())
            clearCurrentNote();
    }

private:
    SalekHightechAudioProcessor& proc;
    juce::ADSR amp, filtEnv;
    int note = 0;
    float vel = 0.0f, pitchBend = 0.0f, modWheel = 0.0f;
    double freq = 440.0, phase1=0.0, phase2=0.0, phase3=0.0, lfoPhase=0.0;
};

class Sound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
}

SalekHightechAudioProcessor::SalekHightechAudioProcessor()
: AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
  apvts(*this, nullptr, "PARAMETERS", createParameters())
{
    for (int i=0; i<32; ++i) synth.addVoice(new Voice(*this));
    synth.addSound(new Sound());
}

juce::AudioProcessorValueTreeState::ParameterLayout SalekHightechAudioProcessor::createParameters()
{
    using P = juce::AudioParameterFloat;
    using C = juce::AudioParameterChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    auto add = [&](const char* id, const char* name, float lo, float hi, float def) {
        l.add(std::make_unique<P>(id,name,juce::NormalisableRange<float>(lo,hi,0.001f),def));
    };

    add("OSC1_LEVEL","OSC 1 LEVEL",0,1,.7f); add("OSC2_LEVEL","OSC 2 LEVEL",0,1,.45f); add("OSC3_LEVEL","OSC 3 LEVEL",0,1,.35f);
    add("OSC1_WT","OSC 1 WT",0,1,.0f); add("OSC2_WT","OSC 2 WT",0,1,.3f); add("OSC3_WT","OSC 3 WT",0,1,.65f);
    add("OSC1_WARP","OSC 1 WARP",-1,1,0); add("OSC2_WARP","OSC 2 WARP",-1,1,0); add("OSC3_WARP","OSC 3 WARP",-1,1,0);
    add("OSC2_DETUNE","OSC 2 DETUNE",-100,100,0); add("OSC3_DETUNE","OSC 3 DETUNE",-100,100,0);
    add("FM","FM",0,1,.25f); add("RING","RING MOD",0,1,0); add("SUB","SUB",0,1,.15f); add("NOISE","NOISE",0,1,0);
    add("CUTOFF","CUTOFF",30,20000,9000); add("RESONANCE","RESONANCE",0,1,.2f); add("FILTER_ENV","FILTER ENV", -1,1,.25f);
    add("LFO_RATE","LFO RATE",0.05f,30,.5f); add("LFO_AMOUNT","LFO AMOUNT",0,1,.1f);
    l.add(std::make_unique<C>("LFO_SYNC","LFO SYNC",juce::StringArray{"FREE","1/4","1/2","1/1","2/1","4/1","8/1"},0));
    for (int i=1;i<=8;++i) add(("MACRO"+std::to_string(i)).c_str(),("MACRO "+std::to_string(i)).c_str(),0,1,0);
    add("DRIVE","DRIVE",0,1,.05f); add("DELAY","DELAY",0,1,0); add("CHORUS","CHORUS",0,1,.1f); add("REVERB","REVERB",0,1,.1f);
    add("MASTER","MASTER",0,1,.75f);
    return l;
}

void SalekHightechAudioProcessor::prepareToPlay(double sr, int block)
{
    sampleRate=sr;
    synth.setCurrentPlaybackSampleRate(sr);
    limiter.prepare({sr,(juce::uint32)block,2});
    limiter.setThreshold(-1.0f); limiter.setRelease(60.0f);
    reverb.setSampleRate(sr);
    chorus.prepare({sr,(juce::uint32)block,2});
    chorus.setCentreDelay(7.0f); chorus.setDepth(0.25f); chorus.setMix(0.2f);
    delay.setMaximumDelayInSamples((int)(sr*2.0));
    delay.prepare({sr,(juce::uint32)block,2});
}

void SalekHightechAudioProcessor::releaseResources() {}

bool SalekHightechAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SalekHightechAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synth.renderNextBlock(buffer,midi,0,buffer.getNumSamples());

    const int n=buffer.getNumSamples();
    juce::dsp::AudioBlock<float> block(buffer);
    const float drive=apvts.getRawParameterValue("DRIVE")->load();
    if (drive>0.001f)
    {
        auto* l=buffer.getWritePointer(0);
        auto* r=buffer.getWritePointer(1);
        for(int i=0;i<n;++i) { l[i]=std::tanh(l[i]*(1.0f+drive*12.0f)); r[i]=std::tanh(r[i]*(1.0f+drive*12.0f)); }
    }

    chorus.setMix(apvts.getRawParameterValue("CHORUS")->load());
    chorus.process(juce::dsp::ProcessContextReplacing<float>(block));

    juce::dsp::Reverb::Parameters rp;
    rp.wetLevel=apvts.getRawParameterValue("REVERB")->load();
    rp.dryLevel=1.0f-rp.wetLevel*0.4f;
    reverb.setParameters(rp);
    reverb.processStereo(buffer.getArrayOfWritePointers(),n);

    const float master=apvts.getRawParameterValue("MASTER")->load();
    buffer.applyGain(master);
    limiter.process(juce::dsp::ProcessContextReplacing<float>(block));

    if (auto* pos = getPlayHead())
        if (auto posInfo=pos->getPosition())
            if (posInfo->getBpm()) bpm=*posInfo->getBpm();
}

juce::AudioProcessorEditor* SalekHightechAudioProcessor::createEditor()
{
    return new SalekHightechAudioProcessorEditor(*this);
}

void SalekHightechAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml=apvts.copyState().createXml())
        copyXmlToBinary(*xml,dest);
}

void SalekHightechAudioProcessor::setStateInformation(const void* data,int size)
{
    if (auto xml=getXmlFromBinary(data,size))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

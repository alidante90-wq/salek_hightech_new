#include "PluginEditor.h"

SalekHightechAudioProcessorEditor::SalekHightechAudioProcessorEditor(SalekHightechAudioProcessor& p)
: AudioProcessorEditor(&p), proc(p)
{
    setResizable(true,true);
    setSize(1280,780);
    setResizeLimits(980,620,2200,1400);

    addKnob("OSC1_LEVEL","OSC 1");
    addKnob("OSC2_LEVEL","OSC 2");
    addKnob("OSC3_LEVEL","OSC 3");
    addKnob("OSC1_WT","WT 1");
    addKnob("OSC2_WT","WT 2");
    addKnob("OSC3_WT","WT 3");
    addKnob("OSC1_WARP","WARP 1");
    addKnob("OSC2_WARP","WARP 2");
    addKnob("OSC3_WARP","WARP 3");
    addKnob("FM","FM");
    addKnob("RING","RM");
    addKnob("SUB","SUB");
    addKnob("NOISE","NOISE");
    addKnob("CUTOFF","CUTOFF");
    addKnob("RESONANCE","RES");
    addKnob("FILTER_ENV","ENV");
    addKnob("LFO_RATE","LFO");
    addKnob("LFO_AMOUNT","LFO AMT");
    for(int i=1;i<=8;++i) addKnob("MACRO"+juce::String(i),"M"+juce::String(i));
    addKnob("DRIVE","DRIVE");
    addKnob("DELAY","DELAY");
    addKnob("CHORUS","CHORUS");
    addKnob("REVERB","REVERB");
    addKnob("MASTER","MASTER");

    sync=std::make_unique<juce::ComboBox>();
    sync->addItemList(juce::StringArray{"FREE","1/4","1/2","1/1","2/1","4/1","8/1"},1);
    addAndMakeVisible(*sync);
    syncAttachment=std::make_unique<ComboAttachment>(proc.apvts,"LFO_SYNC",*sync);
    startTimerHz(30);
}

void SalekHightechAudioProcessorEditor::addKnob(const juce::String& id,const juce::String& name)
{
    auto s=std::make_unique<juce::Slider>();
    s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,70,18);
    s->setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff28f5ff));
    s->setColour(juce::Slider::thumbColourId,juce::Colour(0xffff3bd4));
    addAndMakeVisible(*s);
    auto a=std::make_unique<SliderAttachment>(proc.apvts,id,*s);
    knobs.push_back({s.get(),name});
    sliders.push_back(std::move(s));
    attachments.push_back(std::move(a));
    labels.emplace_back();
    auto& lab=labels.back();
    lab.setText(name,juce::dontSendNotification);
    lab.setJustificationType(juce::Justification::centred);
    lab.setColour(juce::Label::textColourId,juce::Colour(0xffd9faff));
    addAndMakeVisible(lab);
}

void SalekHightechAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto b=getLocalBounds().toFloat();
    juce::ColourGradient bg(juce::Colour(0xff050018),0,0,juce::Colour(0xff16002f),b.getWidth(),b.getHeight(),true);
    g.setGradientFill(bg); g.fillAll();

    // Lightweight cached-style geometry: no image assets, no expensive animation.
    g.setColour(juce::Colour(0xff10153a)); g.fillRoundedRectangle(b.reduced(12),28.0f);
    g.setColour(juce::Colour(0xff2cf2ff).withAlpha(.28f));
    g.drawRoundedRectangle(b.reduced(12),28.0f,2.0f);

    g.setColour(juce::Colour(0xffff39d7));
    g.setFont(juce::FontOptions(44.0f).withStyle("Bold"));
    g.drawText("SALEK HIGHTECH",35,20,620,58,juce::Justification::left);

    g.setColour(juce::Colour(0xff78ff3d));
    g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    g.drawText("HYPERCORE // REALTIME SYNTHESIS // 170–190+ BPM",38,72,500,22,juce::Justification::left);

    auto core=getLocalBounds().withTrimmedTop(105).withTrimmedBottom(185).reduced(24);
    auto c=core.getCentre().toFloat();
    float r=juce::jmin(core.getWidth(),core.getHeight())*.27f;
    g.setColour(juce::Colour(0xff151c48)); g.fillEllipse(c.x-r,c.y-r,r*2,r*2);
    g.setColour(juce::Colour(0xff28f5ff).withAlpha(.75f)); g.drawEllipse(c.x-r,c.y-r,r*2,r*2,3.0f);
    g.setColour(juce::Colour(0xffff3bd4).withAlpha(.5f)); g.drawEllipse(c.x-r*.78f,c.y-r*.78f,r*1.56f,r*1.56f,2.0f);

    juce::Path wave;
    for(int i=0;i<240;++i)
    {
        float x=(float)i/239.0f;
        float y=std::sin(x*juce::MathConstants<float>::twoPi*4.0f + phase)*.35f
              + .16f*std::sin(x*juce::MathConstants<float>::twoPi*11.0f-phase*2.0f);
        float px=c.x-r*.82f+x*r*1.64f, py=c.y+y*r*.78f;
        if(i==0) wave.startNewSubPath(px,py); else wave.lineTo(px,py);
    }
    g.setColour(juce::Colour(0xff78ff3d)); g.strokePath(wave,juce::PathStrokeType(2.5f));
    g.setColour(juce::Colours::white.withAlpha(.55f));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("SONIC CORE",c.x-70,c.y+r*.78f,140,22,juce::Justification::centred);

    // Section labels / modulation lanes.
    g.setColour(juce::Colour(0xffff9a32));
    g.setFont(juce::FontOptions(15.0f).withStyle("Bold"));
    g.drawText("OSCILLATOR MATRIX",32,108,260,24,juce::Justification::left);
    g.drawText("MODULATION FIELD",32,core.getBottom()+12,260,24,juce::Justification::left);
    g.drawText("MACRO // FX",core.getRight()-260,core.getBottom()+12,240,24,juce::Justification::right);

    // Visible modulation connections: actual LFO amount and envelope destinations are real DSP.
    g.setColour(juce::Colour(0xff28f5ff).withAlpha(.28f));
    for(int k=0;k<4;++k)
        g.drawLine(50.0f,core.getBottom()+40.0f+k*20.0f,
                   getWidth()-50.0f,core.getBottom()+40.0f+k*20.0f,1.0f);
}

void SalekHightechAudioProcessorEditor::resized()
{
    auto area=getLocalBounds().reduced(24);
    area.removeFromTop(100);
    auto core=area.removeFromTop((int)(getHeight()*.47f));
    area.removeFromTop(36);
    auto bottom=area;

    const int cols=8;
    const int cellW=juce::jmax(90,bottom.getWidth()/cols);
    const int cellH=92;

    for(size_t i=0;i<knobs.size();++i)
    {
        int row=(int)i/cols, col=(int)i%cols;
        auto cell=bottom.withTrimmedLeft(col*cellW).withTrimmedRight(bottom.getWidth()-(col+1)*cellW)
                         .withTrimmedTop(row*cellH).withTrimmedBottom(bottom.getHeight()-(row+1)*cellH);
        knobs[i].s->setBounds(cell.reduced(8).withTrimmedBottom(22));
        labels[i].setBounds(cell.reduced(8).withTrimmedTop(70).withTrimmedBottom(0));
    }
    sync->setBounds(core.getX()+20,core.getBottom()-40,100,24);
}

void SalekHightechAudioProcessorEditor::timerCallback()
{
    phase += 0.018f;
    if(phase>juce::MathConstants<float>::twoPi) phase-=juce::MathConstants<float>::twoPi;
    repaint(); // lightweight refresh
}

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SalekHightechAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit SalekHightechAudioProcessorEditor(SalekHightechAudioProcessor&);
    ~SalekHightechAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    SalekHightechAudioProcessor& proc;
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;
    std::unique_ptr<juce::ComboBox> sync;
    std::unique_ptr<ComboAttachment> syncAttachment;
    std::vector<juce::Label> labels;

    struct Knob { juce::Slider* s; juce::String name; };
    std::vector<Knob> knobs;

    void addKnob(const juce::String& id, const juce::String& name);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SalekHightechAudioProcessorEditor)
};

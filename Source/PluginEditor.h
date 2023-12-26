/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Components
//#include "juce_gui_basics/widgets/juce_Slider.h"

//==============================================================================
/**
*/
class ConvolverAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Slider::Listener 
{
public:
    ConvolverAudioProcessorEditor (ConvolverAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~ConvolverAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:

    // Listen to a slider
    void sliderValueChanged (juce::Slider* slider) override;

    // Load IR file
    void loadIR();

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ConvolverAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& valueTreeState;

    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;

    // dry/wet slider
    juce::Label dryWetLabel;
    juce::Slider drywet;
    std::unique_ptr<SliderAttachment> dryWetAttachment;

    // Stores the currently loaded file name for IR
    juce::TextButton open_ir_button;

    // file chooser
    std::unique_ptr<juce::FileChooser> ir_chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConvolverAudioProcessorEditor)
};

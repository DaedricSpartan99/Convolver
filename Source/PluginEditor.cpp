/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ConvolverAudioProcessorEditor::ConvolverAudioProcessorEditor (ConvolverAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), open_ir_button("Open"), valueTreeState(vts)
{

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    // label
    dryWetLabel.setText ("Dry/Wet", juce::dontSendNotification);
    addAndMakeVisible (dryWetLabel);

    // these define the parameters of our slider object
    drywet.setSliderStyle (juce::Slider::LinearVertical);
    drywet.setRange (0.0, 1.0, 0.05);
    drywet.setTextBoxStyle (juce::Slider::NoTextBox, true, 90, 0); // manually insert value
    drywet.setPopupDisplayEnabled (true, false, this);
    drywet.setTextValueSuffix ("Dry/Wet");
    //drywet.setValue(1.0);
    
    // register parameters
    dryWetAttachment.reset (new SliderAttachment (valueTreeState, "drywet", drywet));

    // this function adds the slider to the editor
    addAndMakeVisible (&drywet);

    // button click callback 
    open_ir_button.onClick = [this] { loadIR(); };

    // make button visible
    addAndMakeVisible (&open_ir_button);

    // add the listener to the slider
    drywet.addListener (this);
}

ConvolverAudioProcessorEditor::~ConvolverAudioProcessorEditor()
{
}

void ConvolverAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    audioProcessor.setWetProportion(drywet.getValue());
}

void ConvolverAudioProcessorEditor::loadIR()
{
    ir_chooser = std::make_unique<juce::FileChooser> ("Please select the wav you want to load...",
                                               juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                               "*.wav");
 
    auto folderChooserFlags = juce::FileBrowserComponent::openMode; // | juce::FileBrowserComponent::canSelectDirectories;
 
    ir_chooser->launchAsync (folderChooserFlags, [this] (const juce::FileChooser& chooser)
    {
        juce::File&& wav(chooser.getResult());
        
        // update button
        open_ir_button.setButtonText(wav.getFileName());
        open_ir_button.changeWidthToFitText();

        // load the file into the processor (as rvalue)
        audioProcessor.setIRFile(std::move(wav));
    });
}    


//==============================================================================
void ConvolverAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.fillAll (juce::Colours::white);

    g.setColour (juce::Colours::black);
    g.setFont (15.0f);
    g.drawFittedText ("Convolver", getLocalBounds(), juce::Justification::centred, 1);
}

void ConvolverAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    // sets the position and size of the slider with arguments (x, y, width, height)
    drywet.setBounds (40, 30, 20, getHeight() - 60);

    // button bounds
    open_ir_button.setBounds(70, 20, 50, 20);
    open_ir_button.changeWidthToFitText ();
}

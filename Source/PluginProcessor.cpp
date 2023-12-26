/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ConvolverAudioProcessor::ConvolverAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties() 
                      #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
      parameters(*this, nullptr, 
         juce::Identifier ("APVTS"),
           {
               std::make_unique<juce::AudioParameterFloat> (
                   "drywet",            // parameterID
                   "DryWet",            // parameter name
                   0.0f,              // minimum value
                   1.0f,              // maximum value
                   1.0f),             // default value
           }
         )
#endif
{
}

ConvolverAudioProcessor::~ConvolverAudioProcessor()
{
}

void ConvolverAudioProcessor::setIRFile(juce::File&& file) {

    // affect IR
    impulse_response = std::move(file);
    
    // update convolution
    updateConvolution();
}


void ConvolverAudioProcessor::setTrim(float min, float max) {
    
    trim_min = min;
    trim_max = max;

    // update convolution
    updateConvolution();
}

void ConvolverAudioProcessor::updateConvolution() {
  
    // load file (TODO: trim option)
    convolution.loadImpulseResponse(impulse_response, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, 0);

    // re-prepare convolution
    //convolution.prepare(specs);

    DBG ( "Convolution updated" );
}

void ConvolverAudioProcessor::setWetProportion(float value) {

    dryWetMixer.setWetMixProportion(value);

    // prepare dry/wet
    //dryWetMixer.prepare(specs);

    DBG ( "Dry/Wet updated" );
}

//==============================================================================
const juce::String ConvolverAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ConvolverAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ConvolverAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ConvolverAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ConvolverAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ConvolverAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ConvolverAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ConvolverAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ConvolverAudioProcessor::getProgramName (int index)
{
    return {};
}

void ConvolverAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ConvolverAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    //
    //convBuffer = juce::AudioBuffer<float>(getTotalNumInputChannels(), samplesPerBlock);

    // specify sampling parameters
    juce::dsp::ProcessSpec specs;

    specs.sampleRate = getSampleRate();  
    specs.maximumBlockSize = getBlockSize();
    specs.numChannels = getTotalNumInputChannels();

    // prepare convolution
    convolution.prepare(specs);

    // prepare dry/wet
    dryWetMixer.prepare(specs);

    DBG ( "Sources are ready " );
}

void ConvolverAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ConvolverAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ConvolverAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    using namespace juce;

    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();


    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
   

    // 
    auto inoutBlock = dsp::AudioBlock<float> (buffer).getSubsetChannelBlock (0, (size_t) totalNumInputChannels);
    dsp::ProcessContextReplacing<float> procContext(inoutBlock);

    // copy dry samples
    /*AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);
    auto dryBlock = dsp::AudioBlock<float> (dryBuffer).getSubsetChannelBlock (0, (size_t) totalNumInputChannels);*/
    dryWetMixer.pushDrySamples(inoutBlock);

    // process convolution
    convolution.process(procContext);

    // process dry/wet
    dryWetMixer.mixWetSamples(inoutBlock);

    /*for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        const auto* convData = convBuffer.getReadPointer (channel);

        // Mix into dry/wet 
        //*channelData = *convData; //*channelData * (1.0 - drywet) + *convData * drywet;
    }*/
}

//==============================================================================
bool ConvolverAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ConvolverAudioProcessor::createEditor()
{
    return new ConvolverAudioProcessorEditor (*this, parameters);
}

//==============================================================================
void ConvolverAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ConvolverAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ConvolverAudioProcessor();
}

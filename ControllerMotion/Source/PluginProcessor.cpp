/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

//==============================================================================
MIDIControllerMotionAudioProcessor::MIDIControllerMotionAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    tempoBpm = 120.0;
    lastBufferTimestamp = 0;
    
    // Note this plugin allows phrase length 1 beat for "real time" control.
    // (The clip variation plugins start at 4 beats.)
    addParameter (phraseBeats = new juce::AudioParameterChoice (
        "phraseBeats", // parameterID
        "Phrase length", // parameter name
        juce::StringArray( {"1 beat", "4 beats", "8 beats", "16 beats", "32 beats", "64 beats"} ),
        2 // default index
    ));

    for (int i=0; i<CBR_CCMOTION_NUM_PARAMS; i++) {
        int controllerNumber = i + 1;

        std::ostringstream paramIdentifier;
        paramIdentifier << "target" << controllerNumber;
        std::ostringstream paramName;
        paramName << "Target " << controllerNumber;

        currentValue[i] = 0;
        lastOutputCC[i] = 0;
        addParameter (destinationValue[i] = new juce::AudioParameterFloat (
            paramIdentifier.str(), // parameterID
            paramName.str(), // parameter name
            0.0,
            1.0,
            0.0
        ));
    }

    addParameter (firstCCNumber = new juce::AudioParameterInt (
        "firstCC", // parameterID
        "First CC number", // parameter name
        1,
        127,
        1
    ));
    addParameter (channelNumber = new juce::AudioParameterInt (
        "channelNumber", // parameterID
        "Channel", // parameter name
        1,
        16,
        1
    ));
}

double MIDIControllerMotionAudioProcessor::getPhraseBeats ()
{
    int selected = phraseBeats->getIndex();
    
    switch (selected) {
        case 0: return 1;
        case 1: return 4;
        case 2: return 8;
        case 3: return 16;
        case 4: return 32;
        case 5: return 64;
    }
    
    return 4;
}

MIDIControllerMotionAudioProcessor::~MIDIControllerMotionAudioProcessor()
{
}

//==============================================================================
const juce::String MIDIControllerMotionAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MIDIControllerMotionAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MIDIControllerMotionAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MIDIControllerMotionAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MIDIControllerMotionAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MIDIControllerMotionAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MIDIControllerMotionAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MIDIControllerMotionAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MIDIControllerMotionAudioProcessor::getProgramName (int index)
{
    return {};
}

void MIDIControllerMotionAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MIDIControllerMotionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void MIDIControllerMotionAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MIDIControllerMotionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

double MIDIControllerMotionAudioProcessor::samplesToBeats(juce::int64 timestamp) {
    double secondsPerBeat = 60.0 / tempoBpm;
    double samplesPerBeat = secondsPerBeat * getSampleRate();
    
    return (timestamp / samplesPerBeat);
}

double MIDIControllerMotionAudioProcessor::beatsToPhrase(double beats) {
    return (beats / getPhraseBeats());
}

bool MIDIControllerMotionAudioProcessor::timeRangeStraddlesPhraseChange (juce::int64 time1, juce::int64 time2) {
    double a = samplesToBeats(time1);
    double b = samplesToBeats(time2);

    // Fudge factor alert!
    // Sometimes we get a floating point rounding error.
    // E.g. aPhrase = 2, bPhrase = 2.99998
    // In this case the phrase is different and we need to catch it.
    // We add 0.001 phrases because that's much smaller
    // than the period we're concerned with, and larger
    // than the rounding error.
    double aPhrase = beatsToPhrase(a) + 0.001;
    double bPhrase = beatsToPhrase(b) + 0.001;

    // Determine whether to use current channel or param channel for this event.
    // If phrase boundary has occurred since start of block, use param.
    return ( std::floor(aPhrase) < std::floor(bPhrase) );
}

void MIDIControllerMotionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    outputMidiBuffer.clear();
  
    juce::int64 playheadTimeSamples = 0;

    bool isPlaying = false;
    juce::AudioPlayHead::CurrentPositionInfo playheadPosition;
    juce::AudioPlayHead* playhead = AudioProcessor::getPlayHead();
    if (playhead) {
        playhead->getCurrentPosition(playheadPosition);
        isPlaying = playheadPosition.isPlaying;
        playheadTimeSamples = playheadPosition.timeInSamples;
        tempoBpm = playheadPosition.bpm;
    }

    // Determine time left in current phrase (normalised 0-1).
    double currentPhrase = beatsToPhrase(
        samplesToBeats(playheadTimeSamples)
    );
    double wholePhrase = 0;
    double currentPhrasePosition = modf(currentPhrase, &wholePhrase);
    double phraseRemaining = 1.0 - currentPhrasePosition;
    
    // How long is current block in phrase time?
    juce::int64 blockTimeSamples = playheadTimeSamples - lastBufferTimestamp;
    double blockTimeBeats = samplesToBeats(blockTimeSamples);
    double blockPhraseTime = beatsToPhrase(
        blockTimeBeats
    );
    
    for (int i=0; i<CBR_CCMOTION_NUM_PARAMS; i++) {
        int controllerNumber = i + *firstCCNumber;

        // Interpolate current => destination value.
        double increment = blockPhraseTime / phraseRemaining;
        double targetValue = destinationValue[i]->get();
        double distanceToTarget = targetValue - currentValue[i];
        double outputValue = currentValue[i] + increment * distanceToTarget;
        
        // We have jumped time or looped around (or are paused).
        // Jump to the target value ASAP.
        if (!isPlaying || blockTimeSamples < 0) {
            outputValue = targetValue;
        }
        
        int newCCValue = juce::MidiMessage::floatValueToMidiByte(outputValue);

        // If the value has changed, output a CC event.
        if ( lastOutputCC[i] != newCCValue ) {        
            // Add event immediately (in future can align these with beats and/or phrase boundary).
            int channel = *channelNumber;
            midiMessages.addEvent(
                juce::MidiMessage::controllerEvent(
                   channel,
                   controllerNumber,
                   newCCValue
                ),
                0
            );
            lastOutputCC[i] = newCCValue;
        }
        
        // Update the floating-point value of the param.
        currentValue[i] = outputValue;
    }
    
    lastBufferTimestamp = playheadTimeSamples;
}

//==============================================================================
bool MIDIControllerMotionAudioProcessor::hasEditor() const
{
    return false;
}

juce::AudioProcessorEditor* MIDIControllerMotionAudioProcessor::createEditor()
{
    return 0;
}

//==============================================================================
void MIDIControllerMotionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void MIDIControllerMotionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MIDIControllerMotionAudioProcessor();
}

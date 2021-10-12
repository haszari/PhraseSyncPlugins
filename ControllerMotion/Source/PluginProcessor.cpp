/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

//==============================================================================
MIDIControllerMotionAudioProcessor::MIDIControllerMotionAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    :
    AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
        parameters (*this, nullptr, juce::Identifier (JucePlugin_Name),
            {
                // Note this plugin allows phrase length 1 beat for "real time" control.
                // (The clip variation plugins start at 4 beats.)
                std::make_unique<juce::AudioParameterChoice> (
                    "phraseBeats", // parameterID
                    "Phrase length", // parameter name
                    juce::StringArray( {"1 beat", "4 beats", "8 beats", "16 beats", "32 beats", "64 beats"} ),
                    2 // default index
                ),
            
            // Moving CC parameters. The number of these params should match CBR_CCMOTION_NUM_PARAMS constant.
                std::make_unique<juce::AudioParameterFloat> (
                    "target1", // parameterID
                    "Target 1", // parameter name
                    0.0, 1.0, 0.0
                ),
                std::make_unique<juce::AudioParameterFloat> (
                    "target2", // parameterID
                    "Target 2", // parameter name
                    0.0, 1.0, 0.0
                ),
                std::make_unique<juce::AudioParameterFloat> (
                    "target3", // parameterID
                    "Target 3", // parameter name
                    0.0, 1.0, 0.0
                ),
                std::make_unique<juce::AudioParameterFloat> (
                    "target4", // parameterID
                    "Target 4", // parameter name
                    0.0, 1.0, 0.0
                ),
            
                std::make_unique<juce::AudioParameterInt> (
                    "firstCCNumber", // parameterID
                    "First CC number", // parameter name
                    1,
                    127,
                    1
                ),
                std::make_unique<juce::AudioParameterInt> (
                    "channelNumber", // parameterID
                    "Channel", // parameter name
                    1,
                    16,
                    1
                ),

                // Config parameters for outputing phrase position and length.
                // This allows showing phrase info in LED displays or VU meters etc.

                // Output phrase position % over the range 1-127.
                // Specify CC=0 to disable.
                 std::make_unique<juce::AudioParameterInt> (
                    "outPhrasePosCCNumber", 
                    "CC out - Phrase position",
                    0,
                    127,
                    0
                ),
                std::make_unique<juce::AudioParameterInt> (
                    "outPhrasePosChannel", 
                    "Ch out - Phrase position", 
                    1,
                    16,
                    16
                ),
                // Output phrase length in as a CC value.
                // TBD how this value is encoded - targeting a 7-LED VU meter on Traktor Kontrol S4.
                // Specify CC=0 to disable.
                 std::make_unique<juce::AudioParameterInt> (
                    "outPhraseLengthCCNumber", 
                    "CC out - Phrase length",
                    0,
                    127,
                    0
                ),
                std::make_unique<juce::AudioParameterInt> (
                    "outPhraseLengthChannel", 
                    "Ch out - Phrase length", 
                    1,
                    16,
                    16
                ),

           } )
#endif
{
    tempoBpm = 120.0;
    lastBufferTimestamp = 0;
    
    // Assuming it's ok to just cast these to specific param type.
    phraseBeats = (juce::AudioParameterChoice*)parameters.getParameter("phraseBeats");
    firstCCNumber = (juce::AudioParameterInt*)parameters.getParameter("firstCCNumber");
    channelNumber = (juce::AudioParameterInt*)parameters.getParameter("channelNumber");

    for (int i=0; i<CBR_CCMOTION_NUM_PARAMS; i++) {
        int controllerNumber = i + 1;

        std::ostringstream paramIdentifier;
        paramIdentifier << "target" << controllerNumber;

        currentValue[i] = 0;
        lastOutputCC[i] = 0;
        destinationValue[i] = (juce::AudioParameterFloat*)parameters.getParameter(paramIdentifier.str());
    }
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

    if (isPlaying) {
        outputPhraseInfoAsCCs(currentPhrasePosition, midiMessages);
    }
    
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

void MIDIControllerMotionAudioProcessor::outputPhraseInfoAsCCs (double position, juce::MidiBuffer& midiMessages)
{
    juce::AudioParameterInt* ccNumber;
    juce::AudioParameterInt* channel;
    int cc, ch;

    // Phrase position.
    ccNumber = (juce::AudioParameterInt*)parameters.getParameter("outPhrasePosCCNumber");
    channel = (juce::AudioParameterInt*)parameters.getParameter("outPhrasePosChannel");
    cc = ccNumber->get();
    ch = channel->get();
    if ( cc ) {
        midiMessages.addEvent(
            juce::MidiMessage::controllerEvent(
                ch,
                cc,
                juce::MidiMessage::floatValueToMidiByte(position)
            ),
            0
        );
    }

    // Phrase length.
    int selected = phraseBeats->getIndex(); // Option index, 0-5.
    const int ledCount = 7; // Full LED range is 0-7.
    const double ccPerLed = 127.0 / ledCount;
    int ccValue = selected * ccPerLed;
    ccNumber = (juce::AudioParameterInt*)parameters.getParameter("outPhraseLengthCCNumber");
    channel = (juce::AudioParameterInt*)parameters.getParameter("outPhraseLengthChannel");
    cc = ccNumber->get();
    ch = channel->get();
    if ( cc ) {
        midiMessages.addEvent(
            juce::MidiMessage::controllerEvent(
                ch,
                cc,
                ccValue
            ),
            0
        );
    }


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
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MIDIControllerMotionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MIDIControllerMotionAudioProcessor();
}

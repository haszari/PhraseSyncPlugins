/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

//==============================================================================
MIDIClipVariationsAudioProcessor::MIDIClipVariationsAudioProcessor()
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
                std::make_unique<juce::AudioParameterInt> (
                    "channel", // parameterID
                    "Channel", // parameter name
                    1,   // minimum value
                    16,   // maximum value
                    1
                ),

                // Now allows phrase length 1 beat for consistency with ControllerMotion param.
                std::make_unique<juce::AudioParameterChoice> (
                    "phraseBeats", // parameterID
                    "Phrase length", // parameter name
                    juce::StringArray( {"1 beat", "4 beats", "8 beats", "16 beats", "32 beats", "64 beats"} ),
                    2 // default index
                )
            } )
#endif
{
    tempoBpm = 120.0;
    lastBufferTimestamp = 0;
    currentAllowedChannel = 1;
    
    selectedChannel = (juce::AudioParameterInt*)parameters.getParameter("channel");
    phraseBeats = (juce::AudioParameterChoice*)parameters.getParameter("phraseBeats");
}

int MIDIClipVariationsAudioProcessor::getPhraseBeats ()
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
MIDIClipVariationsAudioProcessor::~MIDIClipVariationsAudioProcessor()
{
}

//==============================================================================
const juce::String MIDIClipVariationsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MIDIClipVariationsAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MIDIClipVariationsAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MIDIClipVariationsAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MIDIClipVariationsAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MIDIClipVariationsAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MIDIClipVariationsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MIDIClipVariationsAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MIDIClipVariationsAudioProcessor::getProgramName (int index)
{
    return {};
}

void MIDIClipVariationsAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MIDIClipVariationsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void MIDIClipVariationsAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MIDIClipVariationsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

double MIDIClipVariationsAudioProcessor::samplesToBeats(juce::int64 timestamp) {
    double secondsPerBeat = 60.0 / tempoBpm;
    double samplesPerBeat = secondsPerBeat * getSampleRate();
    
    return (timestamp / samplesPerBeat);
}

double MIDIClipVariationsAudioProcessor::beatsToPhrase(double beats) {    
    return (beats / getPhraseBeats());
}

bool MIDIClipVariationsAudioProcessor::timeRangeStraddlesPhraseChange (juce::int64 time1, juce::int64 time2) {
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

bool MIDIClipVariationsAudioProcessor::shouldPlayMidiMessage (juce::MidiMessage message, juce::int64 blockTime, juce::int64 eventTime)
{
    if (! message.isNoteOn()) {
        // pass anything except note-ons
        return true;
    }
    
    int channel = currentAllowedChannel;
    
    // Determine whether to use current channel or param channel for this event.
    // If phrase boundary has occurred since start of block, use param.
    if ( timeRangeStraddlesPhraseChange(blockTime, eventTime) ) {
        channel = *selectedChannel;
    }

    return (message.getChannel() == channel);
}


void MIDIClipVariationsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    outputMidiBuffer.clear();
  
    const int allowChannel = *selectedChannel;

    juce::int64 playheadTimeSamples = 0;

    juce::AudioPlayHead::CurrentPositionInfo playheadPosition;
    juce::AudioPlayHead* playhead = AudioProcessor::getPlayHead();
    if (playhead) {
        playhead->getCurrentPosition(playheadPosition);
        playheadTimeSamples = playheadPosition.timeInSamples;
        tempoBpm = playheadPosition.bpm;
    
        if (! playheadPosition.isPlaying) {
            currentAllowedChannel = allowChannel;
        }
        else {
            // Determine if the last block straddled a phrase boundary.
            bool lastBlockNewPhrase = timeRangeStraddlesPhraseChange(lastBufferTimestamp, playheadTimeSamples);
            // Or if the transport has looped back around start.
            bool reloopNewPhrase = (lastBufferTimestamp > playheadTimeSamples);
            // If so, apply the channel param.
            if (lastBlockNewPhrase || reloopNewPhrase) {
                currentAllowedChannel = allowChannel;
            }
        }
    }
    else {
        currentAllowedChannel = allowChannel;
    }

    for (auto m: midiMessages)
    {
        auto message = m.getMessage();
        auto timestamp = message.getTimeStamp();
        
        if (this->shouldPlayMidiMessage(message, playheadTimeSamples, playheadTimeSamples + timestamp)) {
            outputMidiBuffer.addEvent(message, timestamp);
        }

    }

    midiMessages.swapWith(outputMidiBuffer);
    
    lastBufferTimestamp = playheadTimeSamples;
}

//==============================================================================
bool MIDIClipVariationsAudioProcessor::hasEditor() const
{
    return false;
}

juce::AudioProcessorEditor* MIDIClipVariationsAudioProcessor::createEditor()
{
    return 0;
}

//==============================================================================
void MIDIClipVariationsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MIDIClipVariationsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new MIDIClipVariationsAudioProcessor();
}

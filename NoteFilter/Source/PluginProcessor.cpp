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
                    "variation", // parameterID
                    "Variation", // parameter name
                    1,   // minimum value
                    16,   // maximum value TBD
                    1
                ),

                // Now allows phrase length 1 beat for consistency with ControllerMotion param.
                std::make_unique<juce::AudioParameterChoice> (
                    "phraseBeats", // parameterID
                    "Phrase length", // parameter name
                    juce::StringArray( {"1 beat", "4 beats", "8 beats", "16 beats", "32 beats", "64 beats"} ),
                    2 // default index
                ),
            
                std::make_unique<juce::AudioParameterChoice> (
                    "notesPerVariation", // parameterID
                    "Variation height", // parameter name
                    juce::StringArray( {"6 semitones / half octave", "1 octave", "2 octaves", "3 octaves"} ),
                    1 // default index
                )
            } )
#endif
{
    tempoBpm = 120.0;
    lastBufferTimestamp = 0;
    currentVariation = 0; // Zero based .. is that confusing, compared to channel plugin?
    
    selectedVariation = (juce::AudioParameterInt*)parameters.getParameter("variation");
    phraseBeats = (juce::AudioParameterChoice*)parameters.getParameter("phraseBeats");
    notesPerVariation = (juce::AudioParameterChoice*)parameters.getParameter("notesPerVariation");
}

int MIDIClipVariationsAudioProcessor::getSemitonesPerVariation ()
{
    int selected = notesPerVariation->getIndex();
    
    // This looks like pow(2, index) but it's not.
    switch (selected) {
        case 0: return 6;
        case 1: return 12;
        case 2: return 24;
        case 3: return 36;
    }
    
    return 12;
}

int MIDIClipVariationsAudioProcessor::getPhraseBeats ()
{
    int selected = phraseBeats->getIndex();
    
    // This looks like pow(2, index) but it's not.
    switch (selected) {
        case 0: return 4;
        case 1: return 8;
        case 2: return 16;
        case 3: return 32;
        case 4: return 64;
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

bool MIDIClipVariationsAudioProcessor::processNote (juce::MidiMessage& message, juce::int64 blockTime, juce::int64 eventTime)
{
    if (! message.isNoteOnOrOff()) {
        return true;
    }

    auto originalNote = message.getNoteNumber();
    
    int variation = currentVariation;
    int variationHeight = getSemitonesPerVariation();
    
    // If phrase boundary has occurred since start of block, use the new selected variation.
    if ( timeRangeStraddlesPhraseChange(blockTime, eventTime) ) {
        variation = *selectedVariation;
    }

    int variationStartNote = variation * variationHeight;

    // Transpose all notes down into normalised range.
    // Note 1: (pun intended!) this modifies the passed in note by reference.
    // Note 2: this seems to start at the second-lowest octave in the DAWs I tried (Reaper, Bitwig).
    // I had expected note zero would be C-2, bottom of the range.
    message.setNoteNumber(originalNote - variationStartNote);
    
    // We may need special handling of note offs now - we need to track them and ensure they get played,
    // and not play all of them, since they might clash with existing notes (after transpose).
    // The bug is that note-offs near the end of the phrase may get lost – dangling notes.
//    if (message.isNoteOff()) {
//        // Pass all note-offs.
//        return true;
//    }
    
    // Filter notes within the (normalised) range.
    bool noteInVariation = (message.getNoteNumber() >= 0) && (message.getNoteNumber() < (0 + variationHeight));
    return noteInVariation;
}


void MIDIClipVariationsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    outputMidiBuffer.clear();
  
    const int variation = *selectedVariation;

    juce::int64 playheadTimeSamples = 0;

    juce::AudioPlayHead::CurrentPositionInfo playheadPosition;
    juce::AudioPlayHead* playhead = AudioProcessor::getPlayHead();
    if (playhead) {
        playhead->getCurrentPosition(playheadPosition);
        playheadTimeSamples = playheadPosition.timeInSamples;
        tempoBpm = playheadPosition.bpm;
    
        if (! playheadPosition.isPlaying) {
            currentVariation = variation;
        }
        else {
            // Determine if the last block straddled a phrase boundary.
            bool lastBlockNewPhrase = timeRangeStraddlesPhraseChange(lastBufferTimestamp, playheadTimeSamples);
            // Or if the transport has looped back around start.
            bool reloopNewPhrase = (lastBufferTimestamp > playheadTimeSamples);
            // If so, apply the channel param.
            if (lastBlockNewPhrase || reloopNewPhrase) {
                currentVariation = variation;
            }
        }
    }
    else {
        currentVariation = variation;
    }

    for (auto m: midiMessages)
    {
        auto message = m.getMessage();
        auto timestamp = message.getTimeStamp();
        
        auto unprocessedMessage = message;
        
        // Process the current note.
        // This determines if it is in the current variation's note range,
        // AND transposes the note down into normal range (passed by ref).
        if (this->processNote(message, playheadTimeSamples, playheadTimeSamples + timestamp)) {
            if (message.isNoteOnOrOff()) {
                std::cout << message.getNoteNumber() << " "
                    << unprocessedMessage.getDescription() << " playing as "
                    << message.getDescription() << std:: endl;
            }
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

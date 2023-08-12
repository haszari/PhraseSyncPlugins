/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

const int LineTogglerAudioProcessor::notesPerLine[] = { 2, 2, 4, 4 };

// 2 octaves below first line note (36). In future might shift to MIDI zero.
const int LineTogglerAudioProcessor::lineControlNotes[] = { 12, 13, 14, 15 };

//==============================================================================
LineTogglerAudioProcessor::LineTogglerAudioProcessor()
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
            std::make_unique<juce::AudioParameterBool> (
                "lineEnable1", // parameterID
                "Enable line 1", // parameter name
                true
            ),
            std::make_unique<juce::AudioParameterBool> (
                "lineEnable2", // parameterID
                "Enable line 2", // parameter name
                true
            ),
            std::make_unique<juce::AudioParameterBool> (
                "lineEnable3", // parameterID
                "Enable line 3", // parameter name
                true
            ),
            std::make_unique<juce::AudioParameterBool> (
                "lineEnable4", // parameterID
                "Enable line 4", // parameter name
                true
            ),

            } )
#endif
{
    for (int i=0; i<CBR_TOGGLELINES_NUM_LINES; i++) {
        int lineNumber = i + 1;

        std::ostringstream paramIdentifier;
        paramIdentifier << "lineEnable" << lineNumber;

        allowLinePlayback[i] = (juce::AudioParameterBool*)parameters.getParameter(paramIdentifier.str());

        lineGate[i] = true;
    }

}

LineTogglerAudioProcessor::~LineTogglerAudioProcessor()
{
}

//==============================================================================
const juce::String LineTogglerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LineTogglerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LineTogglerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LineTogglerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LineTogglerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LineTogglerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int LineTogglerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LineTogglerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String LineTogglerAudioProcessor::getProgramName (int index)
{
    return {};
}

void LineTogglerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void LineTogglerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void LineTogglerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LineTogglerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

/**
 * Determine if a MIDI note is a synth note for a line.
 *
 * @param midiNoteNumber The MIDI note number to check.
 * @return The slot index for the note, or -1 if the note is not in a slot.
*/
int LineTogglerAudioProcessor::getSlotIndexForNote(const int midiNoteNumber) {
    int slotStartNote = CBR_TOGGLELINES_FIRST_MIDI_NOTE;

    // Iterate over each line, and check if the note is in the line's range.
    for (int i = 0; i < CBR_TOGGLELINES_NUM_LINES; i++)
    {
        int nextSlotStart = slotStartNote + notesPerLine[i];
        if ( midiNoteNumber >= slotStartNote && midiNoteNumber < nextSlotStart ) {
            return  i;
        }
        slotStartNote = nextSlotStart;
    }

    // Not in a slot!
    return -1;
}

/**
 * Determine if a MIDI note is a control note for a line.
 *
 * @param midiNoteNumber The MIDI note number to check.
 * @return The slot index that the note controls, or -1.
*/
int LineTogglerAudioProcessor::getSlotIndexForControlNote(const int midiNoteNumber) {
    for (int i = 0; i < CBR_TOGGLELINES_NUM_LINES; i++)
    {
        if ( midiNoteNumber == lineControlNotes[i] ) {
            return  i;
        }
    }

    return -1;
}

const juce::MidiMessageMetadata* findNoteOnEvent(
    juce::MidiBuffer& midiMessages,
    int afterTimestamp,
    int midiNoteValue
) {

    for (const juce::MidiMessageMetadata metadata : midiMessages) {
       const juce:: MidiMessage event = metadata.getMessage();
        if (event.isNoteOn()) {
            if (event.getNoteNumber() == midiNoteValue &&
                metadata.samplePosition >= afterTimestamp) {

                // THIS IS A POINTER – we should avoid this and use a reference or a copy. (Compile warning)
                return &metadata;
            }
        }
    }

    return nullptr;
}

void LineTogglerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    outputMidiBuffer.clear();

    // TODO: Check transport state, if not playing back then disable all gates / let everything though.
    // TODO: Pass through all unrelated MIDI events (not control notes or notes in lines).

    // Loop over each slot / line.
    for (int slotIndex = 0; slotIndex < CBR_TOGGLELINES_NUM_LINES; slotIndex++) {
        const int bufferEndTime = buffer.getNumSamples();
        int processStartTime = 0, processEndTime = buffer.getNumSamples();
        int controlNoteSearchStart = 0;
        bool nextGateState = lineGate[slotIndex];

        // Iterate through the buffer one control note at a time.
        while ( processStartTime < bufferEndTime ) {
            // Search for next control note change for the current slot.
            const juce::MidiMessageMetadata* controlNoteOn = findNoteOnEvent(midiMessages, controlNoteSearchStart, lineControlNotes[slotIndex]);

            // We found a control note:
            // - Adjust time range to before the note takes effect.
            // - Stash the toggle param value to apply it for next iteration.
            if (controlNoteOn != nullptr) {
                processEndTime = controlNoteOn->samplePosition - 1;
                controlNoteSearchStart++;
                nextGateState = allowLinePlayback[slotIndex]->get();
            }
            else {
                processEndTime = buffer.getNumSamples();
            }

            // Now we have a time range to process - processStartTime…processEndTime.
            // We have at least two tasks for this time period:
            // 1. Apply the current line gate - let notes through or not.
            // 2. If param changes gate state in this period, apply the change at the end.
            // 3. (bonus) Gate ALL control note events (on and off) so they don't play synth notes. Could do this manually but cleaner to do within the plugin automatically.

            // Loop over all events.
            for (auto midiMessageIt = midiMessages.begin(); midiMessageIt != midiMessages.end(); ++midiMessageIt) {

                const juce::MidiMessageMetadata metadata = *midiMessageIt;
                const juce::MidiMessage m = metadata.getMessage();

                // Skip events not in our time period of interest.
                if ( metadata.samplePosition < processStartTime ||
                    metadata.samplePosition > processEndTime ) {
                    continue;
                }

                // Find out what kind of event this is.
                const int noteNumber = m.getNoteNumber();
                const int eventSlotIndex = this->getSlotIndexForNote(noteNumber); // It's a note in a line.
                const bool isNoteOn = m.isNoteOn();

                // Is this event in a line? If so, appropriately gate it.
                if ( eventSlotIndex != -1 ) {
                    if ( eventSlotIndex == slotIndex ) {
                        if ( isNoteOn && lineGate[slotIndex] ) {
                            outputMidiBuffer.addEvent(m, metadata.samplePosition);
                        }
                        else if ( ! isNoteOn ) {
                            outputMidiBuffer.addEvent(m, metadata.samplePosition);
                        }
                    }

                }
            } // End loop over all events.

            // Now we've processed all the events up to processEndTime, iterate.
            lineGate[slotIndex] = nextGateState;
            processStartTime = processEndTime;
        } // End while loop for each control note for the current slot.
    } // End loop over each slot / line.

    midiMessages.swapWith(outputMidiBuffer);
}

//==============================================================================
bool LineTogglerAudioProcessor::hasEditor() const
{
    return false;
}

juce::AudioProcessorEditor* LineTogglerAudioProcessor::createEditor()
{
    return 0;
}

//==============================================================================
void LineTogglerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LineTogglerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new LineTogglerAudioProcessor();
}

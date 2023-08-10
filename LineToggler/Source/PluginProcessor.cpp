/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

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
            std::make_unique<juce::AudioParameterBool> (
                "lineEnable5", // parameterID
                "Enable line 5", // parameter name
                true
            ),
            std::make_unique<juce::AudioParameterBool> (
                "lineEnable6", // parameterID
                "Enable line 6", // parameter name
                true
            ),
            std::make_unique<juce::AudioParameterBool> (
                "lineEnable7", // parameterID
                "Enable line 7", // parameter name
                true
            ),
            std::make_unique<juce::AudioParameterBool> (
                "lineEnable8", // parameterID
                "Enable line 8", // parameter name
                true
            ),
            std::make_unique<juce::AudioParameterBool> (
                "lineEnable9", // parameterID
                "Enable line 9", // parameter name
                true
            ),
            std::make_unique<juce::AudioParameterBool> (
                "lineEnable10", // parameterID
                "Enable line 10", // parameter name
                true
            ),
            std::make_unique<juce::AudioParameterBool> (
                "lineEnable11", // parameterID
                "Enable line 11", // parameter name
                true
            ),
            std::make_unique<juce::AudioParameterBool> (
                "lineEnable12", // parameterID
                "Enable line 12", // parameter name
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

void LineTogglerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    outputMidiBuffer.clear();

    const int firstLineNote = 36;
    
    // Copy everything to outputMidiBuffer EXCEPT:
    // - note ons for each line (C1 … B2)
    for (auto midiMessageIt = midiMessages.begin(); midiMessageIt != midiMessages.end(); ++midiMessageIt) {
        const juce::MidiMessageMetadata metadata = *midiMessageIt;
        const juce::MidiMessage m = metadata.getMessage();
        
        // Pass all non-note-on events through. We only filter note ons.
        if (! m.isNoteOn() ) {
            outputMidiBuffer.addEvent(m, metadata.samplePosition);
            continue;
        }
        
        const int noteNumber = m.getNoteNumber();
        const int slotIndex = noteNumber - firstLineNote;
        
        // Pass notes below the line range.
        if ( slotIndex < 0 ) {
            outputMidiBuffer.addEvent(m, metadata.samplePosition);
            continue;
        }
        // Pass notes above the line range.
        if ( slotIndex > CBR_TOGGLELINES_NUM_LINES ) {
            outputMidiBuffer.addEvent(m, metadata.samplePosition);
            continue;
        }
            
        // If we get to here, the current event is a note-on for a sampler line.
        // We might mute it if the param is disabled.
        
        if ( allowLinePlayback[slotIndex]->get() ) {
            outputMidiBuffer.addEvent(m, metadata.samplePosition);
        }
    }
    
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

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// Hard-coded 12 lines for now - one octave of sampler slots.
#define CBR_TOGGLELINES_NUM_LINES 4
// Lines start at C1 and go for an octave. @see getSlotIndexForNote().
#define CBR_TOGGLELINES_FIRST_MIDI_NOTE 36

//==============================================================================
/**
*/
class LineTogglerAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    LineTogglerAudioProcessor();
    ~LineTogglerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    int getSlotIndexForNote(const int midiNoteNumber);
    int getSlotIndexForControlNote(const int midiNoteNumber);

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LineTogglerAudioProcessor)

    static const int notesPerLine[CBR_TOGGLELINES_NUM_LINES];
    // MIDI note value for control note for each line - e.g. C-2 up.
    static const int lineControlNotes[CBR_TOGGLELINES_NUM_LINES];

    // State of each line - true = gate open / is playing.
    bool lineGate[CBR_TOGGLELINES_NUM_LINES];

    juce::AudioProcessorValueTreeState parameters;

    // Store a direct pointer to each param for convenience.
    juce::AudioParameterBool *allowLinePlayback[CBR_TOGGLELINES_NUM_LINES] ;

    juce::MidiBuffer outputMidiBuffer;
};

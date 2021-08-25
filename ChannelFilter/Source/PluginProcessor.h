/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class MIDIClipVariationsAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    MIDIClipVariationsAudioProcessor();
    ~MIDIClipVariationsAudioProcessor() override;

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
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MIDIClipVariationsAudioProcessor)
    
    double samplesToBeats (juce::int64 timestamp);
    double beatsToPhrase (double beats);
    bool shouldPlayMidiMessage (juce::MidiMessage message,juce::int64 blockTime, juce::int64 eventTime);

    int getPhraseBeats ();

    juce::AudioParameterInt* selectedChannel;
    juce::AudioParameterChoice* phraseBeats;

    double tempoBpm;
    int currentAllowedChannel;
    juce::int64 lastBufferTimestamp;

    juce::MidiBuffer outputMidiBuffer;
};

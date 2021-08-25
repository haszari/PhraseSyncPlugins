/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class MIDIClipVariationsAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    MIDIClipVariationsAudioProcessorEditor (MIDIClipVariationsAudioProcessor&);
    ~MIDIClipVariationsAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MIDIClipVariationsAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MIDIClipVariationsAudioProcessorEditor)
};

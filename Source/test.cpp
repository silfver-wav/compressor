/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "dsp/include/Compressor.h"
#include "dsp/include/LevelEnvelopeFollower.h"

//==============================================================================
/**
*/
class SmplcompAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    SmplcompAudioProcessor();
    ~SmplcompAudioProcessor();

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const String getProgramName(int index) override;
    void changeProgramName(int index, const String& newName) override;

    //==============================================================================
    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void parameterChanged(const String& parameterID, float newValue) override;
    AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    Atomic<float> gainReduction;
    Atomic<float> currentInput;
    Atomic<float> currentOutput;

private:
    //==============================================================================
    AudioProcessorValueTreeState parameters;
    Compressor compressor;
    LevelEnvelopeFollower inLevelFollower;
    LevelEnvelopeFollower outLevelFollower;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmplcompAudioProcessor)
};
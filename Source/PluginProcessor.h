/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "Compressor.h"
#include "LevelEnvelopeFollower.h"

struct Filters
{
public:
    using Filter = juce::dsp::LinkwitzRileyFilter<float>;
    //          HP   LP
    Filter AP, LP1, LP2,
               HP1, HP2;

    juce::AudioParameterFloat* lowCrossover{ nullptr };     // HighPass
    juce::AudioParameterFloat* highCrossover{ nullptr };    // LowPass
    juce::AudioParameterBool* solo{ nullptr };              // Solo AllPass
    std::array<juce::AudioBuffer<float>, 3> filtersBuffers;

    void prepare(const juce::dsp::ProcessSpec& spec) {
        AP.prepare(spec);
        LP1.prepare(spec);
        HP1.prepare(spec);
        LP2.prepare(spec);
        HP2.prepare(spec);
    }

    void process(juce::dsp::ProcessContextReplacing<float>& fb0Ctx,
        juce::dsp::ProcessContextReplacing<float>& fb1Ctx,
        juce::dsp::ProcessContextReplacing<float>& fb2Ctx) {
        LP1.process(fb0Ctx);
        AP.process(fb0Ctx);

        HP1.process(fb1Ctx);
        filtersBuffers[2] = filtersBuffers[1];
        LP2.process(fb1Ctx);

        HP2.process(fb2Ctx);
    }
};

//==============================================================================
/**
*/
class CompressorAudioProcessor  : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    CompressorAudioProcessor();
    ~CompressorAudioProcessor() override;

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

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    juce::Atomic<float> gainReduction;
    juce::Atomic<float> currentInput;
    juce::Atomic<float> currentOutput;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    Compressor compressor;
    LevelEnvelopeFollower inLevelFollower;
    LevelEnvelopeFollower outLevelFollower;

    // Filters filters;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorAudioProcessor)
};

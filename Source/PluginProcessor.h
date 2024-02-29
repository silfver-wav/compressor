/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct Compressor
{
public:
    juce::AudioParameterFloat* attack{ nullptr };
    juce::AudioParameterFloat* release{ nullptr };
    juce::AudioParameterFloat* threeshold{ nullptr };
    juce::AudioParameterChoice* ratio{ nullptr };

    void prepare(const juce::dsp::ProcessSpec& spec) {
        compressor.prepare(spec);
    }

    void updateCompressorSettings() {
        compressor.setAttack(attack->get());
        compressor.setRelease(release->get());
        compressor.setRatio(ratio->getCurrentChoiceName().getFloatValue());
        compressor.setThreshold(threeshold->get());
    }

    void process(juce::AudioBuffer<float>& buffer, juce::AudioParameterBool* bypass) {
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto context = juce::dsp::ProcessContextReplacing<float>(block);
        context.isBypassed = bypass->get();
        compressor.process(context);
    }
private:
    juce::dsp::Compressor<float> compressor;
};

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
class CompressorAudioProcessor  : public juce::AudioProcessor
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

    using APVTS = juce::AudioProcessorValueTreeState;
    static APVTS::ParameterLayout createParameterLayout();

    APVTS apvts{ *this, nullptr, "Parameters", createParameterLayout() };
private:
    // juce::dsp::Limiter<float> limiter; TODO: implement latter

    Compressor compressor;
    Filters filters;

    juce::dsp::Gain<float> makeUpGain;
    juce::AudioParameterFloat* makeUpGainParam { nullptr };

    template<typename T, typename U>
    void applyGain(T& buffer, U& gain) {
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto ctx = juce::dsp::ProcessContextReplacing<float>(block);
        gain.process(ctx);
    }

    juce::AudioParameterBool* bypass { nullptr };  
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorAudioProcessor)
};

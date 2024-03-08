/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Constants.h"
#include <cstdint>

//==============================================================================
CompressorAudioProcessor::CompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
                        : AudioProcessor(BusesProperties()
                    #if ! JucePlugin_IsMidiEffect
                    #if ! JucePlugin_IsSynth
                            .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    #endif
                            .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                    #endif
                        ), parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
                    #endif
{
    //Add parameter listener
    parameters.addParameterListener("inputgain", this);
    parameters.addParameterListener("makeup", this);
    parameters.addParameterListener("threshold", this);
    parameters.addParameterListener("ratio", this);
    parameters.addParameterListener("knee", this);
    parameters.addParameterListener("attack", this);
    parameters.addParameterListener("release", this);
    parameters.addParameterListener("mix", this);

    gainReduction.set(0.0f);
    currentInput.set(-std::numeric_limits<float>::infinity());
    currentOutput.set(-std::numeric_limits<float>::infinity());
}

CompressorAudioProcessor::~CompressorAudioProcessor()
{
}

//==============================================================================
const juce::String CompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CompressorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CompressorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CompressorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double CompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CompressorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int CompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CompressorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String CompressorAudioProcessor::getProgramName (int index)
{
    return {};
}

void CompressorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void CompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    // Prepare dsp classes
    compressor.prepare(spec);
    inLevelFollower.prepare(sampleRate);
    outLevelFollower.prepare(sampleRate);

    // Set envelope follower for level meter to measure over 300ms time frame
    inLevelFollower.setPeakDecay(0.3f);
    outLevelFollower.setPeakDecay(0.3f);
}

void CompressorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void CompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    //Update input peak metering
    inLevelFollower.updatePeak(buffer.getArrayOfReadPointers(), totalNumInputChannels, numSamples);
    currentInput.set(juce::Decibels::gainToDecibels(inLevelFollower.getPeak()));

    // Do compressor processing
    compressor.process(buffer);

    // Update gain reduction metering
    gainReduction.set(compressor.getMaxGainReduction());

    // Update output peak metering
    outLevelFollower.updatePeak(buffer.getArrayOfReadPointers(), totalNumInputChannels, numSamples);
    currentOutput = juce::Decibels::gainToDecibels(outLevelFollower.getPeak());
}

//==============================================================================
bool CompressorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* CompressorAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
    // return new CompressorAudioProcessorEditor (*this);
}

//==============================================================================
void CompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void CompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

void CompressorAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "inputgain") compressor.setInput(newValue);
    else if (parameterID == "threshold") compressor.setThreshold(newValue);
    else if (parameterID == "ratio") compressor.setRatio(newValue);
    else if (parameterID == "knee") compressor.setKnee(newValue);
    else if (parameterID == "attack") compressor.setAttack(newValue);
    else if (parameterID == "release") compressor.setRelease(newValue);
    else if (parameterID == "makeup") compressor.setMakeup(newValue);
    else if (parameterID == "mix") compressor.setMix(newValue);
}

juce::AudioProcessorValueTreeState::ParameterLayout CompressorAudioProcessor::createParameterLayout() {
    using namespace juce;
    
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back(std::make_unique<AudioParameterBool>("power", "Power", true));

    params.push_back(std::make_unique<AudioParameterFloat>("inputgain", "Input",
                                                        NormalisableRange<float>(
                                                            Constants::Parameter::inputStart,
                                                            Constants::Parameter::inputEnd,
                                                            Constants::Parameter::inputInterval), 0.0f,
                                                        String(),
                                                        AudioProcessorParameter::genericParameter,
                                                        [](float value, float)
                                                        {
                                                            return String(value, 1) + " dB";
                                                        }));


    params.push_back(std::make_unique<AudioParameterFloat>("threshold", "Tresh",
                                                        NormalisableRange<float>(
                                                            Constants::Parameter::thresholdStart,
                                                            Constants::Parameter::thresholdEnd,
                                                            Constants::Parameter::thresholdInterval), -10.0f,
                                                        String(), AudioProcessorParameter::genericParameter,
                                                        [](float value, float maxStrLen)
                                                        {
                                                            return String(value, 1) + " dB";
                                                        }));

    params.push_back(std::make_unique<AudioParameterFloat>("ratio", "Ratio",
                                                        NormalisableRange<float>(
                                                            Constants::Parameter::ratioStart,
                                                            Constants::Parameter::ratioEnd,
                                                            Constants::Parameter::ratioInterval, 0.5f), 2.0f,
                                                        String(), AudioProcessorParameter::genericParameter,
                                                        [](float value, float)
                                                        {
                                                            if (value > 23.9f) return String("Infinity") + ":1";
                                                            return String(value, 1) + ":1";
                                                        }));

    params.push_back(std::make_unique<AudioParameterFloat>("knee", "Knee",
                                                           NormalisableRange<float>(
                                                                   Constants::Parameter::kneeStart,
                                                                   Constants::Parameter::kneeEnd,
                                                                   Constants::Parameter::kneeInterval),
                                                           6.0f, String(), AudioProcessorParameter::genericParameter,
                                                           [](float value, float)
                                                           {
                                                               return String(value, 1) + " dB";
                                                           }));

    params.push_back(std::make_unique<AudioParameterFloat>("attack", "Attack",
                                                        NormalisableRange<float>(
                                                            Constants::Parameter::attackStart,
                                                            Constants::Parameter::attackEnd,
                                                            Constants::Parameter::attackInterval, 0.5f), 2.0f,
                                                        "ms",
                                                        AudioProcessorParameter::genericParameter,
                                                        [](float value, float)
                                                        {
                                                            if (value == 100.0f) return String(value, 0) + " ms";
                                                            return String(value, 2) + " ms";
                                                        }));

    params.push_back(std::make_unique<AudioParameterFloat>("release", "Release",
                                                        NormalisableRange<float>(
                                                            Constants::Parameter::releaseStart,
                                                            Constants::Parameter::releaseEnd,
                                                            Constants::Parameter::releaseInterval, 0.35f),
                                                        140.0f,
                                                        String(),
                                                        AudioProcessorParameter::genericParameter,
                                                        [](float value, float)
                                                        {
                                                            if (value <= 100) return String(value, 2) + " ms";
                                                            if (value >= 1000)
                                                                return String(value * 0.001f, 2) + " s";
                                                            return String(value, 1) + " ms";
                                                        }));

    params.push_back(std::make_unique<AudioParameterFloat>("makeup", "Makeup",
                                                           NormalisableRange<float>(
                                                                   Constants::Parameter::makeupStart,
                                                                   Constants::Parameter::makeupEnd,
                                                                   Constants::Parameter::makeupInterval), 0.0f,
                                                           String(),
                                                           AudioProcessorParameter::genericParameter,
                                                           [](float value, float)
                                                           {
                                                               return String(value, 1) + " dB ";
                                                           }));

    params.push_back(std::make_unique<AudioParameterFloat>("mix", "Mix",
                                                        NormalisableRange<float>(
                                                            Constants::Parameter::mixStart,
                                                            Constants::Parameter::mixEnd,
                                                            Constants::Parameter::mixInterval),
                                                        1.0f, "%", AudioProcessorParameter::genericParameter,
                                                        [](float value, float)
                                                        {
                                                            return String(value * 100.0f, 1) + " %";
                                                        }));

    return {params.begin(), params.end()};
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorAudioProcessor();
}

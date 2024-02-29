/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CompressorAudioProcessor::CompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Compressor
    //==============================================================================
    compressor.attack = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("Attack"));
    jassert(compressor.attack != nullptr);

    compressor.release = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("Release"));
    jassert(compressor.release != nullptr);

    compressor.threeshold = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("Threeshold"));
    jassert(compressor.threeshold != nullptr);

    compressor.ratio = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("Ratio"));
    jassert(compressor.ratio != nullptr);
    //==============================================================================
    
    // LP & HP Filter
    //==============================================================================
    filters.lowCrossover = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("LowPass"));
    jassert(filters.lowCrossover != nullptr);
    filters.highCrossover = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("HighPass"));
    jassert(filters.highCrossover != nullptr);

    // Compressor Pass
    filters.AP.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
    // HP
    filters.LP1.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    filters.HP1.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    // LP
    filters.LP2.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    filters.HP2.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    filters.solo = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("Solo"));
    jassert(filters.solo != nullptr);
    //==============================================================================

    makeUpGainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("MakeUpGain"));
    jassert(makeUpGainParam != nullptr);
    bypass = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("Bypass"));
    jassert(bypass != nullptr);
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
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;

    compressor.prepare(spec);
    filters.prepare(spec);

    makeUpGain.prepare(spec);
    makeUpGain.setRampDurationSeconds(0.05); // 50 ms

    for (auto& buffer : filters.filtersBuffers) {
        buffer.setSize(spec.numChannels, samplesPerBlock);
    }
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
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    compressor.updateCompressorSettings();
    makeUpGain.setGainDecibels(makeUpGainParam->get());

    for (auto& fb : filters.filtersBuffers) {
        fb = buffer;
    }

    auto hpCutoff = filters.lowCrossover->get();
    filters.LP1.setCutoffFrequency(hpCutoff);
    filters.HP1.setCutoffFrequency(hpCutoff);

    auto lpCutoff = filters.highCrossover->get();
    filters.AP.setCutoffFrequency(lpCutoff);
    filters.LP2.setCutoffFrequency(lpCutoff);
    filters.HP2.setCutoffFrequency(lpCutoff);

    auto fb0Block = juce::dsp::AudioBlock<float>(filters.filtersBuffers[0]);
    auto fb1Block = juce::dsp::AudioBlock<float>(filters.filtersBuffers[1]);
    auto fb2Block = juce::dsp::AudioBlock<float>(filters.filtersBuffers[2]);

    auto fb0Ctx = juce::dsp::ProcessContextReplacing<float>(fb0Block);
    auto fb1Ctx = juce::dsp::ProcessContextReplacing<float>(fb1Block);
    auto fb2Ctx = juce::dsp::ProcessContextReplacing<float>(fb2Block);

    filters.process(fb0Ctx, fb1Ctx, fb2Ctx);
    compressor.process(filters.filtersBuffers[1], bypass);

    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    if (bypass->get()) {
        return;
    }

    buffer.clear();

    auto addFilterBand = [nc = numChannels, ns = numSamples](auto& inputBuffer, const auto& source) {
        for (auto i = 0; i < nc; ++i) {
            inputBuffer.addFrom(i, 0, source, i, 0, ns);
        }
    };

    addFilterBand(buffer, filters.filtersBuffers[1]);
    if (!filters.solo->get()) {
        addFilterBand(buffer, filters.filtersBuffers[0]);
        addFilterBand(buffer, filters.filtersBuffers[2]);
    }

    applyGain(buffer, makeUpGain);
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

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void CompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    
    // Check tree is valid
    if (tree.isValid()) {
        apvts.replaceState(tree);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout CompressorAudioProcessor::createParameterLayout() {
    APVTS::ParameterLayout layout;

    using namespace juce;

    auto threesholdRange = NormalisableRange<float>(-60, 12, 1, 1);
    auto attackReleaseRange = NormalisableRange<float>(5, 500, 1, 1);
    std::vector<double> ratioRange = { 1, 1.5, 2, 3, 4 , 5, 6, 7, 8, 9, 10, 15, 20, 50, 100 };

    juce::StringArray ratioStrings;
    for (auto ratio : ratioRange) {
        ratioStrings.add( juce::String(ratio, 1) );
    }

    auto gainRange = juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f);

    layout.add(std::make_unique<AudioParameterFloat>("MakeUpGain",
                                                    "MakeUpGain",
                                                    gainRange,
                                                    0));

    layout.add(std::make_unique<AudioParameterFloat>("Threeshold", 
                                                    "Threeshold", 
                                                    threesholdRange,
                                                    0));

    layout.add(std::make_unique<AudioParameterFloat>("Attack",
                                                    "Attack",
                                                    attackReleaseRange,
                                                    50));

    layout.add(std::make_unique<AudioParameterFloat>("Release",
                                                    "Release",
                                                    attackReleaseRange,
                                                    250));

    layout.add(std::make_unique<AudioParameterChoice>("Ratio",
                                                    "Ratio",
                                                    ratioStrings,
                                                    3));

    layout.add(std::make_unique<AudioParameterBool>("Bypass", "Bypass", true));


    layout.add(std::make_unique<AudioParameterFloat>("HighPass",
                                                    "HighPass",
                                                    NormalisableRange<float>(1000, 20000, 1, 1),
                                                    20000));
    
    layout.add(std::make_unique<AudioParameterFloat>("LowPass",
                                                    "LowPass",
                                                    NormalisableRange<float>(20, 999, 1, 1),
                                                    20));

    layout.add(std::make_unique<AudioParameterBool>("Solo",
                                                    "Solo",
                                                    false));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorAudioProcessor();
}

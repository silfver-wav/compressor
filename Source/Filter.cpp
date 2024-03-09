/*
  ==============================================================================

    Filter.cpp
    Created: 8 Mar 2024 10:46:11pm
    Author:  Linus

  ==============================================================================
*/

#include "Filter.h"

Filter::Filter()
{
    // Compressor Pass
    AP.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
    // HP
    LP1.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    HP1.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    // LP
    LP2.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    HP2.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
}

void Filter::prepare(const juce::dsp::ProcessSpec& spec)
{
    AP.prepare(spec);
    LP1.prepare(spec);
    HP1.prepare(spec);
    LP2.prepare(spec);
    HP2.prepare(spec);

    for (auto& buffer : filtersBuffers) {
        buffer.setSize(spec.numChannels, spec.maximumBlockSize);
    }
}

juce::AudioBuffer<float>& Filter::bufferSplit(juce::AudioBuffer<float>& buffer)
{
    numSamples = buffer.getNumSamples();
    numChannels = buffer.getNumChannels();

    // copy the input buffer into the filtersBuffer array
    for (int channel = 0; channel < numChannels; ++channel) {
        filtersBuffers[0].copyFrom(channel, 0, buffer, channel, 0, numSamples);
        filtersBuffers[1].copyFrom(channel, 0, buffer, channel, 0, numSamples);
        filtersBuffers[2].copyFrom(channel, 0, buffer, channel, 0, numSamples);
    }

    // update filters cutoffs 
    auto hpCutoff = lowCrossover;
    LP1.setCutoffFrequency(hpCutoff);
    HP1.setCutoffFrequency(hpCutoff);
    auto lpCutoff = highCrossover;
    AP.setCutoffFrequency(lpCutoff);
    LP2.setCutoffFrequency(lpCutoff);
    HP2.setCutoffFrequency(lpCutoff);

    return filtersBuffers[1];
}

void Filter::process(juce::AudioBuffer<float>& buffer)
{
    // Process each filter buffer
    LP1.process(juce::dsp::ProcessContextReplacing<float>(juce::dsp::AudioBlock<float>(filtersBuffers[0])));
    AP.process(juce::dsp::ProcessContextReplacing<float>(juce::dsp::AudioBlock<float>(filtersBuffers[0])));
    HP1.process(juce::dsp::ProcessContextReplacing<float>(juce::dsp::AudioBlock<float>(filtersBuffers[1])));
    LP2.process(juce::dsp::ProcessContextReplacing<float>(juce::dsp::AudioBlock<float>(filtersBuffers[1])));
    HP2.process(juce::dsp::ProcessContextReplacing<float>(juce::dsp::AudioBlock<float>(filtersBuffers[2])));

    // Clear the output buffer
    buffer.clear();

    // Sum the outputs of all filters
    for (int channel = 0; channel < numChannels; ++channel) {
        for (int sample = 0; sample < numSamples; ++sample) {
            if (solo) {
                buffer.addSample(channel, sample, filtersBuffers[1].getSample(channel, sample));
            }
            else
            {
                buffer.addSample(channel, sample, filtersBuffers[0].getSample(channel, sample));
                buffer.addSample(channel, sample, filtersBuffers[1].getSample(channel, sample));
                buffer.addSample(channel, sample, filtersBuffers[2].getSample(channel, sample));
            }
        }
    }
}

void Filter::setLowCrossover(float lowCrossover)
{
    this->lowCrossover = lowCrossover;
}

void Filter::setHighCrossover(float highCrossover)
{
    this->highCrossover = highCrossover;
}

void Filter::setSolo()
{
    solo = !solo;
}
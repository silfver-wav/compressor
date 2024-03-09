/*
  ==============================================================================

    Filter.h
    Created: 8 Mar 2024 10:45:58pm
    Author:  Linus

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"


class Filter
{
public:
    Filter();

    void prepare(const juce::dsp::ProcessSpec& spec);

    juce::AudioBuffer<float>& bufferSplit(juce::AudioBuffer<float>& buffer);
    void process(juce::AudioBuffer<float>& buffer);

    void setLowCrossover(float);
    void setHighCrossover(float);
    void setSolo();

    
private:
    //                                        HP   LP
    juce::dsp::LinkwitzRileyFilter<float> AP, LP1, LP2,
                                              HP1, HP2;
    float lowCrossover;
    float highCrossover;
    bool solo { false };

    int numSamples;
    int numChannels;

    std::array<juce::AudioBuffer<float>, 3> filtersBuffers;
};
/*
  ==============================================================================

    GainComputer.cpp
    Created: 7 Mar 2024 10:07:19pm
    Author:  Linus

  ==============================================================================
*/

#include "GainComputer.h"
#include <algorithm>
#include <limits>
#include <cmath>
#include "../JuceLibraryCode/JuceHeader.h"

GainComputer::GainComputer()
{
    threshold = -20.0f;
    ratio = 2.0f;
    slope = 1.0f / ratio - 1.0f;
    knee = 6.0f;
    kneeWidth = 3.0f;
}



void GainComputer::setThreshold(float threshold)
{
    if (this->threshold != threshold) 
    {
        this->threshold = threshold;
    }
}

void GainComputer::setRatio(float ratio)
{
    if (this->ratio == ratio)
        return;

    this->ratio = ratio;
    if (ratio > 23.9f)
        this->ratio = -std::numeric_limits<float>::infinity(); // You have become a limiter
    slope = 1.0f / ratio - 1.0f;
}

void GainComputer::setKnee(float knee)
{
    if (this->knee != knee)
    {
        this->knee = knee;
        kneeWidth = knee / 2.0f;
    }
    
}

float GainComputer::applyCompression(float input)
{
    const float overshoot = input - threshold;

    // No compression
    if (overshoot < -kneeWidth)
        return input;
    
    // Compression within the knee range
    if (overshoot <= kneeWidth/2)
        return input + (slope * juce::square(overshoot + kneeWidth)) / knee;
    
    // Full compression (overshoot has exceeded the knee range)
    return threshold + overshoot/ratio;
}

void GainComputer::applyCompressionToBuffer(float* buffer, int numSamples)
{
    for (int i = 0; i < numSamples; i++)
    {
        const float level = std::max(abs(buffer[i]), 1e-6f);
        float levelInDecibels = juce::Decibels::gainToDecibels(level);
        buffer[i] = applyCompression(levelInDecibels);
    }
}
/*
  ==============================================================================

    Compressor.cpp
    Created: 8 Mar 2024 9:18:26am
    Author:  Linus

  ==============================================================================
*/

#include "Compressor.h"

Compressor::~Compressor()
{
    rawSidechainSignal = nullptr;
}

void Compressor::prepare(const juce::dsp::ProcessSpec& spec)
{
    ballistics.prepare(spec.sampleRate);
    originalSignal.setSize(2, spec.maximumBlockSize);
    sidechainSignal.resize(spec.maximumBlockSize, 0.0f);
    rawSidechainSignal = sidechainSignal.data();
    originalSignal.clear();
}

// Gain Computer setters
void Compressor::setThreshold(float threshold)
{
    gainComputer.setThreshold(threshold);
}

void Compressor::setRatio(float ratio)
{
    gainComputer.setRatio(ratio);
}

void Compressor::setKnee(float knee)
{
    gainComputer.setKnee(knee);
}

// Ballistics setters
void Compressor::setAttack(float attack)
{
    ballistics.setAttack(attack * 0.001);
}

void Compressor::setRelease(float release)
{
    ballistics.setRelease(release * 0.001);
}

// General setters
void Compressor::setPower()
{
    bypassed = !bypassed;
}

void Compressor::setInput(float input)
{
    this->input = input;
}

void Compressor::setMakeup(float makeup)
{
    this->makeup = makeup;
}

void Compressor::setMix(float mix)
{
    this->mix = mix;
}

// Getters
float Compressor::getMakeup()
{
    return makeup;
}

double Compressor::getSampleRate()
{
    return procSpec.sampleRate;
}

float Compressor::getMaxGainReduction()
{
    return maxGainReduction;
}

void Compressor::process(juce::AudioBuffer<float>& buffer)
{
    if (bypassed)
        return;

    using namespace juce;

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    jassert(numSamples == static_cast<int>(sidechainSignal.size()));

    // Clear any old samples
    originalSignal.clear();
    juce::FloatVectorOperations::fill(rawSidechainSignal, 0.0f, numSamples);
    maxGainReduction = 0.0f;

    // Apply input gain
    applyInputGain(buffer, numSamples);

    // Get max l/r amplitude values and fill sidechain signal
    FloatVectorOperations::abs(rawSidechainSignal, buffer.getReadPointer(0), numSamples);
    FloatVectorOperations::max(rawSidechainSignal, rawSidechainSignal, buffer.getReadPointer(1), numSamples);

    // Compute attenuation - converts side-chain signal from linear to logarithmic domain
    gainComputer.applyCompressionToBuffer(rawSidechainSignal, numSamples);

    // Smooth attenuation - still logarithmic
    ballistics.applyBallistics(rawSidechainSignal, numSamples);

    // Get minimum = max. gain reduction from side chain buffer
    maxGainReduction = FloatVectorOperations::findMinimum(rawSidechainSignal, numSamples);

    // Add makeup gain and convert side-chain to linear domain
    for (int i = 0; i < numSamples; ++i)
        sidechainSignal[i] = Decibels::decibelsToGain(sidechainSignal[i] + makeup);

    // Copy buffer to original signal
    for (int i = 0; i < numChannels; ++i)
        originalSignal.copyFrom(i, 0, buffer, i, 0, numSamples);

    // Multiply attenuation with buffer - apply compression
    for (int i = 0; i < numChannels; ++i)
        FloatVectorOperations::multiply(buffer.getWritePointer(i), rawSidechainSignal, numSamples);

    // Mix dry & wet signal
    for (int i = 0; i < numChannels; ++i)
    {
        float* channelData = buffer.getWritePointer(i); //wet signal
        FloatVectorOperations::multiply(channelData, mix, numSamples);
        FloatVectorOperations::addWithMultiply(channelData, originalSignal.getReadPointer(i), 1 - mix, numSamples);
    }
}

inline void Compressor::applyInputGain(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (prevInput == input)
        buffer.applyGain(0, numSamples, juce::Decibels::decibelsToGain(prevInput));
    else
    {
        buffer.applyGainRamp(0, numSamples, juce::Decibels::decibelsToGain(prevInput), juce::Decibels::decibelsToGain(input));
        prevInput = input;
    }
}
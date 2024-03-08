/*
  ==============================================================================

    Compressor.h
    Created: 7 Mar 2024 9:24:32pm
    Author:  Linus

  ==============================================================================
*/

#pragma once
#include "LevelDetector.h"
#include "GainComputer.h"
#include "../JuceLibraryCode/JuceHeader.h"

class Compressor
{
public:

    Compressor() = default;
    ~Compressor();

    void prepare(const juce::dsp::ProcessSpec& spec);

    // Gain Computer setters
    void setThreshold(float);

    void setRatio(float);

    void setKnee(float);

    // Ballistics setters
    void setAttack(float);

    void setRelease(float);

    // General setters
    void setPower();

    void setInput(float);

    void setMakeup(float);

    void setMix(float);

    // Getters
    float getMakeup();

    double getSampleRate();

    float getMaxGainReduction();

    void process(juce::AudioBuffer<float>& buffer);
private:
    inline void applyInputGain(juce::AudioBuffer<float>&, int);

    //Directly initialize process spec to avoid debugging problems
    juce::dsp::ProcessSpec procSpec{-1, 0, 0};

    juce::AudioBuffer<float> originalSignal;
    std::vector<float> sidechainSignal;
    float* rawSidechainSignal{ nullptr };

    LevelDetector ballistics;
    GainComputer gainComputer;

    float input{ 0.0f };
    float prevInput{ 0.0f };
    float makeup{ 0.0f };
    bool bypassed{ false };
    float mix{ 1.0f };
    float maxGainReduction{ 0.0f };
};
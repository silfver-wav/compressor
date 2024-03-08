/*
  ==============================================================================

    LevelDetector.cpp
    Created: 7 Mar 2024 10:07:42pm
    Author:  Linus

  ==============================================================================
*/

#include "LevelDetector.h"
#include "../JuceLibraryCode/JuceHeader.h"

// fs = sampling frequency
void LevelDetector::prepare(const double& fs)
{
    sampleRate = fs;
    // Calculate attack coefficient
    attackCoefficient = calculateCoefficient(attackTimeInSeconds);
    // Calculate release coefficient
    releaseCoefficient = calculateCoefficient(releaseTimeInSeconds);
    state01 = 0.0;
    state02 = 0.0;
}

void LevelDetector::setAttack(const double& attack)
{
    if (attack != attackTimeInSeconds)
    {
        attackTimeInSeconds = attack;
        attackCoefficient = calculateCoefficient(attackTimeInSeconds);
    }
}
void LevelDetector::setRelease(const double& release)
{
    if (release != releaseTimeInSeconds)
    {
        releaseTimeInSeconds = release;
        releaseCoefficient = calculateCoefficient(releaseTimeInSeconds);
    }
}
double LevelDetector::getAttack()
{
    return attackTimeInSeconds;
}
double LevelDetector::getRelease()
{
    return releaseTimeInSeconds;
}
double LevelDetector::getAttackCoefficient()
{
    return attackCoefficient;
}
double LevelDetector::getReleaseCoefficient()
{
    return releaseCoefficient;
}

float LevelDetector::processPeakBranched(const float& input)
{
    //Smooth branched peak detector
    if (input < state01)
        state01 = attackCoefficient * state01 + (1 - attackCoefficient) * input; //  (smoothing factor/coefficient) * (previous output sample) + (coefficient - 1)*(input sample)
    else
        state01 = releaseCoefficient * state01 + (1 - releaseCoefficient) * input; //  (smoothing factor/coefficient) * (previous output sample) + (coefficient - 1)*(input sample)
    return static_cast<float>(state01);
}   

float LevelDetector::precessPeakDecoupled(const float& input)
{
    const double inputDouble = static_cast<double>(input);
    state02 = juce::jmax(inputDouble, releaseCoefficient * state02 + (1 - releaseCoefficient) * inputDouble);
    state01 = attackCoefficient * state01 + (1 - attackCoefficient) * state02;
    return static_cast<float>(state01);
}


void LevelDetector::applyBallistics(float* buffer, int numSamples)
{
    for (int i = 0; i < numSamples; i++)
    {
        buffer[i] = processPeakBranched(buffer[i]);
    }
}

double LevelDetector::calculateCoefficient(const double& time)
{
    return exp(-1 / (time * sampleRate));
}
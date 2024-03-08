/*
  ==============================================================================

    LevelDetector.h
    Created: 7 Mar 2024 10:05:25pm
    Author:  Linus

  ==============================================================================
*/

#pragma once

class LevelDetector
{
public:
	LevelDetector() = default;

    void prepare(const double& fs);

    void setAttack(const double&);
    void setRelease(const double&);
    double getAttack();
    double getRelease();
    double getAttackCoefficient();
    double getReleaseCoefficient();

    float processPeakBranched(const float&);
    float precessPeakDecoupled(const float&);
    void applyBallistics(float*, int);

private:
    double calculateCoefficient(const double&);
    double attackTimeInSeconds{ 0.01 }, attackCoefficient{ 0.0 };
    double releaseTimeInSeconds{ 0.14 }, releaseCoefficient{ 0.0 };
    double state01{ 0.0 }, state02{ 0.0 };
    double sampleRate{ 0.0 };
    bool autoAttack{ false };
    bool autoRelease{ false };

};
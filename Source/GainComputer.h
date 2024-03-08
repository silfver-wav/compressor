/*
  ==============================================================================

    GainComputer.h
    Created: 7 Mar 2024 10:05:13pm
    Author:  Linus

  ==============================================================================
*/

#pragma once

class GainComputer
{
public:
    GainComputer();

    void setThreshold(float);
    void setRatio(float);
    void setKnee(float);

    float applyCompression(float);

    void applyCompressionToBuffer(float*, int);

private:
    float threshold;
    float ratio;
    float knee, kneeWidth;
    float slope;
};

/*
  ==============================================================================

    Oscillators.cpp
    Author:  eric carmi

    Oscillator types:

    Traditional
        Sine, Triangle, Square, Saw

    Nonlinear / Dynamical Systems
        Chua, Vanderpol, Henon, Circle Map, Lorenz, Duffing, Pendulum, double Pendulum,


    This class is based on differential equations to simplify calculations
    The finite different method for sine waves suffers from instability at high frequencies
          as well as error between the desired and actual frequency (starts around 3 kHz)
    But for an LFO, this is fine

  ==============================================================================
*/

#include "Oscillators.h"

Oscillators::Oscillators()
    : delay1(0.1),delay2(0.1),delay3(0.1),
      currentOscillatorType(1),
      oscParam1(5.0),oscParam2(0.1),oscParam3(0.2),
       oscFreq(440), sampleRate(48000.0), sampleTime(1.0/sampleRate)

{

}
Oscillators::~Oscillators()
{

}

void Oscillators::getNextSample(float *outSamples, float input)
{
    switch(currentOscillatorType)
    {
        // Sine
        case 1: {
            float t1 = delay1 + (oscFreq*oscFreq)*sampleTime * delay2;
            float t2 = delay2 - sampleTime* t1 + input;
            outSamples[0] = t1;
            outSamples[1] = t2;
            delay1 = outSamples[0];
            delay2 = outSamples[1];
            break;
        }

        // Triangle
        case 2: {
            oscCounter+=1;
            float t3 = fmodf(oscCounter, oscParam2);
            outSamples[0] = t3 > (oscParam2/2.0) ? 4*(t3/oscParam2)-3.0 + input : -4*(t3/oscParam2)+1.0 + input;
            break;
        }

        // Square
        case 3: {
            oscCounter+=1;
            float t4 = fmodf(oscCounter, oscParam2);
            outSamples[0] = t4 > (oscParam2/2.0) ? 1 + input : -1 + input;
           // outSamples[0] = oscParam2;
            break;
        }

        // Saw
        case 4: {
            oscCounter+=1;
            float t5 = fmodf(oscCounter, oscParam2);
            outSamples[0] = 2*(t5/oscParam2-0.5) + input;
            break;
        }

        // AA Square
        case 5: {
            oscCounter+=1;
            int t4 = fmod(oscCounter, oscParam2/2.0);
            float t5 = fmodf(oscCounter, oscParam2);
            int sign = t5 > (oscParam2/2.0) ? 1 : -1;
            delta = t4 && 1;
            outSamples[0] = sign*delta;
            break;
        }

        // AA Saw
        case 6: {
            oscCounter+=1;
            int t4 = fmod(oscCounter, oscParam2);
            float t5 = fmodf(oscCounter, oscParam2);
            delta = t4 && 1;
            if(not delta){ outSamples[0] = 0 + input; delta2 = true; } // Half reset on first trigger
            else if(delta2){ outSamples[0] = -1 + input; delta2 = false; }// Full reset on second
            else{ outSamples[0] = 2*(t5/oscParam2-0.5) + input;}
            break;
        }

        // Vanderpol / relaxation
        case 7: {

            float v1 = delay1 + oscFreq*oscFreq*sampleTime*delay2;
            float v2 = delay2 + + sampleTime*((1.0 - delay2*delay2) - v1) + input;
            outSamples[0] = v1;
            outSamples[1] = v2;
            delay1 = outSamples[0];
            delay2 = outSamples[1];
            break;
        }

        //Duffing
        case 8: {
            outSamples[0] = delay1 + sampleTime * delay2 + input;
            outSamples[1] = delay2 + sampleTime * (duffing3*delay2 - duffing1*delay1 - duffing2*delay1*delay1*delay1);
            delay1 = outSamples[0];
            delay2 = outSamples[1];
            break;
        }

        //Chua
        case 9: {
            outSamples[0] = delay1 + sampleTime * chuaParam1 * ( delay2 - delay1 - chuaNL(delay1)) + input;
            outSamples[1] = delay2 + sampleTime * (delay1 - delay2 + delay3);
            outSamples[2] = delay3 - sampleTime * chuaParam2 * delay2;
            delay1 = outSamples[0];
            delay2 = outSamples[1];
            delay3 = outSamples[2];
        }

        //Lorenz
        case 10: {
            outSamples[0] = delay1 + sampleTime * lorenz1 * ( delay2 - delay1 ) + input;
            outSamples[1] = delay2 + sampleTime * (delay1*(lorenz3 - delay3) - delay2);
            outSamples[2] = delay3 + sampleTime *(delay1 * delay2 - lorenz2 * delay3);
            delay1 = outSamples[0];
            delay2 = outSamples[1];
            delay3 = outSamples[2];
            break;
        }
    }
}

// Initialize can be called to reset initial conditions
void Oscillators::initializeChaos()
{
    delay1 = 0.9;
    delay2 = -0.4;
    delay3 = -0.6;
}

void Oscillators::reset()
{
    delay1 = 0.1;
    delay2 = 0.1;
    delay3 = 0.1;
    oscCounter = 0.0;

}

void Oscillators::resetICs(double* ICs)
{
    delay1 = ICs[0];
    delay2 = ICs[1];
    delay3 = ICs[2];
}

void Oscillators::setSampleRate(float SR)
{
    sampleRate = SR;
    sampleTime = 1.0/sampleRate;
}

float Oscillators::getSampleRate()
{
    return sampleRate;
}

void Oscillators::setFrequency(float freq)
{
    oscFreq = TAU*freq;
    oscParam1 = oscFreq/sampleRate;
    oscParam2 = 1.0/oscParam1*TAU;
    duffing1 = -1.0 - 10.0*oscFreq;

}

float Oscillators::getFrequency() // In Hz
{
    return oscFreq/TAU;
}

void Oscillators::setType(int T)
{
    currentOscillatorType = T;
    if(T == Oscillators::Lorenz || T == Oscillators::Chua)
        initializeChaos();
    reset();

}

int Oscillators::getType()
{
    return currentOscillatorType;
}

float Oscillators::chuaNL(float x)
{
    return -1.0*chua2*x - 0.5*(chua1-chua2)*(fabs(x+1) - fabs(x-1));
}

/*
  ==============================================================================

    Oscillators.cpp
    Author:  eric carmi

  ==============================================================================
*/

#include <math.h>

class Oscillators
{
public:
    Oscillators();
    ~Oscillators();

    enum OscTypes{
        Sine = 1,
        Triangle = 2,
        Square = 3,
        Saw = 4,
        AASquare = 5,
        AASaw = 6,
        Duffing = 7,
        Vanderpol = 8,
        Chua = 9,
        Lorenz = 10,
    };

    void getNextSample(float *outSamples, float input);
    // Initialize can be called to reset initial conditions
    void initializeChaos();

    void reset();
    void resetICs(double* ICs);

    void setSampleRate(float SR);
    float getSampleRate();

    void setFrequency(float freq);
    float getFrequency();

    void setType(int T);
    int getType();

    float chuaNL(float x);

private:

    float delay1, delay2, delay3;
    double oscCounter = 0.0;
    int currentOscillatorType;
    const double PI = 3.141592653589793;
    const double TAU = 2.0*PI;
    float oscParam1, oscParam2, oscParam3;
    const int chuaParam1 = 4;
    const double chuaParam2 = 100.0/7.0;
    float oscFreq;
    float sampleRate;
    float sampleTime;

    float chua1 = 8.0/7.0f;
    float chua2 = 5.0/7.0f;

    // Duffing params
    double duffing1 = -1; // alpha < -1, use this as control
    const int duffing2 = 5;
    const double duffing3 = 0.02;

    // Lorenz: sigma, beta, rho
    const int lorenz1 = 10;
    const double lorenz2 = 8.0/3.0;
    const int lorenz3 = 28;

    int delta, delta2;

};

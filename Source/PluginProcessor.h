/*
  ==============================================================================

    Phazer Processor

  ==============================================================================
*/

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "Oscillators.h"

class PhazerAudioProcessor  : public AudioProcessor,
                              public IIRFilter,
                              public IIRCoefficients
{
public:
    //==============================================================================
    PhazerAudioProcessor();
    ~PhazerAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    //==============================================================================
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        //jassert (! isUsingDoublePrecision());
        process (buffer, midiMessages);
    }

    #ifndef JucePlugin_PreferredChannelConfigurations
        bool setPreferredBusArrangement (bool isInput, int bus, const AudioChannelSet& preferredSet) ;
    #endif

    //==============================================================================
    bool hasEditor() const override                                             { return true; }
    AudioProcessorEditor* createEditor() override;

    //==============================================================================
    const String getName() const override                                       { return JucePlugin_Name; }

    bool acceptsMidi() const override;
    bool producesMidi() const override;

    double getTailLengthSeconds() const override                                { return 0.0; }

    //==============================================================================
    int getNumPrograms() override                                               { return 1; }
    int getCurrentProgram() override                                            { return 0; }
    void setCurrentProgram (int /*index*/) override                             {}
    const String getProgramName (int /*index*/) override                        { return String(); }
    void changeProgramName (int /*index*/, const String& /*name*/) override     {}

    //==============================================================================
    void getStateInformation (MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    int lastUIWidth, lastUIHeight;

    AudioParameterFloat* rateParam;
    AudioParameterFloat* depthParam;
    AudioParameterFloat* widthParam;
    AudioParameterFloat* centerfreqParam;
    AudioParameterFloat* mixParam;
    AudioParameterFloat* IC1Param;
    AudioParameterFloat* IC2Param;
    AudioParameterFloat* IC3Param;
    AudioParameterBool* powerParam;
    AudioParameterInt* stageParam;

    Oscillators oscillator;
    bool crossFadeFlag = false;

    IIRFilter filterAP1L, filterAP1R;
    IIRFilter APtype2L, APtype2R;
    IIRCoefficients coeff;

    double ICs[3] = {0.0f, 0.0f, 0.0f};
    bool resetICflag = false;

private:
    //==============================================================================
    void process (AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    void mainEffectLoop (AudioBuffer<float>& buffer);
    // Crossfade buffer is used to smooth transitions between switching the effect on and off
    // When power is switched, the effect will slowly come in
    // This requires copying every buffer into the crossfade buffer until it is needed
    AudioBuffer<float> crossFadeBuffer;

    int LFOcounter;
    double LFOangle, LFOdelta;

    bool effectOn = true;
    bool apType = false;
    double sampRate, sampTime, Omega;
    int nums, chans;
    float rate, depth, width, mix, centerfreq, BW;
    float tau, bAP0, bAP1, aAP0, aAP1;
    int numStages;
    float osc[3];
    double a0, a1, a2, b0, b1, b2, Re, z2;
    float leftin, rightin, leftout, rightout;

    LinearSmoothedValue<double> SMdepth, SMrate, SMwidth, SMcenterfreq, SMmix, SMnumstages;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhazerAudioProcessor)
};

#endif  // PLUGINPROCESSOR_H_INCLUDED

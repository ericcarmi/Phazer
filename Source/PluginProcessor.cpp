#include "PluginProcessor.h"
#include "PluginEditor.h"

PhazerAudioProcessor::PhazerAudioProcessor()
    : lastUIWidth (400),
      lastUIHeight (450),
      rateParam(nullptr),
      depthParam (nullptr),
      widthParam (nullptr),
      centerfreqParam(nullptr),
      mixParam(nullptr),
      LFOcounter(0)

{
    addParameter(rateParam = new AudioParameterFloat ("rate", "Rate", 0.001f, 20.0f, 0.5f));
    addParameter(depthParam = new AudioParameterFloat ("depth", "Depth", 0.3f, 0.99f, 0.8f));
    addParameter(widthParam = new AudioParameterFloat ("width", "Width", 0.1f, 0.9f, 0.5f));
    addParameter(centerfreqParam = new AudioParameterFloat ("freq", "Freq", 100.0f, 4000.0f, 999.977f));
    addParameter(mixParam = new AudioParameterFloat ("mix", "Mix", -1.0f, 1.0f, 0.0f));
    addParameter(stageParam = new AudioParameterInt ("stages", "Stages", 1, 16, 4));
    addParameter(powerParam = new AudioParameterBool("power", "Power", true));
    addParameter(IC1Param = new AudioParameterFloat ("ic1", "Ic1", -1.0f, 1.0f, 0.0f));
    addParameter(IC2Param = new AudioParameterFloat ("ic2", "Ic2", -1.0f, 1.0f, 0.0f));
    addParameter(IC3Param = new AudioParameterFloat ("ic3", "Ic3", -1.0f, 1.0f, 0.0f));

    oscillator.setType(Oscillators::Sine);
}

PhazerAudioProcessor::~PhazerAudioProcessor()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PhazerAudioProcessor::setPreferredBusArrangement (bool isInput, int bus, const AudioChannelSet& preferredSet)
{
    // Reject any bus arrangements that are not compatible with your plugin

    const int numChannels = preferredSet.size();

#if JucePlugin_IsMidiEffect
    if (numChannels != 0)
        return false;
#elif JucePlugin_IsSynth
    if (isInput || (numChannels != 1 && numChannels != 2))
        return false;
#else
    if (numChannels != 1 && numChannels != 2)
        return false;

    if (! setPreferredBusArrangement (! isInput, bus, preferredSet))
        return false;
#endif

    return setPreferredBusArrangement (isInput, bus, preferredSet);
}
#endif

bool PhazerAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool PhazerAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

//==============================================================================
void PhazerAudioProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    crossFadeBuffer.setSize(2, samplesPerBlock);

    SMmix.reset(newSampleRate, 0.1);
    SMrate.reset(newSampleRate, 0.1);
    SMwidth.reset(newSampleRate, 0.1);
    SMcenterfreq.reset(newSampleRate, 0.1);
    SMdepth.reset(newSampleRate, 0.1);

    LFOangle = 0.0;
    oscillator.setSampleRate(newSampleRate);
    reset();
}

void PhazerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    //keyboardState.reset();
}

void PhazerAudioProcessor::reset()
{
    // Use this method as the place to clear any delay lines, buffers, etc, as it
    // means there's been a break in the audio's continuity.
}

void PhazerAudioProcessor::process (AudioBuffer<float>& buffer,MidiBuffer& midiMessages)
{
    const int numSamples = buffer.getNumSamples();

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, numSamples);

    effectOn = *powerParam;
    if(effectOn)
    {
        mainEffectLoop(buffer);
    }

    if(crossFadeFlag)
    {
        // Mix the newly processed buffer with the previous buffer
        // Old buffer starts with amplitude of 1, decreases to 0; vice versa for new buffer
        const float del = (double)(1.0/numSamples);

        for(int chan = 0; chan > buffer.getNumChannels(); ++chan)
        {
            float* newBuff = buffer.getWritePointer(chan);
            float* oldBuff = crossFadeBuffer.getWritePointer(chan);

            for(int samp = 0; samp < numSamples; ++samp)
            {
                const float outSamp = newBuff[samp]*(samp+del) + oldBuff[samp]*(1-(samp+del));
                newBuff[samp] = outSamp;
            }
        }
        crossFadeFlag = false;
    }

    crossFadeBuffer.makeCopyOf(buffer);
}

void PhazerAudioProcessor::mainEffectLoop (AudioBuffer<float>& buffer)
{
    nums = buffer.getNumSamples();
    chans = buffer.getNumChannels();
    sampRate = getSampleRate();
    sampTime = 1.0/sampRate;
    Omega = 2.0 * double_Pi / sampRate;

    SMrate.setValue(*rateParam);
    SMdepth.setValue(*depthParam);
    SMwidth.setValue(*widthParam);
    SMmix.setValue(*mixParam);
    SMcenterfreq.setValue(*centerfreqParam);
    SMnumstages.setValue(*stageParam);

    if(resetICflag)
    {
        oscillator.resetICs(ICs);
        resetICflag = false;
    }
    if( chans == 2 )
    {

        float* leftChannel = buffer.getWritePointer (0);
        float* rightChannel = buffer.getWritePointer (1);

        for( int samp = 0; samp < nums; samp++)
        {
            leftin = leftChannel[samp];
            rightin = rightChannel[samp];

            rate = SMrate.getNextValue();
            depth = SMdepth.getNextValue();
            width = SMwidth.getNextValue();
            mix = SMmix.getNextValue();
            centerfreq = SMcenterfreq.getNextValue();

            // Smoothing number of stages does not help artifacts while changing it quickly.
            // Might need to do some sort of cross fade buffer
            numStages = static_cast<int>(SMnumstages.getNextValue());

            LFOdelta = Omega*rate;
            LFOangle += LFOdelta;

            oscillator.setFrequency(rate);
            oscillator.getNextSample(osc, 0.0);
            BW = centerfreq/3 * width;
            Re = -2.0*depth * cos(Omega*(centerfreq + BW*osc[0]));
            z2 = depth*depth;

            a0 = 1.0;
            a1 = Re;
            a2 = z2;
            b0 = z2;
            b1 = Re;
            b2 = 1.0;

            leftout = leftin;
            rightout = rightin;

            filterAP1L.setCoefficients(IIRCoefficients(b0,b1,b2,a0,a1,a2));
            filterAP1R.setCoefficients(IIRCoefficients(b0,b1,b2,a0,a1,a2));

            for(int stage = 0; stage < 4*numStages; ++stage)
            {
                leftout = filterAP1L.processSingleSampleRaw(leftout);
                rightout = filterAP1R.processSingleSampleRaw(rightout);
            }

            leftChannel[samp]  = leftin + leftout * mix;
            rightChannel[samp] = rightin  + rightout * mix;
        }
    }

    if( chans == 1 )
    {
        float* leftChannel = buffer.getWritePointer (0);

        for( int samp = 0; samp < nums; samp++)
        {

            leftin = leftChannel[samp];

            rate = SMrate.getNextValue();
            depth = SMdepth.getNextValue();
            width = SMwidth.getNextValue();
            mix = SMmix.getNextValue();
            centerfreq = SMcenterfreq.getNextValue();
            numStages = static_cast<int>(SMnumstages.getNextValue());

            LFOdelta = Omega*rate;
            LFOangle += LFOdelta;

            oscillator.setFrequency(rate);
            oscillator.getNextSample(osc, 0.0);
            BW = centerfreq/3 * width;
            Re = -2.0*depth * cos(Omega*(centerfreq + BW*osc[0]));
            z2 = depth*depth;

            a0 = 1.0;
            a1 = Re;
            a2 = z2;
            b0 = z2;
            b1 = Re;
            b2 = 1.0;

            float leftout = leftin;
            filterAP1L.setCoefficients(IIRCoefficients(b0,b1,b2,a0,a1,a2));

            for(int stage = 0; stage < 4*numStages; ++stage)
            {
                leftout = filterAP1L.processSingleSampleRaw(leftout);
            }

            leftChannel[samp]  = leftin  + leftout * mix;
        }
    }
}

//==============================================================================
AudioProcessorEditor* PhazerAudioProcessor::createEditor()
{
    return new PhazerAudioProcessorEditor (*this);
}

//==============================================================================
void PhazerAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // Here's an example of how you can use XML to make it easy and more robust:
//
//    // Create an outer XML element..
//    ScopedPointer<XmlElement> xml (parameters.state.createXml());
//
////    // add some attributes to it..
//    xml->setAttribute ("uiWidth", lastUIWidth);
//    xml->setAttribute ("uiHeight", lastUIHeight);
//
//    // Store the values of all our parameters, using their param ID as the XML attribute
//    copyXmlToBinary (*xml, destData);

}

void PhazerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

//    // This getXmlFromBinary() helper function retrieves our XML from the binary blob..
//    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
//
//    if (xmlState != nullptr)
//    {
//        // make sure that it's actually our type of XML object..
//        if (xmlState->hasTagName ("Phazer_Settings"))
//        {
//            // ok, now pull out our last window size..
//            lastUIWidth  = jmax (xmlState->getIntAttribute ("uiWidth", lastUIWidth), 400);
//            lastUIHeight = jmax (xmlState->getIntAttribute ("uiHeight", lastUIHeight), 200);
//
//            ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
//            if (xmlState != nullptr)
//                if (xmlState->hasTagName (parameters.state.getType()))
//                    parameters.state = ValueTree::fromXml (*xmlState);
//        }
//    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhazerAudioProcessor();
}

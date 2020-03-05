
#ifndef PLUGINEDITOR_H_INCLUDED
#define PLUGINEDITOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
/** This is the editor component that our filter will display.
*/
class PhazerAudioProcessorEditor  : public AudioProcessorEditor,
                                    public Button::Listener,
                                    public ComboBox::Listener
{
public:
    PhazerAudioProcessorEditor (PhazerAudioProcessor&);
    ~PhazerAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    void setupCustomLookAndFeelColours  (LookAndFeel_V3& laf);

private:

    class ParameterSlider;
    struct CustomLookAndFeel;
    TextButton stage4Button, stage8Button, stage12Button, stage16Button, chorusButton;

    Colour powerOn = Colours::green;
    Colour powerOff = Colours::red;
    ShapeButton powerButton, resetICsButton;

    void switchStage(Button* button);

    void buttonClicked(Button* button) override
    {
        switchStage(button);
    }

    void comboBoxChanged(ComboBox* box) override;

    ComboBox oscillatorBox;

    Label rateLabel, depthLabel, ratedepthLabel, mixLabel, centerfreqLabel, companyLabel, stageLabel, icLabel, resetLabel;
    ScopedPointer<ParameterSlider> rateSlider, depthSlider, ratedepthSlider, centerfreqSlider, mixSlider, stageSlider;
    // Sliders for controlling initial conditions of chaotic oscillators
    ScopedPointer<ParameterSlider> IC1Slider, IC2Slider, IC3Slider;

    //==============================================================================
    PhazerAudioProcessor& getProcessor() const
    {
        return static_cast<PhazerAudioProcessor&> (processor);
    }

};

#endif  // PLUGINEDITOR_H_INCLUDED

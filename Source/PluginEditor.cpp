#include "PluginProcessor.h"
#include "PluginEditor.h"

class PhazerAudioProcessorEditor::ParameterSlider   : public Slider,
                                                      private Timer
{
public:
    ParameterSlider (AudioProcessorParameter& p)
        : Slider (p.getName (256)), param (p)
    {
        setRange (0.0, 1.0, 0.0);
        startTimerHz (30);
        updateSliderPos();

    }

    void valueChanged() override
    {
        param.setValueNotifyingHost ((float) Slider::getValue());
    }

    void timerCallback() override       { updateSliderPos(); }

    void startedDragging() override     { param.beginChangeGesture(); }
    void stoppedDragging() override     { param.endChangeGesture();   }

    double getValueFromText (const String& text) override   { return param.getValueForText (text); }
    String getTextFromValue (double value) override         { return param.getText ((float) value, 1024) + getTextValueSuffix(); }

    void updateSliderPos()
    {
        const float newValue = param.getValue();

        if (newValue != (float) Slider::getValue() && ! isMouseButtonDown())
            Slider::setValue (newValue);
    }

    void mouseDown(const MouseEvent& e) override
    {
        ModifierKeys modK = ModifierKeys::getCurrentModifiersRealtime();

        if(modK.isCtrlDown() && modK.isLeftButtonDown())
        {
            // Need to edit the PopUpDisplayComponent, a bubbleMessage
            auto* t = getCurrentPopupDisplay();
            auto p = t->getProperties();
        }

        else
        {
            Slider::mouseDown(e);
        }
    }

    AudioProcessorParameter& param;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterSlider)
};

struct PhazerAudioProcessorEditor::CustomLookAndFeel    : public LookAndFeel_V3
{

    void drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, Slider& slider) override
    {
        const float radius = jmin (width / 2, height / 2) - 2.0f;
        const float centreX = x + width * 0.5f;
        const float centreY = y + height * 0.5f;
        const float rx = centreX - radius;
        const float ry = centreY - radius;
        const float rw = radius * 2.0f;
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const bool isMouseOver = slider.isMouseOverOrDragging() && slider.isEnabled();

        if (slider.isEnabled())
            g.setColour (slider.findColour (Slider::rotarySliderFillColourId).withAlpha (isMouseOver ? 1.0f : 0.7f));
        else
            g.setColour (Colour (0x80808080));

        {
            Rectangle<float> r (rx, ry, rw, rw);
            Path filledArc;
            filledArc.addPieSegment (rx, ry, rw, rw, rotaryStartAngle, angle, 0.8);
            //filledArc.addPieSegment (rx, ry, rw, rw, rotaryStartAngle, rotaryStartAngle+2*double_Pi, 0.1);
            g.fillPath (filledArc);

            Path knob;
            knob.addEllipse(rx, ry, rw, rw);
            knob.applyTransform(AffineTransform::scale(0.8, 0.8, centreX, centreY));
            g.setColour(Colours::black);
            g.setGradientFill(ColourGradient(Colours::lightgrey, centreX*.3, centreY*.3, Colours::black, centreX, centreY, true));
            g.fillPath(knob);

            Path needle;
            const double mangle = angle - double_Pi/2.0;
            needle.addLineSegment(Line<float>(centreX,centreY,centreX+0.4*rw*cos(mangle),centreY+0.4*rw*sin(mangle)), 2.0);
            g.setColour(Colours::white);
            g.fillPath(needle);

        }
        {
            g.setColour(Colours::black);

            const float lineThickness = jmin (15.0f, jmin (width, height) * 0.45f) * 0.1f;
            Path outlineArc;
            outlineArc.addPieSegment (rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, 0.8);
            g.strokePath (outlineArc, PathStrokeType (lineThickness));
        }
    }
};

void PhazerAudioProcessorEditor::setupCustomLookAndFeelColours (LookAndFeel_V3& laf)
{
    laf.setColour (Slider::thumbColourId, Colour::greyLevel (0.95f));
    laf.setColour (Slider::textBoxOutlineColourId, Colours::transparentWhite);
    laf.setColour (Slider::rotarySliderFillColourId, Colours::red);
    laf.setColour (Slider::rotarySliderOutlineColourId, Colours::red);
    laf.setColour (Slider::textBoxTextColourId, Colours::white);
    laf.setColour (Slider::textBoxBackgroundColourId, Colours::black);
    laf.setColour (Slider::textBoxOutlineColourId, Colours::black);
    laf.setColour (Slider::textBoxHighlightColourId, Colours::lightblue);

}

//==============================================================================
PhazerAudioProcessorEditor::PhazerAudioProcessorEditor(PhazerAudioProcessor& owner)
    : AudioProcessorEditor (owner),
      powerButton("Power", powerOff, powerOff, powerOn),
      resetICsButton("resetICs", powerOff, powerOff, powerOn),
      rateLabel(String(), "Rate"),
      depthLabel(String(), "Depth"),
      ratedepthLabel(String(), "Width"),
      mixLabel(String(), "Mix"),
      centerfreqLabel(String(), "Freq"),
      companyLabel(String(), "Phazer"),
      stageLabel(String(), "Stages"),
      icLabel(String(), "ICs"),
      resetLabel(String(),"rst")

{

    CustomLookAndFeel* claf = new CustomLookAndFeel();
    setupCustomLookAndFeelColours(*claf);

    Path circleButton;
    circleButton.addEllipse(0, 0, 20, 20);

    powerButton.setShape(circleButton, true, true, true);
    addAndMakeVisible(powerButton);
    powerButton.addListener(this);
    bool onoff = *getProcessor().powerParam;
    if(onoff)
        powerButton.setColours(powerOn, powerOn, powerOff);
    else
        powerButton.setColours(powerOff, powerOff, powerOn);


    resetICsButton.setShape(circleButton, true, true, true);
    addAndMakeVisible(resetICsButton);
    resetICsButton.addListener(this);
    resetICsButton.setVisible(false);

    addAndMakeVisible(rateSlider = new ParameterSlider (*getProcessor().rateParam));
    rateSlider->setSliderStyle(Slider::Rotary);
    rateSlider->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);
    rateSlider->setLookAndFeel(claf);
    rateSlider->setTextValueSuffix(" Hz");
  //  rateSlider->setPopupDisplayEnabled(true, true, nullptr);


    addAndMakeVisible(IC1Slider = new ParameterSlider (*getProcessor().IC1Param));
    IC1Slider->setSliderStyle(Slider::Rotary);
    IC1Slider->setTextBoxStyle(Slider::TextBoxBelow, false, 0, 0);
    IC1Slider->setLookAndFeel(claf);
    IC1Slider->setColour(Slider::rotarySliderFillColourId, Colours::red);
    IC1Slider->setVisible(false);
  //  IC1Slider->setPopupDisplayEnabled(true, true, nullptr);

    addAndMakeVisible(IC2Slider = new ParameterSlider (*getProcessor().IC2Param));
    IC2Slider->setSliderStyle(Slider::Rotary);
    IC2Slider->setTextBoxStyle(Slider::TextBoxBelow, false, 0, 0);
    IC2Slider->setLookAndFeel(claf);
    IC2Slider->setColour(Slider::rotarySliderFillColourId, Colours::green);
    IC2Slider->setVisible(false);
  //  IC2Slider->setPopupDisplayEnabled(true, true, nullptr);

    addAndMakeVisible(IC3Slider = new ParameterSlider (*getProcessor().IC3Param));
    IC3Slider->setSliderStyle(Slider::Rotary);
    IC3Slider->setTextBoxStyle(Slider::TextBoxBelow, false, 0, 0);
    IC3Slider->setLookAndFeel(claf);
    IC3Slider->setColour(Slider::rotarySliderFillColourId, Colours::blue);
    IC3Slider->setVisible(false);
  //  IC3Slider->setPopupDisplayEnabled(true, true, nullptr);

    addAndMakeVisible(depthSlider = new ParameterSlider (*getProcessor().depthParam));
    depthSlider->setSliderStyle(Slider::Rotary);
    depthSlider->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);
    depthSlider->setLookAndFeel(claf);
    depthSlider->setColour(Slider::rotarySliderFillColourId, Colours::red);
    //depthSlider->setPopupDisplayEnabled(true, true, nullptr);

    addAndMakeVisible(ratedepthSlider = new ParameterSlider (*getProcessor().widthParam));
    ratedepthSlider->setSliderStyle(Slider::Rotary);
    ratedepthSlider->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);
    ratedepthSlider->setLookAndFeel(claf);
    ratedepthSlider->setColour(Slider::rotarySliderFillColourId, Colours::cyan);
  //  ratedepthSlider->setPopupDisplayEnabled(true, true, nullptr);

    addAndMakeVisible(mixSlider = new ParameterSlider (*getProcessor().mixParam));
    mixSlider->setSliderStyle(Slider::Rotary);
    mixSlider->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);
    mixSlider->setLookAndFeel(claf);
    mixSlider->setColour(Slider::rotarySliderFillColourId, Colours::black);
    //mixSlider->setPopupDisplayEnabled(true, true, nullptr);

    addAndMakeVisible(centerfreqSlider = new ParameterSlider (*getProcessor().centerfreqParam));
    centerfreqSlider->setSliderStyle(Slider::Rotary);
    centerfreqSlider->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);
    centerfreqSlider->setLookAndFeel(claf);
    centerfreqSlider->setColour(Slider::rotarySliderFillColourId, Colours::cyan);
    //centerfreqSlider->setPopupDisplayEnabled(true, true, nullptr);
    centerfreqSlider->setTextValueSuffix(" Hz");

    // Setup the labels
    rateLabel.setColour(Label::textColourId, Colours::black);
    rateLabel.setFont (Font(String("Superclarendon"), 25.0f, Font::bold));
    rateLabel.setJustificationType(Justification::centred);
    rateLabel.attachToComponent (rateSlider, false);

    icLabel.setColour(Label::textColourId, Colours::black);
    icLabel.setFont (Font(String("Superclarendon"), 25.0f, Font::bold));
    icLabel.setJustificationType(Justification::centred);
    icLabel.attachToComponent (IC2Slider, true);

    depthLabel.setColour(Label::textColourId, Colours::black);
    depthLabel.setFont (Font(String("Superclarendon"), 25.0f, Font::bold));
    depthLabel.setJustificationType(Justification::centred);
    depthLabel.attachToComponent (depthSlider, false);

    ratedepthLabel.setColour(Label::textColourId, Colours::black);
    ratedepthLabel.setFont (Font(String("Superclarendon"), 25.0f, Font::bold));
    ratedepthLabel.setJustificationType(Justification::centred);
    ratedepthLabel.attachToComponent (ratedepthSlider, false);

    mixLabel.setColour(Label::textColourId, Colours::black);
    mixLabel.setFont (Font(String("Superclarendon"), 25.0f, Font::bold));
    mixLabel.setJustificationType(Justification::centred);
    mixLabel.attachToComponent (mixSlider, false);

    centerfreqLabel.setColour(Label::textColourId, Colours::black);
    centerfreqLabel.setFont (Font(String("Superclarendon"), 25.0f, Font::bold));
    centerfreqLabel.setJustificationType(Justification::centred);
    centerfreqLabel.attachToComponent (centerfreqSlider, false);

    addAndMakeVisible(companyLabel);
    companyLabel.setFont (Font(String("Arial"), 30.0f, Font::bold));
    companyLabel.setColour(Label::textColourId, Colours::goldenrod);

    addAndMakeVisible(stageLabel);
    stageLabel.setColour(Label::textColourId, Colours::black);
    stageLabel.setFont (Font(String("Superclarendon"), 25.0f, Font::bold));
    stageLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(resetLabel);
    resetLabel.setColour(Label::textColourId, Colours::black);
    resetLabel.setFont (Font(String("Superclarendon"), 20.0f, Font::bold));
    //resetLabel.attachToComponent(&resetICsButton, false);
    resetLabel.setJustificationType(Justification::bottom);
    resetLabel.setVisible(false);

    addAndMakeVisible(stage4Button);
    stage4Button.setColour(TextButton::textColourOnId, Colours::white);
    stage4Button.setColour(TextButton::textColourOffId, Colours::white);
    stage4Button.setColour(TextButton::buttonColourId, Colours::blue);
    stage4Button.setColour(TextButton::buttonOnColourId, Colours::lightgrey);
    stage4Button.setButtonText("4");
    stage4Button.addListener(this);
    stage4Button.setClickingTogglesState(true);
    stage4Button.setRadioGroupId(1);
    stage4Button.setToggleState(true, dontSendNotification);

    addAndMakeVisible(stage8Button);
    stage8Button.setColour(TextButton::textColourOnId, Colours::white);
    stage8Button.setColour(TextButton::textColourOffId, Colours::white);
    stage8Button.setColour(TextButton::buttonColourId, Colours::blue);
    stage8Button.setColour(TextButton::buttonOnColourId, Colours::lightgrey);
    stage8Button.setButtonText("8");
    stage8Button.addListener(this);
    stage8Button.setClickingTogglesState(true);
    stage8Button.setRadioGroupId(1);

    addAndMakeVisible(stage12Button);
    stage12Button.setColour(TextButton::textColourOnId, Colours::white);
    stage12Button.setColour(TextButton::textColourOffId, Colours::white);
    stage12Button.setColour(TextButton::buttonColourId, Colours::blue);
    stage12Button.setColour(TextButton::buttonOnColourId, Colours::lightgrey);
    stage12Button.setButtonText("12");
    stage12Button.addListener(this);
    stage12Button.setClickingTogglesState(true);
    stage12Button.setRadioGroupId(1);

    addAndMakeVisible(stage16Button);
    stage16Button.setColour(TextButton::textColourOnId, Colours::white);
    stage16Button.setColour(TextButton::textColourOffId, Colours::white);
    stage16Button.setColour(TextButton::buttonColourId, Colours::blue);
    stage16Button.setColour(TextButton::buttonOnColourId, Colours::lightgrey);
    stage16Button.setButtonText("16");
    stage16Button.addListener(this);
    stage16Button.setClickingTogglesState(true);
    stage16Button.setRadioGroupId(1);

    addAndMakeVisible(oscillatorBox);
    oscillatorBox.addItem("Sine", 1);
    oscillatorBox.addItem("Triangle", 2);
    oscillatorBox.addItem("Square", 3);
    oscillatorBox.addItem("Saw", 4);
    oscillatorBox.addItem("AASquare", 5);
    oscillatorBox.addItem("AASaw", 6);
    oscillatorBox.addItem("Vanderpol", 7);
    oscillatorBox.addItem("Duffing", 8);
    oscillatorBox.addItem("Chua", 9);
    oscillatorBox.addItem("Lorenz", 10);
    oscillatorBox.setSelectedId(1);
    oscillatorBox.addListener(this);

    // set our component's initial size to be the last one that was stored in the filter's settings
    setSize (owner.lastUIWidth,owner.lastUIHeight);
}

PhazerAudioProcessorEditor::~PhazerAudioProcessorEditor()
{

}

void PhazerAudioProcessorEditor::switchStage(Button* button)
{

    if(button == &stage4Button)
    {
        *getProcessor().stageParam = 4;
    }

    if(button == &stage8Button)
    {
        *getProcessor().stageParam = 8;
    }

    if(button == &stage12Button)
    {
        *getProcessor().stageParam = 12;
    }

    if(button == &stage16Button)
    {
        *getProcessor().stageParam = 16;
    }

    if(button == &chorusButton)
    {
        *getProcessor().stageParam = 50;
    }


    if(button == &powerButton)
    {
        bool onoff = *getProcessor().powerParam;
        getProcessor().crossFadeFlag = true;    // Always triggered whenever power is switched either way
                                                // Will be set to false within the processor
        *getProcessor().powerParam = not onoff;
        if(onoff)
        {
            powerButton.setColours(powerOff, powerOff, powerOn);
            //*getProcessor().parameters.getRawParameterValue("power") = 0.0;
        }
        else
        {
            powerButton.setColours(powerOn, powerOn, powerOff);
           // *getProcessor().parameters.getRawParameterValue("power") = 1.0;
        }
    }

    if(button == &resetICsButton)
    {
        double ics[3] =  {IC1Slider->getValue(),IC2Slider->getValue(),IC3Slider->getValue()};
        *getProcessor().ICs = *ics;
        getProcessor().resetICflag = true;

    }
}

void PhazerAudioProcessorEditor::comboBoxChanged(ComboBox *box)
{
    if(box == &oscillatorBox)
    {
        const int osc = oscillatorBox.getSelectedId();
        getProcessor().oscillator.setType(osc);

        if(osc == (Oscillators::Sine) || osc == (Oscillators::Triangle) || osc == (Oscillators::Square) || osc == (Oscillators::Saw) || osc == (Oscillators::AASaw) || osc == (Oscillators::AASquare))
        {
            rateSlider->setVisible(true);

            IC1Slider->setVisible(false);
            IC2Slider->setVisible(false);
            IC3Slider->setVisible(false);
            resetICsButton.setVisible(false);
            resetLabel.setVisible(false);
        }

        // Duffing and Vanderpol have frequency control and sensitivity to ICs
        else if(osc == Oscillators::Duffing || osc == Oscillators::Vanderpol)
        {
            rateSlider->setVisible(true);
            resetICsButton.setVisible(true);
            resetLabel.setVisible(true);

            IC1Slider->setVisible(false);
            IC2Slider->setVisible(false);
            IC3Slider->setVisible(false);
        }

        else
        {
            rateSlider->setVisible(false);

            IC1Slider->setVisible(true);
            IC2Slider->setVisible(true);
            IC3Slider->setVisible(true);
            resetICsButton.setVisible(true);
            resetLabel.setVisible(true);
        }
    }
}

//==============================================================================
void PhazerAudioProcessorEditor::paint (Graphics& g)
{
    g.setColour(Colours::slategrey);
    g.fillAll();
    g.setColour(Colours::black);
    g.fillRect(0, 0, getWidth(), 30);

}

void PhazerAudioProcessorEditor::resized()
{

    // IC sliders replace rate control when a chaotic oscillator is selected
    IC1Slider->setBounds           (40,    30,  75, 75);
    IC2Slider->setBounds           (40,    105,  75, 75);
    IC3Slider->setBounds           (40,    180,  75, 75);
    rateSlider->setBounds          (0,   55,  150, 150);
    resetICsButton.setBounds       (5,    230,   25, 25);
    resetLabel.setBounds           (0,    205,   30, 25);
    mixSlider->setBounds           (125,   170, 150, 150);
    depthSlider->setBounds         (0,     290,  150, 150);
    ratedepthSlider->setBounds     (250,   290, 150, 150);
    centerfreqSlider->setBounds    (250,   55, 150, 150);

    stage4Button.setBounds (160,65,40,30);
    stage8Button.setBounds (200,65,40,30);
    stage12Button.setBounds(160,97,40,30);
    stage16Button.setBounds(200,97,40,30);

    powerButton.setBounds(getWidth()-30,5,25,25);
    oscillatorBox.setBounds(0, 0, 100, 30);
    stageLabel.setBounds(155, 30, 90, 40);
    companyLabel.setBounds(getWidth()/2-50 , 0, 100, 30);

    getProcessor().lastUIWidth = getWidth();
    getProcessor().lastUIHeight = getHeight();
}

//==============================================================================

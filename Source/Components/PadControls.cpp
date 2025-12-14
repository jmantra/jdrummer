#include "PadControls.h"

PadControls::PadControls()
{
    // Title
    titleLabel.setText("Pad Controls", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFCCCCCC));
    addAndMakeVisible(titleLabel);
    
    // Pad name
    padNameLabel.setText("Kick", juce::dontSendNotification);
    padNameLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    padNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF00BFFF));
    padNameLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(padNameLabel);
    
    // Volume slider
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    volumeSlider.setRange(0.0, 100.0, 1.0);
    volumeSlider.setValue(100.0, juce::dontSendNotification);
    volumeSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF00BFFF));
    volumeSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFFFFFFF));
    volumeSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF333333));
    volumeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFEEEEEE));
    volumeSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF2A2A2A));
    volumeSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xFF444444));
    volumeSlider.setTextValueSuffix(" %");
    volumeSlider.addListener(this);
    addAndMakeVisible(volumeSlider);
    
    volumeLabel.setText("Volume", juce::dontSendNotification);
    volumeLabel.setFont(juce::Font(12.0f));
    volumeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));
    volumeLabel.attachToComponent(&volumeSlider, true);
    addAndMakeVisible(volumeLabel);
    
    // Pan slider
    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    panSlider.setRange(-100.0, 100.0, 1.0);
    panSlider.setValue(0.0, juce::dontSendNotification);
    panSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFFFF5722));
    panSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFFFFFFF));
    panSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF333333));
    panSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFEEEEEE));
    panSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF2A2A2A));
    panSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xFF444444));
    panSlider.addListener(this);
    addAndMakeVisible(panSlider);
    
    panLabel.setText("Pan", juce::dontSendNotification);
    panLabel.setFont(juce::Font(12.0f));
    panLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));
    panLabel.attachToComponent(&panSlider, true);
    addAndMakeVisible(panLabel);
}

PadControls::~PadControls()
{
    volumeSlider.removeListener(this);
    panSlider.removeListener(this);
}

void PadControls::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));
    
    // Draw subtle border
    g.setColour(juce::Colour(0xFF333333));
    g.drawRect(getLocalBounds(), 1);
}

void PadControls::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    
    auto topArea = bounds.removeFromTop(50);
    titleLabel.setBounds(topArea.removeFromTop(20));
    topArea.removeFromTop(5);
    padNameLabel.setBounds(topArea);
    
    bounds.removeFromTop(15);
    
    // Leave space for labels on the left
    auto sliderArea = bounds;
    sliderArea.removeFromLeft(60);  // Space for labels
    
    auto volumeArea = sliderArea.removeFromTop(30);
    volumeSlider.setBounds(volumeArea);
    
    sliderArea.removeFromTop(15);
    
    auto panArea = sliderArea.removeFromTop(30);
    panSlider.setBounds(panArea);
}

void PadControls::setSelectedPad(int midiNote, const juce::String& padName)
{
    currentNote = midiNote;
    padNameLabel.setText(padName + " (Note " + juce::String(midiNote) + ")", juce::dontSendNotification);
}

void PadControls::setVolume(float volume)
{
    volumeSlider.setValue(volume * 100.0, juce::dontSendNotification);
}

void PadControls::setPan(float pan)
{
    panSlider.setValue(pan * 100.0, juce::dontSendNotification);
}

float PadControls::getVolume() const
{
    return static_cast<float>(volumeSlider.getValue() / 100.0);
}

float PadControls::getPan() const
{
    return static_cast<float>(panSlider.getValue() / 100.0);
}

void PadControls::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &volumeSlider)
    {
        if (onVolumeChanged)
            onVolumeChanged(currentNote, getVolume());
    }
    else if (slider == &panSlider)
    {
        if (onPanChanged)
            onPanChanged(currentNote, getPan());
    }
}





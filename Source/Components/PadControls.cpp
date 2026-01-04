/*
    PadControls.cpp
    ===============
    
    Implementation of per-pad volume, pan, and mute controls.
*/

#include "PadControls.h"

PadControls::PadControls()
{
    // Title label showing selected pad name
    titleLabel.setText("Kick", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, accentColour);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);
    
    // Volume label
    volumeLabel.setText("Volume", juce::dontSendNotification);
    volumeLabel.setFont(juce::Font(12.0f));
    volumeLabel.setColour(juce::Label::textColourId, textColour);
    volumeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(volumeLabel);
    
    // Volume slider
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.5);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    volumeSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF00BFFF));
    volumeSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFFFFFFF));
    volumeSlider.onValueChange = [this]() {
        if (onVolumeChanged)
            onVolumeChanged(selectedNote, static_cast<float>(volumeSlider.getValue()));
    };
    addAndMakeVisible(volumeSlider);
    
    // Pan label
    panLabel.setText("Pan", juce::dontSendNotification);
    panLabel.setFont(juce::Font(12.0f));
    panLabel.setColour(juce::Label::textColourId, textColour);
    panLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(panLabel);
    
    // Pan slider
    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.setValue(0.0);
    panSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    panSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF666666));
    panSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFCCCCCC));
    panSlider.onValueChange = [this]() {
        if (onPanChanged)
            onPanChanged(selectedNote, static_cast<float>(panSlider.getValue()));
    };
    addAndMakeVisible(panSlider);
    
    // Mute button
    muteButton.setButtonText("MUTE");
    muteButton.setClickingTogglesState(true);
    muteButton.onClick = [this]() {
        updateMuteButtonAppearance();
        if (onMuteChanged)
            onMuteChanged(selectedNote, muteButton.getToggleState());
    };
    updateMuteButtonAppearance();
    addAndMakeVisible(muteButton);
}

PadControls::~PadControls()
{
}

void PadControls::paint(juce::Graphics& g)
{
    g.fillAll(backgroundColour);
    
    // Border
    g.setColour(juce::Colour(0xFF333344));
    g.drawRect(getLocalBounds(), 1);
}

void PadControls::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Title at top
    titleLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(5);
    
    // Volume row
    auto volumeRow = bounds.removeFromTop(25);
    volumeLabel.setBounds(volumeRow.removeFromLeft(50));
    volumeSlider.setBounds(volumeRow);
    
    bounds.removeFromTop(5);
    
    // Pan row
    auto panRow = bounds.removeFromTop(25);
    panLabel.setBounds(panRow.removeFromLeft(50));
    panSlider.setBounds(panRow);
    
    bounds.removeFromTop(10);
    
    // Mute button - large and prominent
    auto muteRow = bounds.removeFromTop(50);
    muteButton.setBounds(muteRow.withSizeKeepingCentre(200, 40));
}

void PadControls::setSelectedPad(int midiNote, const juce::String& padName)
{
    selectedNote = midiNote;
    selectedPadName = padName;
    titleLabel.setText(padName, juce::dontSendNotification);
}

void PadControls::setVolume(float volume)
{
    volumeSlider.setValue(volume, juce::dontSendNotification);
}

float PadControls::getVolume() const
{
    return static_cast<float>(volumeSlider.getValue());
}

void PadControls::setPan(float pan)
{
    panSlider.setValue(pan, juce::dontSendNotification);
}

float PadControls::getPan() const
{
    return static_cast<float>(panSlider.getValue());
}

void PadControls::setMute(bool muted)
{
    muteButton.setToggleState(muted, juce::dontSendNotification);
    updateMuteButtonAppearance();
}

bool PadControls::getMute() const
{
    return muteButton.getToggleState();
}

void PadControls::updateMuteButtonAppearance()
{
    if (muteButton.getToggleState())
    {
        // Muted - red background
        muteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFCC3333));
        muteButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFFFFFF));
        muteButton.setButtonText("MUTED");
    }
    else
    {
        // Not muted - dark background
        muteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333344));
        muteButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFAAAAAA));
        muteButton.setButtonText("MUTE");
    }
}

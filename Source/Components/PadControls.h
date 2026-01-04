/*
    PadControls.h
    =============
    
    UI component for per-pad volume, pan, and mute controls.
    Displayed at the bottom of the Drum Kit tab.
*/

#pragma once

#include "../JuceHeader.h"

class PadControls : public juce::Component
{
public:
    PadControls();
    ~PadControls() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Set the currently selected pad
    void setSelectedPad(int midiNote, const juce::String& padName);
    
    // Get/set volume (0.0 to 1.0)
    void setVolume(float volume);
    float getVolume() const;
    
    // Get/set pan (-1.0 left to 1.0 right)
    void setPan(float pan);
    float getPan() const;
    
    // Get/set mute state
    void setMute(bool muted);
    bool getMute() const;
    
    // Callbacks for when values change
    std::function<void(int note, float volume)> onVolumeChanged;
    std::function<void(int note, float pan)> onPanChanged;
    std::function<void(int note, bool muted)> onMuteChanged;

private:
    int selectedNote = 36;  // Default to kick drum
    juce::String selectedPadName = "Kick";
    
    // UI Components
    juce::Label titleLabel;
    juce::Label volumeLabel;
    juce::Slider volumeSlider;
    juce::Label panLabel;
    juce::Slider panSlider;
    juce::TextButton muteButton;
    
    // Update mute button appearance based on state
    void updateMuteButtonAppearance();
    
    // Colors
    juce::Colour backgroundColour{0xFF1E1E2E};
    juce::Colour textColour{0xFFEEEEEE};
    juce::Colour accentColour{0xFF00BFFF};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PadControls)
};

#pragma once

#include "JuceHeader.h"

class PadControls : public juce::Component,
                    public juce::Slider::Listener
{
public:
    PadControls();
    ~PadControls() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Set which pad is currently being edited
    void setSelectedPad(int midiNote, const juce::String& padName);
    
    // Set current values
    void setVolume(float volume);
    void setPan(float pan);
    
    // Get current values
    float getVolume() const;
    float getPan() const;
    
    // Callbacks
    std::function<void(int, float)> onVolumeChanged;  // note, volume
    std::function<void(int, float)> onPanChanged;     // note, pan

private:
    void sliderValueChanged(juce::Slider* slider) override;
    
    juce::Label titleLabel;
    juce::Label padNameLabel;
    
    juce::Slider volumeSlider;
    juce::Label volumeLabel;
    
    juce::Slider panSlider;
    juce::Label panLabel;
    
    int currentNote = 36;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PadControls)
};


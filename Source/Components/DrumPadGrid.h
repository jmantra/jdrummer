#pragma once

#include "JuceHeader.h"
#include "DrumPad.h"

class DrumPadGrid : public juce::Component
{
public:
    DrumPadGrid();
    ~DrumPadGrid() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Trigger visual feedback for a specific MIDI note (called from MIDI input)
    void triggerPadVisual(int midiNote);
    
    // Get the currently selected pad's MIDI note
    int getSelectedNote() const { return selectedNote; }
    
    // Callbacks
    std::function<void(int, float)> onPadPressed;   // note, velocity
    std::function<void(int)> onPadReleased;         // note
    std::function<void(int)> onPadSelected;         // note

private:
    void createPads();
    void selectPad(int midiNote);
    
    juce::OwnedArray<DrumPad> pads;
    int selectedNote = 36;  // Default to kick drum
    
    // Standard GM drum mapping for our 16 pads (2 rows of 8)
    struct PadInfo
    {
        int note;
        const char* name;
        juce::Colour colour;
    };
    
    static const std::vector<PadInfo>& getPadInfos();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumPadGrid)
};


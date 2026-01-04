/*
    GroovesPanel.h
    ==============
    
    Main panel containing the groove browser and composer.
    This is displayed when the "GROOVES" tab is selected.
*/

#pragma once

#include "../JuceHeader.h"
#include "../GrooveManager.h"
#include "GrooveBrowser.h"
#include "GrooveComposer.h"

// Forward declaration
class JdrummerAudioProcessor;

class GroovesPanel : public juce::Component,
                      public juce::DragAndDropContainer,
                      public juce::Timer
{
public:
    GroovesPanel();
    ~GroovesPanel() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Set references to processor and groove manager
    void setProcessor(JdrummerAudioProcessor* processor);
    void setGrooveManager(GrooveManager* manager);
    
    // Refresh all content
    void refresh();
    
    // Update playing state
    void updatePlayingState();
    
    // Timer callback to update BPM display
    void timerCallback() override;

private:
    JdrummerAudioProcessor* audioProcessor = nullptr;
    GrooveManager* grooveManager = nullptr;
    
    // Child components
    GrooveBrowser grooveBrowser;
    GrooveComposer grooveComposer;
    
    // Playback controls
    juce::TextButton previewButton;
    juce::TextButton stopButton;
    juce::ToggleButton loopToggle;
    juce::Label bpmLabel;
    
    // Setup callbacks between components
    void setupCallbacks();
    
    // Preview a groove
    void previewGroove(int categoryIndex, int grooveIndex);
    
    // Stop preview
    void stopPreview();
    
    // Handle external drag from groove browser
    void startGrooveDrag(int categoryIndex, int grooveIndex);
    bool isDragging = false;
    
    // Colors
    juce::Colour backgroundColour{0xFF1A1A2E};
    juce::Colour textColour{0xFFEEEEEE};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GroovesPanel)
};



/*
    PluginEditor.h
    ==============
    
    The main UI (graphical user interface) for the plugin.
    
    JUCE UI ARCHITECTURE
    --------------------
    - AudioProcessorEditor is the base class for plugin UIs
    - It's a Component, so it can contain other Components
    - The editor holds a reference to the processor to read/write state
    - JUCE uses a "retained mode" GUI: you define what to draw,
      and JUCE handles when to draw it
    
    EDITOR LIFECYCLE
    ----------------
    1. DAW calls processor.createEditor()
    2. Editor is constructed with reference to processor
    3. User interacts with editor
    4. Editor modifies processor state through callbacks
    5. DAW closes plugin window, editor is destroyed
    6. Editor may be created/destroyed multiple times while processor lives
*/

#pragma once

#include "JuceHeader.h"
#include "PluginProcessor.h"
#include "Components/DrumPadGrid.h"
#include "Components/PadControls.h"
#include "Components/GroovesPanel.h"
#include "Components/BandmatePanel.h"

/*
    INHERITANCE FROM MULTIPLE CLASSES
    ---------------------------------
    We inherit from:
    1. juce::AudioProcessorEditor - base class for plugin UIs
    2. juce::Timer - allows periodic callbacks (for checking MIDI triggers)
    
    In JUCE, it's common to inherit from multiple classes to add
    capabilities like timers, listeners, button handlers, etc.
*/
class JdrummerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      public juce::Timer
{
public:
    /*
        CONSTRUCTOR
        -----------
        Takes a REFERENCE to the processor (not a copy, not a pointer).
        
        References (&) vs Pointers (*):
        - References must be initialized and can't be null
        - References can't be reassigned to point elsewhere
        - References use . syntax, pointers use -> syntax
        
        We use a reference because the processor always exists
        and we never need to reassign it.
    */
    JdrummerAudioProcessorEditor(JdrummerAudioProcessor&);
    ~JdrummerAudioProcessorEditor() override;

    // Draw the editor's background and visual elements
    void paint(juce::Graphics&) override;
    
    // Position child components when size changes
    void resized() override;
    
    // Called periodically to check for MIDI triggers
    void timerCallback() override;

private:
    /*
        PRIVATE HELPER METHODS
        ----------------------
        These are internal implementation details.
        Private methods keep the public interface clean.
    */
    
    // Wire up all the callbacks between components
    void setupCallbacks();
    
    // Update the pad controls to show the selected pad's settings
    void updatePadControlsForSelectedPad();
    
    // Populate the kit dropdown with available soundfonts
    void populateKitComboBox();
    
    // Called when user selects a different kit
    void onKitComboBoxChanged();
    
    // Convert a MIDI note number to a human-readable name
    juce::String getPadNameForNote(int note);
    
    /*
        REFERENCE TO PROCESSOR
        ----------------------
        We store a REFERENCE (not a pointer) to the audio processor.
        
        This allows us to:
        - Read processor state (which kit is loaded, etc.)
        - Trigger notes when pads are clicked
        - Update processor when user changes settings
    */
    JdrummerAudioProcessor& audioProcessor;
    
    /*
        UI COMPONENTS
        -------------
        These are the visible elements of our interface.
        
        In JUCE, you typically:
        1. Declare components as member variables
        2. Configure them in the constructor
        3. Position them in resized()
        4. They're automatically destroyed when the editor is destroyed
    */
    
    // Header area
    juce::Label titleLabel;      // Shows "jdrummer"
    juce::Label kitLabel;        // Shows "Kit:"
    juce::ComboBox kitComboBox;  // Dropdown to select drum kit
    
    // Tab buttons for switching views
    juce::TextButton drumKitTabButton;
    juce::TextButton groovesTabButton;
    juce::TextButton bandmateTabButton;
    
    // Track which tab is active (0 = Drum Kit, 1 = Grooves, 2 = Bandmate)
    int currentTab = 0;
    
    // Main area - the drum pad grid (shown when Drum Kit tab is selected)
    DrumPadGrid drumPadGrid;
    
    // Bottom area - controls for the selected pad
    PadControls padControls;
    
    // Grooves panel (shown when Grooves tab is selected)
    GroovesPanel groovesPanel;
    
    // Bandmate panel (shown when Bandmate tab is selected)
    BandmatePanel bandmatePanel;
    
    // Switch between tabs
    void showTab(int tabIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JdrummerAudioProcessorEditor)
};

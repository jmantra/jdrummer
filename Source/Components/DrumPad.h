/*
    DrumPad.h
    =========
    
    A single drum pad component that can be clicked to trigger sounds.
    
    JUCE COMPONENT MODEL
    --------------------
    In JUCE, everything visible on screen is a "Component".
    Components form a TREE structure (parent-child relationships).
    
    Each Component has:
    - A position and size (bounds)
    - A paint() method to draw itself
    - Mouse/keyboard event handlers
    - Optional child components
    
    The framework calls paint() whenever the component needs to be redrawn.
    You request a redraw by calling repaint().
*/

#pragma once

#include "JuceHeader.h"

/*
    MULTIPLE INHERITANCE
    --------------------
    This class inherits from TWO classes:
    1. juce::Component - base class for all visible UI elements
    2. juce::Timer - allows periodic callbacks (for animation)
    
    Multiple inheritance can be complex in C++, but it's common in JUCE
    for adding functionality like timers, listeners, etc.
*/
class DrumPad : public juce::Component,
                public juce::Timer
{
public:
    /*
        CONSTRUCTOR PARAMETERS
        ----------------------
        - int midiNote: The MIDI note number this pad triggers (36-127)
        - const juce::String& name: Display name ("Kick", "Snare", etc.)
                                   'const' = won't modify it
                                   '&' = reference (avoids copying the string)
        - juce::Colour padColour: The base color for this pad
    */
    DrumPad(int midiNote, const juce::String& name, juce::Colour padColour);
    
    // Destructor - cleans up resources (stops the timer)
    ~DrumPad() override;

    /*
        OVERRIDDEN COMPONENT METHODS
        ----------------------------
        These are virtual methods from juce::Component that we customize.
    */
    
    // Called to draw the component - DO NOT call this directly, call repaint()
    void paint(juce::Graphics& g) override;
    
    // Called when the component is resized - set up child component positions here
    void resized() override;
    
    // Mouse event handlers - called by JUCE when mouse interacts with this component
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    
    // Called periodically by the Timer (for glow animation)
    void timerCallback() override;
    
    /*
        CUSTOM PUBLIC METHODS
        ---------------------
    */
    
    // Trigger the glow effect (called when MIDI note is received)
    void triggerVisualFeedback();
    
    // Getter methods - 'const' means they don't modify the object
    int getMidiNote() const { return midiNote; }
    juce::String getPadName() const { return padName; }
    bool isSelected() const { return selected; }
    
    // Change selection state
    void setSelected(bool shouldBeSelected);
    
    /*
        CALLBACK FUNCTIONS (std::function)
        -----------------------------------
        These allow external code to be notified when things happen.
        
        std::function<void(int, float)> means:
        - Returns void
        - Takes int and float as parameters
        
        This is the OBSERVER PATTERN - the pad notifies listeners
        without knowing who they are.
    */
    std::function<void(int, float)> onPadPressed;   // (note, velocity)
    std::function<void(int)> onPadReleased;         // (note)
    std::function<void(int)> onPadSelected;         // (note)

private:
    /*
        MEMBER VARIABLES
        ----------------
        Private members store the state of this pad.
        Naming convention: many programmers prefix member variables
        with 'm_' or use other conventions. Here we use plain names.
    */
    
    int midiNote;              // MIDI note number (36 = C1, 37 = C#1, etc.)
    juce::String padName;      // Display name
    juce::Colour baseColour;   // The pad's main color
    
    bool pressed = false;      // Is the pad currently pressed?
    bool selected = false;     // Is this pad selected for editing?
    float glowIntensity = 0.0f; // Current glow brightness (0.0 to 1.0)
    
    /*
        STATIC CONST
        ------------
        'static' means shared by ALL instances of this class.
        'constexpr' means computed at compile time (more efficient).
        
        This defines how fast the glow fades out per timer tick.
    */
    static constexpr float glowDecay = 0.08f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumPad)
};

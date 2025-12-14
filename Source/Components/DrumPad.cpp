/*
    DrumPad.cpp
    ===========
    
    Implementation of the DrumPad component.
    Demonstrates: custom painting, animation, mouse handling, timers.
*/

#include "DrumPad.h"

/*
    CONSTRUCTOR WITH INITIALIZER LIST
    ---------------------------------
    The syntax after the colon (:) is the INITIALIZER LIST.
    It initializes member variables BEFORE the constructor body runs.
    
    This is more efficient than assignment in the body because:
    - Members are constructed directly with the right values
    - No default construction followed by assignment
*/
DrumPad::DrumPad(int midiNote, const juce::String& name, juce::Colour padColour)
    : midiNote(midiNote),        // Initialize member 'midiNote' with parameter 'midiNote'
      padName(name),             // Initialize member 'padName' with parameter 'name'
      baseColour(padColour)      // Initialize member 'baseColour' with parameter 'padColour'
{
    // setOpaque(false) tells JUCE this component has transparency
    // (needed for the glow effect that extends beyond our bounds)
    setOpaque(false);
    
    // Start a timer that calls timerCallback() 60 times per second
    // This creates smooth animation for the glow effect
    startTimerHz(60);
}

DrumPad::~DrumPad()
{
    // Always stop the timer in the destructor!
    // Otherwise the timer might try to call us after we're destroyed.
    stopTimer();
}

/*
    PAINT METHOD
    ------------
    This is where all drawing happens. JUCE calls this when:
    - The component first appears
    - You call repaint()
    - The window is exposed after being hidden
    
    The Graphics object 'g' provides all drawing operations.
    
    COORDINATE SYSTEM:
    - Origin (0,0) is top-left
    - X increases to the right
    - Y increases downward
*/
void DrumPad::paint(juce::Graphics& g)
{
    // Get our bounds as a float rectangle, reduced by 3 pixels on each side
    // toFloat() converts int Rectangle to float for smoother drawing
    auto bounds = getLocalBounds().toFloat().reduced(3.0f);
    float cornerRadius = 8.0f;
    
    /*
        GLOW EFFECT
        -----------
        Draw an expanded, semi-transparent version of the pad behind it.
        This creates a "glow" effect when glowIntensity > 0.
    */
    if (glowIntensity > 0.01f)  // Only draw if visible (optimization)
    {
        // Expand the rectangle based on glow intensity
        auto glowBounds = bounds.expanded(glowIntensity * 8.0f);
        
        // withAlpha() creates a semi-transparent version of the color
        g.setColour(baseColour.withAlpha(glowIntensity * 0.5f));
        
        // Draw a filled rounded rectangle
        g.fillRoundedRectangle(glowBounds, cornerRadius + glowIntensity * 4.0f);
    }
    
    /*
        PAD BACKGROUND WITH GRADIENT
        ----------------------------
        Gradients create a 3D effect, making the pad look like a button.
    */
    
    // Adjust color based on pressed state
    // TERNARY OPERATOR: condition ? value_if_true : value_if_false
    juce::Colour bgColour = pressed ? baseColour.brighter(0.3f) : baseColour.darker(0.2f);
    
    // Create a vertical gradient (lighter at top, darker at bottom)
    juce::ColourGradient gradient(
        bgColour.brighter(0.15f),  // Start color (top)
        bounds.getX(), bounds.getY(),  // Start position
        bgColour.darker(0.15f),    // End color (bottom)
        bounds.getX(), bounds.getBottom(),  // End position
        false  // Not radial (linear gradient)
    );
    
    // Apply the gradient and fill the shape
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerRadius);
    
    /*
        BORDER
        ------
        Draw an outline around the pad.
        Selected pads get a white, thicker border.
    */
    juce::Colour borderColour = selected ? juce::Colour(0xFFFFFFFF) : baseColour.brighter(0.4f);
    g.setColour(borderColour);
    // drawRoundedRectangle draws just the outline (not filled)
    g.drawRoundedRectangle(bounds, cornerRadius, selected ? 2.5f : 1.5f);
    
    /*
        INNER HIGHLIGHT
        ---------------
        A subtle highlight at the top creates a glossy look.
    */
    auto innerBounds = bounds.reduced(2.0f);
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    // removeFromTop() returns the top portion and modifies the rectangle
    g.drawRoundedRectangle(innerBounds.removeFromTop(innerBounds.getHeight() * 0.5f), 
                           cornerRadius - 1.0f, 1.0f);
    
    /*
        TEXT DRAWING
        ------------
        Draw the pad name centered in the pad.
    */
    g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    // drawText with Justification::centred centers the text in the bounds
    g.drawText(padName, bounds, juce::Justification::centred);
    
    // Draw the MIDI note number in the corner (smaller, dimmer)
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(9.0f));
    g.drawText(juce::String(midiNote), bounds.reduced(5.0f), juce::Justification::bottomRight);
}

/*
    RESIZED
    -------
    Called when the component's size changes.
    We don't have child components, so nothing to do here.
*/
void DrumPad::resized()
{
    // No child components to position
}

/*
    MOUSE DOWN
    ----------
    Called when the user clicks on this component.
    
    juce::ignoreUnused() suppresses "unused parameter" warnings.
    We don't need the event details here, but must accept the parameter.
*/
void DrumPad::mouseDown(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    
    pressed = true;
    glowIntensity = 1.0f;  // Start the glow
    repaint();  // Request a redraw to show the pressed state
    
    /*
        CALLING CALLBACKS
        -----------------
        We check if the callback is set before calling it.
        This is important because the callback might be null/empty.
        
        'if (onPadPressed)' is shorthand for 'if (onPadPressed != nullptr)'
        for function objects, this checks if they're callable.
    */
    if (onPadPressed)
        onPadPressed(midiNote, 0.8f);  // Fixed velocity for mouse clicks
    
    if (onPadSelected)
        onPadSelected(midiNote);
}

void DrumPad::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    
    pressed = false;
    repaint();
    
    if (onPadReleased)
        onPadReleased(midiNote);
}

/*
    TIMER CALLBACK
    --------------
    Called periodically by the timer (60 times per second).
    We use this to animate the glow fade-out.
    
    ANIMATION CONCEPT:
    Each frame, we reduce glowIntensity slightly.
    This creates a smooth fade from 1.0 to 0.0.
*/
void DrumPad::timerCallback()
{
    if (glowIntensity > 0.01f)  // Still glowing?
    {
        glowIntensity -= glowDecay;  // Reduce glow
        
        if (glowIntensity < 0.0f)
            glowIntensity = 0.0f;  // Clamp to zero
        
        repaint();  // Redraw with new glow level
    }
}

/*
    TRIGGER VISUAL FEEDBACK
    -----------------------
    Called externally (e.g., when MIDI is received) to trigger the glow.
*/
void DrumPad::triggerVisualFeedback()
{
    glowIntensity = 1.0f;  // Full glow
    repaint();
}

void DrumPad::setSelected(bool shouldBeSelected)
{
    // Only repaint if the state actually changes
    if (selected != shouldBeSelected)
    {
        selected = shouldBeSelected;
        repaint();
    }
}

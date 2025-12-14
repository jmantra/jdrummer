/*
    GrooveComposer.h
    ================
    
    UI component for composing groove sequences.
    Users can drag grooves here to build a drum part, then drag
    the entire composition to their DAW.
    
    Similar to the "Composer" section in MT Power DrumKit.
*/

#pragma once

#include "../JuceHeader.h"
#include "../GrooveManager.h"

class GrooveComposer : public juce::Component,
                       public juce::DragAndDropTarget,
                       public juce::DragAndDropContainer
{
public:
    GrooveComposer();
    ~GrooveComposer() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    
    // Set the groove manager
    void setGrooveManager(GrooveManager* manager);
    
    // Refresh the display
    void refresh();
    
    // DragAndDropTarget interface
    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;
    void itemDragEnter(const SourceDetails& details) override;
    void itemDragExit(const SourceDetails& details) override;
    
    // Callbacks
    std::function<void()> onPlayClicked;
    std::function<void()> onStopClicked;
    std::function<void()> onClearClicked;
    std::function<void()> onCompositionChanged;
    
    // Set playing state (for button appearance)
    void setPlaying(bool isPlaying);

private:
    GrooveManager* grooveManager = nullptr;
    
    // UI Components
    juce::Label titleLabel;
    juce::Label hintLabel;
    juce::TextButton playButton;
    juce::TextButton clearButton;
    
    // Composer item display
    struct ItemRect
    {
        int composerIndex;
        juce::Rectangle<int> bounds;
    };
    std::vector<ItemRect> itemRects;
    
    int hoveredItemIndex = -1;
    int selectedItemIndex = -1;
    bool dragOver = false;
    bool isPlaying = false;
    
    // Calculate item rectangles based on current size
    void updateItemRects();
    
    // Get item at position
    int getItemAtPosition(juce::Point<int> pos);
    
    // Colors
    juce::Colour backgroundColour{0xFF1A1A1A};
    juce::Colour itemColour{0xFF3A5A7A};
    juce::Colour selectedItemColour{0xFF00BFFF};
    juce::Colour textColour{0xFFEEEEEE};
    juce::Colour dimTextColour{0xFF666666};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrooveComposer)
};


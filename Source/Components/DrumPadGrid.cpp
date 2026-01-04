#include "DrumPadGrid.h"

const std::vector<DrumPadGrid::PadInfo>& DrumPadGrid::getPadInfos()
{
    // Define colors for different drum types
    static const juce::Colour kickColour(0xFF2196F3);     // Blue
    static const juce::Colour snareColour(0xFFFF5722);    // Orange
    static const juce::Colour hihatColour(0xFF4CAF50);    // Green
    static const juce::Colour tomColour(0xFF9C27B0);      // Purple
    static const juce::Colour cymbalColour(0xFFFFEB3B);   // Yellow
    static const juce::Colour percColour(0xFF00BCD4);     // Cyan
    
    static const std::vector<PadInfo> infos = {
        // Bottom row (lower sounds)
        { 36, "Kick",      kickColour },
        { 38, "Snare",     snareColour },
        { 40, "Snare 2",   snareColour.darker(0.1f) },
        { 41, "Lo Tom",    tomColour },
        { 43, "Mid Tom",   tomColour.brighter(0.1f) },
        { 45, "Hi Tom",    tomColour.brighter(0.2f) },
        { 47, "Mid Tom 2", tomColour.brighter(0.15f) },
        { 48, "Hi Tom 2",  tomColour.brighter(0.25f) },
        
        // Top row (higher sounds)
        { 42, "HH Closed", hihatColour },
        { 44, "HH Pedal",  hihatColour.darker(0.1f) },
        { 46, "HH Open",   hihatColour.brighter(0.1f) },
        { 49, "Crash",     cymbalColour },
        { 51, "Ride",      cymbalColour.darker(0.1f) },
        { 53, "Ride Bell", cymbalColour.brighter(0.1f) },
        { 39, "Clap",      percColour },
        { 37, "Rim",       percColour.darker(0.1f) }
    };
    
    return infos;
}

DrumPadGrid::DrumPadGrid()
{
    createPads();
}

DrumPadGrid::~DrumPadGrid()
{
}

void DrumPadGrid::createPads()
{
    const auto& padInfos = getPadInfos();
    
    for (const auto& info : padInfos)
    {
        auto* pad = new DrumPad(info.note, info.name, info.colour);
        
        pad->onPadPressed = [this](int note, float velocity) {
            if (onPadPressed)
                onPadPressed(note, velocity);
        };
        
        pad->onPadReleased = [this](int note) {
            if (onPadReleased)
                onPadReleased(note);
        };
        
        pad->onPadSelected = [this](int note) {
            selectPad(note);
            if (onPadSelected)
                onPadSelected(note);
        };
        
        addAndMakeVisible(pad);
        pads.add(pad);
    }
    
    // Select the first pad (kick) by default
    if (pads.size() > 0)
    {
        pads[0]->setSelected(true);
        selectedNote = pads[0]->getMidiNote();
    }
}

void DrumPadGrid::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1A1A1A));
}

void DrumPadGrid::resized()
{
    if (pads.isEmpty())
        return;
    
    auto bounds = getLocalBounds().reduced(10);
    
    const int numCols = 8;
    const int numRows = 2;
    const int padding = 8;
    
    float padWidth = (bounds.getWidth() - (numCols - 1) * padding) / static_cast<float>(numCols);
    float padHeight = (bounds.getHeight() - (numRows - 1) * padding) / static_cast<float>(numRows);
    
    for (int i = 0; i < pads.size() && i < numCols * numRows; ++i)
    {
        int row = i / numCols;
        int col = i % numCols;
        
        // Reverse row order so bottom row is first in the array (for logical MIDI ordering)
        int displayRow = numRows - 1 - row;
        
        float x = bounds.getX() + col * (padWidth + padding);
        float y = bounds.getY() + displayRow * (padHeight + padding);
        
        pads[i]->setBounds(static_cast<int>(x), static_cast<int>(y), 
                          static_cast<int>(padWidth), static_cast<int>(padHeight));
    }
}

void DrumPadGrid::triggerPadVisual(int midiNote)
{
    for (auto* pad : pads)
    {
        if (pad->getMidiNote() == midiNote)
        {
            pad->triggerVisualFeedback();
            break;
        }
    }
}

void DrumPadGrid::selectPad(int midiNote)
{
    selectedNote = midiNote;
    
    for (auto* pad : pads)
    {
        pad->setSelected(pad->getMidiNote() == midiNote);
    }
}











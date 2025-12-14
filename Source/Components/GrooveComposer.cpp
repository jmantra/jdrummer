/*
    GrooveComposer.cpp
    ==================
    
    Implementation of the groove composer timeline.
*/

#include "GrooveComposer.h"

GrooveComposer::GrooveComposer()
{
    // Title label
    titleLabel.setText("COMPOSER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, textColour);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);
    
    // Hint label
    hintLabel.setText("Drag and drop grooves here to build your composition", 
                      juce::dontSendNotification);
    hintLabel.setFont(juce::Font(11.0f));
    hintLabel.setColour(juce::Label::textColourId, dimTextColour);
    hintLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(hintLabel);
    
    // Play button
    playButton.setButtonText("▶");
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A5A2A));
    playButton.setColour(juce::TextButton::textColourOffId, textColour);
    playButton.onClick = [this]() {
        if (isPlaying)
        {
            if (onStopClicked)
                onStopClicked();
        }
        else
        {
            if (onPlayClicked)
                onPlayClicked();
        }
    };
    addAndMakeVisible(playButton);
    
    // Clear button
    clearButton.setButtonText("CLEAR");
    clearButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF5A2A2A));
    clearButton.setColour(juce::TextButton::textColourOffId, textColour);
    clearButton.onClick = [this]() {
        if (onClearClicked)
            onClearClicked();
    };
    addAndMakeVisible(clearButton);
}

GrooveComposer::~GrooveComposer()
{
}

void GrooveComposer::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(backgroundColour);
    
    // Border
    if (dragOver)
    {
        g.setColour(selectedItemColour);
        g.drawRect(getLocalBounds(), 2);
    }
    else
    {
        g.setColour(juce::Colour(0xFF333333));
        g.drawRect(getLocalBounds(), 1);
    }
    
    // Draw timeline area
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(25);  // Title area
    bounds.removeFromLeft(45);  // Play button area
    bounds.removeFromRight(60);  // Clear button area
    
    // Timeline background
    g.setColour(juce::Colour(0xFF252525));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    
    // Draw composer items
    if (grooveManager != nullptr)
    {
        const auto& items = grooveManager->getComposerItems();
        
        if (items.empty())
        {
            // Show hint when empty
            hintLabel.setVisible(true);
        }
        else
        {
            hintLabel.setVisible(false);
            
            // Draw each item
            for (size_t i = 0; i < itemRects.size(); ++i)
            {
                const auto& rect = itemRects[i];
                
                if (rect.composerIndex < 0 || rect.composerIndex >= static_cast<int>(items.size()))
                    continue;
                
                const auto& item = items[rect.composerIndex];
                const Groove* groove = grooveManager->getGroove(item.grooveCategoryIndex, item.grooveIndex);
                
                if (groove == nullptr)
                    continue;
                
                // Item background
                juce::Colour itemBg = itemColour;
                if (static_cast<int>(i) == selectedItemIndex)
                    itemBg = selectedItemColour;
                else if (static_cast<int>(i) == hoveredItemIndex)
                    itemBg = itemColour.brighter(0.2f);
                
                g.setColour(itemBg);
                g.fillRoundedRectangle(rect.bounds.toFloat(), 3.0f);
                
                // Item border
                g.setColour(itemBg.brighter(0.3f));
                g.drawRoundedRectangle(rect.bounds.toFloat(), 3.0f, 1.0f);
                
                // Item text
                g.setColour(textColour);
                g.setFont(juce::Font(10.0f));
                
                juce::String displayName = groove->name;
                if (rect.bounds.getWidth() < 60)
                    displayName = displayName.substring(0, 6) + "...";
                
                g.drawText(displayName, rect.bounds.reduced(4, 2), 
                          juce::Justification::centred, true);
            }
        }
    }
}

void GrooveComposer::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Title at top left
    auto topRow = bounds.removeFromTop(25);
    titleLabel.setBounds(topRow.removeFromLeft(100));
    
    // Play button on the left
    auto leftArea = bounds.removeFromLeft(35);
    playButton.setBounds(leftArea.withSizeKeepingCentre(30, 30));
    bounds.removeFromLeft(5);
    
    // Clear button on the right
    auto rightArea = bounds.removeFromRight(55);
    clearButton.setBounds(rightArea.withSizeKeepingCentre(50, 25));
    bounds.removeFromRight(5);
    
    // Hint label centered in remaining area
    hintLabel.setBounds(bounds);
    
    updateItemRects();
}

void GrooveComposer::setGrooveManager(GrooveManager* manager)
{
    grooveManager = manager;
    refresh();
}

void GrooveComposer::refresh()
{
    updateItemRects();
    repaint();
}

void GrooveComposer::setPlaying(bool playing)
{
    isPlaying = playing;
    playButton.setButtonText(playing ? "■" : "▶");
    playButton.setColour(juce::TextButton::buttonColourId, 
                         playing ? juce::Colour(0xFF5A5A2A) : juce::Colour(0xFF2A5A2A));
}

void GrooveComposer::updateItemRects()
{
    itemRects.clear();
    
    if (grooveManager == nullptr)
        return;
    
    const auto& items = grooveManager->getComposerItems();
    if (items.empty())
        return;
    
    // Calculate the timeline bounds
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(25);
    bounds.removeFromLeft(45);
    bounds.removeFromRight(60);
    bounds = bounds.reduced(4);
    
    // Calculate total length
    double totalBeats = grooveManager->getComposerLengthInBeats();
    if (totalBeats <= 0)
        return;
    
    double pixelsPerBeat = static_cast<double>(bounds.getWidth()) / totalBeats;
    
    for (size_t i = 0; i < items.size(); ++i)
    {
        const auto& item = items[i];
        
        ItemRect ir;
        ir.composerIndex = static_cast<int>(i);
        
        int x = bounds.getX() + static_cast<int>(item.startBeat * pixelsPerBeat);
        int width = static_cast<int>(item.lengthInBeats * pixelsPerBeat);
        
        // Minimum width for visibility
        if (width < 20)
            width = 20;
        
        ir.bounds = juce::Rectangle<int>(x, bounds.getY(), width, bounds.getHeight());
        itemRects.push_back(ir);
    }
}

int GrooveComposer::getItemAtPosition(juce::Point<int> pos)
{
    for (size_t i = 0; i < itemRects.size(); ++i)
    {
        if (itemRects[i].bounds.contains(pos))
            return static_cast<int>(i);
    }
    return -1;
}

void GrooveComposer::mouseDown(const juce::MouseEvent& e)
{
    int clickedItem = getItemAtPosition(e.getPosition());
    
    if (e.mods.isRightButtonDown() && clickedItem >= 0)
    {
        // Right-click to remove
        if (grooveManager != nullptr)
        {
            grooveManager->removeFromComposer(clickedItem);
            selectedItemIndex = -1;
            refresh();
            
            if (onCompositionChanged)
                onCompositionChanged();
        }
    }
    else
    {
        selectedItemIndex = clickedItem;
        repaint();
    }
}

void GrooveComposer::mouseDrag(const juce::MouseEvent& e)
{
    if (selectedItemIndex >= 0 && grooveManager != nullptr)
    {
        // Start drag for external drop
        const auto& items = grooveManager->getComposerItems();
        if (selectedItemIndex < static_cast<int>(items.size()))
        {
            // Export and start dragging
            juce::File midiFile = grooveManager->exportCompositionToTempFile();
            if (midiFile.existsAsFile())
            {
                performExternalDragDropOfFiles({midiFile.getFullPathName()}, false, this);
            }
        }
    }
    
    juce::ignoreUnused(e);
}

// DragAndDropTarget implementation
bool GrooveComposer::isInterestedInDragSource(const SourceDetails& details)
{
    // Check if this is a groove being dragged
    if (auto* obj = details.description.getDynamicObject())
    {
        if (obj->hasProperty("type") && obj->getProperty("type").toString() == "groove")
            return true;
    }
    return false;
}

void GrooveComposer::itemDropped(const SourceDetails& details)
{
    dragOver = false;
    
    if (grooveManager == nullptr)
        return;
    
    if (auto* obj = details.description.getDynamicObject())
    {
        if (obj->hasProperty("type") && obj->getProperty("type").toString() == "groove")
        {
            int categoryIndex = obj->getProperty("categoryIndex");
            int grooveIndex = obj->getProperty("grooveIndex");
            
            grooveManager->addToComposer(categoryIndex, grooveIndex);
            refresh();
            
            if (onCompositionChanged)
                onCompositionChanged();
        }
    }
    
    repaint();
}

void GrooveComposer::itemDragEnter(const SourceDetails& details)
{
    juce::ignoreUnused(details);
    dragOver = true;
    repaint();
}

void GrooveComposer::itemDragExit(const SourceDetails& details)
{
    juce::ignoreUnused(details);
    dragOver = false;
    repaint();
}


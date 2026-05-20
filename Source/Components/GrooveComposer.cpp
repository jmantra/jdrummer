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
    playButton.setButtonText("Play");
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
    
    // Export button - exports MIDI and opens folder (for Bitwig compatibility)
    exportButton.setButtonText("Export MIDI");
    exportButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF3A6A3A));
    exportButton.setColour(juce::TextButton::textColourOffId, textColour);
    exportButton.onClick = [this]() {
        if (grooveManager == nullptr)
            return;
        
        const auto& items = grooveManager->getComposerItems();
        if (items.empty())
        {
            DBG("GrooveComposer: No items to export");
            return;
        }
        
        // Export the composition
        juce::File exportedFile = grooveManager->exportCompositionToTempFile();
        
        if (exportedFile.existsAsFile())
        {
            // Open the containing folder and select the file
            exportedFile.revealToUser();
            DBG("GrooveComposer: Exported and revealed: " + exportedFile.getFullPathName());
        }
    };
    addAndMakeVisible(exportButton);
}

GrooveComposer::~GrooveComposer()
{
    stopTimer();
}

void GrooveComposer::timerCallback()
{
    // Repaint to update playhead position and note highlighting
    repaint();
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
    bounds.removeFromLeft(50);  // Play button area
    bounds.removeFromRight(60);  // Clear button area
    
    // Timeline background
    g.setColour(juce::Colour(0xFF252525));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    
    // Draw composer items
    if (grooveManager != nullptr)
    {
        const auto& items = grooveManager->getComposerItems();
        
        // Always hide hint label
        hintLabel.setVisible(false);
        
        if (!items.empty())
        {
            // Get current playback position for highlighting
            double playbackPos = grooveManager->getComposerPlaybackPosition();
            bool isPlayingBack = playbackPos >= 0;
            
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
                
                // Draw MIDI notes as mini piano-roll visualization
                if (groove->isLoaded && !groove->events.empty() && groove->lengthInBeats > 0)
                {
                    // Tighter drum range for better vertical spread (most drums are 35-57)
                    const int minNote = 35;   // Kick drum
                    const int maxNote = 57;   // Crash cymbal 2
                    const float noteRange = static_cast<float>(maxNote - minNote);
                    
                    // Calculate drawable area inside the item (with small padding)
                    auto noteArea = rect.bounds.reduced(2, 14);  // More top padding for label
                    
                    // Calculate position within this groove for highlighting
                    double posInGroove = -1.0;
                    if (isPlayingBack)
                    {
                        posInGroove = playbackPos - item.startBeat;
                        // Handle looping - wrap position within selected bar range
                        if (posInGroove >= item.lengthInBeats)
                            posInGroove = -1.0;  // Past this item
                    }
                    
                    for (const auto& event : groove->events)
                    {
                        if (event.message.isNoteOn())
                        {
                            int note = event.message.getNoteNumber();
                            float velocity = event.message.getFloatVelocity();
                            
                            // Clamp notes to display range (don't skip, just clamp position)
                            int displayNote = juce::jlimit(minNote, maxNote, note);
                            
                            // Skip notes that are past the selected bar length
                            if (event.timeInBeats >= item.lengthInBeats)
                                continue;
                            
                            // X: map timeInBeats to item width (use item.lengthInBeats for selected bar range)
                            float xRatio = static_cast<float>(event.timeInBeats / item.lengthInBeats);
                            float x = noteArea.getX() + xRatio * noteArea.getWidth();
                            
                            // Y: map note number to item height (higher notes at top)
                            float yRatio = 1.0f - (static_cast<float>(displayNote - minNote) / noteRange);
                            float y = noteArea.getY() + yRatio * (noteArea.getHeight() - 4);
                            
                            // Note size - width based on velocity, fixed height
                            float noteWidth = 2.0f + velocity * 2.0f;
                            float noteHeight = 2.0f;
                            
                            // Check if this note is currently being played (within 0.15 beats of playhead)
                            bool noteIsPlaying = false;
                            if (posInGroove >= 0)
                            {
                                double timeSinceNote = posInGroove - event.timeInBeats;
                                if (timeSinceNote >= 0 && timeSinceNote < 0.2)
                                    noteIsPlaying = true;
                            }
                            
                            // Color based on note type with velocity-based brightness
                            juce::Colour noteColour;
                            if (note == 36 || note == 35)  // Kick drums
                                noteColour = juce::Colour(0xFFFF6B6B);  // Red
                            else if (note == 38 || note == 40)  // Snare drums
                                noteColour = juce::Colour(0xFF4ECDC4);  // Teal
                            else if (note >= 42 && note <= 46)  // Hi-hats
                                noteColour = juce::Colour(0xFFFFE66D);  // Yellow
                            else if (note >= 49 && note <= 57)  // Cymbals
                                noteColour = juce::Colour(0xFFFF9F43);  // Orange
                            else if (note >= 47 && note <= 48)  // Toms
                                noteColour = juce::Colour(0xFFA29BFE);  // Purple
                            else
                                noteColour = juce::Colour(0xFFDFE6E9);  // Light gray
                            
                            // Apply velocity-based alpha
                            noteColour = noteColour.withAlpha(0.6f + velocity * 0.4f);
                            
                            // Highlight notes being played - make them brighter and larger
                            if (noteIsPlaying)
                            {
                                noteColour = juce::Colours::white;
                                noteWidth *= 2.0f;
                                noteHeight = 4.0f;
                                y -= 1.0f;  // Center the larger note
                            }
                            
                            g.setColour(noteColour);
                            g.fillRect(x, y, noteWidth, noteHeight);
                        }
                    }
                    
                    // Draw playhead line if playing within this groove
                    if (posInGroove >= 0 && posInGroove < item.lengthInBeats)
                    {
                        float xRatio = static_cast<float>(posInGroove / item.lengthInBeats);
                        float playheadX = noteArea.getX() + xRatio * noteArea.getWidth();
                        
                        g.setColour(juce::Colours::white.withAlpha(0.9f));
                        g.drawVerticalLine(static_cast<int>(playheadX), 
                                          static_cast<float>(noteArea.getY()), 
                                          static_cast<float>(noteArea.getBottom()));
                    }
                }
                
                // Item border
                g.setColour(itemBg.brighter(0.3f));
                g.drawRoundedRectangle(rect.bounds.toFloat(), 3.0f, 1.0f);
                
                // Item text (draw at bottom with semi-transparent background for readability)
                juce::String displayName = groove->name;
                if (rect.bounds.getWidth() < 60)
                    displayName = displayName.substring(0, 6) + "...";
                
                auto textBounds = rect.bounds.reduced(2);
                auto textHeight = 12;
                auto textArea = textBounds.removeFromBottom(textHeight);
                
                // Semi-transparent background for text
                g.setColour(juce::Colour(0x99000000));
                g.fillRect(textArea.toFloat());
                
                g.setColour(textColour);
                g.setFont(juce::Font(9.0f));
                g.drawText(displayName, textArea.reduced(2, 0), 
                          juce::Justification::centredLeft, true);
            }
        }
    }
}

void GrooveComposer::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Title at top left
    auto topRow = bounds.removeFromTop(25);
    titleLabel.setBounds(topRow.removeFromLeft(85));
    
    // Export MIDI button
    topRow.removeFromLeft(5);
    exportButton.setBounds(topRow.removeFromLeft(80));
    
    // Play button on the left
    auto leftArea = bounds.removeFromLeft(45);
    playButton.setBounds(leftArea.withSizeKeepingCentre(40, 25));
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
    playButton.setButtonText(playing ? "Stop" : "Play");
    playButton.setColour(juce::TextButton::buttonColourId, 
                         playing ? juce::Colour(0xFF5A5A2A) : juce::Colour(0xFF2A5A2A));
    
    // Start/stop repaint timer for playhead and note highlighting
    if (playing)
        startTimerHz(30);  // 30fps update for smooth playhead
    else
        stopTimer();
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
    bounds.removeFromLeft(50);
    bounds.removeFromRight(60);
    bounds = bounds.reduced(4);
    
    // Calculate total length
    double totalBeats = grooveManager->getComposerLengthInBeats();
    if (totalBeats <= 0)
        return;
    
    // Use a fixed reference width: 8 bars (32 beats) = full width
    // If content exceeds this, expand to fit
    const double referenceBeats = 32.0;  // 8 bars at 4/4 time
    double viewportBeats = juce::jmax(referenceBeats, totalBeats);
    double pixelsPerBeat = static_cast<double>(bounds.getWidth()) / viewportBeats;
    
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
    // Allow dragging from timeline items
    if (grooveManager != nullptr && !isDraggingExternal)
    {
        if (e.getDistanceFromDragStart() < 8)
            return;
        
        startExternalDrag();
    }
}

void GrooveComposer::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    // Reset drag state on mouse up (backup in case callback doesn't fire)
    juce::Timer::callAfterDelay(500, [this]() {
        isDraggingExternal = false;
    });
}

void GrooveComposer::buttonClicked(juce::Button* button)
{
    juce::ignoreUnused(button);
}

void GrooveComposer::startExternalDrag()
{
    if (grooveManager == nullptr || isDraggingExternal)
        return;
    
    const auto& items = grooveManager->getComposerItems();
    if (items.empty())
    {
        DBG("GrooveComposer: No items in composer to drag");
        return;
    }
    
    // Export the composition
    lastExportedFile = grooveManager->exportCompositionToTempFile();
    
    if (!lastExportedFile.existsAsFile())
    {
        DBG("GrooveComposer: Failed to export composition");
        return;
    }
    
    isDraggingExternal = true;
    
    DBG("GrooveComposer: Starting external drag with file: " + lastExportedFile.getFullPathName());
    
    juce::StringArray files;
    files.add(lastExportedFile.getFullPathName());
    
    // Use callback to know when drag is complete
    bool success = performExternalDragDropOfFiles(files, true, nullptr, [this]() {
        isDraggingExternal = false;
        DBG("GrooveComposer: External drag completed");
    });
    
    if (!success)
    {
        isDraggingExternal = false;
        DBG("GrooveComposer: Failed to start external drag");
    }
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



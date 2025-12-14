/*
    GrooveBrowser.cpp
    =================
    
    Implementation of the groove browsing UI.
*/

#include "GrooveBrowser.h"

// DraggableGrooveListBox implementation for external drag & drop
void GrooveBrowser::DraggableGrooveListBox::mouseDrag(const juce::MouseEvent& e)
{
    juce::ListBox::mouseDrag(e);
    
    // Start external drag after moving a certain distance
    if (e.getDistanceFromDragStart() > 10)
    {
        browser.startExternalDrag();
    }
}

GrooveBrowser::GrooveBrowser()
    : grooveListBox(*this),
      grooveListModel(*this)
{
    // Category label
    categoryLabel.setText("STYLE", juce::dontSendNotification);
    categoryLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    categoryLabel.setColour(juce::Label::textColourId, textColour);
    categoryLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(categoryLabel);
    
    // Groove label
    grooveLabel.setText("GROOVES", juce::dontSendNotification);
    grooveLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    grooveLabel.setColour(juce::Label::textColourId, textColour);
    grooveLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(grooveLabel);
    
    // Category list box
    categoryListBox.setModel(this);
    categoryListBox.setColour(juce::ListBox::backgroundColourId, backgroundColour);
    categoryListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0xFF333333));
    categoryListBox.setRowHeight(28);
    categoryListBox.setOutlineThickness(1);
    addAndMakeVisible(categoryListBox);
    
    // Groove list box
    grooveListBox.setModel(&grooveListModel);
    grooveListBox.setColour(juce::ListBox::backgroundColourId, backgroundColour);
    grooveListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0xFF333333));
    grooveListBox.setRowHeight(24);
    grooveListBox.setOutlineThickness(1);
    grooveListBox.setMultipleSelectionEnabled(false);
    addAndMakeVisible(grooveListBox);
    
    // Bar count label
    barCountLabel.setText("Bars:", juce::dontSendNotification);
    barCountLabel.setFont(juce::Font(12.0f));
    barCountLabel.setColour(juce::Label::textColourId, dimTextColour);
    barCountLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(barCountLabel);
    
    // Bar count combo box
    barCountComboBox.addItem("All", 1);      // ID 1 = all bars (value 0)
    barCountComboBox.addItem("1 Bar", 2);    // ID 2 = 1 bar
    barCountComboBox.addItem("2 Bars", 3);   // ID 3 = 2 bars
    barCountComboBox.addItem("3 Bars", 4);   // ID 4 = 3 bars
    barCountComboBox.addItem("4 Bars", 5);   // ID 5 = 4 bars
    barCountComboBox.addItem("8 Bars", 6);   // ID 6 = 8 bars
    barCountComboBox.addItem("16 Bars", 7);  // ID 7 = 16 bars
    barCountComboBox.setSelectedId(5);  // Default to 4 bars
    barCountComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF2A2A2A));
    barCountComboBox.setColour(juce::ComboBox::textColourId, textColour);
    barCountComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xFF444444));
    addAndMakeVisible(barCountComboBox);
    
    // Add to composer button
    addToComposerButton.setButtonText("+ Add");
    addToComposerButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A5A2A));
    addToComposerButton.setColour(juce::TextButton::textColourOffId, textColour);
    addToComposerButton.onClick = [this]() { onAddToComposerClicked(); };
    addAndMakeVisible(addToComposerButton);
    
    // Drag to DAW button - explicit button for drag & drop
    dragToDAWButton.setButtonText("Drag to DAW");
    dragToDAWButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF4A4A6A));
    dragToDAWButton.setColour(juce::TextButton::textColourOffId, textColour);
    dragToDAWButton.setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    dragToDAWButton.addListener(this);
    addAndMakeVisible(dragToDAWButton);
}

GrooveBrowser::~GrooveBrowser()
{
    dragToDAWButton.removeListener(this);
}

void GrooveBrowser::mouseDown(const juce::MouseEvent& e)
{
    // Check if the click is on the drag button
    if (dragToDAWButton.getBounds().contains(e.getPosition()))
    {
        // We'll handle drag in mouseDrag
    }
}

void GrooveBrowser::mouseDrag(const juce::MouseEvent& e)
{
    // If dragging from the drag button area
    if (dragToDAWButton.getBounds().contains(e.getMouseDownPosition()) && 
        e.getDistanceFromDragStart() > 5)
    {
        startExternalDrag();
    }
}

void GrooveBrowser::buttonClicked(juce::Button* button)
{
    if (button == &dragToDAWButton)
    {
        // Show instructions - actual drag happens on mouse drag
        DBG("GrooveBrowser: Click and drag this button to your DAW");
    }
}

void GrooveBrowser::paint(juce::Graphics& g)
{
    g.fillAll(backgroundColour);
    
    // Draw subtle border
    g.setColour(juce::Colour(0xFF333333));
    g.drawRect(getLocalBounds(), 1);
}

void GrooveBrowser::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Split into left (categories) and right (grooves) panels
    auto leftPanel = bounds.removeFromLeft(bounds.getWidth() / 3);
    bounds.removeFromLeft(10);  // Spacing
    auto rightPanel = bounds;
    
    // Category panel
    categoryLabel.setBounds(leftPanel.removeFromTop(24));
    leftPanel.removeFromTop(5);
    categoryListBox.setBounds(leftPanel);
    
    // Groove panel
    grooveLabel.setBounds(rightPanel.removeFromTop(24));
    rightPanel.removeFromTop(5);
    
    // Bottom row: bar selector and buttons
    auto bottomRow = rightPanel.removeFromBottom(30);
    
    // Bar count selector on the left
    barCountLabel.setBounds(bottomRow.removeFromLeft(35));
    bottomRow.removeFromLeft(5);
    barCountComboBox.setBounds(bottomRow.removeFromLeft(70));
    bottomRow.removeFromLeft(10);
    
    // Add button
    addToComposerButton.setBounds(bottomRow.removeFromLeft(60));
    bottomRow.removeFromLeft(10);
    
    // Drag to DAW button takes the rest
    dragToDAWButton.setBounds(bottomRow);
    
    rightPanel.removeFromBottom(5);
    
    grooveListBox.setBounds(rightPanel);
}

void GrooveBrowser::setGrooveManager(GrooveManager* manager)
{
    grooveManager = manager;
    refresh();
}

void GrooveBrowser::refresh()
{
    selectedCategoryIndex = -1;
    selectedGrooveIndex = -1;
    
    categoryListBox.updateContent();
    grooveListBox.updateContent();
    
    // Select first category if available
    if (grooveManager != nullptr && !grooveManager->getCategories().empty())
    {
        selectedCategoryIndex = 0;
        categoryListBox.selectRow(0);
        onCategorySelected(0);
    }
}

// ListBoxModel implementation for categories
int GrooveBrowser::getNumRows()
{
    if (grooveManager == nullptr)
        return 0;
    return static_cast<int>(grooveManager->getCategories().size());
}

void GrooveBrowser::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (grooveManager == nullptr)
        return;
    
    const auto& categories = grooveManager->getCategories();
    if (rowNumber < 0 || rowNumber >= static_cast<int>(categories.size()))
        return;
    
    // Background
    if (rowIsSelected)
    {
        g.setColour(selectedColour.withAlpha(0.3f));
        g.fillRect(0, 0, width, height);
        g.setColour(selectedColour);
        g.fillRect(0, 0, 3, height);  // Selection indicator
    }
    
    // Draw folder icon (simple representation)
    g.setColour(rowIsSelected ? selectedColour : juce::Colour(0xFFD4A855));
    g.fillRect(8, height / 2 - 6, 14, 12);
    g.setColour(backgroundColour);
    g.fillRect(8, height / 2 - 6, 6, 3);
    
    // Text
    g.setColour(rowIsSelected ? textColour : dimTextColour);
    g.setFont(juce::Font(13.0f));
    g.drawText(categories[rowNumber].name, 28, 0, width - 32, height, juce::Justification::centredLeft);
}

void GrooveBrowser::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    onCategorySelected(row);
}

void GrooveBrowser::listBoxItemDoubleClicked(int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    onCategorySelected(row);
}

void GrooveBrowser::onCategorySelected(int categoryIndex)
{
    if (grooveManager == nullptr)
        return;
    
    const auto& categories = grooveManager->getCategories();
    if (categoryIndex < 0 || categoryIndex >= static_cast<int>(categories.size()))
        return;
    
    selectedCategoryIndex = categoryIndex;
    selectedGrooveIndex = -1;
    
    grooveListBox.updateContent();
    
    // Select first groove if available
    if (!categories[categoryIndex].grooves.empty())
    {
        selectedGrooveIndex = 0;
        grooveListBox.selectRow(0);
        
        if (onGrooveSelected)
            onGrooveSelected(selectedCategoryIndex, selectedGrooveIndex);
    }
}

void GrooveBrowser::onAddToComposerClicked()
{
    if (selectedCategoryIndex >= 0 && selectedGrooveIndex >= 0)
    {
        if (onGrooveAddToComposer)
            onGrooveAddToComposer(selectedCategoryIndex, selectedGrooveIndex, getSelectedBarCount());
    }
}

int GrooveBrowser::getSelectedBarCount() const
{
    int selectedId = barCountComboBox.getSelectedId();
    
    switch (selectedId)
    {
        case 1: return 0;   // "All" - full groove
        case 2: return 1;   // 1 bar
        case 3: return 2;   // 2 bars
        case 4: return 3;   // 3 bars
        case 5: return 4;   // 4 bars
        case 6: return 8;   // 8 bars
        case 7: return 16;  // 16 bars
        default: return 4;  // Default to 4 bars
    }
}

// GrooveListModel implementation
int GrooveBrowser::GrooveListModel::getNumRows()
{
    if (browser.grooveManager == nullptr || browser.selectedCategoryIndex < 0)
        return 0;
    
    const auto& categories = browser.grooveManager->getCategories();
    if (browser.selectedCategoryIndex >= static_cast<int>(categories.size()))
        return 0;
    
    return static_cast<int>(categories[browser.selectedCategoryIndex].grooves.size());
}

void GrooveBrowser::GrooveListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, 
                                                       int width, int height, bool rowIsSelected)
{
    if (browser.grooveManager == nullptr || browser.selectedCategoryIndex < 0)
        return;
    
    const auto& categories = browser.grooveManager->getCategories();
    if (browser.selectedCategoryIndex >= static_cast<int>(categories.size()))
        return;
    
    const auto& grooves = categories[browser.selectedCategoryIndex].grooves;
    if (rowNumber < 0 || rowNumber >= static_cast<int>(grooves.size()))
        return;
    
    // Background
    if (rowIsSelected)
    {
        g.setColour(browser.selectedColour.withAlpha(0.3f));
        g.fillRect(0, 0, width, height);
        g.setColour(browser.selectedColour);
        g.fillRect(0, 0, 3, height);
    }
    else if (rowNumber % 2 == 1)
    {
        g.setColour(juce::Colour(0xFF252525));
        g.fillRect(0, 0, width, height);
    }
    
    // Draw MIDI file icon
    g.setColour(rowIsSelected ? browser.selectedColour : juce::Colour(0xFF6688AA));
    g.drawRect(8, height / 2 - 5, 10, 10, 1);
    g.fillRect(10, height / 2 - 3, 6, 2);
    g.fillRect(10, height / 2 + 1, 4, 2);
    
    // Text
    g.setColour(rowIsSelected ? browser.textColour : browser.dimTextColour);
    g.setFont(juce::Font(12.0f));
    g.drawText(grooves[rowNumber].name, 24, 0, width - 28, height, juce::Justification::centredLeft);
}

void GrooveBrowser::GrooveListModel::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    
    if (browser.grooveManager == nullptr || browser.selectedCategoryIndex < 0)
        return;
    
    browser.selectedGrooveIndex = row;
    
    if (browser.onGrooveSelected)
        browser.onGrooveSelected(browser.selectedCategoryIndex, row);
}

void GrooveBrowser::GrooveListModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    
    if (browser.grooveManager == nullptr || browser.selectedCategoryIndex < 0)
        return;
    
    browser.selectedGrooveIndex = row;
    
    if (browser.onGrooveDoubleClicked)
        browser.onGrooveDoubleClicked(browser.selectedCategoryIndex, row);
}

juce::var GrooveBrowser::GrooveListModel::getDragSourceDescription(const juce::SparseSet<int>& selectedRows)
{
    if (browser.grooveManager == nullptr || browser.selectedCategoryIndex < 0)
        return {};
    
    if (selectedRows.isEmpty())
        return {};
    
    int row = selectedRows[0];
    
    // Return a description that identifies this groove
    juce::DynamicObject* obj = new juce::DynamicObject();
    obj->setProperty("type", "groove");
    obj->setProperty("categoryIndex", browser.selectedCategoryIndex);
    obj->setProperty("grooveIndex", row);
    
    return juce::var(obj);
}

/*
    START EXTERNAL DRAG
    -------------------
    Initiates an external file drag for dropping MIDI files into DAWs.
    This is what makes the grooves draggable to the DAW's timeline.
*/
void GrooveBrowser::startExternalDrag()
{
    // Prevent multiple simultaneous drag operations
    if (isDragging)
        return;
    
    if (grooveManager == nullptr || selectedCategoryIndex < 0 || selectedGrooveIndex < 0)
    {
        DBG("GrooveBrowser: Cannot start drag - no groove selected");
        return;
    }
    
    // Get the MIDI file path for the selected groove
    juce::File midiFile = grooveManager->exportGrooveToTempFile(selectedCategoryIndex, selectedGrooveIndex);
    
    DBG("GrooveBrowser: Starting external drag with file: " + midiFile.getFullPathName());
    
    if (midiFile.existsAsFile())
    {
        isDragging = true;
        
        // Start external drag with the MIDI file
        juce::StringArray files;
        files.add(midiFile.getFullPathName());
        
        bool success = performExternalDragDropOfFiles(files, false, this, [this]() {
            isDragging = false;
            DBG("GrooveBrowser: External drag completed");
        });
        
        if (!success)
        {
            isDragging = false;
            DBG("GrooveBrowser: Failed to start external drag");
        }
    }
    else
    {
        DBG("GrooveBrowser: MIDI file does not exist: " + midiFile.getFullPathName());
    }
}


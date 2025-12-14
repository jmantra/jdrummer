/*
    GrooveBrowser.h
    ===============
    
    UI component for browsing and selecting grooves organized by category.
    Similar to the "Style" and "Grooves" panels in MT Power DrumKit.
    
    Features:
    - Category list (left panel) showing groove folders
    - Groove list (right panel) showing grooves in selected category
    - Double-click to preview a groove
    - Drag & drop support for dragging grooves to DAW
*/

#pragma once

#include "../JuceHeader.h"
#include "../GrooveManager.h"

class GrooveBrowser : public juce::Component,
                      public juce::ListBoxModel,
                      public juce::DragAndDropContainer,
                      private juce::Button::Listener
{
public:
    GrooveBrowser();
    ~GrooveBrowser() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Set the groove manager to browse
    void setGrooveManager(GrooveManager* manager);
    
    // Refresh the lists from the groove manager
    void refresh();
    
    // ListBoxModel implementation for category list
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;
    
    // Mouse handling for drag button
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    
    // Button listener
    void buttonClicked(juce::Button* button) override;
    
    // Callbacks
    std::function<void(int categoryIndex, int grooveIndex)> onGrooveSelected;
    std::function<void(int categoryIndex, int grooveIndex)> onGrooveDoubleClicked;
    std::function<void(int categoryIndex, int grooveIndex, int barCount)> onGrooveAddToComposer;
    
    // Get the selected bar count for adding to composer
    int getSelectedBarCount() const;
    
    // Get current selection
    int getSelectedCategoryIndex() const { return selectedCategoryIndex; }
    int getSelectedGrooveIndex() const { return selectedGrooveIndex; }

private:
    // Custom ListBox that supports external drag & drop to DAWs
    class DraggableGrooveListBox : public juce::ListBox
    {
    public:
        DraggableGrooveListBox(GrooveBrowser& owner) : browser(owner) {}
        
        void mouseDrag(const juce::MouseEvent& e) override;
        
    private:
        GrooveBrowser& browser;
    };
    
    // Internal list box model for grooves (within selected category)
    class GrooveListModel : public juce::ListBoxModel
    {
    public:
        GrooveListModel(GrooveBrowser& owner) : browser(owner) {}
        
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;
        juce::var getDragSourceDescription(const juce::SparseSet<int>& selectedRows) override;
        
    private:
        GrooveBrowser& browser;
    };
    
    GrooveManager* grooveManager = nullptr;
    
    // UI Components
    juce::Label categoryLabel;
    juce::Label grooveLabel;
    juce::ListBox categoryListBox;
    DraggableGrooveListBox grooveListBox;
    juce::TextButton addToComposerButton;
    juce::TextButton dragToDAWButton;  // Explicit drag button for DAW
    juce::ComboBox barCountComboBox;   // How many bars to add
    juce::Label barCountLabel;
    
    GrooveListModel grooveListModel;
    
    // Start external file drag for DAW integration
    void startExternalDrag();
    
    // Flag to prevent multiple drag operations
    bool isDragging = false;
    
    int selectedCategoryIndex = -1;
    int selectedGrooveIndex = -1;
    
    // Update the groove list when category changes
    void onCategorySelected(int categoryIndex);
    
    // Handle add to composer button click
    void onAddToComposerClicked();
    
    // Colors
    juce::Colour backgroundColour{0xFF1E1E1E};
    juce::Colour headerColour{0xFF2A2A2A};
    juce::Colour selectedColour{0xFF00BFFF};
    juce::Colour textColour{0xFFEEEEEE};
    juce::Colour dimTextColour{0xFF888888};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrooveBrowser)
};


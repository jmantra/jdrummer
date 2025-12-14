#pragma once

#include "JuceHeader.h"

class KitSelector : public juce::Component,
                    public juce::TextEditor::Listener
{
public:
    KitSelector();
    ~KitSelector() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Set the list of available kits
    void setAvailableKits(const juce::StringArray& kits);
    
    // Select a kit by name
    void selectKit(const juce::String& kitName);
    
    // Get the currently selected kit name
    juce::String getSelectedKitName() const;
    
    // Callback when a kit is selected
    std::function<void(const juce::String&)> onKitSelected;

private:
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;
    
    void showKitMenu();
    void filterKits();

    juce::TextEditor searchBox;
    juce::TextButton kitButton;
    juce::Label titleLabel;
    
    juce::StringArray allKits;
    juce::StringArray filteredKits;
    juce::String selectedKitName;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KitSelector)
};

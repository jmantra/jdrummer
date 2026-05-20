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
    
    // ===== PRESET SELECTION =====
    
    // Set the list of available presets for the current kit
    void setAvailablePresets(const juce::StringArray& presets);
    
    // Select a preset by index
    void selectPreset(int presetIndex);
    
    // Get the currently selected preset index
    int getSelectedPresetIndex() const;
    
    // Callback when a preset is selected (passes preset index)
    std::function<void(int)> onPresetSelected;

private:
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;
    
    void showKitMenu();
    void showPresetMenu();
    void filterKits();

    juce::TextEditor searchBox;
    juce::TextButton kitButton;
    juce::TextButton presetButton;
    juce::Label titleLabel;
    juce::Label presetLabel;
    
    juce::StringArray allKits;
    juce::StringArray filteredKits;
    juce::String selectedKitName;
    
    juce::StringArray availablePresets;
    int selectedPresetIndex = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KitSelector)
};

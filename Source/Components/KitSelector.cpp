#include "KitSelector.h"

KitSelector::KitSelector()
{
    // Title label
    titleLabel.setText("Drum Kit", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFCCCCCC));
    addAndMakeVisible(titleLabel);
    
    // Search box
    searchBox.setTextToShowWhenEmpty("Search kits...", juce::Colour(0xFF666666));
    searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF2A2A2A));
    searchBox.setColour(juce::TextEditor::textColourId, juce::Colour(0xFFEEEEEE));
    searchBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xFF444444));
    searchBox.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xFF00BFFF));
    searchBox.addListener(this);
    addAndMakeVisible(searchBox);
    
    // Kit selection button (replaces ComboBox)
    kitButton.setButtonText("Select Kit...");
    kitButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A2A2A));
    kitButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFEEEEEE));
    kitButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFFFFFFF));
    kitButton.onClick = [this]() {
        DBG("KitSelector: Button clicked, showing menu");
        showKitMenu();
    };
    addAndMakeVisible(kitButton);
}

KitSelector::~KitSelector()
{
    searchBox.removeListener(this);
}

void KitSelector::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));
    
    // Draw a subtle border
    g.setColour(juce::Colour(0xFF333333));
    g.drawRect(getLocalBounds(), 1);
}

void KitSelector::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    titleLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(5);
    
    searchBox.setBounds(bounds.removeFromTop(28));
    bounds.removeFromTop(8);
    
    kitButton.setBounds(bounds.removeFromTop(28));
}

void KitSelector::setAvailableKits(const juce::StringArray& kits)
{
    allKits = kits;
    filteredKits = kits;
    
    DBG("KitSelector::setAvailableKits - received " + juce::String(kits.size()) + " kits");
    
    if (kits.size() > 0 && selectedKitName.isEmpty())
    {
        selectedKitName = kits[0];
        kitButton.setButtonText(selectedKitName);
    }
}

void KitSelector::selectKit(const juce::String& kitName)
{
    if (allKits.contains(kitName))
    {
        selectedKitName = kitName;
        kitButton.setButtonText(kitName);
        DBG("KitSelector::selectKit - selected: " + kitName);
    }
}

juce::String KitSelector::getSelectedKitName() const
{
    return selectedKitName;
}

void KitSelector::showKitMenu()
{
    juce::PopupMenu menu;
    
    // Add filtered kits to the menu
    int itemId = 1;
    for (const auto& kit : filteredKits)
    {
        bool isTicked = (kit == selectedKitName);
        menu.addItem(itemId++, kit, true, isTicked);
    }
    
    if (filteredKits.isEmpty())
    {
        menu.addItem(-1, "(No kits found)", false);
    }
    
    // Show the menu and handle selection
    menu.showMenuAsync(juce::PopupMenu::Options()
        .withTargetComponent(&kitButton)
        .withMinimumWidth(kitButton.getWidth()),
        [this](int result) {
            if (result > 0 && result <= filteredKits.size())
            {
                juce::String kitName = filteredKits[result - 1];
                DBG("KitSelector: Menu selected: " + kitName);
                
                selectedKitName = kitName;
                kitButton.setButtonText(kitName);
                
                if (onKitSelected)
                {
                    DBG("KitSelector: Calling onKitSelected for: " + kitName);
                    onKitSelected(kitName);
                }
            }
        });
}

void KitSelector::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == &searchBox)
    {
        filterKits();
    }
}

void KitSelector::textEditorReturnKeyPressed(juce::TextEditor& editor)
{
    juce::ignoreUnused(editor);
    // Select the first filtered kit if available
    if (filteredKits.size() > 0)
    {
        selectedKitName = filteredKits[0];
        kitButton.setButtonText(selectedKitName);
        
        if (onKitSelected)
            onKitSelected(selectedKitName);
    }
}

void KitSelector::textEditorEscapeKeyPressed(juce::TextEditor& editor)
{
    if (&editor == &searchBox)
    {
        searchBox.setText("", false);
        filterKits();
    }
}

void KitSelector::textEditorFocusLost(juce::TextEditor& editor)
{
    juce::ignoreUnused(editor);
}

void KitSelector::filterKits()
{
    juce::String searchText = searchBox.getText().toLowerCase();
    
    if (searchText.isEmpty())
    {
        filteredKits = allKits;
    }
    else
    {
        filteredKits.clear();
        for (const auto& kit : allKits)
        {
            if (kit.toLowerCase().contains(searchText))
            {
                filteredKits.add(kit);
            }
        }
    }
    
    DBG("KitSelector::filterKits - " + juce::String(filteredKits.size()) + " kits match filter");
}

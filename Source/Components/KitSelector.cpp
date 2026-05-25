#include "KitSelector.h"

KitSelector::KitSelector()
{
    // Title label
    titleLabel.setText("Drum Kit", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
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

    // Preset label
    presetLabel.setText("Preset:", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    presetLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF999999));
    addAndMakeVisible(presetLabel);

    // Preset selection button
    presetButton.setButtonText("Default");
    presetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF252525));
    presetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFCCCCCC));
    presetButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFFFFFFF));
    presetButton.onClick = [this]() {
        DBG("KitSelector: Preset button clicked, showing preset menu");
        showPresetMenu();
    };
    addAndMakeVisible(presetButton);
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

    // Preset section (only show if we have multiple presets)
    if (availablePresets.size() > 1)
    {
        bounds.removeFromTop(10);
        presetLabel.setBounds(bounds.removeFromTop(16));
        bounds.removeFromTop(3);
        presetButton.setBounds(bounds.removeFromTop(24));
        presetLabel.setVisible(true);
        presetButton.setVisible(true);
    }
    else
    {
        presetLabel.setVisible(false);
        presetButton.setVisible(false);
    }
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

// ===== PRESET SELECTION =====

void KitSelector::setAvailablePresets(const juce::StringArray& presets)
{
    availablePresets = presets;
    selectedPresetIndex = 0;

    DBG("KitSelector::setAvailablePresets - received " + juce::String(presets.size()) + " presets");

    if (presets.size() > 0)
    {
        presetButton.setButtonText(presets[0]);
    }
    else
    {
        presetButton.setButtonText("Default");
    }

    // Trigger a resize to show/hide preset controls
    resized();
}

void KitSelector::selectPreset(int presetIndex)
{
    if (presetIndex >= 0 && presetIndex < availablePresets.size())
    {
        selectedPresetIndex = presetIndex;
        presetButton.setButtonText(availablePresets[presetIndex]);
        DBG("KitSelector::selectPreset - selected: " + juce::String(presetIndex) +
            " (" + availablePresets[presetIndex] + ")");
    }
}

int KitSelector::getSelectedPresetIndex() const
{
    return selectedPresetIndex;
}

void KitSelector::showPresetMenu()
{
    if (availablePresets.isEmpty())
        return;

    juce::PopupMenu menu;

    int itemId = 1;
    for (const auto& preset : availablePresets)
    {
        bool isTicked = ((itemId - 1) == selectedPresetIndex);
        menu.addItem(itemId++, preset, true, isTicked);
    }

    menu.showMenuAsync(juce::PopupMenu::Options()
        .withTargetComponent(&presetButton)
        .withMinimumWidth(presetButton.getWidth()),
        [this](int result) {
            if (result > 0 && result <= availablePresets.size())
            {
                int presetIndex = result - 1;
                juce::String presetName = availablePresets[presetIndex];
                DBG("KitSelector: Preset menu selected: " + juce::String(presetIndex) +
                    " (" + presetName + ")");

                selectedPresetIndex = presetIndex;
                presetButton.setButtonText(presetName);

                if (onPresetSelected)
                {
                    DBG("KitSelector: Calling onPresetSelected for index: " + juce::String(presetIndex));
                    onPresetSelected(presetIndex);
                }
            }
        });
}

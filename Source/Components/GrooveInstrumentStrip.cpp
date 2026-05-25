/*
    GrooveInstrumentStrip.cpp
    =========================
*/

#include "GrooveInstrumentStrip.h"

GrooveInstrumentStrip::InstrumentIcon::InstrumentIcon(DrumInstrument inst)
    : instrument(inst)
{
}

void GrooveInstrumentStrip::InstrumentIcon::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const auto colour = getInstrumentColour(instrument);
    constexpr float cornerRadius = 4.0f;

    if (!interactive)
    {
        g.setColour(juce::Colour(0xFF383838));
        g.fillRoundedRectangle(bounds, cornerRadius);
        g.setColour(juce::Colour(0xFF666666));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
        g.setColour(presentInGroove ? juce::Colour(0xFFCCCCCC) : juce::Colour(0xFFAAAAAA));
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText(getInstrumentLabel(instrument), bounds, juce::Justification::centred);
        return;
    }

    if (enabled)
    {
        g.setColour(juce::Colour(0xFFE8C547).withAlpha(presentInGroove ? 1.0f : 0.55f));
        g.fillRoundedRectangle(bounds, cornerRadius);
        g.setColour(juce::Colours::black.withAlpha(0.85f));
    }
    else
    {
        g.setColour(juce::Colour(0xFF383838));
        g.fillRoundedRectangle(bounds, cornerRadius);
        g.setColour(juce::Colour(0xFF666666));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
        g.setColour(presentInGroove ? juce::Colour(0xFFCCCCCC) : juce::Colour(0xFFAAAAAA));
    }

    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText(getInstrumentLabel(instrument), bounds, juce::Justification::centred);

    if (enabled)
    {
        g.setColour(colour.withAlpha(0.9f));
        g.fillEllipse(bounds.getX() + 3.0f, bounds.getBottom() - 5.0f, 4.0f, 4.0f);
    }
}

void GrooveInstrumentStrip::InstrumentIcon::mouseDown(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);

    if (!interactive)
        return;

    enabled = !enabled;

    if (onToggled)
        onToggled(instrument, enabled);

    repaint();
}

void GrooveInstrumentStrip::InstrumentIcon::setEnabledState(bool isEnabled)
{
    enabled = isEnabled;
    repaint();
}

void GrooveInstrumentStrip::InstrumentIcon::setPresentInGroove(bool present)
{
    presentInGroove = present;
    repaint();
}

void GrooveInstrumentStrip::InstrumentIcon::setInteractive(bool isInteractive)
{
    interactive = isInteractive;
    repaint();
}

GrooveInstrumentStrip::DensitySlider::DensitySlider() = default;

void GrooveInstrumentStrip::DensitySlider::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f, 4.0f);
    const float trackY = bounds.getCentreY();

    g.setColour(interactive ? juce::Colour(0xFF444444) : juce::Colour(0xFF333333));
    g.drawLine(bounds.getX(), trackY, bounds.getRight(), trackY, 1.0f);

    const float stepWidth = bounds.getWidth() / static_cast<float>(kMaxDensityLevel);

    for (int i = 0; i < kDensitySteps; ++i)
    {
        const float x = bounds.getX() + stepWidth * static_cast<float>(i);
        g.setColour(interactive ? juce::Colour(0xFF666666) : juce::Colour(0xFF444444));
        g.fillEllipse(x - 2.0f, trackY - 2.0f, 4.0f, 4.0f);
    }

    const float knobX = bounds.getX() + stepWidth * static_cast<float>(level);
    g.setColour(interactive ? juce::Colour(0xFFE8C547) : juce::Colour(0xFF666666));
    g.fillEllipse(knobX - 4.0f, trackY - 4.0f, 8.0f, 8.0f);
}

void GrooveInstrumentStrip::DensitySlider::mouseDown(const juce::MouseEvent& e)
{
    if (!interactive)
        return;

    setLevelFromPosition(static_cast<float>(e.x));
}

void GrooveInstrumentStrip::DensitySlider::mouseDrag(const juce::MouseEvent& e)
{
    if (!interactive)
        return;

    setLevelFromPosition(static_cast<float>(e.x));
}

void GrooveInstrumentStrip::DensitySlider::setLevel(uint8_t newLevel)
{
    level = static_cast<uint8_t>(juce::jlimit(0, kMaxDensityLevel, static_cast<int>(newLevel)));
    repaint();
}

void GrooveInstrumentStrip::DensitySlider::setInteractive(bool isInteractive)
{
    interactive = isInteractive;
    repaint();
}

void GrooveInstrumentStrip::DensitySlider::setLevelFromPosition(float x)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f, 4.0f);
    const float stepWidth = bounds.getWidth() / static_cast<float>(kMaxDensityLevel);
    const int newLevel = juce::jlimit(0, kMaxDensityLevel,
                                      static_cast<int>(std::round((x - bounds.getX()) / stepWidth)));

    if (static_cast<uint8_t>(newLevel) != level)
    {
        level = static_cast<uint8_t>(newLevel);
        repaint();

        if (onLevelChanged)
            onLevelChanged(level);
    }
}

GrooveInstrumentStrip::InstrumentRow::InstrumentRow(int rowIndex)
    : row(rowIndex)
{
    rowLabel.setText(getInstrumentRowLabel(rowIndex), juce::dontSendNotification);
    rowLabel.setFont(juce::Font(8.0f));
    rowLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF666666));
    rowLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(rowLabel);

    const auto rowInstruments = getInstrumentsForRow(rowIndex);
    for (size_t i = 0; i < icons.size(); ++i)
    {
        if (rowInstruments[i] == DrumInstrument::Count)
            continue;

        icons[i] = std::make_unique<InstrumentIcon>(rowInstruments[i]);
        addAndMakeVisible(*icons[i]);
    }

    addAndMakeVisible(densitySlider);
    densitySlider.onLevelChanged = [this](uint8_t level) {
        if (onDensityChanged)
            onDensityChanged(level);
    };
}

void GrooveInstrumentStrip::InstrumentRow::resized()
{
    auto bounds = getLocalBounds();
    rowLabel.setBounds(bounds.removeFromLeft(82).reduced(0, 1));

    const int iconWidth = 36;
    const int iconGap = 3;

    for (auto& icon : icons)
    {
        if (icon != nullptr)
        {
            icon->setBounds(bounds.removeFromLeft(iconWidth).reduced(0, 1));
            bounds.removeFromLeft(iconGap);
        }
    }

    densitySlider.setBounds(bounds.reduced(2, 0));
}

void GrooveInstrumentStrip::InstrumentRow::setInteractive(bool isInteractive)
{
    rowLabel.setColour(juce::Label::textColourId, isInteractive ? juce::Colour(0xFFE8C547) : juce::Colour(0xFF666666));

    for (auto& icon : icons)
    {
        if (icon != nullptr)
            icon->setInteractive(isInteractive);
    }

    densitySlider.setInteractive(isInteractive);
}

void GrooveInstrumentStrip::InstrumentRow::setDensityLevel(uint8_t level)
{
    densitySlider.setLevel(level);
}

void GrooveInstrumentStrip::InstrumentRow::setInstrumentEnabled(DrumInstrument inst, bool enabled)
{
    const auto rowInstruments = getInstrumentsForRow(row);
    for (size_t i = 0; i < icons.size(); ++i)
    {
        if (icons[i] != nullptr && rowInstruments[i] == inst)
            icons[i]->setEnabledState(enabled);
    }
}

void GrooveInstrumentStrip::InstrumentRow::setInstrumentPresent(DrumInstrument inst, bool present)
{
    const auto rowInstruments = getInstrumentsForRow(row);
    for (size_t i = 0; i < icons.size(); ++i)
    {
        if (icons[i] != nullptr && rowInstruments[i] == inst)
            icons[i]->setPresentInGroove(present);
    }
}

GrooveInstrumentStrip::GrooveInstrumentStrip()
{
    hintLabel.setText("Select a block to add instruments", juce::dontSendNotification);
    hintLabel.setFont(juce::Font(8.0f));
    hintLabel.setColour(juce::Label::textColourId, dimTextColour);
    hintLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(hintLabel);

    for (int row = 0; row < 3; ++row)
    {
        rows[static_cast<size_t>(row)] = std::make_unique<InstrumentRow>(row);
        rows[static_cast<size_t>(row)]->onInstrumentToggled = [this](DrumInstrument inst, bool enabled) {
            handleInstrumentToggle(inst, enabled);
        };
        rows[static_cast<size_t>(row)]->onDensityChanged = [this, row](uint8_t level) {
            handleDensityChange(row, level);
        };

        const auto rowInstruments = getInstrumentsForRow(row);
        for (size_t i = 0; i < rowInstruments.size(); ++i)
        {
            if (rowInstruments[i] == DrumInstrument::Count)
                continue;

            if (auto* icon = rows[static_cast<size_t>(row)]->icons[i].get())
            {
                icon->onToggled = [this](DrumInstrument inst, bool enabled) {
                    handleInstrumentToggle(inst, enabled);
                };
            }
        }

        addAndMakeVisible(*rows[static_cast<size_t>(row)]);
    }

    resetButton.setButtonText("Reset");
    resetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF3A3A3A));
    resetButton.setColour(juce::TextButton::textColourOffId, activeAccent);
    resetButton.onClick = [this]() {
        if (grooveManager == nullptr)
            return;

        const int selectedIndex = grooveManager->getSelectedComposerItem();
        if (selectedIndex < 0)
            return;

        grooveManager->resetComposerItemSettings(selectedIndex);
        refresh();

        if (onInstrumentsChanged)
            onInstrumentsChanged();
    };
    addAndMakeVisible(resetButton);
}

void GrooveInstrumentStrip::paint(juce::Graphics& g)
{
    g.fillAll(backgroundColour);
}

void GrooveInstrumentStrip::resized()
{
    auto bounds = getLocalBounds().reduced(2, 1);
    const int resetWidth = 40;
    auto resetArea = bounds.removeFromRight(resetWidth).reduced(1, 0);
    resetButton.setBounds(resetArea.withSizeKeepingCentre(resetWidth, 18));

    const int rowHeight = bounds.getHeight() / 3;

    for (auto& row : rows)
    {
        if (row != nullptr)
            row->setBounds(bounds.removeFromTop(rowHeight));
    }

    hintLabel.setBounds(getLocalBounds().reduced(6, 0));
}

void GrooveInstrumentStrip::setGrooveManager(GrooveManager* manager)
{
    grooveManager = manager;
    refresh();
}

void GrooveInstrumentStrip::refresh()
{
    updateControls();
    repaint();
}

void GrooveInstrumentStrip::handleInstrumentToggle(DrumInstrument inst, bool enabled)
{
    if (grooveManager == nullptr)
        return;

    const int selectedIndex = grooveManager->getSelectedComposerItem();
    if (selectedIndex < 0)
        return;

    grooveManager->setComposerItemInstrumentEnabled(selectedIndex, inst, enabled);

    if (onInstrumentsChanged)
        onInstrumentsChanged();
}

void GrooveInstrumentStrip::handleDensityChange(int rowIndex, uint8_t level)
{
    if (grooveManager == nullptr)
        return;

    const int selectedIndex = grooveManager->getSelectedComposerItem();
    if (selectedIndex < 0)
        return;

    grooveManager->setComposerRowDensity(selectedIndex, rowIndex, level);

    if (onInstrumentsChanged)
        onInstrumentsChanged();
}

void GrooveInstrumentStrip::updateControls()
{
    const bool hasItems = grooveManager != nullptr && !grooveManager->getComposerItems().empty();
    const int selectedIndex = grooveManager != nullptr ? grooveManager->getSelectedComposerItem() : -1;
    const bool hasSelection = hasItems && selectedIndex >= 0;

    setVisible(hasItems);
    hintLabel.setVisible(hasItems && !hasSelection);
    resetButton.setVisible(hasItems);
    resetButton.setEnabled(hasSelection);

    for (auto& row : rows)
    {
        if (row != nullptr)
            row->setVisible(hasItems);
    }

    if (!hasItems)
        return;

    std::set<DrumInstrument> presentInstruments;

    if (hasSelection)
    {
        const auto& item = grooveManager->getComposerItems()[static_cast<size_t>(selectedIndex)];
        presentInstruments = grooveManager->getInstrumentsInGroove(item.grooveCategoryIndex, item.grooveIndex);
    }

    for (int row = 0; row < 3; ++row)
    {
        auto& rowComponent = rows[static_cast<size_t>(row)];
        if (rowComponent == nullptr)
            continue;

        rowComponent->setInteractive(hasSelection);

        if (hasSelection)
            rowComponent->setDensityLevel(grooveManager->getComposerRowDensity(selectedIndex, row));

        const auto rowInstruments = getInstrumentsForRow(row);
        for (size_t i = 0; i < rowInstruments.size(); ++i)
        {
            if (rowInstruments[i] == DrumInstrument::Count)
                continue;

            if (auto* icon = rowComponent->icons[i].get())
            {
                icon->setInteractive(hasSelection);
                icon->setPresentInGroove(!hasSelection || presentInstruments.count(rowInstruments[i]) > 0);

                if (hasSelection)
                    icon->setEnabledState(grooveManager->isComposerItemInstrumentEnabled(selectedIndex, rowInstruments[i]));
                else
                    icon->setEnabledState(false);
            }
        }
    }
}

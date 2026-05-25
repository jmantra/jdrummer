/*
    GrooveInstrumentStrip.h
    =======================
    Logic Pro-style row icons + 5-step density sliders for composer blocks.
*/

#pragma once

#include "../JuceHeader.h"
#include "../GrooveManager.h"
#include "../DrumInstrumentMap.h"
#include <array>
#include <memory>

class GrooveInstrumentStrip : public juce::Component
{
public:
    GrooveInstrumentStrip();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setGrooveManager(GrooveManager* manager);
    void refresh();

    std::function<void()> onInstrumentsChanged;

private:
    class InstrumentIcon : public juce::Component
    {
    public:
        explicit InstrumentIcon(DrumInstrument instrument);

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;

        void setEnabledState(bool enabled);
        void setPresentInGroove(bool present);
        void setInteractive(bool interactive);

        std::function<void(DrumInstrument, bool)> onToggled;

    private:
        DrumInstrument instrument;
        bool enabled = true;
        bool presentInGroove = true;
        bool interactive = false;
    };

    class DensitySlider : public juce::Component
    {
    public:
        DensitySlider();

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;

        void setLevel(uint8_t level);
        void setInteractive(bool interactive);

        std::function<void(uint8_t)> onLevelChanged;

    private:
        void setLevelFromPosition(float x);
        uint8_t level = 0;
        bool interactive = false;
    };

    class InstrumentRow : public juce::Component
    {
    public:
        InstrumentRow(int rowIndex);

        void resized() override;
        void setInteractive(bool interactive);
        void setDensityLevel(uint8_t level);
        void setInstrumentEnabled(DrumInstrument inst, bool enabled);
        void setInstrumentPresent(DrumInstrument inst, bool present);

        std::function<void(DrumInstrument, bool)> onInstrumentToggled;
        std::function<void(uint8_t)> onDensityChanged;

        std::array<std::unique_ptr<InstrumentIcon>, 3> icons;

    private:
        int row;
        juce::Label rowLabel;
        DensitySlider densitySlider;
    };

    void handleInstrumentToggle(DrumInstrument inst, bool enabled);
    void handleDensityChange(int rowIndex, uint8_t level);
    void updateControls();

    GrooveManager* grooveManager = nullptr;

    juce::Label hintLabel;
    juce::TextButton resetButton;
    std::array<std::unique_ptr<InstrumentRow>, 3> rows;

    juce::Colour backgroundColour{ 0xFF1E1E1E };
    juce::Colour dimTextColour{ 0xFF666666 };
    juce::Colour activeAccent{ 0xFFE8C547 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrooveInstrumentStrip)
};

/*
    DrumInstrumentMap.h
    ===================
    Maps GM drum MIDI notes to Logic-style instrument groups for groove filtering.
*/

#pragma once

#include "JuceHeader.h"
#include <array>
#include <set>

enum class DrumInstrument : uint8_t
{
    Kick = 0,
    Snare,
    Clap,
    HiHat,
    Shaker,
    Cymbal,
    Tom,
    Percussion,
    Count
};

constexpr uint8_t kAllInstrumentsEnabled = 0xFF;
constexpr int kDensitySteps = 5;
constexpr int kMaxDensityLevel = kDensitySteps - 1;

inline uint8_t instrumentBit(DrumInstrument inst)
{
    return static_cast<uint8_t>(1u << static_cast<uint8_t>(inst));
}

inline bool isInstrumentEnabled(DrumInstrument inst, uint8_t enabledMask)
{
    return (enabledMask & instrumentBit(inst)) != 0;
}

inline uint8_t setInstrumentEnabled(DrumInstrument inst, uint8_t enabledMask, bool enabled)
{
    if (enabled)
        return static_cast<uint8_t>(enabledMask | instrumentBit(inst));
    return static_cast<uint8_t>(enabledMask & ~instrumentBit(inst));
}

inline DrumInstrument instrumentForNote(int midiNote)
{
    switch (midiNote)
    {
        case 35: case 36:
            return DrumInstrument::Kick;
        case 37: case 38: case 40:
            return DrumInstrument::Snare;
        case 39:
            return DrumInstrument::Clap;
        case 42: case 44: case 46:
            return DrumInstrument::HiHat;
        case 54: case 70: case 73:
            return DrumInstrument::Shaker;
        case 49: case 51: case 52: case 55: case 57: case 59:
            return DrumInstrument::Cymbal;
        case 41: case 43: case 45: case 47: case 48: case 50:
            return DrumInstrument::Tom;
        default:
            if (midiNote >= 56 && midiNote <= 81)
                return DrumInstrument::Percussion;
            return DrumInstrument::Percussion;
    }
}

inline bool isNoteEnabled(int note, uint8_t enabledMask)
{
    return isInstrumentEnabled(instrumentForNote(note), enabledMask);
}

inline juce::String getInstrumentLabel(DrumInstrument inst)
{
    switch (inst)
    {
        case DrumInstrument::Kick:       return "Kick";
        case DrumInstrument::Snare:      return "Snare";
        case DrumInstrument::Clap:       return "Clap";
        case DrumInstrument::HiHat:      return "HH";
        case DrumInstrument::Shaker:     return "Shaker";
        case DrumInstrument::Cymbal:     return "Cym";
        case DrumInstrument::Tom:        return "Tom";
        case DrumInstrument::Percussion: return "Perc";
        default:                         return {};
    }
}

inline juce::String getInstrumentRowLabel(int rowIndex)
{
    switch (rowIndex)
    {
        case 0: return "Kick/Snare/Tom";
        case 1: return "Cym/Hi-Hat";
        case 2: return "Clap/Perc";
        default: return {};
    }
}

inline int getRowForInstrument(DrumInstrument inst)
{
    switch (inst)
    {
        case DrumInstrument::Kick:
        case DrumInstrument::Snare:
        case DrumInstrument::Tom:
            return 0;
        case DrumInstrument::HiHat:
        case DrumInstrument::Shaker:
        case DrumInstrument::Cymbal:
            return 1;
        case DrumInstrument::Clap:
        case DrumInstrument::Percussion:
            return 2;
        default:
            return 2;
    }
}

inline int getDefaultNoteForInstrument(DrumInstrument inst)
{
    switch (inst)
    {
        case DrumInstrument::Kick:       return 36;
        case DrumInstrument::Snare:      return 38;
        case DrumInstrument::Clap:       return 39;
        case DrumInstrument::HiHat:      return 42;
        case DrumInstrument::Shaker:     return 54;
        case DrumInstrument::Cymbal:     return 49;
        case DrumInstrument::Tom:        return 45;
        case DrumInstrument::Percussion: return 56;
        default:                         return 36;
    }
}

inline juce::String getInstrumentRowLabelLong(int rowIndex)
{
    switch (rowIndex)
    {
        case 0: return "Kick, Snare & Claps";
        case 1: return "Cymbals, Shaker & Hi-Hat";
        case 2: return "Percussion";
        default: return {};
    }
}

inline juce::Colour getInstrumentColour(DrumInstrument inst)
{
    switch (inst)
    {
        case DrumInstrument::Kick:       return juce::Colour(0xFFFF6B6B);
        case DrumInstrument::Snare:      return juce::Colour(0xFF4ECDC4);
        case DrumInstrument::Clap:       return juce::Colour(0xFF74B9FF);
        case DrumInstrument::HiHat:      return juce::Colour(0xFFFFE66D);
        case DrumInstrument::Shaker:     return juce::Colour(0xFF55EFC4);
        case DrumInstrument::Cymbal:     return juce::Colour(0xFFFF9F43);
        case DrumInstrument::Tom:        return juce::Colour(0xFFA29BFE);
        case DrumInstrument::Percussion: return juce::Colour(0xFFDFE6E9);
        default:                         return juce::Colours::grey;
    }
}

inline juce::Colour getNoteColour(int note)
{
    return getInstrumentColour(instrumentForNote(note));
}

inline std::array<DrumInstrument, 3> getInstrumentsForRow(int rowIndex)
{
    switch (rowIndex)
    {
        case 0:  return { DrumInstrument::Kick, DrumInstrument::Snare, DrumInstrument::Tom };
        case 1:  return { DrumInstrument::HiHat, DrumInstrument::Shaker, DrumInstrument::Cymbal };
        default: return { DrumInstrument::Clap, DrumInstrument::Percussion, DrumInstrument::Count };
    }
}

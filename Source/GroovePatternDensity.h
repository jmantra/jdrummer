/*
    GroovePatternDensity.h
    ======================
    Builds thinned/generated effective MIDI events for composer blocks.
*/

#pragma once

#include "GrooveManager.h"
#include "DrumInstrumentMap.h"
#include <array>

class GroovePatternDensity
{
public:
    static std::vector<Groove::MidiEvent> buildEffectiveEvents(
        const Groove& groove,
        double itemLengthInBeats,
        uint8_t enabledInstruments,
        const std::array<uint8_t, 3>& rowDensity);
};

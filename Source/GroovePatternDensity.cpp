/*
    GroovePatternDensity.cpp
    ========================
*/

#include "GroovePatternDensity.h"
#include <algorithm>
#include <cmath>

namespace
{
    struct ScoredNote
    {
        double timeInBeats = 0.0;
        int noteNumber = 0;
        float velocity = 0.0f;
        bool isGenerated = false;
    };

    constexpr double kGeneratedNoteOffDelayBeats = 0.05;

    constexpr double kMergeToleranceBeats = 0.04;
    constexpr float kGeneratedVelocity = 0.7f;

    int addedHitsPerBar(DrumInstrument inst, uint8_t densityLevel)
    {
        const int level = juce::jlimit(0, kMaxDensityLevel, static_cast<int>(densityLevel));

        switch (inst)
        {
            case DrumInstrument::Kick:
                return std::array<int, kDensitySteps>{ 0, 1, 2, 2, 3 }[static_cast<size_t>(level)];
            case DrumInstrument::Snare:
                return std::array<int, kDensitySteps>{ 0, 1, 2, 2, 2 }[static_cast<size_t>(level)];
            case DrumInstrument::Clap:
                return std::array<int, kDensitySteps>{ 0, 1, 2, 2, 2 }[static_cast<size_t>(level)];
            case DrumInstrument::HiHat:
                return std::array<int, kDensitySteps>{ 0, 2, 4, 8, 8 }[static_cast<size_t>(level)];
            case DrumInstrument::Shaker:
                return std::array<int, kDensitySteps>{ 0, 1, 2, 4, 6 }[static_cast<size_t>(level)];
            case DrumInstrument::Cymbal:
                return std::array<int, kDensitySteps>{ 0, 1, 1, 2, 2 }[static_cast<size_t>(level)];
            case DrumInstrument::Tom:
                return std::array<int, kDensitySteps>{ 0, 1, 1, 1, 2 }[static_cast<size_t>(level)];
            case DrumInstrument::Percussion:
                return std::array<int, kDensitySteps>{ 0, 1, 1, 2, 3 }[static_cast<size_t>(level)];
            default:
                return 0;
        }
    }

    bool isNearExisting(double timeInBeats, const std::vector<ScoredNote>& notes)
    {
        for (const auto& note : notes)
        {
            if (std::abs(note.timeInBeats - timeInBeats) < kMergeToleranceBeats)
                return true;
        }
        return false;
    }

    std::vector<double> templatePositionsForBar(DrumInstrument inst, uint8_t densityLevel, int numerator)
    {
        std::vector<double> positions;
        if (addedHitsPerBar(inst, densityLevel) <= 0)
            return positions;

        switch (inst)
        {
            case DrumInstrument::Kick:
            {
                if (densityLevel >= 1) positions.push_back(0.0);
                if (densityLevel >= 2) positions.push_back(2.0);
                if (densityLevel >= 4) { positions.push_back(1.0); positions.push_back(3.0); }
                break;
            }
            case DrumInstrument::Snare:
            {
                if (densityLevel >= 1) positions.push_back(1.0);
                if (densityLevel >= 2) positions.push_back(3.0);
                break;
            }
            case DrumInstrument::Clap:
            {
                if (densityLevel >= 1) positions.push_back(1.0);
                if (densityLevel >= 2) positions.push_back(3.0);
                if (densityLevel >= 4) { positions.push_back(0.5); positions.push_back(2.5); }
                break;
            }
            case DrumInstrument::HiHat:
            {
                const double step = (densityLevel >= 3) ? 0.5 : 1.0;
                for (double t = 0.0; t < static_cast<double>(numerator); t += step)
                    positions.push_back(t);
                break;
            }
            case DrumInstrument::Shaker:
            {
                const double step = (densityLevel >= 4) ? 0.5 : (densityLevel >= 3) ? 1.0 : 2.0;
                for (double t = 0.0; t < static_cast<double>(numerator); t += step)
                    positions.push_back(t);
                break;
            }
            case DrumInstrument::Cymbal:
            {
                if (densityLevel >= 1) positions.push_back(0.0);
                if (densityLevel >= 3) positions.push_back(2.0);
                if (densityLevel >= 4) { positions.push_back(1.0); positions.push_back(3.0); }
                break;
            }
            case DrumInstrument::Tom:
            {
                if (densityLevel >= 1) positions.push_back(2.0);
                if (densityLevel >= 3) positions.push_back(1.0);
                if (densityLevel >= 4) { positions.push_back(0.0); positions.push_back(3.0); }
                break;
            }
            case DrumInstrument::Percussion:
            {
                if (densityLevel >= 1) positions.push_back(0.0);
                if (densityLevel >= 3) positions.push_back(2.0);
                if (densityLevel >= 4) { positions.push_back(1.5); positions.push_back(3.0); }
                break;
            }
            default:
                break;
        }

        return positions;
    }

    std::vector<Groove::MidiEvent> copyOriginalEvents(const Groove& groove, double lengthInBeats)
    {
        std::vector<Groove::MidiEvent> result;

        for (const auto& evt : groove.events)
        {
            if (evt.timeInBeats >= lengthInBeats)
                continue;

            if (evt.message.isNoteOnOrOff())
                result.push_back(evt);
        }

        return result;
    }

    std::vector<ScoredNote> collectOriginalNotes(
        DrumInstrument inst,
        const Groove& groove,
        double itemLengthInBeats)
    {
        std::vector<ScoredNote> notes;

        for (const auto& evt : groove.events)
        {
            if (!evt.message.isNoteOn())
                continue;
            if (evt.timeInBeats >= itemLengthInBeats)
                continue;
            if (instrumentForNote(evt.message.getNoteNumber()) != inst)
                continue;

            ScoredNote note;
            note.timeInBeats = evt.timeInBeats;
            note.noteNumber = evt.message.getNoteNumber();
            note.velocity = evt.message.getFloatVelocity();
            notes.push_back(note);
        }

        return notes;
    }

    std::vector<ScoredNote> buildAddedInstrumentNotes(
        DrumInstrument inst,
        uint8_t densityLevel,
        const Groove& groove,
        double itemLengthInBeats)
    {
        if (densityLevel == 0)
            return {};

        std::vector<ScoredNote> notes;
        const int defaultNote = getDefaultNoteForInstrument(inst);
        const double beatsPerBar = static_cast<double>(groove.numerator);
        const int numBars = std::max(1, static_cast<int>(std::ceil(itemLengthInBeats / beatsPerBar)));
        const auto originalNotes = collectOriginalNotes(inst, groove, itemLengthInBeats);
        const int targetAdded = addedHitsPerBar(inst, densityLevel) * numBars;
        const auto barTemplate = templatePositionsForBar(inst, densityLevel, groove.numerator);

        for (int bar = 0; bar < numBars; ++bar)
        {
            for (double posInBar : barTemplate)
            {
                if (static_cast<int>(notes.size()) >= targetAdded)
                    break;

                const double timeInBeats = bar * beatsPerBar + posInBar;
                if (timeInBeats >= itemLengthInBeats)
                    continue;
                if (isNearExisting(timeInBeats, originalNotes))
                    continue;
                if (isNearExisting(timeInBeats, notes))
                    continue;

                ScoredNote generated;
                generated.timeInBeats = timeInBeats;
                generated.noteNumber = defaultNote;
                generated.velocity = kGeneratedVelocity;
                generated.isGenerated = true;
                notes.push_back(generated);
            }
        }

        std::sort(notes.begin(), notes.end(),
                  [](const ScoredNote& a, const ScoredNote& b) { return a.timeInBeats < b.timeInBeats; });

        if (static_cast<int>(notes.size()) > targetAdded)
            notes.resize(static_cast<size_t>(targetAdded));

        return notes;
    }
}

std::vector<Groove::MidiEvent> GroovePatternDensity::buildEffectiveEvents(
    const Groove& groove,
    double itemLengthInBeats,
    uint8_t enabledInstruments,
    const std::array<uint8_t, 3>& rowDensity)
{
    std::vector<Groove::MidiEvent> result = copyOriginalEvents(groove, itemLengthInBeats);

    for (int i = 0; i < static_cast<int>(DrumInstrument::Count); ++i)
    {
        const auto inst = static_cast<DrumInstrument>(i);
        if (!isInstrumentEnabled(inst, enabledInstruments))
            continue;

        const int row = getRowForInstrument(inst);
        const uint8_t density = rowDensity[static_cast<size_t>(row)];
        if (density == 0)
            continue;

        const auto notes = buildAddedInstrumentNotes(inst, density, groove, itemLengthInBeats);

        for (const auto& note : notes)
        {
            Groove::MidiEvent onEvent;
            onEvent.timeInBeats = note.timeInBeats;
            onEvent.message = juce::MidiMessage::noteOn(1, note.noteNumber, note.velocity);
            result.push_back(onEvent);

            Groove::MidiEvent offEvent;
            offEvent.timeInBeats = note.timeInBeats + kGeneratedNoteOffDelayBeats;
            offEvent.message = juce::MidiMessage::noteOff(1, note.noteNumber);
            result.push_back(offEvent);
        }
    }

    std::sort(result.begin(), result.end(),
              [](const Groove::MidiEvent& a, const Groove::MidiEvent& b) {
                  return a.timeInBeats < b.timeInBeats;
              });

    return result;
}

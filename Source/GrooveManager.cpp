/*
    GrooveManager.cpp
    =================
    
    Implementation of the groove loading and playback system.
*/

#include "GrooveManager.h"
#include "GroovePatternDensity.h"

namespace
{
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
}

GrooveManager::GrooveManager()
{
    // Create a persistent directory for exported MIDI files
    // On Linux, use /tmp for better compatibility with Flatpak sandboxed DAWs
    #if JUCE_LINUX
    tempDir = juce::File("/tmp/jdrummer_exports");
    #else
    tempDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                  .getChildFile("JDrummer_Exports");
    #endif
    tempDir.createDirectory();
    
    // Clean up old export files on startup (files older than 1 hour)
    cleanupOldExports();
}

GrooveManager::~GrooveManager()
{
    // Don't delete the export directory on shutdown - DAWs like Bitwig
    // may still be reading the files asynchronously after the drag operation
}

void GrooveManager::setGroovesPath(const juce::File& path)
{
    juce::ScopedLock sl(lock);
    groovesPath = path;
}

void GrooveManager::scanGrooves()
{
    juce::ScopedLock sl(lock);
    categories.clear();
    
    if (!groovesPath.exists() || !groovesPath.isDirectory())
    {
        DBG("GrooveManager: Grooves path does not exist: " + groovesPath.getFullPathName());
        return;
    }
    
    DBG("GrooveManager: Scanning grooves in " + groovesPath.getFullPathName());
    
    // Find all subdirectories (categories)
    auto subDirs = groovesPath.findChildFiles(juce::File::findDirectories, false);
    subDirs.sort();
    
    for (const auto& dir : subDirs)
    {
        GrooveCategory category;
        category.name = dir.getFileName();
        
        // Find all MIDI files in this category
        auto midiFiles = dir.findChildFiles(juce::File::findFiles, false, "*.mid");
        midiFiles.sort();
        
        for (const auto& midiFile : midiFiles)
        {
            Groove groove;
            groove.name = midiFile.getFileNameWithoutExtension();
            groove.category = category.name;
            groove.file = midiFile;
            groove.isLoaded = false;
            
            category.grooves.push_back(groove);
        }
        
        if (!category.grooves.empty())
        {
            categories.push_back(category);
            DBG("GrooveManager: Found category '" + category.name + "' with " 
                + juce::String(category.grooves.size()) + " grooves");
        }
    }
    
    DBG("GrooveManager: Scan complete. Found " + juce::String(categories.size()) + " categories");
}

bool GrooveManager::loadGroove(int categoryIndex, int grooveIndex)
{
    juce::ScopedLock sl(lock);
    
    if (categoryIndex < 0 || categoryIndex >= static_cast<int>(categories.size()))
        return false;
    
    auto& category = categories[categoryIndex];
    if (grooveIndex < 0 || grooveIndex >= static_cast<int>(category.grooves.size()))
        return false;
    
    auto& groove = category.grooves[grooveIndex];
    
    if (groove.isLoaded)
        return true;  // Already loaded
    
    return parseMidiFile(groove);
}

bool GrooveManager::parseMidiFile(Groove& groove)
{
    if (!groove.file.existsAsFile())
    {
        DBG("GrooveManager: MIDI file not found: " + groove.file.getFullPathName());
        return false;
    }
    
    juce::FileInputStream fileStream(groove.file);
    if (!fileStream.openedOk())
    {
        DBG("GrooveManager: Failed to open MIDI file: " + groove.file.getFullPathName());
        return false;
    }
    
    juce::MidiFile midiFile;
    if (!midiFile.readFrom(fileStream))
    {
        DBG("GrooveManager: Failed to parse MIDI file: " + groove.file.getFullPathName());
        return false;
    }
    
    // Convert to seconds-based timing for easier processing
    midiFile.convertTimestampTicksToSeconds();
    
    groove.events.clear();
    
    // Get time signature if available (default to 4/4)
    groove.numerator = 4;
    groove.denominator = 4;
    
    // Get tempo (default to 120 BPM)
    double tempoBpm = 120.0;
    
    // Process all tracks
    for (int trackIdx = 0; trackIdx < midiFile.getNumTracks(); ++trackIdx)
    {
        const auto* track = midiFile.getTrack(trackIdx);
        if (track == nullptr)
            continue;
        
        for (int eventIdx = 0; eventIdx < track->getNumEvents(); ++eventIdx)
        {
            auto midiEvent = track->getEventPointer(eventIdx);
            auto& message = midiEvent->message;
            
            // Check for tempo changes
            if (message.isTempoMetaEvent())
            {
                tempoBpm = 60000000.0 / message.getTempoSecondsPerQuarterNote() / 1000000.0;
            }
            // Check for time signature
            else if (message.isTimeSignatureMetaEvent())
            {
                int num, denom;
                message.getTimeSignatureInfo(num, denom);
                groove.numerator = num;
                groove.denominator = denom;
            }
            // Store note on/off events
            else if (message.isNoteOnOrOff())
            {
                // Convert time from seconds to beats
                // timeInSeconds * (BPM / 60) = timeInBeats
                double timeInBeats = message.getTimeStamp() * (tempoBpm / 60.0);
                
                Groove::MidiEvent evt;
                evt.timeInBeats = timeInBeats;
                evt.message = message;
                groove.events.push_back(evt);
            }
        }
    }
    
    // Sort events by time
    std::sort(groove.events.begin(), groove.events.end(),
              [](const Groove::MidiEvent& a, const Groove::MidiEvent& b) {
                  return a.timeInBeats < b.timeInBeats;
              });
    
    // Calculate groove length
    groove.lengthInBeats = calculateGrooveLength(groove);
    
    groove.isLoaded = true;
    
    DBG("GrooveManager: Loaded groove '" + groove.name + "' with " 
        + juce::String(groove.events.size()) + " events, length: " 
        + juce::String(groove.lengthInBeats) + " beats");
    
    return true;
}

double GrooveManager::calculateGrooveLength(const Groove& groove)
{
    if (groove.events.empty())
        return 4.0;  // Default to 1 bar in 4/4
    
    // Find the last event time
    double maxTime = 0.0;
    for (const auto& evt : groove.events)
    {
        maxTime = std::max(maxTime, evt.timeInBeats);
    }
    
    // Round up to the nearest bar
    double beatsPerBar = static_cast<double>(groove.numerator);
    double bars = std::ceil(maxTime / beatsPerBar);
    
    // Minimum 1 bar
    if (bars < 1.0)
        bars = 1.0;
    
    return bars * beatsPerBar;
}

Groove* GrooveManager::getGroove(int categoryIndex, int grooveIndex)
{
    juce::ScopedLock sl(lock);
    
    if (categoryIndex < 0 || categoryIndex >= static_cast<int>(categories.size()))
        return nullptr;
    
    auto& category = categories[categoryIndex];
    if (grooveIndex < 0 || grooveIndex >= static_cast<int>(category.grooves.size()))
        return nullptr;
    
    return &category.grooves[grooveIndex];
}

const Groove* GrooveManager::getGroove(int categoryIndex, int grooveIndex) const
{
    if (categoryIndex < 0 || categoryIndex >= static_cast<int>(categories.size()))
        return nullptr;
    
    const auto& category = categories[categoryIndex];
    if (grooveIndex < 0 || grooveIndex >= static_cast<int>(category.grooves.size()))
        return nullptr;
    
    return &category.grooves[grooveIndex];
}

void GrooveManager::startPlayback(int categoryIndex, int grooveIndex)
{
    juce::ScopedLock sl(lock);
    
    // Make sure groove is loaded
    if (!loadGroove(categoryIndex, grooveIndex))
        return;
    
    currentCategoryIndex = categoryIndex;
    currentGrooveIndex = grooveIndex;
    playing = true;
    playbackStartPpq = -1.0;  // Will be set on first processBlock
    lastProcessedPpq = -1.0;
    internalPositionBeats = 0.0;  // Reset internal clock
    
    DBG("GrooveManager: Started playback of groove " + juce::String(grooveIndex) 
        + " in category " + juce::String(categoryIndex));
}

void GrooveManager::stopPlayback()
{
    juce::ScopedLock sl(lock);
    playing = false;
    currentCategoryIndex = -1;
    currentGrooveIndex = -1;
    
    DBG("GrooveManager: Stopped playback");
}

void GrooveManager::processBlock(double bpm, double ppqPosition, bool hostIsPlaying,
                                  int numSamples, std::vector<juce::MidiMessage>& midiOut)
{
    juce::ScopedLock sl(lock);
    
    // Determine if we should use internal timing or DAW timing
    // Use internal timing if:
    // 1. Host is not playing, OR
    // 2. We're in standalone preview mode (useInternalClock is set)
    bool useInternal = !hostIsPlaying || useInternalClock;
    
    // Use internal BPM when in preview mode, otherwise use DAW BPM
    double effectiveBpm;
    if (useInternalClock)
    {
        // Preview mode - always use the internal BPM (set by Groove Matcher)
        effectiveBpm = internalBpm;
    }
    else
    {
        // Normal mode - use DAW BPM if available
        effectiveBpm = (bpm > 0) ? bpm : internalBpm;
    }
    
    // Calculate how many beats this block represents
    double beatsPerSecond = effectiveBpm / 60.0;
    double secondsPerBlock = static_cast<double>(numSamples) / currentSampleRate;
    double beatsThisBlock = beatsPerSecond * secondsPerBlock;
    
    double currentPosition;
    double previousPosition;
    
    if (useInternal && (playing || composerPlaying))
    {
        // Use internal clock - advance by calculated beats
        previousPosition = internalPositionBeats;
        internalPositionBeats += beatsThisBlock;
        currentPosition = internalPositionBeats;
    }
    else
    {
        // Use DAW position
        currentPosition = ppqPosition;
        previousPosition = lastProcessedPpq;
    }
    
    // Handle single groove playback
    if (playing && !composerPlaying)
    {
        const Groove* groove = getGroove(currentCategoryIndex, currentGrooveIndex);
        if (groove == nullptr || !groove->isLoaded)
        {
            playing = false;
            return;
        }
        
        // Initialize playback position on first block
        if (playbackStartPpq < 0.0)
        {
            playbackStartPpq = currentPosition;
            previousPosition = currentPosition;
            internalPositionBeats = 0.0;
        }
        
        // Calculate position within the groove (with looping)
        double groovePosition = currentPosition - playbackStartPpq;
        
        // For internal clock, use direct position
        if (useInternal)
        {
            groovePosition = internalPositionBeats;
        }
        
        if (looping)
        {
            // Wrap around for looping
            while (groovePosition >= groove->lengthInBeats)
            {
                groovePosition -= groove->lengthInBeats;
                if (!useInternal)
                    playbackStartPpq += groove->lengthInBeats;
                else
                    internalPositionBeats -= groove->lengthInBeats;
            }
        }
        else if (groovePosition >= groove->lengthInBeats)
        {
            // Stop at end if not looping
            playing = false;
            return;
        }
        
        // Find events that should trigger in this block
        double lastPosition;
        if (useInternal)
        {
            lastPosition = groovePosition - beatsThisBlock;
            if (lastPosition < 0 && looping)
                lastPosition += groove->lengthInBeats;
            if (lastPosition < 0)
                lastPosition = 0;
        }
        else
        {
            lastPosition = previousPosition - playbackStartPpq;
            if (looping)
            {
                while (lastPosition >= groove->lengthInBeats)
                    lastPosition -= groove->lengthInBeats;
                if (lastPosition < 0)
                    lastPosition = 0;
            }
        }
        
        for (const auto& evt : groove->events)
        {
            // Check if this event falls within the current block
            bool shouldTrigger = false;
            
            if (groovePosition >= lastPosition)
            {
                // Normal case: no loop wrap in this block
                shouldTrigger = (evt.timeInBeats > lastPosition && evt.timeInBeats <= groovePosition);
            }
            else
            {
                // Loop wrapped: check both ends
                shouldTrigger = (evt.timeInBeats > lastPosition || evt.timeInBeats <= groovePosition);
            }
            
            if (shouldTrigger)
            {
                midiOut.push_back(evt.message);
            }
        }
        
        lastProcessedPpq = currentPosition;
    }
    
    // Handle composer playback
    if (composerPlaying)
    {
        if (composerStartPpq < 0.0)
        {
            composerStartPpq = currentPosition;
            previousPosition = currentPosition;
            internalPositionBeats = 0.0;
        }
        
        double composerPosition;
        if (useInternal)
        {
            composerPosition = internalPositionBeats;
        }
        else
        {
            composerPosition = currentPosition - composerStartPpq;
        }
        
        double composerLength = getComposerLengthInBeats();
        
        if (looping)
        {
            while (composerPosition >= composerLength && composerLength > 0)
            {
                composerPosition -= composerLength;
                if (!useInternal)
                    composerStartPpq += composerLength;
                else
                    internalPositionBeats -= composerLength;
            }
        }
        else if (composerPosition >= composerLength)
        {
            composerPlaying = false;
            return;
        }
        
        // Calculate last position for this block
        double lastPositionInComposer;
        if (useInternal)
        {
            lastPositionInComposer = composerPosition - beatsThisBlock;
            if (lastPositionInComposer < 0 && looping && composerLength > 0)
                lastPositionInComposer += composerLength;
            if (lastPositionInComposer < 0)
                lastPositionInComposer = 0;
        }
        else
        {
            lastPositionInComposer = previousPosition - composerStartPpq;
            if (looping && lastPositionInComposer >= composerLength && composerLength > 0)
                lastPositionInComposer = 0;
        }
        
        // Process each composer item
        for (size_t itemIndex = 0; itemIndex < composerItems.size(); ++itemIndex)
        {
            const auto& item = composerItems[itemIndex];
            const Groove* groove = getGroove(item.grooveCategoryIndex, item.grooveIndex);
            if (groove == nullptr || !groove->isLoaded)
                continue;
            
            // Check if current position is within this item
            if (composerPosition >= item.startBeat && 
                composerPosition < item.startBeat + item.lengthInBeats)
            {
                double positionInGroove = composerPosition - item.startBeat;
                double lastPosInGroove = lastPositionInComposer - item.startBeat;
                
                if (lastPosInGroove < 0)
                    lastPosInGroove = 0;
                
                const auto& events = getEffectiveEventsForItemUnlocked(static_cast<int>(itemIndex));

                for (const auto& evt : events)
                {
                    if (evt.timeInBeats >= item.lengthInBeats)
                        continue;

                    bool shouldTrigger = false;

                    if (positionInGroove >= lastPosInGroove)
                    {
                        shouldTrigger = (evt.timeInBeats > lastPosInGroove &&
                                         evt.timeInBeats <= positionInGroove);
                    }
                    else
                    {
                        // Loop wrapped within this item
                        shouldTrigger = (evt.timeInBeats > lastPosInGroove ||
                                         evt.timeInBeats <= positionInGroove);
                    }

                    if (shouldTrigger)
                        midiOut.push_back(evt.message);
                }
            }
        }
        
        lastProcessedPpq = currentPosition;
    }
}

// Composer functions
void GrooveManager::addToComposer(int categoryIndex, int grooveIndex, int barCount)
{
    juce::ScopedLock sl(lock);
    
    // Load the groove if not already loaded
    if (!loadGroove(categoryIndex, grooveIndex))
        return;
    
    const Groove* groove = getGroove(categoryIndex, grooveIndex);
    if (groove == nullptr)
        return;
    
    ComposerItem item;
    item.grooveCategoryIndex = categoryIndex;
    item.grooveIndex = grooveIndex;
    item.startBeat = getComposerLengthInBeats();  // Add at the end
    
    // Calculate length based on bar count
    // barCount of 0 means use the full groove length
    if (barCount <= 0)
    {
        item.lengthInBeats = groove->lengthInBeats;
    }
    else
    {
        // Calculate beats per bar based on time signature
        double beatsPerBar = static_cast<double>(groove->numerator);
        item.lengthInBeats = barCount * beatsPerBar;
        
        // Don't exceed the original groove length
        if (item.lengthInBeats > groove->lengthInBeats)
            item.lengthInBeats = groove->lengthInBeats;
    }
    
    composerItems.push_back(item);
    selectedComposerItemIndex = static_cast<int>(composerItems.size()) - 1;
    rebuildEffectiveEventsNow(selectedComposerItemIndex);
    
    DBG("GrooveManager: Added " + juce::String(barCount) + " bars of groove to composer. "
        + "Length: " + juce::String(item.lengthInBeats) + " beats. "
        + "Total items: " + juce::String(composerItems.size()));
}

void GrooveManager::removeFromComposer(int index)
{
    juce::ScopedLock sl(lock);
    
    if (index < 0 || index >= static_cast<int>(composerItems.size()))
        return;
    
    double removedLength = composerItems[index].lengthInBeats;
    composerItems.erase(composerItems.begin() + index);
    
    if (selectedComposerItemIndex == index)
        selectedComposerItemIndex = -1;
    else if (selectedComposerItemIndex > index)
        --selectedComposerItemIndex;
    
    // Adjust start times for items after the removed one
    for (size_t i = index; i < composerItems.size(); ++i)
    {
        composerItems[i].startBeat -= removedLength;
    }
}

void GrooveManager::clearComposer()
{
    juce::ScopedLock sl(lock);
    composerItems.clear();
    composerPlaying = false;
    selectedComposerItemIndex = -1;
}

void GrooveManager::moveComposerItem(int fromIndex, int toIndex)
{
    juce::ScopedLock sl(lock);
    
    if (fromIndex < 0 || fromIndex >= static_cast<int>(composerItems.size()))
        return;
    if (toIndex < 0 || toIndex >= static_cast<int>(composerItems.size()))
        return;
    if (fromIndex == toIndex)
        return;
    
    ComposerItem item = composerItems[fromIndex];
    composerItems.erase(composerItems.begin() + fromIndex);
    composerItems.insert(composerItems.begin() + toIndex, item);
    
    // Recalculate all start times
    double currentBeat = 0.0;
    for (auto& ci : composerItems)
    {
        ci.startBeat = currentBeat;
        currentBeat += ci.lengthInBeats;
    }
}

double GrooveManager::getComposerLengthInBeats() const
{
    double length = 0.0;
    for (const auto& item : composerItems)
    {
        length += item.lengthInBeats;
    }
    return length;
}

void GrooveManager::startComposerPlayback()
{
    juce::ScopedLock sl(lock);
    
    if (composerItems.empty())
        return;
    
    // Make sure all grooves are loaded and effective events are built
    for (size_t i = 0; i < composerItems.size(); ++i)
    {
        loadGroove(composerItems[i].grooveCategoryIndex, composerItems[i].grooveIndex);

        if (composerItems[i].instrumentSettingsModified)
            rebuildEffectiveEventsNow(static_cast<int>(i));
    }
    
    composerPlaying = true;
    playing = false;  // Stop single groove playback
    composerStartPpq = -1.0;
    lastProcessedPpq = -1.0;
    internalPositionBeats = 0.0;  // Reset internal clock
    
    DBG("GrooveManager: Started composer playback");
}

void GrooveManager::stopComposerPlayback()
{
    juce::ScopedLock sl(lock);
    composerPlaying = false;
    
    DBG("GrooveManager: Stopped composer playback");
}

juce::File GrooveManager::exportGrooveToTempFile(int categoryIndex, int grooveIndex)
{
    juce::ScopedLock sl(lock);
    
    const Groove* groove = getGroove(categoryIndex, grooveIndex);
    if (groove == nullptr || !groove->file.existsAsFile())
        return juce::File();
    
    // Copy the groove file to temp directory for DAW access
    // (Original file may be inside VST3 bundle with restricted access)
    if (!tempDir.exists())
    {
        tempDir.createDirectory();
    }
    
    // Create a unique filename based on category and groove name
    juce::String safeName = groove->name.replaceCharacters(" /\\:*?\"<>|", "_________");
    juce::File destFile = tempDir.getChildFile(safeName + ".mid");
    
    // Copy the original MIDI file to temp location
    if (groove->file.copyFileTo(destFile))
    {
        DBG("GrooveManager: Copied groove to: " + destFile.getFullPathName());
        return destFile;
    }
    else
    {
        DBG("GrooveManager: Failed to copy groove file");
        return groove->file;  // Fallback to original
    }
}

juce::MidiFile GrooveManager::buildCompositionMidiFile() const
{
    juce::MidiFile midiFile;

    if (composerItems.empty())
        return midiFile;

    double totalLengthInBeats = 0.0;
    for (const auto& item : composerItems)
        totalLengthInBeats += item.lengthInBeats;

    midiFile.setTicksPerQuarterNote(480);

    juce::MidiMessageSequence sequence;

    juce::MidiMessage trackName = juce::MidiMessage::textMetaEvent(3, "JDrummer Composition");
    trackName.setTimeStamp(0);
    sequence.addEvent(trackName);

    auto tempoEvent = juce::MidiMessage::tempoMetaEvent(500000);
    tempoEvent.setTimeStamp(0);
    sequence.addEvent(tempoEvent);

    auto timeSigEvent = juce::MidiMessage::timeSignatureMetaEvent(4, 4);
    timeSigEvent.setTimeStamp(0);
    sequence.addEvent(timeSigEvent);

    for (size_t itemIndex = 0; itemIndex < composerItems.size(); ++itemIndex)
    {
        const auto& item = composerItems[itemIndex];
        const auto& events = getEffectiveEventsForItemUnlocked(static_cast<int>(itemIndex));

        for (const auto& evt : events)
        {
            if (evt.timeInBeats < item.lengthInBeats)
            {
                juce::MidiMessage msg = evt.message;
                double ticks = (item.startBeat + evt.timeInBeats) * 480.0;
                msg.setTimeStamp(ticks);
                sequence.addEvent(msg);
            }
        }
    }

    sequence.updateMatchedPairs();
    sequence.sort();

    auto endOfTrack = juce::MidiMessage::endOfTrack();
    endOfTrack.setTimeStamp(totalLengthInBeats * 480.0);
    sequence.addEvent(endOfTrack);

    midiFile.addTrack(sequence);
    return midiFile;
}

bool GrooveManager::writeMidiFile(const juce::MidiFile& midiFile, const juce::File& destination) const
{
    if (destination.existsAsFile())
        destination.deleteFile();

    juce::FileOutputStream stream(destination);
    if (!stream.openedOk())
    {
        DBG("GrooveManager: Failed to open output file");
        return false;
    }

    const bool writeSuccess = midiFile.writeTo(stream);
    stream.flush();

    if (!writeSuccess)
    {
        DBG("GrooveManager: Failed to write MIDI data");
        return false;
    }

    if (destination.existsAsFile() && destination.getSize() > 0)
    {
        DBG("GrooveManager: Exported composition to " + destination.getFullPathName()
            + " (" + juce::String(destination.getSize()) + " bytes)");
        return true;
    }

    DBG("GrooveManager: Export verification failed");
    return false;
}

bool GrooveManager::exportCompositionToFile(const juce::File& destination)
{
    juce::ScopedLock sl(lock);

    if (composerItems.empty() || !destination.getFullPathName().isNotEmpty())
        return false;

    return writeMidiFile(buildCompositionMidiFile(), destination);
}

juce::File GrooveManager::exportCompositionToTempFile()
{
    juce::ScopedLock sl(lock);

    if (composerItems.empty())
        return juce::File();

    juce::File outFile = tempDir.getChildFile("jdrummer_composition_"
        + juce::String(juce::Time::currentTimeMillis()) + ".mid");

    if (!writeMidiFile(buildCompositionMidiFile(), outFile))
        return juce::File();

    return outFile;
}

void GrooveManager::setSelectedComposerItem(int index)
{
    juce::ScopedLock sl(lock);

    if (index < -1 || index >= static_cast<int>(composerItems.size()))
        return;

    selectedComposerItemIndex = index;
}

void GrooveManager::resetComposerItemSettings(int itemIndex)
{
    juce::ScopedLock sl(lock);

    if (itemIndex < 0 || itemIndex >= static_cast<int>(composerItems.size()))
        return;

    auto& item = composerItems[static_cast<size_t>(itemIndex)];
    item.instrumentSettingsModified = false;
    item.enabledInstruments = 0;
    item.rowDensity = { 0, 0, 0 };
    rebuildEffectiveEventsNow(itemIndex);
}

void GrooveManager::setComposerItemInstrumentEnabled(int itemIndex, DrumInstrument inst, bool enabled)
{
    juce::ScopedLock sl(lock);

    if (itemIndex < 0 || itemIndex >= static_cast<int>(composerItems.size()))
        return;

    auto& item = composerItems[static_cast<size_t>(itemIndex)];
    item.instrumentSettingsModified = true;
    item.enabledInstruments = setInstrumentEnabled(inst, item.enabledInstruments, enabled);
    rebuildEffectiveEventsNow(itemIndex);
}

void GrooveManager::setComposerRowDensity(int itemIndex, int rowIndex, uint8_t level)
{
    juce::ScopedLock sl(lock);

    if (itemIndex < 0 || itemIndex >= static_cast<int>(composerItems.size()))
        return;
    if (rowIndex < 0 || rowIndex >= 3)
        return;

    auto& item = composerItems[static_cast<size_t>(itemIndex)];
    item.instrumentSettingsModified = true;
    item.rowDensity[static_cast<size_t>(rowIndex)] =
        static_cast<uint8_t>(juce::jlimit(0, kMaxDensityLevel, static_cast<int>(level)));
    rebuildEffectiveEventsNow(itemIndex);
}

uint8_t GrooveManager::getComposerRowDensity(int itemIndex, int rowIndex) const
{
    juce::ScopedLock sl(lock);

    if (itemIndex < 0 || itemIndex >= static_cast<int>(composerItems.size()))
        return kMaxDensityLevel;
    if (rowIndex < 0 || rowIndex >= 3)
        return kMaxDensityLevel;

    return composerItems[static_cast<size_t>(itemIndex)].rowDensity[static_cast<size_t>(rowIndex)];
}

const std::vector<Groove::MidiEvent>& GrooveManager::getEffectiveEventsForItem(int itemIndex) const
{
    juce::ScopedLock sl(lock);
    return getEffectiveEventsForItemUnlocked(itemIndex);
}

const std::vector<Groove::MidiEvent>& GrooveManager::getEffectiveEventsForItemUnlocked(int itemIndex) const
{
    static const std::vector<Groove::MidiEvent> empty;

    if (itemIndex < 0 || itemIndex >= static_cast<int>(composerItems.size()))
        return empty;

    auto& item = composerItems[static_cast<size_t>(itemIndex)];

    if (!item.effectiveEventsDirty)
        return item.effectiveEvents;

    const Groove* groove = getGroove(item.grooveCategoryIndex, item.grooveIndex);
    if (groove == nullptr || !groove->isLoaded)
    {
        item.effectiveEvents.clear();
    }
    else if (!item.instrumentSettingsModified)
    {
        item.effectiveEvents = copyOriginalEvents(*groove, item.lengthInBeats);
    }
    else
    {
        item.effectiveEvents = GroovePatternDensity::buildEffectiveEvents(
            *groove, item.lengthInBeats, item.enabledInstruments, item.rowDensity);
    }

    item.effectiveEventsDirty = false;
    return item.effectiveEvents;
}

void GrooveManager::rebuildEffectiveEventsNow(int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= static_cast<int>(composerItems.size()))
        return;

    auto& item = composerItems[static_cast<size_t>(itemIndex)];
    const Groove* groove = getGroove(item.grooveCategoryIndex, item.grooveIndex);

    if (groove == nullptr || !groove->isLoaded)
    {
        item.effectiveEvents.clear();
    }
    else if (!item.instrumentSettingsModified)
    {
        item.effectiveEvents = copyOriginalEvents(*groove, item.lengthInBeats);
    }
    else
    {
        item.effectiveEvents = GroovePatternDensity::buildEffectiveEvents(
            *groove, item.lengthInBeats, item.enabledInstruments, item.rowDensity);
    }

    item.effectiveEventsDirty = false;
}

bool GrooveManager::isComposerItemModified(int itemIndex) const
{
    juce::ScopedLock sl(lock);

    if (itemIndex < 0 || itemIndex >= static_cast<int>(composerItems.size()))
        return false;

    return composerItems[static_cast<size_t>(itemIndex)].instrumentSettingsModified;
}

bool GrooveManager::isComposerItemInstrumentEnabled(int itemIndex, DrumInstrument inst) const
{
    juce::ScopedLock sl(lock);

    if (itemIndex < 0 || itemIndex >= static_cast<int>(composerItems.size()))
        return true;

    return isInstrumentEnabled(inst, composerItems[static_cast<size_t>(itemIndex)].enabledInstruments);
}

std::set<DrumInstrument> GrooveManager::getInstrumentsInGroove(int categoryIndex, int grooveIndex) const
{
    juce::ScopedLock sl(lock);

    std::set<DrumInstrument> result;
    const Groove* groove = getGroove(categoryIndex, grooveIndex);
    if (groove == nullptr || !groove->isLoaded)
        return result;

    for (const auto& evt : groove->events)
    {
        if (evt.message.isNoteOn())
            result.insert(instrumentForNote(evt.message.getNoteNumber()));
    }

    return result;
}

void GrooveManager::cleanupOldExports()
{
    // Remove exported MIDI files older than 1 hour
    // This prevents the exports folder from growing indefinitely
    // while ensuring DAWs have plenty of time to read dropped files
    
    if (!tempDir.exists())
        return;
    
    auto now = juce::Time::getCurrentTime();
    auto oneHourAgo = now - juce::RelativeTime::hours(1);
    
    for (auto& file : tempDir.findChildFiles(juce::File::findFiles, false, "*.mid"))
    {
        if (file.getLastModificationTime() < oneHourAgo)
        {
            file.deleteFile();
            DBG("GrooveManager: Cleaned up old export: " + file.getFileName());
        }
    }
}



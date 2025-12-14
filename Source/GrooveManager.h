/*
    GrooveManager.h
    ===============
    
    Manages loading and playback of MIDI groove files.
    
    This class handles:
    - Scanning the Grooves directory for MIDI files organized by category
    - Loading and parsing MIDI files
    - Tempo-synced playback of grooves
    - Exporting grooves/compositions as MIDI files for drag & drop
*/

#pragma once

#include "JuceHeader.h"
#include <map>
#include <vector>

/*
    GROOVE DATA STRUCTURE
    ---------------------
    Represents a single groove (MIDI pattern) that can be played.
*/
struct Groove
{
    juce::String name;              // Display name (filename without extension)
    juce::String category;          // Category/folder name
    juce::File file;                // Full path to the MIDI file
    double lengthInBeats = 4.0;     // Length of the groove in beats
    int numerator = 4;              // Time signature numerator
    int denominator = 4;            // Time signature denominator
    
    // MIDI events sorted by time (in beats/quarter notes)
    struct MidiEvent
    {
        double timeInBeats;         // When the event occurs (in quarter notes)
        juce::MidiMessage message;  // The MIDI message
    };
    std::vector<MidiEvent> events;
    
    bool isLoaded = false;          // Whether MIDI data has been parsed
};

/*
    GROOVE CATEGORY
    ---------------
    A folder containing related grooves (e.g., "Basic Beats", "Fills")
*/
struct GrooveCategory
{
    juce::String name;              // Category name (folder name)
    std::vector<Groove> grooves;    // Grooves in this category
};

/*
    COMPOSER ITEM
    -------------
    Represents a groove placed in the composer timeline
*/
struct ComposerItem
{
    int grooveCategoryIndex;        // Which category
    int grooveIndex;                // Which groove within category
    double startBeat;               // Where it starts in the composition
    double lengthInBeats;           // How long it lasts
};

class GrooveManager
{
public:
    GrooveManager();
    ~GrooveManager();
    
    // Set the path to the Grooves directory
    void setGroovesPath(const juce::File& path);
    juce::File getGroovesPath() const { return groovesPath; }
    
    // Scan the grooves directory and populate categories
    void scanGrooves();
    
    // Get all categories
    const std::vector<GrooveCategory>& getCategories() const { return categories; }
    
    // Load a specific groove's MIDI data (lazy loading)
    bool loadGroove(int categoryIndex, int grooveIndex);
    
    // Get a groove by index
    Groove* getGroove(int categoryIndex, int grooveIndex);
    const Groove* getGroove(int categoryIndex, int grooveIndex) const;
    
    // Playback control
    void startPlayback(int categoryIndex, int grooveIndex);
    void stopPlayback();
    bool isPlaying() const { return playing; }
    void setLooping(bool shouldLoop) { looping = shouldLoop; }
    bool isLooping() const { return looping; }
    
    // Called from processBlock to get MIDI events for current position
    // Returns events that should be triggered at the current DAW position
    void processBlock(double bpm, double ppqPosition, bool isPlaying,
                      int numSamples, std::vector<juce::MidiMessage>& midiOut);
    
    // Composer functions
    // barCount: number of bars to add (0 = use full groove length)
    void addToComposer(int categoryIndex, int grooveIndex, int barCount = 0);
    void removeFromComposer(int index);
    void clearComposer();
    void moveComposerItem(int fromIndex, int toIndex);
    const std::vector<ComposerItem>& getComposerItems() const { return composerItems; }
    double getComposerLengthInBeats() const;
    
    // Start/stop playing the composed sequence
    void startComposerPlayback();
    void stopComposerPlayback();
    bool isComposerPlaying() const { return composerPlaying; }
    
    // Export functions - create MIDI file for drag & drop
    juce::File exportGrooveToTempFile(int categoryIndex, int grooveIndex);
    juce::File exportCompositionToTempFile();
    
    // Set sample rate for timing calculations
    void setSampleRate(double sampleRate) { currentSampleRate = sampleRate; }
    
    // Set internal BPM for preview playback (overrides DAW tempo)
    void setPreviewBPM(double bpm) { internalBpm = bpm; useInternalClock = true; }
    double getPreviewBPM() const { return internalBpm; }
    
    // Reset to use DAW timing (for normal Grooves tab playback)
    void useDAWTiming() { useInternalClock = false; }
    
    // Reset playback position to start (for syncing with audio loop)
    void resetPlaybackPosition() { internalPositionBeats = 0.0; playbackStartPpq = -1.0; }

private:
    // Parse a MIDI file and populate the Groove structure
    bool parseMidiFile(Groove& groove);
    
    // Calculate the length of a groove in beats from its MIDI events
    double calculateGrooveLength(const Groove& groove);
    
    juce::File groovesPath;
    std::vector<GrooveCategory> categories;
    
    // Playback state
    bool playing = false;
    bool looping = true;
    int currentCategoryIndex = -1;
    int currentGrooveIndex = -1;
    double playbackStartPpq = 0.0;
    double lastProcessedPpq = -1.0;
    
    // Internal timing for standalone preview (when DAW isn't playing)
    double internalBpm = 120.0;
    double internalPositionBeats = 0.0;
    bool useInternalClock = true;  // Use internal clock when DAW isn't playing
    
    // Composer state
    std::vector<ComposerItem> composerItems;
    bool composerPlaying = false;
    double composerStartPpq = 0.0;
    
    double currentSampleRate = 44100.0;
    
    // Temporary directory for exported MIDI files
    juce::File tempDir;
    
    juce::CriticalSection lock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrooveManager)
};

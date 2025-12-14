/*
    SoundFontManager.cpp
    ====================
    
    Implementation of the SoundFont loading and playback system.
    
    INTEGRATING C LIBRARIES
    -----------------------
    TinySoundFont is a "header-only" C library - all the code is in one .h file.
    
    TSF_IMPLEMENTATION must be defined ONCE in exactly ONE .cpp file.
    This tells the header to include the implementation code, not just declarations.
    
    If you define TSF_IMPLEMENTATION in multiple files, you get
    "multiple definition" linker errors.
*/

#define TSF_IMPLEMENTATION  // Enable the implementation in this file only
#include "tsf.h"            // TinySoundFont - a simple SF2 player library
#include "SoundFontManager.h"

SoundFontManager::SoundFontManager()
{
    /*
        INITIALIZE PER-NOTE SETTINGS
        ----------------------------
        GM (General MIDI) drum notes range from 35 to 81.
        We initialize all of them with default values.
        
        This loop uses the [] operator to insert key-value pairs.
    */
    for (int note = 35; note <= 81; ++note)
    {
        noteVolumes[note] = 0.5f;  // Default to 50% volume
        notePans[note] = 0.0f;     // Center pan
    }
}

/*
    DESTRUCTOR - CLEANUP
    --------------------
    When this object is destroyed, we must free the soundfont.
    
    This is RAII (Resource Acquisition Is Initialization):
    - Resources acquired in constructor (or loadKit)
    - Released in destructor
    - No manual cleanup needed by the caller
    
    Without this, we'd have a MEMORY LEAK - the soundfont data
    would stay in memory forever!
*/
SoundFontManager::~SoundFontManager()
{
    // ScopedLock is RAII for locks - acquires in constructor, releases in destructor
    juce::ScopedLock sl(lock);
    
    if (soundFont != nullptr)
    {
        tsf_close(soundFont);  // TinySoundFont's cleanup function
        soundFont = nullptr;   // Good practice: set to null after delete
    }
}

juce::StringArray SoundFontManager::getAvailableKits() const
{
    juce::StringArray kits;
    
    // Check if the path is valid
    if (soundFontsPath.exists() && soundFontsPath.isDirectory())
    {
        /*
            FIND FILES
            ----------
            findChildFiles returns an Array of matching files.
            
            Parameters:
            - juce::File::findFiles = only files, not directories
            - false = don't search subdirectories
            - "*.sf2" = only files with .sf2 extension
        */
        auto files = soundFontsPath.findChildFiles(juce::File::findFiles, false, "*.sf2");
        
        // Sort alphabetically for consistent ordering
        files.sort();
        
        // Extract just the filenames without the extension
        for (const auto& file : files)
        {
            kits.add(file.getFileNameWithoutExtension());
        }
    }
    
    return kits;
}

/*
    LOAD KIT
    --------
    Loads a soundfont file and prepares it for playback.
    
    Returns: true on success, false on failure
    
    Thread safety: Uses a lock because this might be called from the UI
    thread while the audio thread is using the soundfont.
*/
bool SoundFontManager::loadKit(const juce::String& kitName)
{
    juce::ScopedLock sl(lock);  // Acquire lock for this scope
    
    // Build the full path to the SF2 file
    auto kitFile = soundFontsPath.getChildFile(kitName + ".sf2");
    
    if (!kitFile.existsAsFile())
    {
        // DBG is JUCE's debug print - only outputs in debug builds
        DBG("SoundFont file not found: " + kitFile.getFullPathName());
        return false;
    }
    
    /*
        CLEANUP PREVIOUS SOUNDFONT
        --------------------------
        Before loading a new one, we must close the old one.
        Otherwise we'd leak memory.
    */
    if (soundFont != nullptr)
    {
        tsf_close(soundFont);
        soundFont = nullptr;
    }
    
    /*
        LOAD THE SOUNDFONT
        ------------------
        tsf_load_filename() is a C function from TinySoundFont.
        
        toRawUTF8() converts juce::String to a C-style string (const char*).
        C libraries don't understand juce::String!
    */
    soundFont = tsf_load_filename(kitFile.getFullPathName().toRawUTF8());
    
    if (soundFont == nullptr)
    {
        DBG("Failed to load soundfont: " + kitFile.getFullPathName());
        return false;
    }
    
    /*
        CONFIGURE THE SOUNDFONT
        -----------------------
        TSF_STEREO_INTERLEAVED means output format is: L0,R0,L1,R1,L2,R2...
        Sample rate must match what the audio system is using.
        Last parameter (0.0f) is gain adjustment.
    */
    tsf_set_output(soundFont, TSF_STEREO_INTERLEAVED, 
                   static_cast<int>(currentSampleRate), 0.0f);
    
    // Limit polyphony to prevent CPU overload
    tsf_set_max_voices(soundFont, 64);
    
    currentKitName = kitName;
    
    /*
        DEBUG OUTPUT
        ------------
        Print information about the loaded soundfont.
        DBG only prints in debug builds (compiled with DEBUG defined).
    */
    int presetCount = tsf_get_presetcount(soundFont);
    DBG("Loaded soundfont: " + kitName + " with " + juce::String(presetCount) + " presets");
    
    // List first few presets for debugging
    for (int i = 0; i < juce::jmin(presetCount, 5); ++i)
    {
        const char* presetName = tsf_get_presetname(soundFont, i);
        DBG("  Preset " + juce::String(i) + ": " + juce::String(presetName ? presetName : "unnamed"));
    }
    
    return true;
}

void SoundFontManager::setSoundFontsPath(const juce::File& path)
{
    soundFontsPath = path;
}

/*
    SET SAMPLE RATE
    ---------------
    Must be called when the audio system's sample rate changes.
    Common rates: 44100, 48000, 88200, 96000 Hz
*/
void SoundFontManager::setSampleRate(double sampleRate)
{
    juce::ScopedLock sl(lock);
    currentSampleRate = sampleRate;
    
    // Update the soundfont if already loaded
    if (soundFont != nullptr)
    {
        tsf_set_output(soundFont, TSF_STEREO_INTERLEAVED, 
                       static_cast<int>(sampleRate), 0.0f);
    }
}

/*
    NOTE ON
    -------
    Triggers a note in the soundfont.
    
    How soundfonts work:
    - A preset (instrument) contains samples mapped to note ranges
    - When you play a note, the soundfont finds the right sample
    - The sample is pitched and played with the given velocity
*/
void SoundFontManager::noteOn(int note, float velocity)
{
    juce::ScopedLock sl(lock);
    
    if (soundFont == nullptr)
        return;
    
    /*
        APPLY PER-NOTE VOLUME
        ---------------------
        .count() returns 1 if the key exists, 0 if not.
        This is safer than just using [] which would create a default entry.
    */
    float vol = noteVolumes.count(note) ? noteVolumes[note] : 0.5f;  // Default 50%
    float adjustedVelocity = velocity * vol;
    
    /*
        TRIGGER THE NOTE
        ----------------
        tsf_note_on parameters:
        - soundFont: the loaded soundfont
        - 0: preset index (first preset, usually the drum kit)
        - note: MIDI note number
        - adjustedVelocity: how hard the note was hit (0.0-1.0)
    */
    int presetIndex = 0;
    int presetCount = tsf_get_presetcount(soundFont);
    
    if (presetCount > 0)
    {
        tsf_note_on(soundFont, presetIndex, note, adjustedVelocity);
    }
}

void SoundFontManager::noteOff(int note)
{
    juce::ScopedLock sl(lock);
    
    if (soundFont == nullptr)
        return;
    
    // Release the note (for instruments with sustain, not usually drums)
    tsf_note_off(soundFont, 0, note);
}

/*
    RENDER AUDIO
    ------------
    This is called by the audio thread to generate samples.
    
    Parameters:
    - outputBuffer: pointer to array of floats
    - numSamples: number of sample FRAMES to generate
    
    Output is stereo interleaved, so buffer needs numSamples * 2 floats.
    
    IMPORTANT: This runs on the audio thread!
    Must be fast and predictable.
*/
void SoundFontManager::renderAudio(float* outputBuffer, int numSamples)
{
    juce::ScopedLock sl(lock);
    
    if (soundFont == nullptr)
    {
        /*
            MEMSET FOR SILENCE
            ------------------
            If no soundfont is loaded, output silence.
            
            memset fills memory with a byte value.
            0 = silence for floating-point audio.
            
            sizeof(float) * numSamples * 2 = total bytes for stereo output
        */
        std::memset(outputBuffer, 0, sizeof(float) * numSamples * 2);
        return;
    }
    
    /*
        RENDER FROM SOUNDFONT
        ---------------------
        TinySoundFont generates audio samples from all playing notes.
        
        Last parameter (0) means "clear buffer first" (not mix).
    */
    tsf_render_float(soundFont, outputBuffer, numSamples, 0);
}

/*
    PER-NOTE VOLUME/PAN SETTERS
    ---------------------------
    jlimit(min, max, value) clamps value to the range [min, max].
    This prevents invalid values from causing problems.
*/
void SoundFontManager::setNoteVolume(int note, float volume)
{
    juce::ScopedLock sl(lock);
    noteVolumes[note] = juce::jlimit(0.0f, 1.0f, volume);
}

void SoundFontManager::setNotePan(int note, float pan)
{
    juce::ScopedLock sl(lock);
    notePans[note] = juce::jlimit(-1.0f, 1.0f, pan);
}

/*
    GETTERS WITH DEFAULT VALUES
    ---------------------------
    These safely return values, using 1.0/0.0 if the note isn't in the map.
    
    .find() returns an iterator to the element, or .end() if not found.
    This is more efficient than calling .count() then [] separately.
*/
float SoundFontManager::getNoteVolume(int note) const
{
    auto it = noteVolumes.find(note);
    return it != noteVolumes.end() ? it->second : 0.5f;  // Default 50%
}

float SoundFontManager::getNotePan(int note) const
{
    auto it = notePans.find(note);
    return it != notePans.end() ? it->second : 0.0f;
}

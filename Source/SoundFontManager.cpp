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
    */
    for (int note = 35; note <= 81; ++note)
    {
        noteVolumes[note] = 0.5f;  // Default to 50% volume
        notePans[note] = 0.0f;     // Center pan
        noteMutes[note] = false;   // Not muted
    }
    
    // Initialize all group pointers to nullptr
    for (int i = 0; i < NUM_OUTPUT_GROUPS; ++i)
    {
        soundFontGroups[i] = nullptr;
    }
}

/*
    DESTRUCTOR - CLEANUP
    --------------------
    When this object is destroyed, we must free all soundfont instances.
*/
SoundFontManager::~SoundFontManager()
{
    juce::ScopedLock sl(lock);
    
    // Close main soundfont
    if (soundFont != nullptr)
    {
        tsf_close(soundFont);
        soundFont = nullptr;
    }
    
    // Close all group soundfonts
    for (int i = 0; i < NUM_OUTPUT_GROUPS; ++i)
    {
        if (soundFontGroups[i] != nullptr)
        {
            tsf_close(soundFontGroups[i]);
            soundFontGroups[i] = nullptr;
        }
    }
}

juce::StringArray SoundFontManager::getAvailableKits() const
{
    juce::StringArray kits;
    
    if (soundFontsPath.exists() && soundFontsPath.isDirectory())
    {
        auto files = soundFontsPath.findChildFiles(juce::File::findFiles, false, "*.sf2");
        files.sort();
        
        for (const auto& file : files)
        {
            kits.add(file.getFileNameWithoutExtension());
        }
    }
    
    return kits;
}

juce::String SoundFontManager::getCurrentKitName() const
{
    juce::ScopedLock sl(lock);
    return currentKitName;
}

/*
    LOAD KIT
    --------
    Loads a soundfont file for the main output AND all individual output groups.
*/
bool SoundFontManager::loadKit(const juce::String& kitName)
{
    juce::ScopedLock sl(lock);
    
    auto kitFile = soundFontsPath.getChildFile(kitName + ".sf2");
    
    if (!kitFile.existsAsFile())
    {
        DBG("SoundFont file not found: " + kitFile.getFullPathName());
        return false;
    }
    
    // Cleanup previous main soundfont
    if (soundFont != nullptr)
    {
        tsf_close(soundFont);
        soundFont = nullptr;
    }
    
    // Cleanup previous group soundfonts
    for (int i = 0; i < NUM_OUTPUT_GROUPS; ++i)
    {
        if (soundFontGroups[i] != nullptr)
        {
            tsf_close(soundFontGroups[i]);
            soundFontGroups[i] = nullptr;
        }
    }
    
    // Load main soundfont
    soundFont = tsf_load_filename(kitFile.getFullPathName().toRawUTF8());
    
    if (soundFont == nullptr)
    {
        DBG("Failed to load soundfont: " + kitFile.getFullPathName());
        return false;
    }
    
    // Configure main soundfont
    tsf_set_output(soundFont, TSF_STEREO_INTERLEAVED, 
                   static_cast<int>(currentSampleRate), 0.0f);
    tsf_set_max_voices(soundFont, 64);
    
    // Load and configure group soundfonts for multi-out
    for (int i = 0; i < NUM_OUTPUT_GROUPS; ++i)
    {
        soundFontGroups[i] = tsf_load_filename(kitFile.getFullPathName().toRawUTF8());
        if (soundFontGroups[i] != nullptr)
        {
            tsf_set_output(soundFontGroups[i], TSF_STEREO_INTERLEAVED,
                           static_cast<int>(currentSampleRate), 0.0f);
            tsf_set_max_voices(soundFontGroups[i], 8);  // Fewer voices per group
        }
    }
    
    currentKitName = kitName;
    
    int presetCount = tsf_get_presetcount(soundFont);
    DBG("Loaded soundfont: " + kitName + " with " + juce::String(presetCount) + " presets");
    
    return true;
}

void SoundFontManager::setSoundFontsPath(const juce::File& path)
{
    soundFontsPath = path;
}

void SoundFontManager::setSampleRate(double sampleRate)
{
    juce::ScopedLock sl(lock);
    currentSampleRate = sampleRate;
    
    // Update main soundfont
    if (soundFont != nullptr)
    {
        tsf_set_output(soundFont, TSF_STEREO_INTERLEAVED, 
                       static_cast<int>(sampleRate), 0.0f);
    }
    
    // Update all group soundfonts
    for (int i = 0; i < NUM_OUTPUT_GROUPS; ++i)
    {
        if (soundFontGroups[i] != nullptr)
        {
            tsf_set_output(soundFontGroups[i], TSF_STEREO_INTERLEAVED,
                           static_cast<int>(sampleRate), 0.0f);
        }
    }
}

/*
    NOTE ON - Main output
    ---------------------
    Triggers a note on the main soundfont instance.
    Applies per-note volume, pan, and mute settings.
*/
void SoundFontManager::noteOn(int note, float velocity)
{
    juce::ScopedLock sl(lock);
    
    if (soundFont == nullptr)
        return;
    
    // Check if muted
    if (noteMutes.count(note) && noteMutes[note])
        return;
    
    // Apply per-note volume
    float vol = noteVolumes.count(note) ? noteVolumes[note] : 0.5f;
    float adjustedVelocity = velocity * vol;
    
    // Apply per-note pan
    float pan = notePans.count(note) ? notePans[note] : 0.0f;
    // TSF pan: 0.0 = left, 0.5 = center, 1.0 = right
    // Our pan: -1.0 = left, 0.0 = center, 1.0 = right
    // Invert because TSF has reversed pan direction
    float tsfPan = (1.0f - pan) / 2.0f;
    
    int presetCount = tsf_get_presetcount(soundFont);
    if (presetCount > 0)
    {
        // Use channel 9 for drums (GM standard) with channel-based note triggering
        tsf_channel_set_presetindex(soundFont, 9, 0);  // Set preset on channel 9
        tsf_channel_set_pan(soundFont, 9, tsfPan);
        tsf_channel_note_on(soundFont, 9, note, adjustedVelocity);
    }
}

void SoundFontManager::noteOff(int note)
{
    juce::ScopedLock sl(lock);
    
    if (soundFont == nullptr)
        return;
    
    // Release the note using channel 9 (GM drum channel)
    tsf_channel_note_off(soundFont, 9, note);
}

/*
    RENDER AUDIO - Main output only
*/
void SoundFontManager::renderAudio(float* outputBuffer, int numSamples)
{
    juce::ScopedLock sl(lock);
    
    if (soundFont == nullptr)
    {
        std::memset(outputBuffer, 0, sizeof(float) * numSamples * 2);
        return;
    }
    
    tsf_render_float(soundFont, outputBuffer, numSamples, 0);
}

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

float SoundFontManager::getNoteVolume(int note) const
{
    auto it = noteVolumes.find(note);
    return it != noteVolumes.end() ? it->second : 0.5f;
}

float SoundFontManager::getNotePan(int note) const
{
    auto it = notePans.find(note);
    return it != notePans.end() ? it->second : 0.0f;
}

void SoundFontManager::setNoteMute(int note, bool muted)
{
    juce::ScopedLock sl(lock);
    noteMutes[note] = muted;
}

bool SoundFontManager::getNoteMute(int note) const
{
    auto it = noteMutes.find(note);
    return it != noteMutes.end() ? it->second : false;
}

// ===== MULTI-OUT SUPPORT =====

void SoundFontManager::setNoteToGroupMapper(std::function<int(int)> mapper)
{
    juce::ScopedLock sl(lock);
    noteToGroupMapper = mapper;
}

/*
    NOTE ON TO GROUP - Multi-out
    ----------------------------
    Triggers a note on a specific output group's soundfont instance.
*/
void SoundFontManager::noteOnToGroup(int note, float velocity, int groupIndex)
{
    juce::ScopedLock sl(lock);
    
    if (groupIndex < 0 || groupIndex >= NUM_OUTPUT_GROUPS)
        return;
    
    tsf* sfGroup = soundFontGroups[groupIndex];
    if (sfGroup == nullptr)
        return;
    
    // Check if muted
    if (noteMutes.count(note) && noteMutes[note])
        return;
    
    // Apply per-note volume
    float vol = noteVolumes.count(note) ? noteVolumes[note] : 0.5f;
    float adjustedVelocity = velocity * vol;
    
    // Apply per-note pan (inverted for TSF)
    float pan = notePans.count(note) ? notePans[note] : 0.0f;
    float tsfPan = (1.0f - pan) / 2.0f;
    
    int presetCount = tsf_get_presetcount(sfGroup);
    if (presetCount > 0)
    {
        // Use channel 9 for drums with channel-based note triggering
        tsf_channel_set_presetindex(sfGroup, 9, 0);
        tsf_channel_set_pan(sfGroup, 9, tsfPan);
        tsf_channel_note_on(sfGroup, 9, note, adjustedVelocity);
    }
}

void SoundFontManager::noteOffToGroup(int note, int groupIndex)
{
    juce::ScopedLock sl(lock);
    
    if (groupIndex < 0 || groupIndex >= NUM_OUTPUT_GROUPS)
        return;
    
    tsf* sfGroup = soundFontGroups[groupIndex];
    if (sfGroup == nullptr)
        return;
    
    // Release the note using channel 9 (GM drum channel)
    tsf_channel_note_off(sfGroup, 9, note);
}

/*
    RENDER AUDIO MULTI-OUT
    ----------------------
    Renders the main mix and all individual output groups.
*/
void SoundFontManager::renderAudioMultiOut(float* mainBuffer,
                                            std::array<float*, NUM_OUTPUT_GROUPS>& groupBuffers,
                                            int numSamples)
{
    juce::ScopedLock sl(lock);
    
    // Render main mix
    if (soundFont != nullptr)
    {
        tsf_render_float(soundFont, mainBuffer, numSamples, 0);
    }
    else
    {
        std::memset(mainBuffer, 0, sizeof(float) * numSamples * 2);
    }
    
    // Render each output group
    for (int i = 0; i < NUM_OUTPUT_GROUPS; ++i)
    {
        if (groupBuffers[i] != nullptr)
        {
            if (soundFontGroups[i] != nullptr)
            {
                tsf_render_float(soundFontGroups[i], groupBuffers[i], numSamples, 0);
            }
            else
            {
                std::memset(groupBuffers[i], 0, sizeof(float) * numSamples * 2);
            }
        }
    }
}

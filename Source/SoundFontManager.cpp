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
#include <cmath>            // For std::abs

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
    
    // Reduce global volume slightly to prevent clipping with multi-layered SoundFonts
    // Multi-layered SFs can sum multiple samples, potentially causing distortion
    tsf_set_volume(soundFont, 0.8f);
    
    // Configure channel 9 for drums using Bank 128 (GM standard for drums)
    // This ensures proper instrument selection for complex SoundFonts
    tsf_channel_set_bank(soundFont, 9, 128);  // Bank 128 = drums
    tsf_channel_set_presetnumber(soundFont, 9, 0, 1);  // Preset 0, with drum mode enabled
    
    // Load and configure group soundfonts for multi-out
    for (int i = 0; i < NUM_OUTPUT_GROUPS; ++i)
    {
        soundFontGroups[i] = tsf_load_filename(kitFile.getFullPathName().toRawUTF8());
        if (soundFontGroups[i] != nullptr)
        {
            tsf_set_output(soundFontGroups[i], TSF_STEREO_INTERLEAVED,
                           static_cast<int>(currentSampleRate), 0.0f);
            tsf_set_max_voices(soundFontGroups[i], 8);  // Fewer voices per group
            
            // Reduce global volume slightly to prevent clipping
            tsf_set_volume(soundFontGroups[i], 0.8f);
            
            // Configure channel 9 for drums using Bank 128
            tsf_channel_set_bank(soundFontGroups[i], 9, 128);
            tsf_channel_set_presetnumber(soundFontGroups[i], 9, 0, 1);  // Drum mode enabled
        }
    }
    
    currentKitName = kitName;
    
    // Enumerate available presets in this SoundFont
    availablePresets.clear();
    int presetCount = tsf_get_presetcount(soundFont);
    
    for (int i = 0; i < presetCount; ++i)
    {
        const char* name = tsf_get_presetname(soundFont, i);
        if (name != nullptr)
        {
            PresetInfo info;
            info.index = i;
            info.name = juce::String(name);
            info.bank = 128;  // Assume drums (bank 128)
            info.presetNumber = i;
            availablePresets.push_back(info);
            
            DBG("  Preset " + juce::String(i) + ": " + info.name);
        }
    }
    
    // Reset to first preset
    currentPresetIndex = 0;
    
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
    
    // Apply velocity override if set (for selecting specific velocity layers)
    float effectiveVelocity = velocity;
    auto velOverride = noteVelocityOverrides.find(note);
    if (velOverride != noteVelocityOverrides.end())
    {
        effectiveVelocity = velOverride->second;
    }
    
    // Apply per-note volume
    float vol = noteVolumes.count(note) ? noteVolumes[note] : 0.5f;
    float adjustedVelocity = effectiveVelocity * vol;
    
    // Apply per-note pan with preserve sample pan support
    float pan = notePans.count(note) ? notePans[note] : 0.0f;
    float tsfPan;
    
    if (preserveSamplePan && std::abs(pan) < 0.01f)
    {
        // When preserveSamplePan is enabled and pan is centered,
        // use 0.5 which applies no offset to sample's built-in pan
        tsfPan = 0.5f;
    }
    else
    {
        // When user explicitly pans, use aggressive values to fully override sample pan
        // TSF pan offset = tsfPan - 0.5, so we use values outside 0-1 range
        // to ensure we can overcome any pre-panned samples
        tsfPan = 0.5f + pan;  // Range -0.5 to 1.5 for panOffset -1.0 to +1.0
        // Note: TSF internally clamps the final result, but the offset is applied first
    }
    
    int presetCount = tsf_get_presetcount(soundFont);
    if (presetCount > 0)
    {
        // Use channel 9 for drums (GM standard) - bank already set in loadKit
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

void SoundFontManager::setNoteVelocityOverride(int note, float velocity)
{
    juce::ScopedLock sl(lock);
    if (velocity < 0.0f)
    {
        // Negative value clears the override
        noteVelocityOverrides.erase(note);
    }
    else
    {
        noteVelocityOverrides[note] = juce::jlimit(0.0f, 1.0f, velocity);
    }
}

float SoundFontManager::getNoteVelocityOverride(int note) const
{
    auto it = noteVelocityOverrides.find(note);
    return it != noteVelocityOverrides.end() ? it->second : -1.0f;
}

bool SoundFontManager::hasVelocityOverride(int note) const
{
    return noteVelocityOverrides.find(note) != noteVelocityOverrides.end();
}

void SoundFontManager::clearVelocityOverride(int note)
{
    juce::ScopedLock sl(lock);
    noteVelocityOverrides.erase(note);
}

void SoundFontManager::setPreserveSamplePan(bool preserve)
{
    juce::ScopedLock sl(lock);
    preserveSamplePan = preserve;
}

void SoundFontManager::setGlobalVolume(float volume)
{
    juce::ScopedLock sl(lock);
    globalVolume = juce::jlimit(0.0f, 1.0f, volume);
    
    // Apply to main soundfont
    if (soundFont != nullptr)
    {
        tsf_set_volume(soundFont, globalVolume);
    }
    
    // Apply to all group soundfonts
    for (int i = 0; i < NUM_OUTPUT_GROUPS; ++i)
    {
        if (soundFontGroups[i] != nullptr)
        {
            tsf_set_volume(soundFontGroups[i], globalVolume);
        }
    }
}

// ===== PRESET/LAYER SELECTION =====

std::vector<PresetInfo> SoundFontManager::getAvailablePresets() const
{
    juce::ScopedLock sl(lock);
    return availablePresets;
}

int SoundFontManager::getPresetCount() const
{
    juce::ScopedLock sl(lock);
    return static_cast<int>(availablePresets.size());
}

juce::String SoundFontManager::getCurrentPresetName() const
{
    juce::ScopedLock sl(lock);
    if (currentPresetIndex >= 0 && currentPresetIndex < static_cast<int>(availablePresets.size()))
    {
        return availablePresets[currentPresetIndex].name;
    }
    return juce::String();
}

bool SoundFontManager::setPreset(int presetIndex)
{
    juce::ScopedLock sl(lock);
    
    if (soundFont == nullptr)
        return false;
    
    int presetCount = static_cast<int>(availablePresets.size());
    if (presetIndex < 0 || presetIndex >= presetCount)
        return false;
    
    currentPresetIndex = presetIndex;
    
    // Update main soundfont - use preset index directly
    tsf_channel_set_presetindex(soundFont, 9, presetIndex);
    
    // Update all group soundfonts
    for (int i = 0; i < NUM_OUTPUT_GROUPS; ++i)
    {
        if (soundFontGroups[i] != nullptr)
        {
            tsf_channel_set_presetindex(soundFontGroups[i], 9, presetIndex);
        }
    }
    
    DBG("Switched to preset " + juce::String(presetIndex) + ": " + 
        availablePresets[presetIndex].name);
    
    return true;
}

bool SoundFontManager::setPresetByName(const juce::String& name)
{
    // First, find the preset index without holding the lock for setPreset
    int foundIndex = -1;
    {
        juce::ScopedLock sl(lock);
        for (size_t i = 0; i < availablePresets.size(); ++i)
        {
            if (availablePresets[i].name == name)
            {
                foundIndex = static_cast<int>(i);
                break;
            }
        }
    }
    
    // Now call setPreset outside the lock scope (setPreset acquires its own lock)
    if (foundIndex >= 0)
        return setPreset(foundIndex);
    
    return false;
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
    
    // Apply velocity override if set (for selecting specific velocity layers)
    float effectiveVelocity = velocity;
    auto velOverride = noteVelocityOverrides.find(note);
    if (velOverride != noteVelocityOverrides.end())
    {
        effectiveVelocity = velOverride->second;
    }
    
    // Apply per-note volume
    float vol = noteVolumes.count(note) ? noteVolumes[note] : 0.5f;
    float adjustedVelocity = effectiveVelocity * vol;
    
    // Apply per-note pan with preserve sample pan support
    float pan = notePans.count(note) ? notePans[note] : 0.0f;
    float tsfPan;
    
    if (preserveSamplePan && std::abs(pan) < 0.01f)
    {
        // When preserveSamplePan is enabled and pan is centered,
        // use 0.5 which applies no offset to sample's built-in pan
        tsfPan = 0.5f;
    }
    else
    {
        // When user explicitly pans, use aggressive values to fully override sample pan
        // TSF pan offset = tsfPan - 0.5, so we use values outside 0-1 range
        // to ensure we can overcome any pre-panned samples
        tsfPan = 0.5f + pan;  // Range -0.5 to 1.5 for panOffset -1.0 to +1.0
        // Note: TSF internally clamps the final result, but the offset is applied first
    }
    
    int presetCount = tsf_get_presetcount(sfGroup);
    if (presetCount > 0)
    {
        // Use channel 9 for drums - bank already set in loadKit
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

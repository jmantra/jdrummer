/*
    SoundFontManager.h
    ==================
    
    Manages loading and playback of SF2 (SoundFont) files.
    
    SoundFonts are a format that contains:
    - Audio samples of real instruments
    - Mapping information (which sample plays for which note)
    - Envelope and filter settings
    
    We use TinySoundFont (tsf.h), a lightweight C library for SF2 playback.
    
    MULTI-OUT SUPPORT
    -----------------
    This class supports multi-output routing where each drum pad can be
    sent to its own stereo output for individual mixing in the DAW.
    
    We maintain separate TSF instances for each output group to enable
    independent rendering to different output buses.
*/

#pragma once

#include "JuceHeader.h"
#include <map>
#include <array>
#include <functional>

// Forward declaration - tsf is defined in tsf.h
struct tsf;

class SoundFontManager
{
public:
    // Number of individual output groups (one per drum pad)
    static constexpr int NUM_OUTPUT_GROUPS = 16;
    
    SoundFontManager();
    ~SoundFontManager();
    
    // Get list of available kits (SF2 files in soundFontsPath)
    juce::StringArray getAvailableKits() const;
    
    // Load a kit by name (without .sf2 extension)
    bool loadKit(const juce::String& kitName);
    
    // Get the currently loaded kit name (thread-safe)
    juce::String getCurrentKitName() const;
    
    // Set the path where SF2 files are located
    void setSoundFontsPath(const juce::File& path);
    
    // Get the soundfonts path
    juce::File getSoundFontsPath() const { return soundFontsPath; }
    
    // Set the sample rate for audio rendering
    void setSampleRate(double sampleRate);
    
    // Trigger a note (velocity 0.0 to 1.0) - main output
    void noteOn(int note, float velocity);
    
    // Release a note - main output
    void noteOff(int note);
    
    // Render audio to output buffer (stereo interleaved) - main output only
    void renderAudio(float* outputBuffer, int numSamples);
    
    // Per-note volume control (0.0 to 1.0)
    void setNoteVolume(int note, float volume);
    float getNoteVolume(int note) const;
    
    // Per-note pan control (-1.0 left to 1.0 right)
    void setNotePan(int note, float pan);
    float getNotePan(int note) const;
    
    // Per-note mute control
    void setNoteMute(int note, bool muted);
    bool getNoteMute(int note) const;
    
    // ===== MULTI-OUT SUPPORT =====
    
    // Set the function that maps MIDI notes to output groups
    void setNoteToGroupMapper(std::function<int(int)> mapper);
    
    // Trigger a note on a specific output group (for multi-out)
    void noteOnToGroup(int note, float velocity, int groupIndex);
    
    // Release a note on a specific output group
    void noteOffToGroup(int note, int groupIndex);
    
    // Render audio for all output groups (multi-out)
    // mainBuffer: stereo interleaved buffer for main mix
    // groupBuffers: array of stereo interleaved buffers for each output group
    void renderAudioMultiOut(float* mainBuffer, 
                             std::array<float*, NUM_OUTPUT_GROUPS>& groupBuffers,
                             int numSamples);

private:
    // Main TinySoundFont handle (for main stereo mix)
    tsf* soundFont = nullptr;
    
    // Separate TSF instances for each output group (for multi-out)
    std::array<tsf*, NUM_OUTPUT_GROUPS> soundFontGroups;
    
    // Function to map MIDI note to output group index
    std::function<int(int)> noteToGroupMapper;
    
    // Path to directory containing SF2 files
    juce::File soundFontsPath;
    
    // Currently loaded kit name
    juce::String currentKitName;
    
    // Audio sample rate
    double currentSampleRate = 44100.0;
    
    // Per-note volume, pan, and mute settings
    std::map<int, float> noteVolumes;
    std::map<int, float> notePans;
    std::map<int, bool> noteMutes;
    
    // Thread safety lock (mutable to allow use in const methods)
    mutable juce::CriticalSection lock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFontManager)
};

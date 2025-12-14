/*
    SoundFontManager.h
    ==================
    
    Manages loading and playing SoundFont (SF2) files.
    
    WRAPPER CLASS PATTERN
    ---------------------
    This class "wraps" a C library (TinySoundFont) and provides a
    clean, object-oriented interface. This is a common pattern when
    using C libraries in C++ code.
    
    Benefits:
    - Hides the C API complexity
    - Manages resource cleanup automatically (RAII)
    - Provides type safety
    - Integrates with our error handling style
*/

#pragma once

#include "JuceHeader.h"

/*
    FORWARD DECLARATION
    -------------------
    Instead of including the entire tsf.h header, we just declare
    that a struct called 'tsf' exists.
    
    Benefits:
    - Faster compilation (less code to process)
    - Hides implementation details from users of this header
    - Reduces dependencies
    
    The actual struct is defined in tsf.h, which we only include
    in the .cpp file.
*/
typedef struct tsf tsf;

class SoundFontManager
{
public:
    SoundFontManager();
    ~SoundFontManager();

    /*
        CONST METHODS
        -------------
        Methods marked 'const' promise not to modify the object.
        This allows them to be called on const references/pointers.
        
        Good practice: mark all methods that don't modify state as const.
    */
    
    // Returns list of available kit names (scans the directory)
    juce::StringArray getAvailableKits() const;
    
    // Load a soundfont by name - returns true on success
    bool loadKit(const juce::String& kitName);
    
    // Get pointer to the loaded soundfont (for advanced usage)
    tsf* getSoundFont() { return soundFont; }
    
    // Getter for current kit name
    juce::String getCurrentKitName() const { return currentKitName; }
    
    // Set/get the soundfonts directory path
    void setSoundFontsPath(const juce::File& path);
    juce::File getSoundFontsPath() const { return soundFontsPath; }
    
    // Set the audio sample rate (required before playing)
    void setSampleRate(double sampleRate);
    
    /*
        NOTE CONTROL
        ------------
        These methods trigger and release notes in the soundfont.
        
        Note numbers follow MIDI convention (0-127).
        Velocity is 0.0 to 1.0 (loudness/intensity).
    */
    void noteOn(int note, float velocity);
    void noteOff(int note);
    
    // Render audio samples into a buffer
    // Buffer must have space for numSamples * 2 (stereo interleaved)
    void renderAudio(float* outputBuffer, int numSamples);
    
    /*
        PER-NOTE SETTINGS
        -----------------
        Allow individual volume and pan for each drum sound.
    */
    void setNoteVolume(int note, float volume);  // 0.0 to 1.0
    void setNotePan(int note, float pan);        // -1.0 (left) to 1.0 (right)
    float getNoteVolume(int note) const;
    float getNotePan(int note) const;

private:
    /*
        RAW POINTER TO C LIBRARY RESOURCE
        ---------------------------------
        'tsf*' is a pointer to the TinySoundFont struct.
        
        Since this is a C library, we use a raw pointer (not smart pointer)
        and manually manage the lifetime.
        
        Initialization to 'nullptr' is important - uninitialized pointers
        contain garbage values and can cause crashes!
    */
    tsf* soundFont = nullptr;
    
    juce::File soundFontsPath;
    juce::String currentKitName;
    double currentSampleRate = 44100.0;
    
    /*
        std::map - KEY-VALUE CONTAINER
        ------------------------------
        std::map<int, float> stores pairs of (note number -> value).
        
        It's like a dictionary in Python:
        noteVolumes[36] = 0.8;  // Set kick drum volume to 80%
        float v = noteVolumes[36];  // Get kick drum volume
        
        Uses a balanced tree internally (O(log n) lookup).
        For better performance, consider std::unordered_map (O(1) average).
    */
    std::map<int, float> noteVolumes;
    std::map<int, float> notePans;
    
    /*
        THREAD SAFETY
        -------------
        CriticalSection is JUCE's mutex (mutual exclusion lock).
        
        Why we need this:
        - renderAudio() is called from the audio thread
        - loadKit() might be called from the UI thread
        - They both access 'soundFont'
        - Simultaneous access = undefined behavior/crashes
        
        The lock ensures only one thread can access the resource at a time.
    */
    juce::CriticalSection lock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFontManager)
};

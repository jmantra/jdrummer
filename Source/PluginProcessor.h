/*
    PluginProcessor.h
    =================
    
    This is the HEADER FILE for our audio processor class. In C++, we typically split
    code into two files:
    - .h (header) file: Contains CLASS DECLARATIONS - the "blueprint" of what the class has
    - .cpp (implementation) file: Contains the actual code for the methods
    
    This separation allows other files to know about our class without seeing all the details.
*/

#pragma once  // Preprocessor directive: ensures this file is only included once during compilation

/*
    INCLUDE STATEMENTS
    ------------------
    #include brings in code from other files. Think of it like importing libraries in Python.
    
    - "quotes" are for local/project files
    - <angle brackets> are for system/library files
*/
#include "JuceHeader.h"        // JUCE framework - provides audio, UI, and utility classes
#include "SoundFontManager.h"  // Our custom class for managing SF2 soundfonts
#include "GrooveManager.h"     // Our custom class for managing groove MIDI files

/*
    CLASS DECLARATION
    -----------------
    
    This class INHERITS from juce::AudioProcessor using PUBLIC INHERITANCE.
    
    INHERITANCE is a core OOP concept where a class (child/derived) gets all the 
    properties and methods of another class (parent/base).
    
    juce::AudioProcessor is JUCE's base class for anything that processes audio.
    By inheriting from it, we get:
    - Integration with DAWs (VST3, AU, etc.)
    - Audio buffer handling
    - MIDI message processing
    - State save/restore
    
    We must OVERRIDE certain virtual methods to customize behavior.
*/
class JdrummerAudioProcessor : public juce::AudioProcessor
{
public:
    /*
        PUBLIC SECTION
        --------------
        Everything here is accessible from outside the class.
        This is part of ENCAPSULATION - we control what the outside world can access.
    */
    
    // CONSTRUCTOR: Called when an instance of this class is created
    // The constructor initializes the object and sets up initial state
    JdrummerAudioProcessor();
    
    // DESTRUCTOR: Called when the object is destroyed (marked with ~)
    // The 'override' keyword indicates we're overriding a virtual method from the parent class
    // Virtual destructors are important in inheritance to ensure proper cleanup
    ~JdrummerAudioProcessor() override;

    /*
        OVERRIDDEN VIRTUAL METHODS
        --------------------------
        These methods are declared as 'virtual' in the parent class (AudioProcessor).
        By using 'override', we're providing our own implementation.
        
        This is POLYMORPHISM - the same method name behaves differently based on the class.
        When a DAW calls processBlock(), it doesn't know about JdrummerAudioProcessor specifically,
        but our version of the method runs because of polymorphism.
    */
    
    // Called by the host before playback starts - set up resources here
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    
    // Called when playback stops - clean up resources here
    void releaseResources() override;

    // Returns true if this bus layout is supported
    // 'const' at the end means this method doesn't modify the object
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // THE HEART OF THE PLUGIN: Called repeatedly to process audio
    // Parameters are passed by REFERENCE (&) - this means we're working with the
    // original objects, not copies. More efficient and allows modification.
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Creates the UI (editor) for this processor
    // Returns a POINTER to a new AudioProcessorEditor object
    juce::AudioProcessorEditor* createEditor() override;
    
    // Returns whether this processor has a UI
    bool hasEditor() const override;

    // Returns the name of this plugin
    const juce::String getName() const override;

    // MIDI capability methods - simple boolean getters
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    
    // Returns how long the plugin's "tail" lasts (reverb, delay, etc.)
    double getTailLengthSeconds() const override;

    // Program/preset management methods
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    // STATE PERSISTENCE: Save and restore plugin state
    // This is how the DAW saves your settings when you save a project
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    /*
        CUSTOM PUBLIC METHODS
        ---------------------
        These are methods we've added for our specific functionality.
    */
    
    // Returns a REFERENCE to our SoundFontManager
    // Reference (&) means we return the actual object, not a copy
    // This allows external code to interact with our sound font manager
    SoundFontManager& getSoundFontManager() { return soundFontManager; }
    
    // Returns a REFERENCE to our GrooveManager for groove playback
    GrooveManager& getGrooveManager() { return grooveManager; }
    
    // Methods to trigger sounds from the UI (when user clicks pads)
    void triggerNote(int note, float velocity);
    void releaseNote(int note);
    
    // Get current DAW tempo (for UI display)
    double getCurrentBPM() const { return currentBPM; }
    bool isHostPlaying() const { return hostIsPlaying; }
    
    // Audio preview for Groove Matcher
    void setPreviewAudio(juce::AudioBuffer<float>* buffer, double sampleRate);
    void startPreviewPlayback();
    void stopPreviewPlayback();
    bool isPreviewPlaying() const { return previewPlaying; }
    
    // Returns notes that were triggered by MIDI input (for UI visualization)
    // Returns by VALUE (copy) - safe for cross-thread access
    std::vector<int> getAndClearTriggeredNotes();
    
    /*
        std::function - A CALLABLE WRAPPER
        ----------------------------------
        std::function<void()> can hold any callable that takes no arguments and returns void.
        This includes: regular functions, lambdas, or method pointers.
        
        This is the OBSERVER PATTERN - the processor notifies interested parties
        when something happens (like a kit being loaded).
    */
    std::function<void()> onKitLoaded;

private:
    /*
        PRIVATE SECTION
        ---------------
        Only accessible from within this class.
        This is ENCAPSULATION - hiding implementation details from the outside world.
        
        Benefits:
        - Internal changes don't affect external code
        - Prevents accidental misuse
        - Makes the class easier to understand from outside
    */
    
    // COMPOSITION: Our class HAS-A SoundFontManager
    // This is different from inheritance (IS-A relationship)
    // Composition is often preferred over inheritance for flexibility
    SoundFontManager soundFontManager;
    
    // GrooveManager for handling MIDI groove playback
    GrooveManager grooveManager;
    
    // Buffer for rendering audio from the soundfont
    // std::vector is a dynamic array that can grow/shrink
    std::vector<float> renderBuffer;
    
    // Track notes triggered by MIDI for UI visualization
    std::vector<int> recentlyTriggeredNotes;
    
    // THREAD SAFETY: CriticalSection is a mutex (mutual exclusion lock)
    // Audio processing happens on a different thread than the UI
    // We need to protect shared data from simultaneous access
    juce::CriticalSection triggeredNotesLock;
    
    // DAW tempo and playback state
    double currentBPM = 120.0;
    double currentPPQ = 0.0;
    bool hostIsPlaying = false;
    
    // Audio preview playback
    juce::AudioBuffer<float>* previewBuffer = nullptr;
    double previewSampleRate = 44100.0;
    double previewPosition = 0.0;  // Use double for fractional position (resampling)
    bool previewPlaying = false;
    double hostSampleRate = 44100.0;  // The DAW's sample rate
    juce::CriticalSection previewLock;
    
    /*
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
        --------------------------------------------
        This MACRO (code that expands at compile time) does two things:
        1. Deletes the copy constructor and copy assignment operator
           - Prevents accidental copying of the processor
        2. In debug builds, tracks if objects are properly deleted
           - Helps find memory leaks
    */
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JdrummerAudioProcessor)
};

/*
    PluginProcessor.cpp
    ===================
    
    This is the IMPLEMENTATION FILE for our audio processor.
    Here we write the actual code for all the methods declared in the header.
    
    AUDIO PROCESSING FUNDAMENTALS
    -----------------------------
    Audio plugins process sound in small chunks called "buffers" or "blocks".
    Typical buffer sizes are 128, 256, 512, or 1024 samples.
    
    At 44100 Hz sample rate with a 512 sample buffer:
    - processBlock() is called 44100/512 ≈ 86 times per second
    - Each call must complete in ~11.6 milliseconds or you get audio glitches
    
    This is why audio code must be FAST and avoid:
    - Memory allocation (new/delete)
    - Locks that might block
    - File I/O
    - Anything with unpredictable timing
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

/*
    CONSTRUCTOR WITH INITIALIZER LIST
    ---------------------------------
    The syntax ": AudioProcessor(...)" is an INITIALIZER LIST.
    It runs BEFORE the constructor body {} and is the preferred way to:
    1. Call parent class constructors
    2. Initialize member variables
    
    BusesProperties() is JUCE's way of defining audio input/output configuration.
    We're saying: "This plugin has stereo output, no input"
*/
JdrummerAudioProcessor::JdrummerAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    /*
        FINDING SOUNDFONTS
        ------------------
        We need to find where the SF2 files are located.
        This is tricky because the plugin can run in different contexts:
        - As a standalone app
        - As a VST3 plugin in various DAWs
        - During development vs. after installation
        
        Solution: Try multiple locations in priority order.
    */
    
    juce::File soundFontsPath;
    
    // std::vector is C++'s dynamic array - it can grow and shrink
    // juce::File is JUCE's cross-platform file/directory class
    std::vector<juce::File> searchPaths;
    
    /*
        PREPROCESSOR DIRECTIVES
        -----------------------
        #if, #elif, #else, #endif are evaluated at COMPILE TIME.
        The compiler only includes the code for your platform.
        
        JUCE_MAC, JUCE_WINDOWS, JUCE_LINUX are defined by JUCE
        based on what platform you're compiling for.
    */
#if JUCE_MAC
    // macOS: ~/Library/Application Support/jdrummer/soundfonts/
    searchPaths.push_back(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("jdrummer/soundfonts"));
#elif JUCE_WINDOWS
    // Windows: %APPDATA%/jdrummer/soundfonts/
    searchPaths.push_back(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("jdrummer/soundfonts"));
#else
    // Linux: ~/.local/share/jdrummer/soundfonts/
    auto userHome = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    searchPaths.push_back(userHome.getChildFile(".local/share/jdrummer/soundfonts"));
#endif
    
    // Get path to the running executable
    auto executablePath = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    
#if JUCE_MAC
    // macOS VST3 bundles have a specific structure
    searchPaths.push_back(executablePath.getParentDirectory().getParentDirectory()
        .getChildFile("Resources/soundfonts"));
#else
    // Linux/Windows VST3: jdrummer.vst3/Contents/Resources/soundfonts/
    auto vst3Contents = executablePath.getParentDirectory().getParentDirectory();
    searchPaths.push_back(vst3Contents.getChildFile("Resources/soundfonts"));
#endif
    
    // Next to the executable (for standalone builds)
    searchPaths.push_back(executablePath.getParentDirectory().getChildFile("soundfonts"));
    
    // Current working directory (for development)
    searchPaths.push_back(juce::File::getCurrentWorkingDirectory().getChildFile("soundfonts"));
    
    // Check parent directories (fallback for various build configurations)
    auto parent = executablePath.getParentDirectory();
    for (int i = 0; i < 5; ++i)
    {
        searchPaths.push_back(parent.getChildFile("soundfonts"));
        parent = parent.getParentDirectory();
    }
    
    /*
        RANGE-BASED FOR LOOP
        --------------------
        "for (const auto& path : searchPaths)" is modern C++ syntax meaning:
        "for each path in searchPaths"
        
        - const: we won't modify path
        - auto: compiler figures out the type (juce::File)
        - &: reference, not a copy (more efficient)
    */
    for (const auto& path : searchPaths)
    {
        if (path.exists() && path.isDirectory())
        {
            // Check if directory actually contains SF2 files
            auto sf2Files = path.findChildFiles(juce::File::findFiles, false, "*.sf2");
            if (sf2Files.size() > 0)
            {
                soundFontsPath = path;
                break;  // Exit the loop - we found our soundfonts
            }
        }
    }
    
    if (soundFontsPath.exists())
    {
        soundFontManager.setSoundFontsPath(soundFontsPath);
    }
    
    // Load a default kit
    auto kits = soundFontManager.getAvailableKits();
    
    if (kits.size() > 0)
    {
        // Try to load "Standard" first, otherwise use the first kit
        int defaultIndex = kits.indexOf("Standard");
        if (defaultIndex < 0)  // indexOf returns -1 if not found
            defaultIndex = 0;
        
        soundFontManager.loadKit(kits[defaultIndex]);
    }
    
    /*
        FINDING GROOVES
        ---------------
        Similar to soundfonts, we search for the Grooves directory.
        Grooves are MIDI files organized in subfolders by category.
    */
    juce::File groovesPath;
    std::vector<juce::File> grooveSearchPaths;
    
#if JUCE_MAC
    grooveSearchPaths.push_back(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("jdrummer/Grooves"));
#elif JUCE_WINDOWS
    grooveSearchPaths.push_back(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("jdrummer/Grooves"));
#else
    grooveSearchPaths.push_back(juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile(".local/share/jdrummer/Grooves"));
#endif
    
#if JUCE_MAC
    grooveSearchPaths.push_back(executablePath.getParentDirectory().getParentDirectory()
        .getChildFile("Resources/Grooves"));
#else
    grooveSearchPaths.push_back(vst3Contents.getChildFile("Resources/Grooves"));
#endif
    
    grooveSearchPaths.push_back(executablePath.getParentDirectory().getChildFile("Grooves"));
    grooveSearchPaths.push_back(juce::File::getCurrentWorkingDirectory().getChildFile("Grooves"));
    
    // Check parent directories for Grooves folder
    parent = executablePath.getParentDirectory();
    for (int i = 0; i < 5; ++i)
    {
        grooveSearchPaths.push_back(parent.getChildFile("Grooves"));
        parent = parent.getParentDirectory();
    }
    
    for (const auto& path : grooveSearchPaths)
    {
        if (path.exists() && path.isDirectory())
        {
            // Check if directory contains subdirectories (groove categories)
            auto subDirs = path.findChildFiles(juce::File::findDirectories, false);
            if (subDirs.size() > 0)
            {
                groovesPath = path;
                DBG("Found Grooves directory: " + groovesPath.getFullPathName());
                break;
            }
        }
    }
    
    if (groovesPath.exists())
    {
        grooveManager.setGroovesPath(groovesPath);
        grooveManager.scanGrooves();
    }
}

/*
    DESTRUCTOR
    ----------
    Called when the object is destroyed.
    Empty here because our member variables (soundFontManager, renderBuffer)
    clean themselves up automatically - this is called RAII
    (Resource Acquisition Is Initialization).
*/
JdrummerAudioProcessor::~JdrummerAudioProcessor()
{
}

// Returns the plugin name - JucePlugin_Name is defined by JUCE's build system
const juce::String JdrummerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

// MIDI capability methods - simple getters returning constants
bool JdrummerAudioProcessor::acceptsMidi() const  { return true; }
bool JdrummerAudioProcessor::producesMidi() const { return false; }
bool JdrummerAudioProcessor::isMidiEffect() const { return false; }

// How long does sound continue after note-off? (for drum decay)
double JdrummerAudioProcessor::getTailLengthSeconds() const
{
    return 0.5;  // Half second for drum decay
}

// Program/preset methods - we don't use these but must implement them
int JdrummerAudioProcessor::getNumPrograms() { return 1; }
int JdrummerAudioProcessor::getCurrentProgram() { return 0; }
void JdrummerAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String JdrummerAudioProcessor::getProgramName(int index) 
{ 
    juce::ignoreUnused(index); 
    return {}; 
}
void JdrummerAudioProcessor::changeProgramName(int index, const juce::String& newName) 
{ 
    juce::ignoreUnused(index, newName); 
}

/*
    PREPARE TO PLAY
    ---------------
    Called by the host before audio processing starts.
    This is where you allocate resources and configure for the session.
    
    Parameters:
    - sampleRate: samples per second (e.g., 44100, 48000, 96000)
    - samplesPerBlock: maximum buffer size we'll receive
*/
void JdrummerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Tell the soundfont engine what sample rate to use
    soundFontManager.setSampleRate(sampleRate);
    
    // Tell the groove manager about the sample rate
    grooveManager.setSampleRate(sampleRate);
    
    // Store host sample rate for audio preview resampling
    hostSampleRate = sampleRate;
    
    // Pre-allocate buffer for stereo audio (2 channels)
    // static_cast<size_t> converts int to size_t (unsigned) - required by vector
    renderBuffer.resize(static_cast<size_t>(samplesPerBlock) * 2);
}

// Called when playback stops - clean up resources here
void JdrummerAudioProcessor::releaseResources()
{
    // Nothing to clean up - our resources are managed automatically
}

/*
    BUS LAYOUT SUPPORT
    ------------------
    Tells the host what audio configurations we support.
    We only support stereo output, no input.
*/
bool JdrummerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Reject anything that's not stereo output
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

/*
    PROCESS BLOCK - THE HEART OF THE PLUGIN
    ----------------------------------------
    This is called repeatedly by the host to process audio.
    
    Parameters:
    - buffer: The audio buffer to fill with our output
    - midiMessages: MIDI events that occurred during this block
    
    CRITICAL: This runs on the AUDIO THREAD, not the UI thread!
    Must be fast, lock-free, and never block.
*/
void JdrummerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    /*
        ScopedNoDenormals
        -----------------
        "Denormals" are extremely small floating-point numbers that can
        cause CPUs to slow down dramatically. This temporarily disables them.
        JUCE handles enabling/disabling automatically.
    */
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any channels we're not using
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    /*
        GET DAW TEMPO AND POSITION
        --------------------------
        The AudioPlayHead provides timing information from the DAW.
        We use this to sync groove playback with the DAW's tempo and position.
    */
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpm = position->getBpm())
                currentBPM = *bpm;
            
            if (auto ppq = position->getPpqPosition())
                currentPPQ = *ppq;
            
            hostIsPlaying = position->getIsPlaying();
        }
    }
    
    /*
        PROCESS GROOVE PLAYBACK
        -----------------------
        The GrooveManager outputs MIDI events based on the current DAW position.
        We collect these events and trigger them on the soundfont.
    */
    std::vector<juce::MidiMessage> grooveMidiEvents;
    grooveManager.processBlock(currentBPM, currentPPQ, hostIsPlaying, buffer.getNumSamples(), grooveMidiEvents);
    
    // Trigger notes from groove playback
    for (const auto& msg : grooveMidiEvents)
    {
        if (msg.isNoteOn())
        {
            soundFontManager.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
            
            // Track for UI visualization
            {
                juce::ScopedLock sl(triggeredNotesLock);
                recentlyTriggeredNotes.push_back(msg.getNoteNumber());
            }
        }
        else if (msg.isNoteOff())
        {
            soundFontManager.noteOff(msg.getNoteNumber());
        }
    }

    /*
        PROCESS MIDI MESSAGES
        ---------------------
        MIDI messages have a timestamp within the buffer, but for simplicity
        we process them all at the start of the buffer.
        
        In a more advanced implementation, you'd process them at their
        exact sample position for sample-accurate timing.
    */
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            float velocity = message.getFloatVelocity();  // 0.0 to 1.0
            int note = message.getNoteNumber();           // 0 to 127
            
            // Trigger the drum sound
            soundFontManager.noteOn(note, velocity);
            
            /*
                THREAD-SAFE ACCESS
                ------------------
                We need to tell the UI which notes were triggered.
                The UI runs on a different thread, so we need protection.
                
                ScopedLock is a RAII lock - it acquires the lock on creation
                and releases it automatically when it goes out of scope (the closing brace).
            */
            {
                juce::ScopedLock sl(triggeredNotesLock);
                recentlyTriggeredNotes.push_back(note);
            }  // Lock is released here automatically
        }
        else if (message.isNoteOff())
        {
            int note = message.getNoteNumber();
            soundFontManager.noteOff(note);
        }
    }
    
    // Get number of samples to process
    int numSamples = buffer.getNumSamples();
    
    // Ensure our render buffer is large enough
    if (renderBuffer.size() < static_cast<size_t>(numSamples) * 2)
        renderBuffer.resize(static_cast<size_t>(numSamples) * 2);
    
    /*
        RENDER AUDIO
        ------------
        The soundfont engine fills our buffer with interleaved stereo audio:
        [L0, R0, L1, R1, L2, R2, ...]
        
        .data() returns a raw pointer to the vector's internal array.
    */
    soundFontManager.renderAudio(renderBuffer.data(), numSamples);
    
    /*
        DEINTERLEAVE
        ------------
        JUCE uses separate buffers for each channel, not interleaved.
        We need to split our interleaved data into left and right channels.
        
        getWritePointer() returns a float* pointing to the channel's samples.
    */
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);
    
    for (int i = 0; i < numSamples; ++i)
    {
        leftChannel[i] = renderBuffer[static_cast<size_t>(i) * 2];      // Even indices
        rightChannel[i] = renderBuffer[static_cast<size_t>(i) * 2 + 1]; // Odd indices
    }
    
    // Mix in preview audio if playing (with sample rate conversion)
    {
        juce::ScopedLock sl(previewLock);
        if (previewPlaying && previewBuffer != nullptr && previewBuffer->getNumSamples() > 0)
        {
            int previewSamples = previewBuffer->getNumSamples();
            const float* previewData = previewBuffer->getReadPointer(0);
            
            // Calculate the playback rate ratio for sample rate conversion
            // If audio is 44100 Hz and DAW is 48000 Hz, we need to advance slower
            // to maintain correct pitch
            double playbackRatio = previewSampleRate / hostSampleRate;
            
            for (int i = 0; i < numSamples; ++i)
            {
                // Get the integer and fractional parts of the position
                int pos0 = static_cast<int>(previewPosition);
                int pos1 = pos0 + 1;
                double frac = previewPosition - static_cast<double>(pos0);
                
                // Handle looping - sync groove with audio loop
                if (pos0 >= previewSamples)
                {
                    previewPosition = 0.0;
                    pos0 = 0;
                    pos1 = 1;
                    frac = 0.0;
                    
                    // Reset groove playback to stay in sync with audio
                    grooveManager.resetPlaybackPosition();
                }
                if (pos1 >= previewSamples)
                {
                    pos1 = 0;  // Wrap for interpolation
                }
                
                // Linear interpolation between samples for smooth resampling
                float sample0 = previewData[pos0];
                float sample1 = previewData[pos1];
                float sample = static_cast<float>(sample0 + (sample1 - sample0) * frac);
                
                leftChannel[i] += sample * 0.7f;  // Mix at 70% volume
                rightChannel[i] += sample * 0.7f;
                
                // Advance position by the playback ratio
                previewPosition += playbackRatio;
            }
        }
    }
}

// Does this plugin have a UI?
bool JdrummerAudioProcessor::hasEditor() const
{
    return true;
}

/*
    CREATE EDITOR
    -------------
    Creates and returns the plugin's UI.
    
    Returns a raw pointer (not smart pointer) because JUCE manages
    the editor's lifetime - it will delete it when done.
    
    'new' allocates memory on the heap and constructs the object.
    '*this' passes a reference to this processor to the editor.
*/
juce::AudioProcessorEditor* JdrummerAudioProcessor::createEditor()
{
    return new JdrummerAudioProcessorEditor(*this);
}

/*
    STATE PERSISTENCE
    -----------------
    These methods save/restore the plugin state when the user saves/loads
    a DAW project. We use JUCE's ValueTree - a hierarchical data structure
    that can be easily serialized to XML.
*/
void JdrummerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Create a tree structure to hold our state
    juce::ValueTree state("JdrummerState");
    
    // setProperty adds key-value pairs to the tree
    // nullptr is the UndoManager - we don't need undo for state saving
    state.setProperty("currentKit", soundFontManager.getCurrentKitName(), nullptr);
    state.setProperty("soundFontsPath", soundFontManager.getSoundFontsPath().getFullPathName(), nullptr);
    
    // Create a child tree for per-note settings
    juce::ValueTree noteSettings("NoteSettings");
    for (int note = 35; note <= 81; ++note)
    {
        juce::ValueTree noteSetting("Note");
        noteSetting.setProperty("number", note, nullptr);
        noteSetting.setProperty("volume", soundFontManager.getNoteVolume(note), nullptr);
        noteSetting.setProperty("pan", soundFontManager.getNotePan(note), nullptr);
        noteSettings.appendChild(noteSetting, nullptr);
    }
    state.appendChild(noteSettings, nullptr);
    
    /*
        SMART POINTERS
        --------------
        std::unique_ptr is a "smart pointer" that automatically deletes
        its object when it goes out of scope. No manual delete needed!
        
        This prevents memory leaks - a common bug in C++.
    */
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void JdrummerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Parse the binary data back into XML
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    
    if (xml != nullptr)
    {
        // Convert XML to ValueTree
        juce::ValueTree state = juce::ValueTree::fromXml(*xml);
        
        if (state.isValid())
        {
            // Restore soundfonts path
            juce::String sfPath = state.getProperty("soundFontsPath", "");
            if (sfPath.isNotEmpty())
            {
                juce::File path(sfPath);
                if (path.exists() && path.isDirectory())
                {
                    soundFontManager.setSoundFontsPath(path);
                }
            }
            
            // Restore kit selection
            juce::String kitName = state.getProperty("currentKit", "");
            if (kitName.isNotEmpty())
            {
                soundFontManager.loadKit(kitName);
            }
            
            // Restore per-note settings
            auto noteSettings = state.getChildWithName("NoteSettings");
            if (noteSettings.isValid())
            {
                for (int i = 0; i < noteSettings.getNumChildren(); ++i)
                {
                    auto noteSetting = noteSettings.getChild(i);
                    int note = noteSetting.getProperty("number", 0);
                    float volume = noteSetting.getProperty("volume", 0.5f);  // Default 50%
                    float pan = noteSetting.getProperty("pan", 0.0f);
                    
                    soundFontManager.setNoteVolume(note, volume);
                    soundFontManager.setNotePan(note, pan);
                }
            }
            
            // Notify listeners that state was restored
            if (onKitLoaded)
                onKitLoaded();
        }
    }
}

// Trigger a note from the UI (when user clicks a pad)
void JdrummerAudioProcessor::triggerNote(int note, float velocity)
{
    soundFontManager.noteOn(note, velocity);
}

void JdrummerAudioProcessor::releaseNote(int note)
{
    soundFontManager.noteOff(note);
}

/*
    GET AND CLEAR TRIGGERED NOTES
    -----------------------------
    Thread-safe method to get notes that were triggered since last check.
    
    std::move transfers ownership of the vector's contents without copying.
    This is more efficient than copying the entire vector.
*/
std::vector<int> JdrummerAudioProcessor::getAndClearTriggeredNotes()
{
    juce::ScopedLock sl(triggeredNotesLock);
    std::vector<int> notes = std::move(recentlyTriggeredNotes);
    recentlyTriggeredNotes.clear();
    return notes;
}

/*
    PREVIEW AUDIO PLAYBACK
    ----------------------
    These methods allow the Groove Matcher to play audio clips
    through the plugin's audio output.
*/
void JdrummerAudioProcessor::setPreviewAudio(juce::AudioBuffer<float>* buffer, double sampleRate)
{
    juce::ScopedLock sl(previewLock);
    previewBuffer = buffer;
    previewSampleRate = sampleRate;
    previewPosition = 0.0;
}

void JdrummerAudioProcessor::startPreviewPlayback()
{
    juce::ScopedLock sl(previewLock);
    previewPosition = 0.0;
    previewPlaying = true;
}

void JdrummerAudioProcessor::stopPreviewPlayback()
{
    juce::ScopedLock sl(previewLock);
    previewPlaying = false;
    previewPosition = 0.0;
}

/*
    PLUGIN FACTORY FUNCTION
    -----------------------
    This function is called by the DAW/host to create an instance of our plugin.
    It's the entry point - how the host discovers and instantiates our plugin.
    
    JUCE_CALLTYPE ensures the correct calling convention for the platform.
*/
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JdrummerAudioProcessor();
}

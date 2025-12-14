/*
    AudioAnalyzer.h
    ===============
    
    Analyzes audio files to detect tempo and rhythm patterns.
    Uses minibpm for BPM detection and custom onset detection for rhythm analysis.
    
    This enables the "Bandmate" feature where users can drop in an audio clip
    and get matching drum grooves from the library.
*/

#pragma once

#include "JuceHeader.h"
#include "GrooveManager.h"
#include <vector>

/*
    RHYTHM PATTERN
    --------------
    Represents the detected rhythm of an audio clip as a series of onset times.
*/
struct RhythmPattern
{
    double bpm = 120.0;                     // Detected tempo
    double confidence = 0.0;                 // Confidence in BPM detection (0-1)
    std::vector<double> onsetTimesBeats;    // Onset times in beats
    int beatsPerBar = 4;                     // Assumed time signature
    double lengthInBeats = 0.0;              // Total length in beats
};

/*
    GROOVE MATCH
    ------------
    Represents a potential match between the audio pattern and a groove.
*/
struct GrooveMatch
{
    int categoryIndex;
    int grooveIndex;
    juce::String grooveName;
    juce::String categoryName;
    double matchScore;      // 0-100, higher is better
    double bpmDifference;   // How different the groove's natural tempo is
};

class AudioAnalyzer
{
public:
    AudioAnalyzer();
    ~AudioAnalyzer();
    
    // Load an audio file for analysis
    bool loadAudioFile(const juce::File& file);
    
    // Check if audio is loaded
    bool hasAudio() const { return audioLoaded; }
    
    // Get the loaded audio file info
    juce::String getLoadedFileName() const { return loadedFileName; }
    double getAudioLengthSeconds() const { return audioLengthSeconds; }
    double getAudioSampleRate() const { return audioSampleRate; }
    
    // Analyze the loaded audio and detect rhythm pattern
    bool analyzeAudio();
    
    // Get the detected rhythm pattern
    const RhythmPattern& getDetectedPattern() const { return detectedPattern; }
    
    // Find matching grooves from the library
    std::vector<GrooveMatch> findMatchingGrooves(GrooveManager& grooveManager, int maxResults = 10);
    
    // Clear the loaded audio
    void clear();
    
    // Get analysis progress (0-100)
    int getAnalysisProgress() const { return analysisProgress; }
    
    // Check if analysis is complete
    bool isAnalysisComplete() const { return analysisComplete; }
    
    // Get the audio buffer for playback
    juce::AudioBuffer<float>* getAudioBuffer() { return audioLoaded ? &audioBuffer : nullptr; }

private:
    // Audio data
    juce::AudioBuffer<float> audioBuffer;
    double audioSampleRate = 44100.0;
    double audioLengthSeconds = 0.0;
    juce::String loadedFileName;
    bool audioLoaded = false;
    
    // Analysis results
    RhythmPattern detectedPattern;
    bool analysisComplete = false;
    int analysisProgress = 0;
    
    // Internal analysis methods
    double extractBPMFromFilename(const juce::String& filename);
    double detectBPM();
    std::vector<double> detectOnsets();
    double calculatePatternSimilarity(const RhythmPattern& pattern, const Groove& groove);
    
    // Onset detection parameters
    static constexpr double onsetThreshold = 0.15;
    static constexpr int hopSize = 512;
    static constexpr int windowSize = 1024;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioAnalyzer)
};


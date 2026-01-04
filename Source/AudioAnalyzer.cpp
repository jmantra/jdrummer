/*
    AudioAnalyzer.cpp
    =================
    
    Implementation of audio analysis for tempo detection and groove matching.
*/

#include "AudioAnalyzer.h"
#include "MiniBpm.h"
#include <algorithm>
#include <cmath>

AudioAnalyzer::AudioAnalyzer()
{
}

AudioAnalyzer::~AudioAnalyzer()
{
}

bool AudioAnalyzer::loadAudioFile(const juce::File& file)
{
    clear();
    
    if (!file.existsAsFile())
    {
        DBG("AudioAnalyzer: File does not exist: " + file.getFullPathName());
        return false;
    }
    
    // Create an audio format manager and register formats
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    // Create a reader for the file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    
    if (reader == nullptr)
    {
        DBG("AudioAnalyzer: Could not create reader for: " + file.getFullPathName());
        return false;
    }
    
    // Read the audio data
    audioSampleRate = reader->sampleRate;
    int numSamples = static_cast<int>(reader->lengthInSamples);
    int numChannels = static_cast<int>(reader->numChannels);
    
    if (numSamples == 0)
    {
        DBG("AudioAnalyzer: File has no samples");
        return false;
    }
    
    // Allocate buffer (mono - use left channel only for consistent BPM detection)
    audioBuffer.setSize(1, numSamples);
    audioBuffer.clear();
    
    // Read left channel only (more consistent with command-line minibpm)
    reader->read(&audioBuffer, 0, numSamples, 0, true, false);
    
    audioLengthSeconds = static_cast<double>(numSamples) / audioSampleRate;
    loadedFileName = file.getFileName();
    audioLoaded = true;
    analysisComplete = false;
    analysisProgress = 0;
    
    DBG("AudioAnalyzer: Loaded " + loadedFileName + " (" 
        + juce::String(audioLengthSeconds, 2) + "s, " 
        + juce::String(audioSampleRate) + " Hz)");
    
    return true;
}

void AudioAnalyzer::clear()
{
    audioBuffer.setSize(0, 0);
    audioLoaded = false;
    analysisComplete = false;
    analysisProgress = 0;
    loadedFileName = "";
    audioLengthSeconds = 0.0;
    detectedPattern = RhythmPattern();
}

bool AudioAnalyzer::analyzeAudio()
{
    if (!audioLoaded)
    {
        DBG("AudioAnalyzer: No audio loaded");
        return false;
    }
    
    analysisProgress = 10;
    
    // Step 1: Try to extract BPM from filename first
    double bpm = extractBPMFromFilename(loadedFileName);
    
    if (bpm > 0)
    {
        DBG("AudioAnalyzer: BPM extracted from filename: " + juce::String(bpm));
        detectedPattern.confidence = 1.0;  // High confidence for filename BPM
    }
    else
    {
        // Step 2: Detect BPM using minibpm
        bpm = detectBPM();
        if (bpm <= 0)
        {
            DBG("AudioAnalyzer: Failed to detect BPM");
            return false;
        }
        DBG("AudioAnalyzer: BPM detected by minibpm: " + juce::String(bpm));
    }
    
    detectedPattern.bpm = bpm;
    analysisProgress = 50;
    
    // Step 2: Detect onsets (transients/hits)
    auto onsets = detectOnsets();
    analysisProgress = 80;
    
    // Step 3: Convert onset times to beats
    double beatsPerSecond = bpm / 60.0;
    detectedPattern.onsetTimesBeats.clear();
    
    for (double onsetTime : onsets)
    {
        double beatTime = onsetTime * beatsPerSecond;
        detectedPattern.onsetTimesBeats.push_back(beatTime);
    }
    
    // Calculate total length in beats
    detectedPattern.lengthInBeats = audioLengthSeconds * beatsPerSecond;
    detectedPattern.beatsPerBar = 4;  // Assume 4/4 for now
    
    analysisProgress = 100;
    analysisComplete = true;
    
    DBG("AudioAnalyzer: Analysis complete - BPM: " + juce::String(bpm, 1) 
        + ", Onsets: " + juce::String(detectedPattern.onsetTimesBeats.size())
        + ", Length: " + juce::String(detectedPattern.lengthInBeats, 1) + " beats");
    
    return true;
}

double AudioAnalyzer::extractBPMFromFilename(const juce::String& filename)
{
    // Try to extract BPM from filename patterns like:
    // "drum_loop_120bpm.wav", "beat_85_bpm.wav", "120bpm_groove.wav"
    // "loop_120_BPM.wav", "groove-95bpm.mp3", "beat 140 bpm.wav"
    
    juce::String lowerName = filename.toLowerCase();
    
    // Remove file extension
    int dotPos = lowerName.lastIndexOf(".");
    if (dotPos > 0)
        lowerName = lowerName.substring(0, dotPos);
    
    // Pattern 1: Look for "NNNbpm" or "NNN_bpm" or "NNN-bpm" or "NNN bpm"
    // Use regex-like manual parsing
    
    // Replace common separators with spaces for easier parsing
    lowerName = lowerName.replace("_", " ").replace("-", " ");
    
    // Look for "bpm" and check the number before it
    int bpmIndex = lowerName.indexOf("bpm");
    if (bpmIndex > 0)
    {
        // Get the text before "bpm"
        juce::String beforeBpm = lowerName.substring(0, bpmIndex).trim();
        
        // Extract the last number from beforeBpm
        juce::String numberStr;
        for (int i = beforeBpm.length() - 1; i >= 0; --i)
        {
            juce::juce_wchar c = beforeBpm[i];
            if (c >= '0' && c <= '9')
            {
                numberStr = juce::String::charToString(c) + numberStr;
            }
            else if (!numberStr.isEmpty())
            {
                break;  // Found the end of the number
            }
        }
        
        if (!numberStr.isEmpty())
        {
            double bpm = numberStr.getDoubleValue();
            if (bpm >= 40.0 && bpm <= 250.0)  // Reasonable BPM range
            {
                return bpm;
            }
        }
    }
    
    // Pattern 2: Look for "bpmNNN"
    if (bpmIndex >= 0 && bpmIndex + 3 < lowerName.length())
    {
        juce::String afterBpm = lowerName.substring(bpmIndex + 3).trim();
        
        // Extract the number after "bpm"
        juce::String numberStr;
        for (int i = 0; i < afterBpm.length(); ++i)
        {
            juce::juce_wchar c = afterBpm[i];
            if (c >= '0' && c <= '9')
            {
                numberStr += juce::String::charToString(c);
            }
            else if (!numberStr.isEmpty())
            {
                break;
            }
        }
        
        if (!numberStr.isEmpty())
        {
            double bpm = numberStr.getDoubleValue();
            if (bpm >= 40.0 && bpm <= 250.0)
            {
                return bpm;
            }
        }
    }
    
    // Pattern 3: Look for standalone numbers that could be BPM
    // Split by spaces and look for numbers in BPM range
    juce::StringArray tokens;
    tokens.addTokens(lowerName, " ", "");
    
    for (const auto& token : tokens)
    {
        // Check if token is purely numeric
        bool isNumeric = true;
        for (int i = 0; i < token.length(); ++i)
        {
            if (token[i] < '0' || token[i] > '9')
            {
                isNumeric = false;
                break;
            }
        }
        
        if (isNumeric && !token.isEmpty())
        {
            double value = token.getDoubleValue();
            // Only accept if it's a reasonable BPM (60-200 is most common)
            if (value >= 60.0 && value <= 200.0)
            {
                return value;
            }
        }
    }
    
    return 0.0;  // No BPM found in filename
}

double AudioAnalyzer::detectBPM()
{
    if (!audioLoaded)
        return 0.0;
    
    // Use minibpm for BPM detection (use default range 55-190 to match command-line tool)
    breakfastquay::MiniBPM bpmDetector(static_cast<float>(audioSampleRate));
    bpmDetector.setBPMRange(55.0, 190.0);
    
    const float* samples = audioBuffer.getReadPointer(0);
    int numSamples = audioBuffer.getNumSamples();
    
    double bpm = bpmDetector.estimateTempoOfSamples(samples, numSamples);
    
    // Get all tempo candidates and store the top 3
    auto candidates = bpmDetector.getTempoCandidates();
    detectedPattern.alternativeBpms.clear();
    
    if (!candidates.empty() && bpm > 0)
    {
        // Store up to 3 unique tempo candidates
        for (size_t i = 0; i < std::min(candidates.size(), size_t(3)); ++i)
        {
            detectedPattern.alternativeBpms.push_back(candidates[i]);
        }
        
        // Set confidence based on how dominant the first candidate is
        if (candidates.size() >= 2 && candidates[1] > 0)
        {
            // Higher confidence if first candidate is significantly stronger
            // (candidates are sorted by likelihood)
            detectedPattern.confidence = 0.8;
        }
        else
        {
            detectedPattern.confidence = 0.9;  // Single strong candidate
        }
        
        DBG("AudioAnalyzer: Found " + juce::String(detectedPattern.alternativeBpms.size()) 
            + " tempo candidates");
        for (size_t i = 0; i < detectedPattern.alternativeBpms.size(); ++i)
        {
            DBG("  Candidate " + juce::String(i + 1) + ": " 
                + juce::String(detectedPattern.alternativeBpms[i], 1) + " BPM");
        }
    }
    
    return bpm;
}

std::vector<double> AudioAnalyzer::detectOnsets()
{
    std::vector<double> onsets;
    
    if (!audioLoaded)
        return onsets;
    
    const float* samples = audioBuffer.getReadPointer(0);
    int numSamples = audioBuffer.getNumSamples();
    
    // Simple onset detection using energy difference
    std::vector<float> energyEnvelope;
    std::vector<float> energyDiff;
    
    // Calculate RMS energy in each window
    for (int i = 0; i < numSamples - windowSize; i += hopSize)
    {
        float energy = 0.0f;
        for (int j = 0; j < windowSize; ++j)
        {
            float sample = samples[i + j];
            energy += sample * sample;
        }
        energy = std::sqrt(energy / static_cast<float>(windowSize));
        energyEnvelope.push_back(energy);
    }
    
    if (energyEnvelope.size() < 3)
        return onsets;
    
    // Calculate energy difference (onset detection function)
    for (size_t i = 1; i < energyEnvelope.size(); ++i)
    {
        float diff = energyEnvelope[i] - energyEnvelope[i - 1];
        energyDiff.push_back(std::max(0.0f, diff));  // Only positive changes (attacks)
    }
    
    // Find global statistics for adaptive threshold
    float maxDiff = 0.0f;
    float sumDiff = 0.0f;
    for (float d : energyDiff)
    {
        maxDiff = std::max(maxDiff, d);
        sumDiff += d;
    }
    float meanDiff = sumDiff / static_cast<float>(energyDiff.size());
    
    // Use a lower threshold - percentage of max energy change
    float adaptiveThreshold = std::max(meanDiff * 1.5f, maxDiff * 0.1f);
    
    DBG("AudioAnalyzer: Energy diff stats - max: " + juce::String(maxDiff) 
        + ", mean: " + juce::String(meanDiff)
        + ", threshold: " + juce::String(adaptiveThreshold));
    
    // Find peaks in the energy difference
    for (size_t i = 1; i < energyDiff.size() - 1; ++i)
    {
        if (energyDiff[i] > adaptiveThreshold &&
            energyDiff[i] >= energyDiff[i - 1] &&
            energyDiff[i] >= energyDiff[i + 1])
        {
            // Convert frame index to time in seconds
            double timeSeconds = static_cast<double>((i + 1) * hopSize) / audioSampleRate;
            
            // Avoid detecting onsets too close together (minimum 80ms apart)
            if (onsets.empty() || (timeSeconds - onsets.back()) > 0.08)
            {
                onsets.push_back(timeSeconds);
            }
        }
    }
    
    DBG("AudioAnalyzer: Detected " + juce::String(onsets.size()) + " onsets");
    
    return onsets;
}

std::vector<GrooveMatch> AudioAnalyzer::findMatchingGrooves(GrooveManager& grooveManager, int maxResults)
{
    std::vector<GrooveMatch> matches;
    
    if (!analysisComplete)
    {
        DBG("AudioAnalyzer: Cannot find matches - analysis not complete");
        return matches;
    }
    
    const auto& categories = grooveManager.getCategories();
    
    // Score each groove in the library
    for (size_t catIdx = 0; catIdx < categories.size(); ++catIdx)
    {
        const auto& category = categories[catIdx];
        
        for (size_t grooveIdx = 0; grooveIdx < category.grooves.size(); ++grooveIdx)
        {
            // Load the groove if not already loaded
            grooveManager.loadGroove(static_cast<int>(catIdx), static_cast<int>(grooveIdx));
            
            const Groove* groove = grooveManager.getGroove(static_cast<int>(catIdx), 
                                                           static_cast<int>(grooveIdx));
            if (groove == nullptr || !groove->isLoaded)
                continue;
            
            // Calculate similarity score
            double similarity = calculatePatternSimilarity(detectedPattern, *groove);
            
            GrooveMatch match;
            match.categoryIndex = static_cast<int>(catIdx);
            match.grooveIndex = static_cast<int>(grooveIdx);
            match.categoryName = category.name;
            match.grooveName = groove->name;
            match.matchScore = similarity;
            match.bpmDifference = 0.0;  // Grooves don't have inherent BPM
            
            matches.push_back(match);
        }
    }
    
    // Sort by match score (highest first)
    std::sort(matches.begin(), matches.end(), 
              [](const GrooveMatch& a, const GrooveMatch& b) {
                  return a.matchScore > b.matchScore;
              });
    
    // Return top results
    if (static_cast<int>(matches.size()) > maxResults)
    {
        matches.resize(maxResults);
    }
    
    return matches;
}

double AudioAnalyzer::calculatePatternSimilarity(const RhythmPattern& pattern, const Groove& groove)
{
    // Count note-on events in groove
    int grooveNoteCount = 0;
    for (const auto& evt : groove.events)
    {
        if (evt.message.isNoteOn())
            grooveNoteCount++;
    }
    
    if (pattern.onsetTimesBeats.empty() || grooveNoteCount == 0)
        return 0.0;
    
    // Use a single bar (4 beats in 4/4) for comparison - more robust
    const double barLength = 4.0;
    
    // Create hit patterns quantized to 16th notes (16 slots per bar)
    const int numSlots = 16;
    std::vector<int> audioHits(numSlots, 0);
    std::vector<int> grooveHits(numSlots, 0);
    
    // Fill audio pattern (accumulate hits across all bars)
    for (double beatTime : pattern.onsetTimesBeats)
    {
        double normalizedTime = std::fmod(beatTime, barLength);
        int slot = static_cast<int>((normalizedTime / barLength) * numSlots);
        slot = std::clamp(slot, 0, numSlots - 1);
        audioHits[slot]++;
    }
    
    // Fill groove pattern (accumulate hits across all bars)
    for (const auto& evt : groove.events)
    {
        if (evt.message.isNoteOn())
        {
            double normalizedTime = std::fmod(evt.timeInBeats, barLength);
            int slot = static_cast<int>((normalizedTime / barLength) * numSlots);
            slot = std::clamp(slot, 0, numSlots - 1);
            grooveHits[slot]++;
        }
    }
    
    // Normalize the hit counts
    int maxAudio = 1, maxGroove = 1;
    for (int i = 0; i < numSlots; ++i)
    {
        maxAudio = std::max(maxAudio, audioHits[i]);
        maxGroove = std::max(maxGroove, grooveHits[i]);
    }
    
    // Calculate cosine similarity (more tolerant of density differences)
    double dotProduct = 0.0;
    double normA = 0.0;
    double normB = 0.0;
    
    for (int i = 0; i < numSlots; ++i)
    {
        double a = static_cast<double>(audioHits[i]) / maxAudio;
        double b = static_cast<double>(grooveHits[i]) / maxGroove;
        dotProduct += a * b;
        normA += a * a;
        normB += b * b;
    }
    
    if (normA < 0.0001 || normB < 0.0001)
        return 0.0;
    
    double cosineSim = dotProduct / (std::sqrt(normA) * std::sqrt(normB));
    
    // Also calculate position-based matching with tolerance
    int positionMatches = 0;
    int totalPositions = 0;
    
    for (int i = 0; i < numSlots; ++i)
    {
        bool audioHasHit = audioHits[i] > 0;
        bool grooveHasHit = grooveHits[i] > 0;
        
        // Check this slot and adjacent slots for fuzzy matching
        bool grooveHasNearbyHit = grooveHasHit;
        if (i > 0) grooveHasNearbyHit = grooveHasNearbyHit || (grooveHits[i-1] > 0);
        if (i < numSlots - 1) grooveHasNearbyHit = grooveHasNearbyHit || (grooveHits[i+1] > 0);
        
        bool audioHasNearbyHit = audioHasHit;
        if (i > 0) audioHasNearbyHit = audioHasNearbyHit || (audioHits[i-1] > 0);
        if (i < numSlots - 1) audioHasNearbyHit = audioHasNearbyHit || (audioHits[i+1] > 0);
        
        if (audioHasHit || grooveHasHit)
        {
            totalPositions++;
            if (audioHasHit && grooveHasNearbyHit)
                positionMatches++;
            else if (grooveHasHit && audioHasNearbyHit)
                positionMatches++;
        }
    }
    
    double positionScore = totalPositions > 0 ? 
        static_cast<double>(positionMatches) / totalPositions : 0.0;
    
    // Combine scores (weighted average)
    double finalScore = (cosineSim * 0.6 + positionScore * 0.4) * 100.0;
    
    return finalScore;
}



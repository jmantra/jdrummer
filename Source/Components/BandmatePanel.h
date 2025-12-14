/*
    BandmatePanel.h
    ===============
    
    UI panel for the "Groove Matcher" feature.
    
    Allows users to:
    1. Drop or browse for an audio file
    2. Analyze the audio to detect tempo and rhythm
    3. Find matching grooves from the library
    4. Preview both audio and groove together
    5. Add matched grooves to the composer
    6. Drag and drop to DAW
*/

#pragma once

#include "../JuceHeader.h"
#include "../AudioAnalyzer.h"
#include "../GrooveManager.h"
#include "GrooveComposer.h"

// Forward declaration
class JdrummerAudioProcessor;

class BandmatePanel : public juce::Component,
                      public juce::FileDragAndDropTarget,
                      public juce::Timer
{
public:
    BandmatePanel();
    ~BandmatePanel() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Set references
    void setProcessor(JdrummerAudioProcessor* processor);
    void setGrooveManager(GrooveManager* manager);
    
    // FileDragAndDropTarget interface
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    
    // Timer for analysis progress and playback
    void timerCallback() override;

private:
    JdrummerAudioProcessor* audioProcessor = nullptr;
    GrooveManager* grooveManager = nullptr;
    AudioAnalyzer audioAnalyzer;
    
    // UI Components
    juce::Label titleLabel;
    juce::Label dropZoneLabel;
    juce::TextButton browseButton;
    juce::TextButton analyzeButton;
    juce::TextButton clearButton;
    
    // Audio info display
    juce::Label fileNameLabel;
    juce::Label tempoLabel;
    juce::Label statusLabel;
    double progressValue = 0.0;
    std::unique_ptr<juce::ProgressBar> progressBar;
    
    // Playback controls
    juce::TextButton playBothButton;
    juce::TextButton playAudioButton;
    juce::TextButton playGrooveButton;
    juce::TextButton stopButton;
    juce::Slider volumeSlider;
    juce::Label volumeLabel;
    
    // Match results list
    juce::Label matchesLabel;
    juce::ListBox matchesListBox;
    juce::TextButton addToComposerButton;
    
    // Bar count selector (same as GrooveBrowser)
    juce::Label barCountLabel;
    juce::ComboBox barCountComboBox;
    
    // Composer for building the drum part
    GrooveComposer grooveComposer;
    
    // Match results
    std::vector<GrooveMatch> matchResults;
    int selectedMatchIndex = -1;
    
    // State
    bool isDragOver = false;
    bool isAnalyzing = false;
    
    // Audio playback
    bool isPlayingAudio = false;
    bool isPlayingGroove = false;
    juce::File loadedAudioFile;
    
    // List box model for matches
    class MatchesListModel : public juce::ListBoxModel
    {
    public:
        MatchesListModel(BandmatePanel& owner) : panel(owner) {}
        
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;
        
    private:
        BandmatePanel& panel;
    };
    
    MatchesListModel matchesListModel;
    
    // Helper methods
    void loadAudioFile(const juce::File& file);
    void startAnalysis();
    void onAnalysisComplete();
    void addSelectedMatchToComposer();
    int getSelectedBarCount() const;
    
    // Playback methods
    void playBoth();
    void playAudioOnly();
    void playGrooveOnly();
    void stopPlayback();
    void prepareAudioPlayback();
    
    // Colors
    juce::Colour backgroundColour{0xFF1A1A2E};
    juce::Colour dropZoneColour{0xFF252540};
    juce::Colour accentColour{0xFF00BFFF};
    juce::Colour textColour{0xFFEEEEEE};
    juce::Colour dimTextColour{0xFF888888};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BandmatePanel)
};


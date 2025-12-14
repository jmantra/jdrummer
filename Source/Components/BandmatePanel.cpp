/*
    BandmatePanel.cpp
    =================
    
    Implementation of the Groove Matcher feature panel.
*/

#include "BandmatePanel.h"
#include "../PluginProcessor.h"

BandmatePanel::BandmatePanel()
    : matchesListModel(*this)
{
    
    // Title
    titleLabel.setText("GROOVE MATCHER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, accentColour);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);
    
    // Drop zone label
    dropZoneLabel.setText("Drop an audio file here\nor click Browse", juce::dontSendNotification);
    dropZoneLabel.setFont(juce::Font(14.0f));
    dropZoneLabel.setColour(juce::Label::textColourId, dimTextColour);
    dropZoneLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(dropZoneLabel);
    
    // Browse button
    browseButton.setButtonText("Browse...");
    browseButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF3A3A5A));
    browseButton.setColour(juce::TextButton::textColourOffId, textColour);
    browseButton.onClick = [this]() {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Select an audio file",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav;*.mp3;*.aiff;*.flac;*.ogg");
        
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc) {
                auto results = fc.getResults();
                if (!results.isEmpty())
                {
                    loadAudioFile(results[0]);
                }
            });
    };
    addAndMakeVisible(browseButton);
    
    // Analyze button
    analyzeButton.setButtonText("Analyze & Find Matches");
    analyzeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A5A2A));
    analyzeButton.setColour(juce::TextButton::textColourOffId, textColour);
    analyzeButton.setEnabled(false);
    analyzeButton.onClick = [this]() { startAnalysis(); };
    addAndMakeVisible(analyzeButton);
    
    // Clear button
    clearButton.setButtonText("Clear");
    clearButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF5A2A2A));
    clearButton.setColour(juce::TextButton::textColourOffId, textColour);
    clearButton.onClick = [this]() {
        stopPlayback();
        audioAnalyzer.clear();
        matchResults.clear();
        matchesListBox.updateContent();
        fileNameLabel.setText("", juce::dontSendNotification);
        tempoLabel.setText("", juce::dontSendNotification);
        statusLabel.setText("", juce::dontSendNotification);
        analyzeButton.setEnabled(false);
        loadedAudioFile = juce::File();
        resized();  // Update layout to show browse button
        repaint();
    };
    addAndMakeVisible(clearButton);
    
    // File name label
    fileNameLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    fileNameLabel.setColour(juce::Label::textColourId, textColour);
    fileNameLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(fileNameLabel);
    
    // Tempo label
    tempoLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    tempoLabel.setColour(juce::Label::textColourId, accentColour);
    tempoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(tempoLabel);
    
    // Status label
    statusLabel.setFont(juce::Font(11.0f));
    statusLabel.setColour(juce::Label::textColourId, dimTextColour);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);
    
    // Progress bar
    progressBar = std::make_unique<juce::ProgressBar>(progressValue);
    progressBar->setColour(juce::ProgressBar::foregroundColourId, accentColour);
    progressBar->setColour(juce::ProgressBar::backgroundColourId, juce::Colour(0xFF333333));
    addChildComponent(*progressBar);  // Hidden initially
    
    // Playback controls
    playBothButton.setButtonText("Play Both");
    playBothButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A5A2A));
    playBothButton.setColour(juce::TextButton::textColourOffId, textColour);
    playBothButton.onClick = [this]() { playBoth(); };
    addAndMakeVisible(playBothButton);
    
    playAudioButton.setButtonText("Audio");
    playAudioButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF3A3A5A));
    playAudioButton.setColour(juce::TextButton::textColourOffId, textColour);
    playAudioButton.onClick = [this]() { playAudioOnly(); };
    addAndMakeVisible(playAudioButton);
    
    playGrooveButton.setButtonText("Groove");
    playGrooveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF3A3A5A));
    playGrooveButton.setColour(juce::TextButton::textColourOffId, textColour);
    playGrooveButton.onClick = [this]() { playGrooveOnly(); };
    addAndMakeVisible(playGrooveButton);
    
    stopButton.setButtonText("Stop");
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF5A2A2A));
    stopButton.setColour(juce::TextButton::textColourOffId, textColour);
    stopButton.onClick = [this]() { stopPlayback(); };
    addAndMakeVisible(stopButton);
    
    // Matches label
    matchesLabel.setText("MATCHING GROOVES", juce::dontSendNotification);
    matchesLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    matchesLabel.setColour(juce::Label::textColourId, textColour);
    addAndMakeVisible(matchesLabel);
    
    // Matches list
    matchesListBox.setModel(&matchesListModel);
    matchesListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xFF1E1E1E));
    matchesListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0xFF333333));
    matchesListBox.setRowHeight(28);
    matchesListBox.setOutlineThickness(1);
    addAndMakeVisible(matchesListBox);
    
    // Bar count label
    barCountLabel.setText("Bars:", juce::dontSendNotification);
    barCountLabel.setFont(juce::Font(12.0f));
    barCountLabel.setColour(juce::Label::textColourId, dimTextColour);
    barCountLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(barCountLabel);
    
    // Bar count combo box
    barCountComboBox.addItem("All", 1);
    barCountComboBox.addItem("1 Bar", 2);
    barCountComboBox.addItem("2 Bars", 3);
    barCountComboBox.addItem("4 Bars", 4);
    barCountComboBox.addItem("8 Bars", 5);
    barCountComboBox.setSelectedId(4);  // Default to 4 bars
    barCountComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF2A2A2A));
    barCountComboBox.setColour(juce::ComboBox::textColourId, textColour);
    addAndMakeVisible(barCountComboBox);
    
    // Add to composer button
    addToComposerButton.setButtonText("+ Add to Composer");
    addToComposerButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A5A2A));
    addToComposerButton.setColour(juce::TextButton::textColourOffId, textColour);
    addToComposerButton.onClick = [this]() { addSelectedMatchToComposer(); };
    addAndMakeVisible(addToComposerButton);
    
    // Composer
    addAndMakeVisible(grooveComposer);
    
    // Setup composer callbacks
    grooveComposer.onPlayClicked = [this]() {
        if (grooveManager != nullptr)
        {
            grooveManager->startComposerPlayback();
            grooveComposer.setPlaying(true);
        }
    };
    
    grooveComposer.onStopClicked = [this]() {
        if (grooveManager != nullptr)
        {
            grooveManager->stopComposerPlayback();
            grooveComposer.setPlaying(false);
        }
    };
    
    grooveComposer.onClearClicked = [this]() {
        if (grooveManager != nullptr)
        {
            grooveManager->clearComposer();
            grooveComposer.refresh();
        }
    };
}

BandmatePanel::~BandmatePanel()
{
    stopTimer();
    stopPlayback();
}

void BandmatePanel::paint(juce::Graphics& g)
{
    // Background gradient
    juce::ColourGradient gradient(
        juce::Colour(0xFF1A1A2E), 0.0f, 0.0f,
        juce::Colour(0xFF16213E), 0.0f, static_cast<float>(getHeight()),
        false
    );
    g.setGradientFill(gradient);
    g.fillAll();
    
    // Scanline effect
    g.setColour(juce::Colour(0x08FFFFFF));
    for (int y = 0; y < getHeight(); y += 4)
    {
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));
    }
    
    // Drop zone area
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(30);  // Title
    auto dropZone = bounds.removeFromTop(100);
    
    // Draw drop zone
    g.setColour(isDragOver ? accentColour.withAlpha(0.3f) : dropZoneColour);
    g.fillRoundedRectangle(dropZone.toFloat(), 8.0f);
    
    g.setColour(isDragOver ? accentColour : juce::Colour(0xFF444444));
    g.drawRoundedRectangle(dropZone.toFloat(), 8.0f, isDragOver ? 2.0f : 1.0f);
    
    // Draw dashed border when no file loaded
    if (!audioAnalyzer.hasAudio() && !isDragOver)
    {
        g.setColour(juce::Colour(0xFF555555));
        float dashLengths[] = { 6.0f, 4.0f };
        g.drawDashedLine(juce::Line<float>(dropZone.getX() + 10.0f, dropZone.getY() + 10.0f,
                                            dropZone.getRight() - 10.0f, dropZone.getY() + 10.0f),
                         dashLengths, 2);
    }
    
    // Playback indicator
    if (isPlayingAudio || isPlayingGroove)
    {
        g.setColour(juce::Colour(0xFF00FF00));
        g.fillEllipse(getWidth() - 25.0f, 15.0f, 10.0f, 10.0f);
    }
}

void BandmatePanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(30));
    
    // Drop zone area
    auto dropZone = bounds.removeFromTop(100);
    
    // Position elements inside drop zone
    auto dropContent = dropZone.reduced(10);
    
    if (audioAnalyzer.hasAudio())
    {
        // Show file info - hide browse button, show other controls
        dropZoneLabel.setVisible(false);
        browseButton.setVisible(false);
        clearButton.setVisible(true);
        fileNameLabel.setVisible(true);
        tempoLabel.setVisible(true);
        statusLabel.setVisible(true);
        
        auto topRow = dropContent.removeFromTop(22);
        fileNameLabel.setBounds(topRow.removeFromLeft(topRow.getWidth() - 60));
        clearButton.setBounds(topRow.removeFromRight(55));
        
        dropContent.removeFromTop(3);
        
        auto tempoRow = dropContent.removeFromTop(28);
        tempoLabel.setBounds(tempoRow);
        
        auto statusRow = dropContent.removeFromTop(18);
        statusLabel.setBounds(statusRow);
        
        auto buttonRow = dropContent;
        buttonRow = buttonRow.withSizeKeepingCentre(200, 26);
        
        if (isAnalyzing)
        {
            progressBar->setVisible(true);
            progressBar->setBounds(buttonRow);
            analyzeButton.setVisible(false);
        }
        else
        {
            progressBar->setVisible(false);
            analyzeButton.setVisible(true);
            analyzeButton.setBounds(buttonRow);
        }
    }
    else
    {
        // No audio loaded - show browse button, hide other controls
        dropZoneLabel.setVisible(true);
        browseButton.setVisible(true);
        clearButton.setVisible(false);
        fileNameLabel.setVisible(false);
        tempoLabel.setVisible(false);
        statusLabel.setVisible(false);
        analyzeButton.setVisible(false);
        progressBar->setVisible(false);
        
        dropZoneLabel.setBounds(dropContent.removeFromTop(40));
        
        auto buttonRow = dropContent.withSizeKeepingCentre(100, 26);
        browseButton.setBounds(buttonRow);
    }
    
    bounds.removeFromTop(8);
    
    // Playback controls row
    auto playbackRow = bounds.removeFromTop(30);
    playBothButton.setBounds(playbackRow.removeFromLeft(90));
    playbackRow.removeFromLeft(5);
    playAudioButton.setBounds(playbackRow.removeFromLeft(70));
    playbackRow.removeFromLeft(5);
    playGrooveButton.setBounds(playbackRow.removeFromLeft(70));
    playbackRow.removeFromLeft(5);
    stopButton.setBounds(playbackRow.removeFromLeft(60));
    
    bounds.removeFromTop(8);
    
    // Split remaining space between matches list and composer
    auto matchesArea = bounds.removeFromTop(bounds.getHeight() - 85);
    
    // Matches section
    matchesLabel.setBounds(matchesArea.removeFromTop(20));
    matchesArea.removeFromTop(5);
    
    // Bottom row of matches area: bar count and add button
    auto matchesBottom = matchesArea.removeFromBottom(28);
    barCountLabel.setBounds(matchesBottom.removeFromLeft(35));
    matchesBottom.removeFromLeft(5);
    barCountComboBox.setBounds(matchesBottom.removeFromLeft(70));
    matchesBottom.removeFromLeft(10);
    addToComposerButton.setBounds(matchesBottom.removeFromLeft(140));
    
    matchesArea.removeFromBottom(5);
    matchesListBox.setBounds(matchesArea);
    
    bounds.removeFromTop(8);
    
    // Composer at bottom
    grooveComposer.setBounds(bounds);
}

void BandmatePanel::setProcessor(JdrummerAudioProcessor* processor)
{
    audioProcessor = processor;
}

void BandmatePanel::setGrooveManager(GrooveManager* manager)
{
    grooveManager = manager;
    grooveComposer.setGrooveManager(manager);
}

bool BandmatePanel::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
    {
        juce::File f(file);
        juce::String ext = f.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".mp3" || ext == ".aiff" || 
            ext == ".flac" || ext == ".ogg" || ext == ".aif")
        {
            return true;
        }
    }
    return false;
}

void BandmatePanel::filesDropped(const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(x, y);
    isDragOver = false;
    
    for (const auto& file : files)
    {
        juce::File f(file);
        juce::String ext = f.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".mp3" || ext == ".aiff" || 
            ext == ".flac" || ext == ".ogg" || ext == ".aif")
        {
            loadAudioFile(f);
            break;
        }
    }
    
    repaint();
}

void BandmatePanel::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(files, x, y);
    isDragOver = true;
    repaint();
}

void BandmatePanel::fileDragExit(const juce::StringArray& files)
{
    juce::ignoreUnused(files);
    isDragOver = false;
    repaint();
}

void BandmatePanel::loadAudioFile(const juce::File& file)
{
    stopPlayback();
    
    if (audioAnalyzer.loadAudioFile(file))
    {
        loadedAudioFile = file;
        fileNameLabel.setText(audioAnalyzer.getLoadedFileName(), juce::dontSendNotification);
        tempoLabel.setText("", juce::dontSendNotification);
        statusLabel.setText(juce::String(audioAnalyzer.getAudioLengthSeconds(), 1) + " seconds", 
                           juce::dontSendNotification);
        analyzeButton.setEnabled(true);
        matchResults.clear();
        matchesListBox.updateContent();
        prepareAudioPlayback();
        resized();
        repaint();
    }
}

void BandmatePanel::prepareAudioPlayback()
{
    // Audio buffer is already loaded in audioAnalyzer
    // We'll use the processor's preview system
    if (audioProcessor != nullptr && audioAnalyzer.hasAudio())
    {
        // The audioAnalyzer stores the audio buffer internally
        // We need to pass it to the processor
    }
}

void BandmatePanel::startAnalysis()
{
    if (!audioAnalyzer.hasAudio())
        return;
    
    isAnalyzing = true;
    progressValue = 0.0;
    statusLabel.setText("Analyzing...", juce::dontSendNotification);
    resized();
    
    // Start timer to update progress
    startTimerHz(30);
    
    // Run analysis in background thread
    juce::Thread::launch([this]() {
        bool success = audioAnalyzer.analyzeAudio();
        
        juce::MessageManager::callAsync([this, success]() {
            stopTimer();
            isAnalyzing = false;
            
            if (success)
            {
                onAnalysisComplete();
            }
            else
            {
                statusLabel.setText("Analysis failed", juce::dontSendNotification);
            }
            
            resized();
            repaint();
        });
    });
}

void BandmatePanel::timerCallback()
{
    progressValue = audioAnalyzer.getAnalysisProgress() / 100.0;
    repaint();
}

void BandmatePanel::onAnalysisComplete()
{
    const auto& pattern = audioAnalyzer.getDetectedPattern();
    
    // Update tempo display
    tempoLabel.setText(juce::String(pattern.bpm, 3) + " BPM", juce::dontSendNotification);
    statusLabel.setText(juce::String(pattern.onsetTimesBeats.size()) + " beats detected", 
                       juce::dontSendNotification);
    
    // Find matching grooves
    if (grooveManager != nullptr)
    {
        matchResults = audioAnalyzer.findMatchingGrooves(*grooveManager, 15);
        matchesListBox.updateContent();
        
        if (!matchResults.empty())
        {
            // Select the best match
            matchesListBox.selectRow(0);
            selectedMatchIndex = 0;
            
            // Auto-add the best match to the composer
            addSelectedMatchToComposer();
        }
    }
}

void BandmatePanel::addSelectedMatchToComposer()
{
    if (selectedMatchIndex < 0 || selectedMatchIndex >= static_cast<int>(matchResults.size()))
        return;
    
    if (grooveManager == nullptr)
        return;
    
    const auto& match = matchResults[static_cast<size_t>(selectedMatchIndex)];
    grooveManager->addToComposer(match.categoryIndex, match.grooveIndex, getSelectedBarCount());
    grooveComposer.refresh();
}

int BandmatePanel::getSelectedBarCount() const
{
    int selectedId = barCountComboBox.getSelectedId();
    
    switch (selectedId)
    {
        case 1: return 0;   // "All"
        case 2: return 1;   // 1 bar
        case 3: return 2;   // 2 bars
        case 4: return 4;   // 4 bars
        case 5: return 8;   // 8 bars
        default: return 4;
    }
}

void BandmatePanel::playBoth()
{
    if (grooveManager == nullptr || audioProcessor == nullptr)
        return;
    
    stopPlayback();
    
    // Set the detected BPM for groove playback
    const auto& pattern = audioAnalyzer.getDetectedPattern();
    if (pattern.bpm > 0)
    {
        grooveManager->setPreviewBPM(pattern.bpm);
    }
    
    // Start audio playback through processor
    auto* audioBuffer = audioAnalyzer.getAudioBuffer();
    if (audioBuffer != nullptr)
    {
        audioProcessor->setPreviewAudio(audioBuffer, audioAnalyzer.getAudioSampleRate());
        audioProcessor->startPreviewPlayback();
        isPlayingAudio = true;
    }
    
    // Start groove playback
    if (selectedMatchIndex >= 0 && selectedMatchIndex < static_cast<int>(matchResults.size()))
    {
        const auto& match = matchResults[static_cast<size_t>(selectedMatchIndex)];
        grooveManager->setLooping(true);
        grooveManager->startPlayback(match.categoryIndex, match.grooveIndex);
        isPlayingGroove = true;
    }
    
    startTimerHz(30);
    repaint();
}

void BandmatePanel::playAudioOnly()
{
    if (audioProcessor == nullptr)
        return;
    
    stopPlayback();
    
    auto* audioBuffer = audioAnalyzer.getAudioBuffer();
    if (audioBuffer != nullptr)
    {
        audioProcessor->setPreviewAudio(audioBuffer, audioAnalyzer.getAudioSampleRate());
        audioProcessor->startPreviewPlayback();
        isPlayingAudio = true;
        startTimerHz(30);
    }
    
    repaint();
}

void BandmatePanel::playGrooveOnly()
{
    if (grooveManager == nullptr)
        return;
    
    stopPlayback();
    
    // Set the detected BPM for groove playback
    const auto& pattern = audioAnalyzer.getDetectedPattern();
    if (pattern.bpm > 0)
    {
        grooveManager->setPreviewBPM(pattern.bpm);
    }
    
    if (selectedMatchIndex >= 0 && selectedMatchIndex < static_cast<int>(matchResults.size()))
    {
        const auto& match = matchResults[static_cast<size_t>(selectedMatchIndex)];
        grooveManager->setLooping(true);
        grooveManager->startPlayback(match.categoryIndex, match.grooveIndex);
        isPlayingGroove = true;
        startTimerHz(30);
    }
    
    repaint();
}

void BandmatePanel::stopPlayback()
{
    if (audioProcessor != nullptr)
    {
        audioProcessor->stopPreviewPlayback();
    }
    isPlayingAudio = false;
    
    if (grooveManager != nullptr)
    {
        grooveManager->stopPlayback();
        grooveManager->useDAWTiming();  // Reset to DAW timing after preview
    }
    isPlayingGroove = false;
    
    repaint();
}

// MatchesListModel implementation
int BandmatePanel::MatchesListModel::getNumRows()
{
    return static_cast<int>(panel.matchResults.size());
}

void BandmatePanel::MatchesListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, 
                                                        int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(panel.matchResults.size()))
        return;
    
    const auto& match = panel.matchResults[static_cast<size_t>(rowNumber)];
    
    // Background
    if (rowIsSelected)
    {
        g.setColour(panel.accentColour.withAlpha(0.3f));
        g.fillRect(0, 0, width, height);
        g.setColour(panel.accentColour);
        g.fillRect(0, 0, 3, height);
    }
    else if (rowNumber % 2 == 1)
    {
        g.setColour(juce::Colour(0xFF252525));
        g.fillRect(0, 0, width, height);
    }
    
    // Match score (percentage)
    juce::Colour scoreColour = match.matchScore > 50 ? juce::Colour(0xFF00FF00) :
                               match.matchScore > 25 ? juce::Colour(0xFFFFFF00) :
                               panel.accentColour;
    g.setColour(scoreColour);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(juce::String(static_cast<int>(match.matchScore)) + "%", 
               8, 0, 35, height, juce::Justification::centredLeft);
    
    // Category
    g.setColour(panel.dimTextColour);
    g.setFont(juce::Font(10.0f));
    g.drawText(match.categoryName, 50, 0, 100, height, juce::Justification::centredLeft);
    
    // Groove name
    g.setColour(rowIsSelected ? panel.textColour : panel.dimTextColour);
    g.setFont(juce::Font(12.0f));
    g.drawText(match.grooveName, 155, 0, width - 160, height, juce::Justification::centredLeft);
}

void BandmatePanel::MatchesListModel::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    panel.selectedMatchIndex = row;
}

void BandmatePanel::MatchesListModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    panel.selectedMatchIndex = row;
    panel.addSelectedMatchToComposer();
}


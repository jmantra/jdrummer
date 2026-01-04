/*
    BandmatePanel.cpp
    =================
    
    Implementation of the Groove Matcher feature panel.
*/

#include "BandmatePanel.h"
#include "../PluginProcessor.h"
#include <cmath>

BandmatePanel::BandmatePanel()
    : matchesListBox(*this),
      matchesListModel(*this)
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
    browseButton.setTooltip("Browse for audio file.\nTip: Press Ctrl+H to show hidden files.");
    browseButton.onClick = [this]() {
        // Store the chooser as a member to prevent it from being destroyed before callback
        // On Linux plugins, native file dialogs often fail, so we force non-native mode
        #if JUCE_LINUX
        constexpr bool useNativeDialog = false;  // Force JUCE's built-in browser on Linux
        #else
        constexpr bool useNativeDialog = true;
        #endif
        
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select an audio file (Ctrl+H for hidden files)",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav;*.mp3;*.aiff;*.flac;*.ogg",
            useNativeDialog);
        
        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        
        fileChooser->launchAsync(flags,
            [this](const juce::FileChooser& fc) {
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
        tempoComboBox.clear();
        customBpmEditor.clear();
        selectedBpm = 0.0;
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
    
    // Tempo ComboBox (for selecting detected/alternative tempos)
    tempoComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF2A2A4A));
    tempoComboBox.setColour(juce::ComboBox::textColourId, accentColour);
    tempoComboBox.setColour(juce::ComboBox::outlineColourId, accentColour.withAlpha(0.5f));
    tempoComboBox.onChange = [this]() { updateTempoSelection(); };
    addAndMakeVisible(tempoComboBox);
    
    // Custom BPM text editor
    customBpmEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF2A2A4A));
    customBpmEditor.setColour(juce::TextEditor::textColourId, textColour);
    customBpmEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xFF444444));
    customBpmEditor.setJustification(juce::Justification::centred);
    customBpmEditor.setInputRestrictions(6, "0123456789.");
    customBpmEditor.setTooltip("Enter custom BPM (30-300)");
    addAndMakeVisible(customBpmEditor);
    
    // Use custom BPM button
    useCustomBpmButton.setButtonText("Use");
    useCustomBpmButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A5A2A));
    useCustomBpmButton.setColour(juce::TextButton::textColourOffId, textColour);
    useCustomBpmButton.onClick = [this]() { applyCustomBpm(); };
    addAndMakeVisible(useCustomBpmButton);
    
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
    
    // Sub-tab buttons (Matches vs All Grooves)
    matchesTabButton.setButtonText("Matches");
    matchesTabButton.setColour(juce::TextButton::buttonColourId, accentColour);
    matchesTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFFFFFF));
    matchesTabButton.onClick = [this]() { showSubTab(0); };
    addAndMakeVisible(matchesTabButton);
    
    allGroovesTabButton.setButtonText("All Grooves");
    allGroovesTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
    allGroovesTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFAAAAAA));
    allGroovesTabButton.onClick = [this]() { showSubTab(1); };
    addAndMakeVisible(allGroovesTabButton);
    
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
    
    // All Grooves browser (hidden by default, shown when "All Grooves" tab is selected)
    allGroovesBrowser.setVisible(false);
    allGroovesBrowser.onGrooveAddToComposer = [this](int categoryIndex, int grooveIndex, int barCount) {
        if (grooveManager != nullptr)
        {
            grooveManager->addToComposer(categoryIndex, grooveIndex, barCount);
            grooveComposer.refresh();
        }
    };
    allGroovesBrowser.onGrooveDoubleClicked = [this](int categoryIndex, int grooveIndex) {
        // Preview the groove on double-click
        if (grooveManager != nullptr)
        {
            double bpm = getSelectedBPM();
            if (bpm > 0)
            {
                grooveManager->setPreviewBPM(bpm);
            }
            grooveManager->setLooping(true);
            grooveManager->startPlayback(categoryIndex, grooveIndex);
        }
    };
    allGroovesBrowser.onGrooveDragStarted = [this](int categoryIndex, int grooveIndex) {
        // Handle drag through BandmatePanel's DragAndDropContainer
        startGrooveBrowserDrag(categoryIndex, grooveIndex);
    };
    addAndMakeVisible(allGroovesBrowser);
    
    // Composer
    addAndMakeVisible(grooveComposer);
    
    // Setup composer callbacks
    grooveComposer.onPlayClicked = [this]() {
        if (grooveManager != nullptr)
        {
            // Set the selected BPM before starting composer playback
            double bpm = getSelectedBPM();
            if (bpm > 0)
            {
                grooveManager->setPreviewBPM(bpm);
            }
            
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
        tempoComboBox.setVisible(true);
        customBpmEditor.setVisible(true);
        useCustomBpmButton.setVisible(true);
        statusLabel.setVisible(true);
        
        auto topRow = dropContent.removeFromTop(22);
        fileNameLabel.setBounds(topRow.removeFromLeft(topRow.getWidth() - 60));
        clearButton.setBounds(topRow.removeFromRight(55));
        
        dropContent.removeFromTop(3);
        
        // Tempo row: ComboBox + custom BPM editor + Use button
        auto tempoRow = dropContent.removeFromTop(28);
        auto tempoRowCentered = tempoRow.withSizeKeepingCentre(280, 26);
        tempoComboBox.setBounds(tempoRowCentered.removeFromLeft(140));
        tempoRowCentered.removeFromLeft(5);
        customBpmEditor.setBounds(tempoRowCentered.removeFromLeft(60));
        tempoRowCentered.removeFromLeft(5);
        useCustomBpmButton.setBounds(tempoRowCentered.removeFromLeft(50));
        
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
        tempoComboBox.setVisible(false);
        customBpmEditor.setVisible(false);
        useCustomBpmButton.setVisible(false);
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
    
    // Sub-tab buttons row (Matches / All Grooves)
    auto subTabRow = bounds.removeFromTop(28);
    matchesTabButton.setBounds(subTabRow.removeFromLeft(100));
    subTabRow.removeFromLeft(5);
    allGroovesTabButton.setBounds(subTabRow.removeFromLeft(100));
    
    bounds.removeFromTop(8);
    
    // Split remaining space between content area and composer
    auto contentArea = bounds.removeFromTop(bounds.getHeight() - 85);
    
    if (currentSubTab == 0)
    {
        // Show Matches view
        matchesLabel.setVisible(true);
        matchesListBox.setVisible(true);
        barCountLabel.setVisible(true);
        barCountComboBox.setVisible(true);
        addToComposerButton.setVisible(true);
        allGroovesBrowser.setVisible(false);
        
        // Matches section
        matchesLabel.setBounds(contentArea.removeFromTop(20));
        contentArea.removeFromTop(5);
        
        // Bottom row of matches area: bar count and add button
        auto matchesBottom = contentArea.removeFromBottom(28);
        barCountLabel.setBounds(matchesBottom.removeFromLeft(35));
        matchesBottom.removeFromLeft(5);
        barCountComboBox.setBounds(matchesBottom.removeFromLeft(70));
        matchesBottom.removeFromLeft(10);
        addToComposerButton.setBounds(matchesBottom.removeFromLeft(140));
        
        contentArea.removeFromBottom(5);
        matchesListBox.setBounds(contentArea);
    }
    else
    {
        // Show All Grooves view
        matchesLabel.setVisible(false);
        matchesListBox.setVisible(false);
        barCountLabel.setVisible(false);
        barCountComboBox.setVisible(false);
        addToComposerButton.setVisible(false);
        allGroovesBrowser.setVisible(true);
        
        allGroovesBrowser.setBounds(contentArea);
    }
    
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
    allGroovesBrowser.setGrooveManager(manager);
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
        tempoComboBox.clear();
        customBpmEditor.clear();
        selectedBpm = 0.0;
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
    
    // Capture the groove manager pointer for use in background thread
    GrooveManager* gm = grooveManager;
    
    // Run analysis AND groove matching in background thread
    // This prevents UI blocking from heavy I/O (loading all MIDI files)
    juce::Thread::launch([this, gm]() {
        bool success = audioAnalyzer.analyzeAudio();
        
        // Also find matching grooves in the background thread (heavy I/O)
        std::vector<GrooveMatch> matches;
        if (success && gm != nullptr)
        {
            matches = audioAnalyzer.findMatchingGrooves(*gm, 15);
        }
        
        juce::MessageManager::callAsync([this, success, matches = std::move(matches)]() mutable {
            stopTimer();
            isAnalyzing = false;
            
            if (success)
            {
                // Store the matches computed in background thread
                matchResults = std::move(matches);
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
    if (isAnalyzing)
    {
        // Update progress bar during analysis
        progressValue = audioAnalyzer.getAnalysisProgress() / 100.0;
        repaint();
    }
    else if (isPlayingAudio || isPlayingGroove)
    {
        // Repaint for playback indicator
        repaint();
    }
    else
    {
        // Nothing active - stop the timer to save CPU
        stopTimer();
    }
}

void BandmatePanel::onAnalysisComplete()
{
    const auto& pattern = audioAnalyzer.getDetectedPattern();
    
    // Populate tempo ComboBox with detected and alternative tempos
    tempoComboBox.clear();
    int itemId = 1;
    
    // Add primary detected tempo (first item, will be default selected)
    tempoComboBox.addItem(juce::String(pattern.bpm, 1) + " BPM (detected)", itemId++);
    
    // Add alternative tempos from candidates (skip first one since it's the primary)
    const auto& alternatives = pattern.alternativeBpms;
    for (size_t i = 1; i < alternatives.size(); ++i)
    {
        double altBpm = alternatives[i];
        // Only add if significantly different from primary (> 5 BPM difference)
        if (std::abs(altBpm - pattern.bpm) > 5.0)
        {
            tempoComboBox.addItem(juce::String(altBpm, 1) + " BPM", itemId++);
        }
    }
    
    // Add "Custom..." option at the end
    tempoComboBox.addItem("Custom...", 100);
    
    // Select the primary tempo by default
    tempoComboBox.setSelectedId(1);
    selectedBpm = pattern.bpm;
    
    statusLabel.setText(juce::String(pattern.onsetTimesBeats.size()) + " beats detected", 
                       juce::dontSendNotification);
    
    // matchResults was already populated in background thread (startAnalysis)
    // Just update the UI here
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
    
    // Set the selected BPM for groove playback (may be detected, alternative, or custom)
    double bpm = getSelectedBPM();
    if (bpm > 0)
    {
        grooveManager->setPreviewBPM(bpm);
    }
    
    // Start audio playback through processor
    auto* audioBuffer = audioAnalyzer.getAudioBuffer();
    if (audioBuffer != nullptr)
    {
        audioProcessor->setPreviewAudio(audioBuffer, audioAnalyzer.getAudioSampleRate());
        audioProcessor->startPreviewPlayback();
        isPlayingAudio = true;
    }
    
    // If composer has items, play the composer; otherwise play the selected match
    const auto& composerItems = grooveManager->getComposerItems();
    if (!composerItems.empty())
    {
        // Play composer content at the selected BPM
        grooveManager->startComposerPlayback();
        grooveComposer.setPlaying(true);
        isPlayingGroove = true;
    }
    else if (selectedMatchIndex >= 0 && selectedMatchIndex < static_cast<int>(matchResults.size()))
    {
        // Fall back to playing the selected match
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
    
    // Set the selected BPM for groove playback (may be detected, alternative, or custom)
    double bpm = getSelectedBPM();
    if (bpm > 0)
    {
        grooveManager->setPreviewBPM(bpm);
    }
    
    // If composer has items, play the composer; otherwise play the selected match
    const auto& composerItems = grooveManager->getComposerItems();
    if (!composerItems.empty())
    {
        // Play composer content at the selected BPM
        grooveManager->startComposerPlayback();
        grooveComposer.setPlaying(true);
        isPlayingGroove = true;
        startTimerHz(30);
    }
    else if (selectedMatchIndex >= 0 && selectedMatchIndex < static_cast<int>(matchResults.size()))
    {
        // Fall back to playing the selected match
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
    // Stop the repaint timer to save CPU
    stopTimer();
    
    if (audioProcessor != nullptr)
    {
        audioProcessor->stopPreviewPlayback();
    }
    isPlayingAudio = false;
    
    if (grooveManager != nullptr)
    {
        grooveManager->stopPlayback();          // Stop single groove playback
        grooveManager->stopComposerPlayback();  // Stop composer playback
        grooveManager->useDAWTiming();          // Reset to DAW timing after preview
    }
    grooveComposer.setPlaying(false);  // Update composer UI state
    isPlayingGroove = false;
    
    repaint();
}

double BandmatePanel::getSelectedBPM() const
{
    // Return the currently selected BPM (either from combo box or custom input)
    if (selectedBpm > 0.0)
        return selectedBpm;
    
    // Fall back to detected pattern BPM
    return audioAnalyzer.getDetectedPattern().bpm;
}

void BandmatePanel::updateTempoSelection()
{
    int selectedId = tempoComboBox.getSelectedId();
    
    if (selectedId == 100)
    {
        // "Custom..." selected - highlight the custom BPM editor
        customBpmEditor.grabKeyboardFocus();
        return;
    }
    
    // Get the BPM from the selected item text
    juce::String selectedText = tempoComboBox.getText();
    
    // Parse the BPM value from the text (format: "XXX.X BPM" or "XXX.X BPM (detected)")
    int bpmEndIdx = selectedText.indexOf(" BPM");
    if (bpmEndIdx > 0)
    {
        juce::String bpmStr = selectedText.substring(0, bpmEndIdx);
        selectedBpm = bpmStr.getDoubleValue();
        DBG("BandmatePanel: Selected tempo: " + juce::String(selectedBpm, 1) + " BPM");
    }
}

void BandmatePanel::applyCustomBpm()
{
    juce::String customText = customBpmEditor.getText().trim();
    
    if (customText.isEmpty())
    {
        statusLabel.setText("Enter a BPM value", juce::dontSendNotification);
        return;
    }
    
    double customBpm = customText.getDoubleValue();
    
    // Validate the custom BPM (reasonable range: 30-300)
    if (customBpm < 30.0 || customBpm > 300.0)
    {
        statusLabel.setText("BPM must be between 30-300", juce::dontSendNotification);
        return;
    }
    
    selectedBpm = customBpm;
    
    // Add custom BPM to the combo box if not already present
    bool found = false;
    for (int i = 0; i < tempoComboBox.getNumItems(); ++i)
    {
        if (tempoComboBox.getItemText(i).startsWith(juce::String(customBpm, 1)))
        {
            found = true;
            tempoComboBox.setSelectedItemIndex(i);
            break;
        }
    }
    
    if (!found)
    {
        // Insert custom BPM before the "Custom..." option
        int customItemId = tempoComboBox.getNumItems(); // Use next available ID
        tempoComboBox.addItem(juce::String(customBpm, 1) + " BPM (custom)", customItemId);
        tempoComboBox.setSelectedId(customItemId);
    }
    
    statusLabel.setText("Using " + juce::String(customBpm, 1) + " BPM", juce::dontSendNotification);
    DBG("BandmatePanel: Applied custom BPM: " + juce::String(customBpm, 1));
}

void BandmatePanel::showSubTab(int index)
{
    currentSubTab = index;
    
    // Update tab button styles
    if (index == 0)
    {
        // Matches tab active
        matchesTabButton.setColour(juce::TextButton::buttonColourId, accentColour);
        matchesTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFFFFFF));
        allGroovesTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
        allGroovesTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFAAAAAA));
    }
    else
    {
        // All Grooves tab active
        matchesTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
        matchesTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFAAAAAA));
        allGroovesTabButton.setColour(juce::TextButton::buttonColourId, accentColour);
        allGroovesTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFFFFFF));
    }
    
    // Trigger layout update (visibility handled in resized)
    resized();
    repaint();
}

// DraggableMatchesListBox implementation
BandmatePanel::DraggableMatchesListBox::DraggableMatchesListBox(BandmatePanel& owner)
    : panel(owner)
{
    addMouseListener(&childListener, true);
}

BandmatePanel::DraggableMatchesListBox::~DraggableMatchesListBox()
{
    removeMouseListener(&childListener);
}

void BandmatePanel::DraggableMatchesListBox::mouseDrag(const juce::MouseEvent& e)
{
    if (!dragStarted && e.getDistanceFromDragStart() > 8)
    {
        int row = getRowContainingPosition(e.getMouseDownX(), e.getMouseDownY());
        startDragFromRow(row);
        if (dragStarted) return;
    }
    
    juce::ListBox::mouseDrag(e);
}

void BandmatePanel::DraggableMatchesListBox::mouseUp(const juce::MouseEvent& e)
{
    dragStarted = false;
    juce::ListBox::mouseUp(e);
}

void BandmatePanel::DraggableMatchesListBox::startDragFromRow(int row)
{
    if (row >= 0 && row < static_cast<int>(panel.matchResults.size()))
    {
        dragStarted = true;
        panel.selectedMatchIndex = row;
        selectRow(row);
        panel.startMatchExternalDrag();
    }
}

// ChildMouseListener - intercepts drags from list row child components
void BandmatePanel::DraggableMatchesListBox::ChildMouseListener::mouseDrag(const juce::MouseEvent& e)
{
    if (!listBox.dragStarted && e.getDistanceFromDragStart() > 8)
    {
        auto localPos = listBox.getLocalPoint(e.eventComponent, e.getMouseDownPosition());
        int row = listBox.getRowContainingPosition(localPos.x, localPos.y);
        listBox.startDragFromRow(row);
    }
}

void BandmatePanel::DraggableMatchesListBox::ChildMouseListener::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    listBox.dragStarted = false;
}

void BandmatePanel::startMatchExternalDrag()
{
    if (isMatchDragging)
        return;
    
    if (grooveManager == nullptr || selectedMatchIndex < 0 || 
        selectedMatchIndex >= static_cast<int>(matchResults.size()))
    {
        DBG("BandmatePanel: Cannot start drag - no match selected");
        return;
    }
    
    const auto& match = matchResults[static_cast<size_t>(selectedMatchIndex)];
    
    // Export the selected match groove to a temp file
    juce::File midiFile = grooveManager->exportGrooveToTempFile(match.categoryIndex, match.grooveIndex);
    
    DBG("BandmatePanel: Starting external drag with file: " + midiFile.getFullPathName());
    
    if (midiFile.existsAsFile())
    {
        isMatchDragging = true;
        
        // Copy file path to clipboard as fallback
        juce::SystemClipboard::copyTextToClipboard(midiFile.getFullPathName());
        DBG("BandmatePanel: Copied to clipboard: " + midiFile.getFullPathName());
        
        juce::StringArray files;
        files.add(midiFile.getFullPathName());
        
        performExternalDragDropOfFiles(files, true,
            nullptr, [this]() {
                isMatchDragging = false;
            });
    }
    else
    {
        DBG("BandmatePanel: Failed to export match for drag");
    }
}

void BandmatePanel::startGrooveBrowserDrag(int categoryIndex, int grooveIndex)
{
    if (isMatchDragging)
        return;
    
    if (grooveManager == nullptr || categoryIndex < 0 || grooveIndex < 0)
    {
        DBG("BandmatePanel: Cannot start groove browser drag - invalid indices");
        return;
    }
    
    // Export the groove to a temp file
    juce::File midiFile = grooveManager->exportGrooveToTempFile(categoryIndex, grooveIndex);
    
    DBG("BandmatePanel: Starting groove browser drag with file: " + midiFile.getFullPathName());
    
    if (midiFile.existsAsFile())
    {
        isMatchDragging = true;
        
        // Copy file path to clipboard as fallback
        juce::SystemClipboard::copyTextToClipboard(midiFile.getFullPathName());
        DBG("BandmatePanel: Copied to clipboard: " + midiFile.getFullPathName());
        
        juce::StringArray files;
        files.add(midiFile.getFullPathName());
        
        performExternalDragDropOfFiles(files, true,
            nullptr, [this]() {
                isMatchDragging = false;
            });
    }
    else
    {
        DBG("BandmatePanel: Failed to export groove for drag");
    }
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



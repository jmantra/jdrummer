/*
    GroovesPanel.cpp
    ================
    
    Implementation of the main grooves panel.
*/

#include "GroovesPanel.h"
#include "../PluginProcessor.h"

GroovesPanel::GroovesPanel()
{
    // Add child components
    addAndMakeVisible(grooveBrowser);
    addAndMakeVisible(grooveComposer);
    
    // Preview button
    previewButton.setButtonText("▶ Preview");
    previewButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A5A2A));
    previewButton.setColour(juce::TextButton::textColourOffId, textColour);
    previewButton.onClick = [this]() {
        int cat = grooveBrowser.getSelectedCategoryIndex();
        int groove = grooveBrowser.getSelectedGrooveIndex();
        if (cat >= 0 && groove >= 0)
            previewGroove(cat, groove);
    };
    addAndMakeVisible(previewButton);
    
    // Stop button
    stopButton.setButtonText("■ Stop");
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF5A2A2A));
    stopButton.setColour(juce::TextButton::textColourOffId, textColour);
    stopButton.onClick = [this]() { stopPreview(); };
    addAndMakeVisible(stopButton);
    
    // Loop toggle
    loopToggle.setButtonText("Loop");
    loopToggle.setColour(juce::ToggleButton::textColourId, textColour);
    loopToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFF00BFFF));
    loopToggle.setToggleState(true, juce::dontSendNotification);
    loopToggle.onClick = [this]() {
        if (grooveManager != nullptr)
            grooveManager->setLooping(loopToggle.getToggleState());
    };
    addAndMakeVisible(loopToggle);
    
    // BPM label (will be updated from DAW)
    bpmLabel.setText("BPM: ---", juce::dontSendNotification);
    bpmLabel.setFont(juce::Font(12.0f));
    bpmLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF888888));
    bpmLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(bpmLabel);
    
    setupCallbacks();
}

GroovesPanel::~GroovesPanel()
{
}

void GroovesPanel::paint(juce::Graphics& g)
{
    // Gradient background matching the main editor
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
}

void GroovesPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Top control bar
    auto topBar = bounds.removeFromTop(35);
    previewButton.setBounds(topBar.removeFromLeft(100));
    topBar.removeFromLeft(10);
    stopButton.setBounds(topBar.removeFromLeft(80));
    topBar.removeFromLeft(20);
    loopToggle.setBounds(topBar.removeFromLeft(70));
    
    bpmLabel.setBounds(topBar.removeFromRight(100));
    
    bounds.removeFromTop(10);
    
    // Composer at the bottom
    auto composerBounds = bounds.removeFromBottom(80);
    grooveComposer.setBounds(composerBounds);
    
    bounds.removeFromBottom(10);
    
    // Browser takes the rest
    grooveBrowser.setBounds(bounds);
}

void GroovesPanel::setProcessor(JdrummerAudioProcessor* processor)
{
    audioProcessor = processor;
}

void GroovesPanel::setGrooveManager(GrooveManager* manager)
{
    grooveManager = manager;
    grooveBrowser.setGrooveManager(manager);
    grooveComposer.setGrooveManager(manager);
    
    if (manager != nullptr)
    {
        manager->setLooping(loopToggle.getToggleState());
    }
}

void GroovesPanel::refresh()
{
    grooveBrowser.refresh();
    grooveComposer.refresh();
}

void GroovesPanel::setupCallbacks()
{
    // When a groove is double-clicked, preview it
    grooveBrowser.onGrooveDoubleClicked = [this](int categoryIndex, int grooveIndex) {
        previewGroove(categoryIndex, grooveIndex);
    };
    
    // When "Add to Composer" is clicked
    grooveBrowser.onGrooveAddToComposer = [this](int categoryIndex, int grooveIndex, int barCount) {
        if (grooveManager != nullptr)
        {
            grooveManager->addToComposer(categoryIndex, grooveIndex, barCount);
            grooveComposer.refresh();
        }
    };
    
    // Composer play button
    grooveComposer.onPlayClicked = [this]() {
        if (grooveManager != nullptr)
        {
            grooveManager->startComposerPlayback();
            grooveComposer.setPlaying(true);
        }
    };
    
    // Composer stop button
    grooveComposer.onStopClicked = [this]() {
        if (grooveManager != nullptr)
        {
            grooveManager->stopComposerPlayback();
            grooveComposer.setPlaying(false);
        }
    };
    
    // Composer clear button
    grooveComposer.onClearClicked = [this]() {
        if (grooveManager != nullptr)
        {
            grooveManager->clearComposer();
            grooveComposer.refresh();
        }
    };
}

void GroovesPanel::previewGroove(int categoryIndex, int grooveIndex)
{
    if (grooveManager != nullptr)
    {
        grooveManager->startPlayback(categoryIndex, grooveIndex);
    }
}

void GroovesPanel::stopPreview()
{
    if (grooveManager != nullptr)
    {
        grooveManager->stopPlayback();
        grooveManager->stopComposerPlayback();
        grooveComposer.setPlaying(false);
    }
}

void GroovesPanel::updatePlayingState()
{
    if (grooveManager != nullptr)
    {
        grooveComposer.setPlaying(grooveManager->isComposerPlaying());
    }
}


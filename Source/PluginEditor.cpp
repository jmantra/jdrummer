/*
    PluginEditor.cpp
    ================
    
    Implementation of the plugin's user interface.
    
    KEY JUCE UI CONCEPTS
    --------------------
    
    1. COMPONENTS: Everything visible is a Component
       - Components form a parent-child tree
       - Parents clip and position their children
       - addAndMakeVisible() adds a child and makes it visible
    
    2. PAINTING: Override paint() to draw custom graphics
       - Never call paint() directly - call repaint()
       - JUCE decides when to actually repaint
       - Keep paint() fast for smooth UI
    
    3. LAYOUT: Override resized() to position children
       - Called when component size changes
       - Use setBounds() to position children
       - Use getLocalBounds() to get available space
    
    4. EVENTS: Mouse/keyboard events flow through the tree
       - Events go to the topmost component at that position
       - Components can block or pass events to parents
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

/*
    CONSTRUCTOR
    -----------
    Sets up all the UI components.
    
    INITIALIZER LIST ORDER
    ----------------------
    Members are initialized in the order they're DECLARED in the class,
    not the order they appear in the initializer list!
    
    AudioProcessorEditor(*this) passes our processor to the parent class,
    which stores it and makes it available via getAudioProcessor().
*/
JdrummerAudioProcessorEditor::JdrummerAudioProcessorEditor(JdrummerAudioProcessor& p)
    : AudioProcessorEditor(&p),  // Call parent constructor with pointer to processor
      audioProcessor(p)          // Store reference to processor
{
    /*
        LABEL SETUP
        -----------
        Labels display text. They're simple but commonly used components.
        
        dontSendNotification means "don't trigger any callbacks" -
        useful during initialization when callbacks might not be ready.
    */
    titleLabel.setText("jdrummer", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(28.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF00BFFF));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);  // Add to component tree and make visible
    
    // Kit label
    kitLabel.setText("Kit:", juce::dontSendNotification);
    kitLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    kitLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFCCCCCC));
    addAndMakeVisible(kitLabel);
    
    /*
        COMBOBOX SETUP
        --------------
        ComboBox is a dropdown menu for selecting from a list.
        
        setColour() changes specific color IDs for customization.
        Each component type has its own ColourIds enum.
    */
    kitComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF2A2A2A));
    kitComboBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFEEEEEE));
    kitComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xFF444444));
    kitComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xFF00BFFF));
    
    /*
        LAMBDA CALLBACKS
        ----------------
        onChange is a std::function that gets called when selection changes.
        
        We use a LAMBDA EXPRESSION to define the callback inline:
        
        [this]() { ... }
        
        - [this] = CAPTURE LIST - makes 'this' pointer available inside the lambda
        - () = PARAMETERS - this lambda takes no parameters
        - { ... } = BODY - code to execute
        
        Lambdas are anonymous functions - functions without a name.
        They're great for callbacks because you don't need a separate method.
    */
    kitComboBox.onChange = [this]() { onKitComboBoxChanged(); };
    addAndMakeVisible(kitComboBox);
    
    // Populate the kit dropdown with available soundfonts
    populateKitComboBox();
    
    /*
        TAB BUTTONS
        -----------
        Allow switching between Drum Kit and Grooves views.
    */
    drumKitTabButton.setButtonText("DRUM KIT");
    drumKitTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF00BFFF));
    drumKitTabButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF00BFFF));
    drumKitTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFFFFFF));
    drumKitTabButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFFFFFFF));
    drumKitTabButton.onClick = [this]() { showTab(0); };
    addAndMakeVisible(drumKitTabButton);
    
    groovesTabButton.setButtonText("GROOVES");
    groovesTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
    groovesTabButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF00BFFF));
    groovesTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFAAAAAA));
    groovesTabButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFFFFFFF));
    groovesTabButton.onClick = [this]() { showTab(1); };
    addAndMakeVisible(groovesTabButton);
    
    bandmateTabButton.setButtonText("MATCH");
    bandmateTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
    bandmateTabButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF00BFFF));
    bandmateTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFAAAAAA));
    bandmateTabButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFFFFFFF));
    bandmateTabButton.onClick = [this]() { showTab(2); };
    addAndMakeVisible(bandmateTabButton);
    
    /*
        ADD CHILD COMPONENTS
        --------------------
        These custom components (DrumPadGrid, PadControls) are added
        the same way as built-in components.
        
        They're declared as member variables, so they exist as long
        as the editor exists.
    */
    addAndMakeVisible(drumPadGrid);
    addAndMakeVisible(padControls);
    
    // Setup the grooves panel
    groovesPanel.setProcessor(&audioProcessor);
    groovesPanel.setGrooveManager(&audioProcessor.getGrooveManager());
    addChildComponent(groovesPanel);  // Hidden initially
    
    // Setup the bandmate panel
    bandmatePanel.setProcessor(&audioProcessor);
    bandmatePanel.setGrooveManager(&audioProcessor.getGrooveManager());
    addChildComponent(bandmatePanel);  // Hidden initially
    
    // Connect all the callbacks between components
    setupCallbacks();
    
    // Initialize pad controls with the first pad's settings
    updatePadControlsForSelectedPad();
    
    // Start with Drum Kit tab selected
    showTab(0);
    
    /*
        SET EDITOR SIZE
        ---------------
        This sets the plugin window size.
        The DAW will create a window of this size.
    */
    setSize(950, 650);  // Increased size for tabs and bandmate panel
    
    /*
        START TIMER
        -----------
        startTimerHz(30) means timerCallback() will be called 30 times per second.
        We use this to check for MIDI notes triggered from the DAW.
    */
    startTimerHz(30);
}

JdrummerAudioProcessorEditor::~JdrummerAudioProcessorEditor()
{
    // Stop the timer before destruction
    stopTimer();
}

/*
    POPULATE KIT COMBOBOX
    ---------------------
    Fills the dropdown with available soundfonts.
*/
void JdrummerAudioProcessorEditor::populateKitComboBox()
{
    // Clear existing items without triggering onChange
    kitComboBox.clear(juce::dontSendNotification);
    
    // Get list of kits from the processor
    auto kits = audioProcessor.getSoundFontManager().getAvailableKits();
    auto currentKit = audioProcessor.getSoundFontManager().getCurrentKitName();
    
    int selectedIndex = 0;
    int id = 1;  // ComboBox items need unique IDs starting from 1
    
    for (int i = 0; i < kits.size(); ++i)
    {
        kitComboBox.addItem(kits[i], id++);
        
        // Remember which index is the current kit
        if (kits[i] == currentKit)
            selectedIndex = i;
    }
    
    // Select the current kit without triggering onChange
    if (kits.size() > 0)
        kitComboBox.setSelectedItemIndex(selectedIndex, juce::dontSendNotification);
}

/*
    KIT COMBOBOX CHANGED
    --------------------
    Called when user selects a different kit.
*/
void JdrummerAudioProcessorEditor::onKitComboBoxChanged()
{
    auto kitName = kitComboBox.getText();
    if (kitName.isNotEmpty())
    {
        // Tell the processor to load the new kit
        audioProcessor.getSoundFontManager().loadKit(kitName);
    }
}

/*
    SETUP CALLBACKS
    ---------------
    This wires up all the communication between components.
    
    OBSERVER PATTERN
    ----------------
    Instead of components directly calling each other, we use callbacks:
    - Component A has a callback (std::function)
    - Component B sets that callback to its own method
    - When A's event happens, A calls the callback
    - B's method runs without A knowing about B
    
    Benefits:
    - Loose coupling (components don't know about each other)
    - Easy to change behavior
    - Components can be tested independently
*/
void JdrummerAudioProcessorEditor::setupCallbacks()
{
    /*
        PAD PRESSED CALLBACK
        --------------------
        When a pad is clicked, trigger the note in the processor.
        
        [this] captures the 'this' pointer so we can access audioProcessor.
    */
    drumPadGrid.onPadPressed = [this](int note, float velocity) {
        audioProcessor.triggerNote(note, velocity);
    };
    
    drumPadGrid.onPadReleased = [this](int note) {
        audioProcessor.releaseNote(note);
    };
    
    /*
        PAD SELECTED CALLBACK
        ---------------------
        When a pad is clicked, also update the controls to show that pad's settings.
        
        juce::ignoreUnused() suppresses "unused parameter" warnings.
    */
    drumPadGrid.onPadSelected = [this](int note) {
        juce::ignoreUnused(note);
        updatePadControlsForSelectedPad();
    };
    
    /*
        VOLUME/PAN CALLBACKS
        --------------------
        When user adjusts the sliders, update the processor.
    */
    padControls.onVolumeChanged = [this](int note, float volume) {
        audioProcessor.getSoundFontManager().setNoteVolume(note, volume);
    };
    
    padControls.onPanChanged = [this](int note, float pan) {
        audioProcessor.getSoundFontManager().setNotePan(note, pan);
    };
    
    padControls.onMuteChanged = [this](int note, bool muted) {
        audioProcessor.getSoundFontManager().setNoteMute(note, muted);
    };
    
    /*
        STATE RESTORATION CALLBACK
        --------------------------
        When the processor loads state (from a saved project),
        it calls this to update the UI.
        
        MessageManager::callAsync() schedules code to run on the
        message thread (UI thread). This is important because
        state restoration might happen on a different thread.
    */
    // Use SafePointer to ensure the editor still exists when the callback fires
    juce::Component::SafePointer<JdrummerAudioProcessorEditor> safeThis(this);
    audioProcessor.onKitLoaded = [safeThis]() {
        juce::MessageManager::callAsync([safeThis]() {
            if (safeThis != nullptr)
            {
                safeThis->populateKitComboBox();
                safeThis->updatePadControlsForSelectedPad();
            }
        });
    };
}

/*
    UPDATE PAD CONTROLS FOR SELECTED PAD
    ------------------------------------
    Syncs the control panel with the currently selected pad's settings.
*/
void JdrummerAudioProcessorEditor::updatePadControlsForSelectedPad()
{
    int note = drumPadGrid.getSelectedNote();
    juce::String padName = getPadNameForNote(note);
    
    // Update the controls to show this pad
    padControls.setSelectedPad(note, padName);
    
    // Get current values from the processor
    padControls.setVolume(audioProcessor.getSoundFontManager().getNoteVolume(note));
    padControls.setPan(audioProcessor.getSoundFontManager().getNotePan(note));
    padControls.setMute(audioProcessor.getSoundFontManager().getNoteMute(note));
}

/*
    GET PAD NAME FOR NOTE
    ---------------------
    Maps MIDI note numbers to human-readable drum names.
    
    STATIC LOCAL VARIABLE
    ---------------------
    'static const' means this map is created once and shared by all calls.
    Without 'static', it would be recreated every time the function is called.
*/
juce::String JdrummerAudioProcessorEditor::getPadNameForNote(int note)
{
    static const std::map<int, juce::String> noteNames = {
        { 36, "Kick" },
        { 37, "Rim" },
        { 38, "Snare" },
        { 39, "Clap" },
        { 40, "Snare 2" },
        { 41, "Lo Tom" },
        { 42, "HH Closed" },
        { 43, "Mid Tom" },
        { 44, "HH Pedal" },
        { 45, "Hi Tom" },
        { 46, "HH Open" },
        { 47, "Mid Tom 2" },
        { 48, "Hi Tom 2" },
        { 49, "Crash" },
        { 51, "Ride" },
        { 53, "Ride Bell" }
    };
    
    // .find() looks up the note in the map
    auto it = noteNames.find(note);
    if (it != noteNames.end())
        return it->second;  // Found - return the name
    
    // Not in our map - return a generic name
    return "Note " + juce::String(note);
}

/*
    TIMER CALLBACK
    --------------
    Called 30 times per second to check for MIDI notes from the DAW.
    
    We use a polling approach because the audio thread can't directly
    update the UI - that would cause threading issues.
*/
void JdrummerAudioProcessorEditor::timerCallback()
{
    // Get notes that were triggered since last check
    auto triggeredNotes = audioProcessor.getAndClearTriggeredNotes();
    
    // Light up the corresponding pads
    for (int note : triggeredNotes)
    {
        drumPadGrid.triggerPadVisual(note);
    }
}

/*
    PAINT
    -----
    Draw the editor's background.
    
    This is called by JUCE when the component needs to be drawn.
    Don't call paint() directly - call repaint() to request a redraw.
*/
void JdrummerAudioProcessorEditor::paint(juce::Graphics& g)
{
    /*
        GRADIENT BACKGROUND
        -------------------
        ColourGradient creates a smooth transition between colors.
        
        Parameters:
        - Start color and position
        - End color and position
        - false = linear gradient (not radial)
    */
    juce::ColourGradient gradient(
        juce::Colour(0xFF1A1A2E), 0.0f, 0.0f,     // Dark blue at top
        juce::Colour(0xFF16213E), 0.0f, static_cast<float>(getHeight()),  // Darker at bottom
        false
    );
    g.setGradientFill(gradient);
    g.fillAll();  // Fill the entire component
    
    /*
        SCANLINE EFFECT
        ---------------
        Draw subtle horizontal lines for a tech/retro look.
        0x08FFFFFF = white with very low alpha (nearly transparent)
    */
    g.setColour(juce::Colour(0x08FFFFFF));
    for (int y = 0; y < getHeight(); y += 4)
    {
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));
    }
    
    // Header separator line (cyan with transparency)
    g.setColour(juce::Colour(0xFF00BFFF).withAlpha(0.3f));
    g.drawHorizontalLine(70, 20.0f, static_cast<float>(getWidth()) - 20.0f);
    
    // Content separator line
    g.setColour(juce::Colour(0xFF333333));
    g.drawHorizontalLine(getHeight() - 145, 20.0f, static_cast<float>(getWidth()) - 20.0f);
}

/*
    RESIZED
    -------
    Position all child components when the size changes.
    
    LAYOUT STRATEGY
    ---------------
    We use a "remove from edges" strategy:
    1. Start with the full bounds
    2. Remove pieces from top/bottom/left/right for each component
    3. What's left goes to the main content area
    
    This is flexible and adapts to different sizes.
*/
void JdrummerAudioProcessorEditor::resized()
{
    // Start with full bounds
    auto bounds = getLocalBounds();
    
    /*
        HEADER AREA (70px)
        ------------------
        removeFromTop() returns the top portion and shrinks 'bounds'.
    */
    auto headerBounds = bounds.removeFromTop(70);
    headerBounds = headerBounds.reduced(20, 15);  // Add padding
    
    // Title on the left
    titleLabel.setBounds(headerBounds.removeFromLeft(150));
    headerBounds.removeFromLeft(20);  // Spacing
    
    // Tab buttons in the center
    auto tabArea = headerBounds.removeFromLeft(340);
    drumKitTabButton.setBounds(tabArea.removeFromLeft(100));
    tabArea.removeFromLeft(10);
    groovesTabButton.setBounds(tabArea.removeFromLeft(100));
    tabArea.removeFromLeft(10);
    bandmateTabButton.setBounds(tabArea.removeFromLeft(100));
    
    headerBounds.removeFromLeft(20);  // Spacing
    
    // Kit selector on the right (only visible in Drum Kit tab, but always positioned)
    auto kitArea = headerBounds.removeFromRight(250);
    kitLabel.setBounds(kitArea.removeFromLeft(35));
    kitArea.removeFromLeft(5);  // Spacing
    kitComboBox.setBounds(kitArea);
    
    /*
        BOTTOM CONTROLS AREA (140px) - Only for Drum Kit tab
        ----------------------------
    */
    auto bottomBounds = bounds.removeFromBottom(170);
    bottomBounds = bottomBounds.reduced(20, 10);
    padControls.setBounds(bottomBounds);
    
    /*
        MAIN CONTENT AREA
        -----------------
        Contains either drum pads or grooves panel based on active tab.
    */
    auto mainBounds = bounds.reduced(10, 10);
    drumPadGrid.setBounds(mainBounds);
    
    // Grooves panel takes the full content area (including bottom where padControls would be)
    auto groovesBounds = getLocalBounds();
    groovesBounds.removeFromTop(70);  // Header
    groovesBounds = groovesBounds.reduced(10, 10);
    groovesPanel.setBounds(groovesBounds);
    
    // Bandmate panel takes the same area as grooves panel
    bandmatePanel.setBounds(groovesBounds);
}

/*
    SHOW TAB
    --------
    Switches between Drum Kit, Grooves, and Bandmate views.
*/
void JdrummerAudioProcessorEditor::showTab(int tabIndex)
{
    currentTab = tabIndex;
    
    // Reset all tab button colors
    drumKitTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
    drumKitTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFAAAAAA));
    groovesTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
    groovesTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFAAAAAA));
    bandmateTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
    bandmateTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFAAAAAA));
    
    // Hide all panels
    drumPadGrid.setVisible(false);
    padControls.setVisible(false);
    kitLabel.setVisible(false);
    kitComboBox.setVisible(false);
    groovesPanel.setVisible(false);
    bandmatePanel.setVisible(false);
    
    if (tabIndex == 0)
    {
        // Drum Kit tab
        drumKitTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF00BFFF));
        drumKitTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFFFFFF));
        
        drumPadGrid.setVisible(true);
        padControls.setVisible(true);
        kitLabel.setVisible(true);
        kitComboBox.setVisible(true);
    }
    else if (tabIndex == 1)
    {
        // Grooves tab
        groovesTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF00BFFF));
        groovesTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFFFFFF));
        
        groovesPanel.setVisible(true);
        groovesPanel.refresh();
    }
    else if (tabIndex == 2)
    {
        // Bandmate tab
        bandmateTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF00BFFF));
        bandmateTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFFFFFF));
        
        bandmatePanel.setVisible(true);
    }
    
    repaint();
}

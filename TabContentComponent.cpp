#include "TabContentComponent.h"
#include "PluginProcessor.h" // Required for processor access

// Constructor
TabContentComponent::TabContentComponent(DualChainSampleTriggerProcessor& processor, juce::AudioProcessorValueTreeState& params, int associatedTabIndex)
    : audioProcessor(processor), valueTreeState(params), tabIndex(associatedTabIndex)
{
    lookAndFeel = std::make_unique<CustomLookAndFeel>();
    setLookAndFeel(lookAndFeel.get());

    // Get the specific TabState this component is associated with
    TabState* currentTabState = audioProcessor.getTabState(tabIndex);
    // Fallback if tab state is somehow null, though this shouldn't happen in normal operation
    // If it does, it indicates a deeper issue with tab management in the processor.
    // For robustness, we could create a temporary default TabState or simply not initialize further.
    // However, the design assumes valid TabState from processor.
    // juce::ASSERT(currentTabState != nullptr); // Good for debugging

    // Create Chain 1 Control
    chain1Control = std::make_unique<ChainControlComponent>();
    if (currentTabState && currentTabState->chainManager) { // Check currentTabState and its chainManager
        chain1Control->setSampleManager(currentTabState->chainManager->getSampleManager(0));
        chain1Control->setChainManager(currentTabState->chainManager.get()); // Pass the ChainManager itself
        chain1Control->setChainTitle(currentTabState->chain1Title);
    }
    chain1Control->setChainIndex(0);
    chain1Control->setColour(DualTriggerStyle::chain1Colour);
    addAndMakeVisible(chain1Control.get());

    // Create Chain 2 Control
    chain2Control = std::make_unique<ChainControlComponent>();
     if (currentTabState && currentTabState->chainManager) { // Check currentTabState
        chain2Control->setSampleManager(currentTabState->chainManager->getSampleManager(1));
        chain2Control->setChainManager(currentTabState->chainManager.get()); // Pass the ChainManager itself
        chain2Control->setChainTitle(currentTabState->chain2Title);
    }
    chain2Control->setChainIndex(1);
    chain2Control->setColour(DualTriggerStyle::chain2Colour);
    addAndMakeVisible(chain2Control.get());

    // Chain Title Editors
    chain1TitleEditor = std::make_unique<juce::TextEditor>("chain1TitleEditor");
    if (currentTabState) chain1TitleEditor->setText(currentTabState->chain1Title);
    chain1TitleEditor->setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
    chain1TitleEditor->addListener(this);
    addAndMakeVisible(chain1TitleEditor.get());

    chain2TitleEditor = std::make_unique<juce::TextEditor>("chain2TitleEditor");
    if (currentTabState) chain2TitleEditor->setText(currentTabState->chain2Title);
    chain2TitleEditor->setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
    chain2TitleEditor->addListener(this);
    addAndMakeVisible(chain2TitleEditor.get());

    // Blend Slider & Label (Global Parameter)
    blendSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow);
    blendSlider->setRange(0.0, 1.0, 0.01);
    blendSlider->setDoubleClickReturnValue(true, 0.5);
    blendSlider->addListener(this);
    addAndMakeVisible(blendSlider.get());
    blendAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        valueTreeState, DualChainSampleTriggerProcessor::PARAM_BLEND, *blendSlider);

    blendLabel = std::make_unique<juce::Label>("blendLabel", "Blend");
    blendLabel->setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
    blendLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(blendLabel.get());

    // Main Volume Slider & Label (Global Parameter)
    mainVolumeSlider = std::make_unique<juce::Slider>(juce::Slider::LinearVertical, juce::Slider::TextBoxBelow);
    mainVolumeSlider->setRange(0.0, 1.0, 0.01);
    mainVolumeSlider->setDoubleClickReturnValue(true, 1.0);
    mainVolumeSlider->addListener(this);
    addAndMakeVisible(mainVolumeSlider.get());
    mainVolumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        valueTreeState, DualChainSampleTriggerProcessor::PARAM_MAIN_VOLUME, *mainVolumeSlider);

    mainVolumeLabel = std::make_unique<juce::Label>("mainVolumeLabel", "Main Volume");
    mainVolumeLabel->setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
    mainVolumeLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mainVolumeLabel.get());

    // Piano Roll Component
    pianoRollComponent_ = std::make_unique<PianoRollComponent>();
    pianoRollComponent_->addListener(this);
    // Initial notes for piano roll should reflect global parameters, as they control the active CM
    // This might seem counterintuitive if tabs have different MIDI notes in the future, but for now, notes are global.
    if (valueTreeState.getParameter(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE) && valueTreeState.getParameter(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE))
    {
        int initialNote1 = (int) valueTreeState.getRawParameterValue(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE)->load();
        int initialNote2 = (int) valueTreeState.getRawParameterValue(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE)->load();
        pianoRollComponent_->setInitialNotes(initialNote1, initialNote2);
    }
    pianoRollComponent_->setChainHighlightColour(0, DualTriggerStyle::chain1Colour);
    pianoRollComponent_->setChainHighlightColour(1, DualTriggerStyle::chain2Colour);
    addAndMakeVisible(pianoRollComponent_.get());

    updateUIForTab(); // Initial UI state update
    startTimerHz(30); // For UI updates
}

TabContentComponent::~TabContentComponent()
{
    stopTimer();
    setLookAndFeel(nullptr);
    // Remove listeners to avoid dangling pointers if components are destroyed out of order
    if (blendSlider) blendSlider->removeListener(this);
    if (mainVolumeSlider) mainVolumeSlider->removeListener(this);
    if (pianoRollComponent_) pianoRollComponent_->removeListener(this);
    if (chain1TitleEditor) chain1TitleEditor->removeListener(this);
    if (chain2TitleEditor) chain2TitleEditor->removeListener(this);
    // ChainControlComponents manage their own internal listeners.
}

void TabContentComponent::paint(juce::Graphics& g)
{
    g.fillAll(DualTriggerStyle::backgroundColour); // Or another color to distinguish tab content area
    // Optional: Draw a border or other tab-specific background elements
    // g.setColour(juce::Colours::darkgrey);
    // g.drawRect(getLocalBounds(), 1);
}

void TabContentComponent::resized()
{
    const int padding = DualTriggerStyle::padding; // Use the new padding directly
    const int controlHeight = DualTriggerStyle::controlHeight; // New control height
    const int mainVolumeWidth = 70; // Increased width for better touch
    const int blendAreaHeight = DualTriggerStyle::sliderHeight + 20; // slider + label
    const int pianoRollHeight = 120; // Keep as is or adjust if needed
    const int chainTitleEditorHeight = controlHeight;

    juce::Rectangle<int> localBounds = getLocalBounds().reduced(padding); // Reduce bounds by padding for overall margin

    // Top area for Chain Title Editors
    juce::Rectangle<int> topArea = localBounds.removeFromTop(chainTitleEditorHeight);
    int titleEditorWidth = (topArea.getWidth() - padding) / 2;
    chain1TitleEditor->setBounds(topArea.removeFromLeft(titleEditorWidth));
    topArea.removeFromLeft(padding); // Space between editors
    chain2TitleEditor->setBounds(topArea.removeFromLeft(titleEditorWidth));

    localBounds.removeFromTop(padding); // Space after title editors

    // Bottom area for Piano Roll and Blend slider
    juce::Rectangle<int> bottomArea = localBounds.removeFromBottom(pianoRollHeight + padding + blendAreaHeight);

    juce::Rectangle<int> pianoRollArea = bottomArea.removeFromTop(pianoRollHeight);
    pianoRollComponent_->setBounds(pianoRollArea);

    bottomArea.removeFromTop(padding); // Space between piano roll and blend area

    juce::Rectangle<int> blendLabelArea = bottomArea.removeFromTop(20);
    blendLabel->setBounds(blendLabelArea);
    blendSlider->setBounds(bottomArea); // Blend slider takes remaining space in bottomArea


    // Middle area for Chain Controls and Main Volume
    juce::Rectangle<int> controlsArea = localBounds; // Remaining space after top and bottom are removed

    juce::Rectangle<int> mainVolumeArea = controlsArea.removeFromRight(mainVolumeWidth);
    controlsArea.removeFromRight(padding); // Space between main volume and chain controls

    mainVolumeLabel->setBounds(mainVolumeArea.removeFromTop(25)); // Label height
    mainVolumeSlider->setBounds(mainVolumeArea);

    int chainWidth = (controlsArea.getWidth() - padding) / 2;
    chain1Control->setBounds(controlsArea.removeFromLeft(chainWidth));
    controlsArea.removeFromLeft(padding); // Space between chain controls
    chain2Control->setBounds(controlsArea);
}


void TabContentComponent::sliderValueChanged(juce::Slider* slider)
{
    // These sliders (blend, mainVolume) control global parameters.
    // The APVTS attachments handle updating the processor.
    // We might need to update labels or visual cues here if any.
    if (slider == blendSlider.get())
    {
        float blendValue = static_cast<float>(blendSlider->getValue());
        juce::Colour trackColour = DualTriggerStyle::chain1Colour.interpolatedWith(DualTriggerStyle::chain2Colour, blendValue);
        blendSlider->setColour(juce::Slider::trackColourId, trackColour);
        if (blendValue < 0.33f) blendLabel->setText("Blend: More Chain 1", juce::dontSendNotification);
        else if (blendValue > 0.67f) blendLabel->setText("Blend: More Chain 2", juce::dontSendNotification);
        else blendLabel->setText("Blend: Balanced", juce::dontSendNotification);
    }
    else if (slider == mainVolumeSlider.get())
    {
        float volumeValue = static_cast<float>(mainVolumeSlider->getValue());
        if (volumeValue < 0.01f) mainVolumeLabel->setText("Volume: Muted", juce::dontSendNotification);
        else mainVolumeLabel->setText("Main Volume", juce::dontSendNotification);
    }
    // ChainControlComponent sliders are handled within ChainControlComponent itself.
}

void TabContentComponent::timerCallback()
{
    // This timer is for this specific TabContentComponent.
    // Update its UI based on its associated tab's state.
    updateUIForTab();
}

void TabContentComponent::pianoNoteSelected(int chainIndexInTab, int midiNoteNumber)
{
    // This component is associated with a specific tab.
    // However, MIDI note parameters (PARAM_CHAIN1_NOTE, PARAM_CHAIN2_NOTE) are global
    // and affect the *active* ChainManager in the processor.
    // So, when a note is selected in *any* tab's PianoRoll, it should update these global params.
    if (chainIndexInTab == 0)
    {
        valueTreeState.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE) = midiNoteNumber;
    }
    else if (chainIndexInTab == 1)
    {
        valueTreeState.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE) = midiNoteNumber;
    }
}

void TabContentComponent::textEditorTextChanged(juce::TextEditor& editor)
{
    TabState* currentTabState = audioProcessor.getTabState(tabIndex);
    if (!currentTabState) return;

    if (&editor == chain1TitleEditor.get())
    {
        currentTabState->chain1Title = chain1TitleEditor->getText();
        if (chain1Control) chain1Control->setChainTitle(currentTabState->chain1Title); // Update ChainControl's display
        // TODO: Might need to notify main editor if tab name in TabbedComponent needs update from this,
        // though chain titles are usually internal to the tab content.
    }
    else if (&editor == chain2TitleEditor.get())
    {
        currentTabState->chain2Title = chain2TitleEditor->getText();
        if (chain2Control) chain2Control->setChainTitle(currentTabState->chain2Title);
    }
}

void TabContentComponent::buttonClicked(juce::Button* button)
{
    // Buttons within ChainControlComponents are handled by ChainControlComponent's buttonClicked.
    // This TabContentComponent doesn't have its own global buttons like Save/Load/Reset.
}

void TabContentComponent::updateUIForTab()
{
    TabState* currentTabState = audioProcessor.getTabState(tabIndex);
    if (!currentTabState) {
        // Optionally hide or disable components if the tab state is invalid
        return;
    }
    if (!currentTabState->chainManager) { // Ensure chainManager exists
        return;
    }

    // Update ChainControlComponents
    // Ensure SampleManagers are correctly set (might be redundant if only set at construction,
    // but good for robustness if ChainManagers could be swapped or reloaded within a TabState)
    chain1Control->setSampleManager(currentTabState->chainManager->getSampleManager(0));
    chain1Control->setChainManager(currentTabState->chainManager.get());
    chain1Control->setChainTitle(currentTabState->chain1Title); // Update displayed title
    chain1Control->updateDisplay(); // Tell ChainControl to refresh its internal state (samples, etc.)

    chain2Control->setSampleManager(currentTabState->chainManager->getSampleManager(1));
    chain2Control->setChainManager(currentTabState->chainManager.get());
    chain2Control->setChainTitle(currentTabState->chain2Title);
    chain2Control->updateDisplay();

    // Update chain title editors
    if (chain1TitleEditor->getText() != currentTabState->chain1Title) {
        chain1TitleEditor->setText(currentTabState->chain1Title, juce::dontSendNotification);
    }
    if (chain2TitleEditor->getText() != currentTabState->chain2Title) {
        chain2TitleEditor->setText(currentTabState->chain2Title, juce::dontSendNotification);
    }

    // Update global sliders/labels (blend, main volume)
    // These are driven by global APVTS parameters, so attachments handle their values.
    // Visual updates (like blend color) are handled in sliderValueChanged.
    // Triggering sliderValueChanged artificially or re-applying color logic here:
    sliderValueChanged(blendSlider.get());
    sliderValueChanged(mainVolumeSlider.get());


    // Update PianoRoll notes (reflect global MIDI note parameters)
    // This ensures that if the global MIDI notes change (e.g. by another tab's pianoroll, or host automation),
    // this tab's piano roll display is consistent.
    if (valueTreeState.getParameter(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE) && valueTreeState.getParameter(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE))
    {
        int note1 = (int) valueTreeState.getRawParameterValue(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE)->load();
        int note2 = (int) valueTreeState.getRawParameterValue(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE)->load();
        if (pianoRollComponent_) pianoRollComponent_->setInitialNotes(note1, note2);
    }

    // Repaint if necessary
    repaint();
}

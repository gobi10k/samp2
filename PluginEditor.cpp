#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DualChainSampleTriggerEditor::DualChainSampleTriggerEditor(DualChainSampleTriggerProcessor& p, juce::AudioProcessorValueTreeState& params)
    : AudioProcessorEditor(&p), audioProcessor(p), parameters(params)
{
    // Create the look and feel
    lookAndFeel = std::make_unique<CustomLookAndFeel>();
    setLookAndFeel(lookAndFeel.get());
    
    pianoRollComponent_ = std::make_unique<PianoRollComponent>();
    pianoRollComponent_->addListener(this);
    addAndMakeVisible(pianoRollComponent_.get());
    
    // Initialize piano roll notes from parameters
    // Ensure DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE and PARAM_CHAIN2_NOTE are accessible
    if (parameters.getParameter(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE) && parameters.getParameter(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE))
    {
        int initialNote1 = (int) parameters.getRawParameterValue(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE)->load();
        int initialNote2 = (int) parameters.getRawParameterValue(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE)->load();
        pianoRollComponent_->setInitialNotes(initialNote1, initialNote2);
    }
    pianoRollComponent_->setChainHighlightColour(0, DualTriggerStyle::chain1Colour);
    pianoRollComponent_->setChainHighlightColour(1, DualTriggerStyle::chain2Colour);
    
    // Create the title label
    titleLabel = std::make_unique<juce::Label>("titleLabel", "sample keyboard");
    titleLabel->setFont(juce::Font(DualTriggerStyle::fontSizeHeader * 1.2f).boldened());
    titleLabel->setJustificationType(juce::Justification::centred);
    titleLabel->setColour(juce::Label::textColourId, DualTriggerStyle::textColour);
    addAndMakeVisible(titleLabel.get());

    sessionTitleEditor = std::make_unique<juce::TextEditor>("sessionTitleEditor");
    sessionTitleEditor->setText(titleLabel->getText()); // Initialize with current main title
    sessionTitleEditor->setFont(juce::Font(DualTriggerStyle::fontSizeHeader * 1.2f).boldened()); // Match titleLabel font
    sessionTitleEditor->setJustification(juce::Justification::centred);
    sessionTitleEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack); // Make it blend
    sessionTitleEditor->setColour(juce::TextEditor::textColourId, DualTriggerStyle::textColour);
    sessionTitleEditor->addListener(this); // Re-add listener
    addAndMakeVisible(sessionTitleEditor.get());
    
    // For Chain 1 Title Editor
    chain1TitleLabelEditorLabel = std::make_unique<juce::Label>("chain1TitleLabelEditorLabel", "Chain 1 Title:");
    chain1TitleLabelEditorLabel->setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
    chain1TitleLabelEditorLabel->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(chain1TitleLabelEditorLabel.get());

    chain1TitleEditor = std::make_unique<juce::TextEditor>("chain1TitleEditor");
    // chain1TitleEditor->setText(chain1Control->getTitleText()); // Get initial title from ChainControlComponent // Will set this after chain1Control is initialized
    chain1TitleEditor->setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
    chain1TitleEditor->addListener(this); // Re-add listener
    addAndMakeVisible(chain1TitleEditor.get());

    // For Chain 2 Title Editor
    chain2TitleLabelEditorLabel = std::make_unique<juce::Label>("chain2TitleLabelEditorLabel", "Chain 2 Title:");
    chain2TitleLabelEditorLabel->setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
    chain2TitleLabelEditorLabel->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(chain2TitleLabelEditorLabel.get());

    chain2TitleEditor = std::make_unique<juce::TextEditor>("chain2TitleEditor");
    // chain2TitleEditor->setText(chain2Control->getTitleText()); // Get initial title from ChainControlComponent // Will set this after chain2Control is initialized
    chain2TitleEditor->setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
    chain2TitleEditor->addListener(this); // Re-add listener
    addAndMakeVisible(chain2TitleEditor.get());
    
    // Create the chain 1 control
    chain1Control = std::make_unique<ChainControlComponent>();
    chain1Control->setSampleManager(audioProcessor.getChainManager()->getSampleManager(0));
    chain1Control->setChainManager(audioProcessor.getChainManager());
    chain1Control->setChainIndex(0);
    chain1Control->setColour(DualTriggerStyle::chain1Colour);
    addAndMakeVisible(chain1Control.get());
    chain1TitleEditor->setText(chain1Control->getTitleText()); // Set initial title now that chain1Control exists
    
    // Create the chain 2 control
    chain2Control = std::make_unique<ChainControlComponent>();
    chain2Control->setSampleManager(audioProcessor.getChainManager()->getSampleManager(1));
    chain2Control->setChainManager(audioProcessor.getChainManager());
    chain2Control->setChainIndex(1);
    chain2Control->setColour(DualTriggerStyle::chain2Colour);
    addAndMakeVisible(chain2Control.get());
    chain2TitleEditor->setText(chain2Control->getTitleText()); // Set initial title now that chain2Control exists
    
    // Create the blend slider
    blendSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow);
    blendSlider->setRange(0.0, 1.0, 0.01);
    blendSlider->setValue(0.5);
    blendSlider->setTextValueSuffix("%");
    blendSlider->setColour(juce::Slider::textBoxTextColourId, DualTriggerStyle::textColour);
    blendSlider->setColour(juce::Slider::trackColourId, DualTriggerStyle::chain1Colour);
    blendSlider->setColour(juce::Slider::thumbColourId, DualTriggerStyle::highlightColour);
    blendSlider->setDoubleClickReturnValue(true, 0.5);
    blendSlider->addListener(this);
    addAndMakeVisible(blendSlider.get());
    
    // Create the blend label
    blendLabel = std::make_unique<juce::Label>("blendLabel", "Blend");
    blendLabel->setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
    blendLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(blendLabel.get());
    
    // Create the main volume slider
    mainVolumeSlider = std::make_unique<juce::Slider>(juce::Slider::LinearVertical, juce::Slider::TextBoxBelow);
    mainVolumeSlider->setRange(0.0, 1.0, 0.01);
    mainVolumeSlider->setValue(1.0);
    mainVolumeSlider->setTextValueSuffix("%");
    mainVolumeSlider->setColour(juce::Slider::textBoxTextColourId, DualTriggerStyle::textColour);
    mainVolumeSlider->setColour(juce::Slider::thumbColourId, DualTriggerStyle::highlightColour);
    mainVolumeSlider->setDoubleClickReturnValue(true, 1.0);
    mainVolumeSlider->addListener(this);
    addAndMakeVisible(mainVolumeSlider.get());
    
    // Create the main volume label
    mainVolumeLabel = std::make_unique<juce::Label>("mainVolumeLabel", "Main Volume");
    mainVolumeLabel->setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
    mainVolumeLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mainVolumeLabel.get());
    
    // Create parameter attachments
    blendAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        parameters, DualChainSampleTriggerProcessor::PARAM_BLEND, *blendSlider);
    
    mainVolumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        parameters, DualChainSampleTriggerProcessor::PARAM_MAIN_VOLUME, *mainVolumeSlider);

    // Remove TextEditor Attachments
    // sessionTitleAttachment = ...
    // chain1TitleAttachment = ...
    // chain2TitleAttachment = ...

    // Remove APVTS listeners for titles
    // parameters.addParameterListener(DualChainSampleTriggerProcessor::PARAM_SESSION_TITLE.toString(), this);
    // parameters.addParameterListener(DualChainSampleTriggerProcessor::PARAM_CHAIN1_TITLE.toString(), this);
    // parameters.addParameterListener(DualChainSampleTriggerProcessor::PARAM_CHAIN2_TITLE.toString(), this);
    
    // Set the initial component values
    updateUI();
    
    // Start the timer
    startTimerHz(30); // 30 fps

    saveStateButton = std::make_unique<juce::TextButton>("Save State");
    saveStateButton->addListener(this);
    addAndMakeVisible(saveStateButton.get());

    loadStateButton = std::make_unique<juce::TextButton>("Load State");
    loadStateButton->addListener(this);
    addAndMakeVisible(loadStateButton.get());

    resetStateButton = std::make_unique<juce::TextButton>("Reset State");
    resetStateButton->addListener(this);
    addAndMakeVisible(resetStateButton.get());
    
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(800, 600);
    
    // Make the editor resizable
    setResizable(true, true);
    setResizeLimits(800, 600, 1200, 800);
}

DualChainSampleTriggerEditor::~DualChainSampleTriggerEditor()
{
    // Stop the timer
    stopTimer();
    
    // Remove this as a listener
    blendSlider->removeListener(this);
    mainVolumeSlider->removeListener(this);
    pianoRollComponent_->removeListener(this);
    // Re-add TextEditor listeners if they were removed
    if (chain1TitleEditor)
        chain1TitleEditor->removeListener(this); // This is for general cleanup, will re-add if needed by logic
    if (chain2TitleEditor)
        chain2TitleEditor->removeListener(this);
    if (sessionTitleEditor)
        sessionTitleEditor->removeListener(this);
    // Ensure these are removed if they were previously added for APVTS attachments
    // For safety, let's assume they are removed and re-added if needed.

    if (saveStateButton)
        saveStateButton->removeListener(this);
    if (loadStateButton)
        loadStateButton->removeListener(this);
    if (resetStateButton)
        resetStateButton->removeListener(this);

    // Remove APVTS listeners for titles (if they were added)
    // parameters.removeParameterListener(DualChainSampleTriggerProcessor::PARAM_SESSION_TITLE.toString(), this);
    // parameters.removeParameterListener(DualChainSampleTriggerProcessor::PARAM_CHAIN1_TITLE.toString(), this);
    // parameters.removeParameterListener(DualChainSampleTriggerProcessor::PARAM_CHAIN2_TITLE.toString(), this);
    
    // Clean up the look and feel
    setLookAndFeel(nullptr);
}

//==============================================================================
void DualChainSampleTriggerEditor::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(DualTriggerStyle::backgroundColour);
    
    // Draw a border
    g.setColour(DualTriggerStyle::disabledColour);
    g.drawRect(getLocalBounds(), 1);
}

void DualChainSampleTriggerEditor::resized()
{
    const int margin = DualTriggerStyle::padding * 2; // Typically 16px
    const int headerHeight = DualTriggerStyle::headerHeight * 1.5f; // Typically 42px
    const int mainVolumeWidth = 60;
    const int blendAreaHeight = 60; // Combined height for blend label and slider
    const int pianoRollHeight = 120; // Height for the piano roll

    // Top area for title
    // titleLabel->setBounds(margin, margin, getWidth() - (2 * margin), headerHeight); // Original label
    sessionTitleEditor->setBounds(margin, margin, getWidth() - (2 * margin), headerHeight); // Editor takes its place
    titleLabel->setVisible(false); // Hide original label
    
    float currentY = margin + headerHeight + margin; // currentY starts after the main title

    // Add new editors here
    const int editorHeight = DualTriggerStyle::controlHeight; // Typically 24px
    const int labelWidth = 100; // Increased width for "Chain X Title:"
    const int editorCompWidth = 150; // Width for the text editor box

    chain1TitleLabelEditorLabel->setBounds(margin, currentY, labelWidth, editorHeight);
    chain1TitleEditor->setBounds(margin + labelWidth + DualTriggerStyle::padding, currentY, editorCompWidth, editorHeight);
    currentY += editorHeight + margin;

    chain2TitleLabelEditorLabel->setBounds(margin, currentY, labelWidth, editorHeight);
    chain2TitleEditor->setBounds(margin + labelWidth + DualTriggerStyle::padding, currentY, editorCompWidth, editorHeight);
    currentY += editorHeight + margin; // This currentY will now be used for chain1Control etc.

    // Calculate total height available for the main controls section (chains, main volume)
    // The space used by title and new editors is effectively currentY at this point (considering it started after title + margin)
    // float controlsAreaHeight = getHeight() - headerHeight - blendAreaHeight - pianoRollHeight - (5 * margin); // Original
    // Adjusted calculation:
    float availableHeightForControls = getHeight() - currentY; // Height from under new editors to bottom
    float controlsAreaHeight = availableHeightForControls - blendAreaHeight - pianoRollHeight - (margin * 2); // Subtract other components and their margins (below controls, below piano, bottom)


    if (controlsAreaHeight < 200) controlsAreaHeight = 200; // Minimum height for controls

    // Position the chains and main volume slider within the controlsAreaHeight
    int chainWidth = (getWidth() - (3 * margin) - mainVolumeWidth) / 2;
    if (chainWidth < 200) chainWidth = 200; // Minimum width for a chain

    chain1Control->setBounds(margin,
                               currentY,
                               chainWidth,
                               controlsAreaHeight);
    
    chain2Control->setBounds(margin + chainWidth + margin,
                               currentY,
                               chainWidth,
                               controlsAreaHeight);
    
    mainVolumeLabel->setBounds(getWidth() - mainVolumeWidth - margin,
                                 currentY,
                                 mainVolumeWidth,
                                 30); // Height for the label
                                 
    mainVolumeSlider->setBounds(getWidth() - mainVolumeWidth - margin,
                                  currentY + 30, // Position slider below its label
                                  mainVolumeWidth,
                                  controlsAreaHeight - 30); // Slider takes remaining height in this section

    currentY += controlsAreaHeight + margin;

    // Position Piano Roll
    pianoRollComponent_->setBounds(margin, currentY, getWidth() - (2 * margin), pianoRollHeight);
    currentY += pianoRollHeight + margin;

    // Position Blend Slider area at the bottom
    blendLabel->setBounds(margin,
                            currentY,
                            getWidth() - (2 * margin) - mainVolumeWidth, // Keep consistent with original width calc
                            20); // Height for the label

    blendSlider->setBounds(margin,
                             currentY + 20, // Position slider below its label
                             getWidth() - (2 * margin) - mainVolumeWidth, // Keep consistent
                             blendAreaHeight - 20); // Slider takes remaining height
                             
    // Position Save State Button at the bottom-left
    saveStateButton->setBounds(margin, getHeight() - margin - DualTriggerStyle::controlHeight, 100, DualTriggerStyle::controlHeight);
    // Position Load State Button next to Save State Button
    loadStateButton->setBounds(saveStateButton->getRight() + margin, getHeight() - margin - DualTriggerStyle::controlHeight, 100, DualTriggerStyle::controlHeight);
    // Position Reset State Button next to Load State Button
    resetStateButton->setBounds(loadStateButton->getRight() + margin, getHeight() - margin - DualTriggerStyle::controlHeight, 100, DualTriggerStyle::controlHeight);
}

void DualChainSampleTriggerEditor::sliderValueChanged(juce::Slider* slider)
{
    // Update UI to reflect changes
    updateUI();
}

void DualChainSampleTriggerEditor::timerCallback()
{
    // Update UI to reflect any changes in the processor
    updateUI();

    // Update Piano Roll display from parameters
    if (pianoRollComponent_ != nullptr &&
        parameters.getParameter(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE) &&
        parameters.getParameter(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE))
    {
        int note1 = (int) parameters.getRawParameterValue(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE)->load();
        int note2 = (int) parameters.getRawParameterValue(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE)->load();
        pianoRollComponent_->setInitialNotes(note1, note2);
    }
}

void DualChainSampleTriggerEditor::updateUI()
{
    // Update the blend slider color to reflect the balance
    float blendValue = static_cast<float>(blendSlider->getValue());
    
    juce::Colour trackColour = DualTriggerStyle::chain1Colour.interpolatedWith(
        DualTriggerStyle::chain2Colour, blendValue);
    
    blendSlider->setColour(juce::Slider::trackColourId, trackColour);
    
    // Update the blend label text
    if (blendValue < 0.33f)
    {
        blendLabel->setText("Blend: More Chain 1", juce::dontSendNotification);
    }
    else if (blendValue > 0.67f)
    {
        blendLabel->setText("Blend: More Chain 2", juce::dontSendNotification);
    }
    else
    {
        blendLabel->setText("Blend: Balanced", juce::dontSendNotification);
    }
    
    // Update the chain controls
    chain1Control->updateDisplay();
    chain2Control->updateDisplay();
    
    // Update the main volume label
    float volumeValue = static_cast<float>(mainVolumeSlider->getValue());
    
    if (volumeValue < 0.01f)
    {
        mainVolumeLabel->setText("Volume: Muted", juce::dontSendNotification);
    }
    else
    {
        mainVolumeLabel->setText("Main Volume", juce::dontSendNotification);
    }

    // Update PianoRoll initial notes if parameters change externally
    // This might be better handled by a parameter listener directly in PianoRollComponent
    // or by the processor sending an update message. For now, keep it simple.
    if (parameters.getParameter(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE) && parameters.getParameter(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE))
    {
        int note1 = (int) parameters.getRawParameterValue(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE)->load();
        int note2 = (int) parameters.getRawParameterValue(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE)->load();
        if (pianoRollComponent_) // ensure component exists
             pianoRollComponent_->setInitialNotes(note1, note2);
    }
}

void DualChainSampleTriggerEditor::pianoNoteSelected(int chainIndex, int midiNoteNumber)
{
    if (chainIndex == 0)
    {
        parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE) = midiNoteNumber;
    }
    else if (chainIndex == 1)
    {
        parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE) = midiNoteNumber;
    }
}

void DualChainSampleTriggerEditor::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == sessionTitleEditor.get())
    {
        audioProcessor.setSessionTitleForSaving(sessionTitleEditor->getText());
        // Also update the underlying label if it's still used for display when not editing
        titleLabel->setText(sessionTitleEditor->getText(), juce::dontSendNotification);
    }
    else if (&editor == chain1TitleEditor.get())
    {
        audioProcessor.setChain1TitleForSaving(chain1TitleEditor->getText());
        if (chain1Control)
            chain1Control->setChainTitle(chain1TitleEditor->getText());
    }
    else if (&editor == chain2TitleEditor.get())
    {
        audioProcessor.setChain2TitleForSaving(chain2TitleEditor->getText());
        if (chain2Control)
            chain2Control->setChainTitle(chain2TitleEditor->getText());
    }
}

void DualChainSampleTriggerEditor::buttonClicked(juce::Button* button)
{
    if (button == saveStateButton.get())
    {
        // Titles are now set via textEditorTextChanged, direct call to processor
        audioProcessor.setSessionTitleForSaving(sessionTitleEditor->getText());
        audioProcessor.setChain1TitleForSaving(chain1TitleEditor->getText());
        audioProcessor.setChain2TitleForSaving(chain2TitleEditor->getText());

        chooser = std::make_unique<juce::FileChooser>(
            "Save State as XML",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.xml",
            true, // useOSNativeDialogBox
            false, // treatFilePackagesAsDirectories
            this); // parentComponent (this PluginEditor instance)
        
        auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;

        chooser->launchAsync(flags, [this] (const juce::FileChooser& fc)
        {
            juce::File file = fc.getResult();
            if (file != juce::File{})
            {
                // Ensure .xml extension
                if (!file.hasFileExtension(".xml") && !file.hasFileExtension(".XML"))
                    file = file.withFileExtension(".xml");
                audioProcessor.saveStateToXml(file);
            }
        });
    }
    // Note: Other button clicks like clearButton, etc., are handled within ChainControlComponent's
    // own buttonClicked method if they are part of that component.
    // If there were other buttons directly in PluginEditor, their handlers would go here.
    else if (button == loadStateButton.get())
    {
        chooser = std::make_unique<juce::FileChooser>(
            "Load State from XML",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.xml",
            true, // useOSNativeDialogBox
            false, // treatFilePackagesAsDirectories
            this); // parentComponent

        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(flags, [this] (const juce::FileChooser& fc)
        {
            juce::File file = fc.getResult();
            if (file != juce::File{})
            {
                audioProcessor.loadStateFromXml(file);

            // Re-add explicit UI updates for titles
            sessionTitleEditor->setText(audioProcessor.getSessionTitleForSaving(), juce::dontSendNotification);
            chain1TitleEditor->setText(audioProcessor.getChain1TitleForSaving(), juce::dontSendNotification);
            chain2TitleEditor->setText(audioProcessor.getChain2TitleForSaving(), juce::dontSendNotification);
            
            titleLabel->setText(audioProcessor.getSessionTitleForSaving(), juce::dontSendNotification);
            if (chain1Control) chain1Control->setChainTitle(audioProcessor.getChain1TitleForSaving());
            if (chain2Control) chain2Control->setChainTitle(audioProcessor.getChain2TitleForSaving());

            updateUI(); 
            if (chain1Control) chain1Control->updateDisplay();
            if (chain2Control) chain2Control->updateDisplay();
        }
    }
    else if (button == resetStateButton.get())
    {
        audioProcessor.resetToDefaultState();

        // Re-add explicit UI updates for titles
        sessionTitleEditor->setText(audioProcessor.getSessionTitleForSaving(), juce::dontSendNotification);
        chain1TitleEditor->setText(audioProcessor.getChain1TitleForSaving(), juce::dontSendNotification);
        chain2TitleEditor->setText(audioProcessor.getChain2TitleForSaving(), juce::dontSendNotification);
        
        titleLabel->setText(audioProcessor.getSessionTitleForSaving(), juce::dontSendNotification);
        if (chain1Control) chain1Control->setChainTitle(audioProcessor.getChain1TitleForSaving());
        if (chain2Control) chain2Control->setChainTitle(audioProcessor.getChain2TitleForSaving());

        updateUI(); 
        if (chain1Control) chain1Control->updateDisplay();
        if (chain2Control) chain2Control->updateDisplay();
    }
}

void DualChainSampleTriggerEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // Remove title parameter handling if APVTS listener is kept for other parameters.
    // If APVTS listener is removed entirely from PluginEditor.h, this whole method can be removed.
    // For now, let's assume it's kept for other (non-title) parameters if any.
    // Remove title parameter handling if APVTS listener is kept for other parameters.
    // If APVTS listener is removed entirely from PluginEditor.h, this whole method can be removed.
    // For now, let's assume it's kept for other (non-title) parameters if any.
    // Based on current setup, other parameters (blend, mainVolume) are handled by SliderAttachments.
    // PianoRollComponent note changes are handled by its own listener.
    // General UI updates are in updateUI().
    // Thus, this specific parameterChanged in PluginEditor might not be strictly needed anymore
    // if all parameter-driven UI is covered by attachments or specific callbacks.
    // For this step, we'll leave it empty as no other parameters seem to need manual handling here.
}
// The parameterChanged method definition is fully removed as it's not declared in PluginEditor.h
// and AudioProcessorValueTreeState::Listener is not inherited by the editor.

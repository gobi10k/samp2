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
    titleLabel = std::make_unique<juce::Label>("titleLabel", "Dual-Chain Sample Trigger");
    titleLabel->setFont(juce::Font(DualTriggerStyle::fontSizeHeader * 1.2f).boldened());
    titleLabel->setJustificationType(juce::Justification::centred);
    titleLabel->setColour(juce::Label::textColourId, DualTriggerStyle::textColour);
    addAndMakeVisible(titleLabel.get());
    
    // Create the chain 1 control
    chain1Control = std::make_unique<ChainControlComponent>();
    chain1Control->setSampleManager(audioProcessor.getChainManager()->getSampleManager(0));
    chain1Control->setChainManager(audioProcessor.getChainManager());
    chain1Control->setChainIndex(0);
    chain1Control->setColour(DualTriggerStyle::chain1Colour);
    addAndMakeVisible(chain1Control.get());
    
    // Create the chain 2 control
    chain2Control = std::make_unique<ChainControlComponent>();
    chain2Control->setSampleManager(audioProcessor.getChainManager()->getSampleManager(1));
    chain2Control->setChainManager(audioProcessor.getChainManager());
    chain2Control->setChainIndex(1);
    chain2Control->setColour(DualTriggerStyle::chain2Colour);
    addAndMakeVisible(chain2Control.get());
    
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
    
    // Set the initial component values
    updateUI();
    
    // Start the timer
    startTimerHz(30); // 30 fps
    
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
    titleLabel->setBounds(margin, margin, getWidth() - (2 * margin), headerHeight);
    
    // Calculate total height available for the main controls section (chains, main volume)
    // H = TM + TH + M1 + CH + M2 + PRH + M3 + BAH + BM
    // CH = H - (TH + BAH + PRH + TM + M1 + M2 + M3 + BM)
    // Assuming 5 margins (top, below-title, below-controls, below-piano, bottom)
    float controlsAreaHeight = getHeight() - headerHeight - blendAreaHeight - pianoRollHeight - (5 * margin);
    if (controlsAreaHeight < 200) controlsAreaHeight = 200; // Minimum height for controls

    float currentY = margin + headerHeight + margin;

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

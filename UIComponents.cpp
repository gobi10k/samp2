#include "UIComponents.h"
#include "ChainManager.h"

//==============================================================================
// TriggerIndicator Implementation
//==============================================================================

TriggerIndicator::TriggerIndicator()
    : indicatorColour(DualTriggerStyle::highlightColour),
      intensity(0.0f)
{
    startTimerHz(60); // 60 frames per second for smooth animation
}

TriggerIndicator::~TriggerIndicator()
{
    stopTimer();
}

void TriggerIndicator::setColour(juce::Colour colour)
{
    indicatorColour = colour;
    repaint();
}

void TriggerIndicator::triggerIndicator()
{
    intensity = 1.0f;
    repaint();
}

void TriggerIndicator::paint(juce::Graphics& g)
{
    // Draw the indicator
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    
    // Draw the background
    g.setColour(DualTriggerStyle::controlBackgroundColour);
    g.fillRoundedRectangle(bounds, DualTriggerStyle::cornerRadius);
    
    // Draw the lit indicator
    if (intensity > 0.0f)
    {
        g.setColour(indicatorColour.withAlpha(intensity));
        g.fillRoundedRectangle(bounds, DualTriggerStyle::cornerRadius);
    }
    
    // Draw the border
    g.setColour(DualTriggerStyle::disabledColour);
    g.drawRoundedRectangle(bounds, DualTriggerStyle::cornerRadius, 1.0f);
}

void TriggerIndicator::timerCallback()
{
    // Fade out the indicator
    if (intensity > 0.0f)
    {
        intensity -= 0.05f;
        
        if (intensity < 0.0f)
            intensity = 0.0f;
        
        repaint();
    }
}

//==============================================================================
// TriggerButton Implementation
//==============================================================================

TriggerButton::TriggerButton(const juce::String& name)
    : juce::Button(name),
      buttonColour(DualTriggerStyle::highlightColour)
{
}

void TriggerButton::setColour(juce::Colour colour)
{
    buttonColour = colour;
    repaint();
}

void TriggerButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    
    // Choose the color based on the button state
    juce::Colour baseColour = buttonColour;
    
    if (shouldDrawButtonAsDown)
    {
        baseColour = buttonColour.brighter(0.5f);
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        baseColour = buttonColour.brighter(0.2f);
    }
    
    // Draw the button background
    g.setColour(baseColour.withAlpha(shouldDrawButtonAsDown ? 0.8f : 0.6f));
    g.fillRoundedRectangle(bounds, DualTriggerStyle::cornerRadius);
    
    // Draw the border
    g.setColour(DualTriggerStyle::disabledColour);
    g.drawRoundedRectangle(bounds, DualTriggerStyle::cornerRadius, 1.0f);
    
    // Draw the text
    g.setColour(DualTriggerStyle::textColour);
    g.setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
    g.drawText(getName(), getLocalBounds(), juce::Justification::centred);
}

//==============================================================================
// WaveformDisplay Implementation
//==============================================================================

WaveformDisplay::WaveformDisplay()
    : currentSample(nullptr),
      waveformColour(DualTriggerStyle::textColour),
      playingAnimationAlpha(0.0f),
      isDraggingStartMarker(false),
      isDraggingEndMarker(false)
{
    // Start the timer for animations
    startTimerHz(60); // 60 frames per second
}

WaveformDisplay::~WaveformDisplay()
{
    stopTimer();
}

void WaveformDisplay::setSample(AudioSample* sample)
{
    currentSample = sample;
    repaint();
}

void WaveformDisplay::setWaveformColour(juce::Colour colour)
{
    waveformColour = colour;
    repaint();
}

void WaveformDisplay::triggerPlayingAnimation()
{
    playingAnimationAlpha = 1.0f;
    startTimerHz(60); // Make sure the timer is running to animate the effect
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(DualTriggerStyle::controlBackgroundColour);
    
    // Draw a border - highlight the border if a sample is playing
    if (playingAnimationAlpha > 0.0f)
    {
        // Pulsating border for playing sample
        float borderWidth = 2.0f + playingAnimationAlpha * 1.0f;
        g.setColour(waveformColour.withAlpha(0.7f + playingAnimationAlpha * 0.3f));
        g.drawRoundedRectangle(0.0f, 0.0f, static_cast<float>(getWidth()), static_cast<float>(getHeight()),
                              static_cast<float>(DualTriggerStyle::cornerRadius), borderWidth);
    }
    else
    {
        // Normal border when not playing
        g.setColour(DualTriggerStyle::disabledColour);
        g.drawRoundedRectangle(0.0f, 0.0f, static_cast<float>(getWidth()), static_cast<float>(getHeight()),
                              static_cast<float>(DualTriggerStyle::cornerRadius), 1.0f);
    }
    
    if (currentSample == nullptr)
    {
        // Draw a message if no sample is loaded
        g.setColour(DualTriggerStyle::disabledColour);
        g.setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
        g.drawText("No Sample Loaded", getLocalBounds(), juce::Justification::centred);
        return;
    }
    
    // Safety check for thumbnail access - use a local variable
    juce::AudioThumbnail* thumbnailPtr = nullptr;
    
    try {
        thumbnailPtr = &currentSample->getThumbnail();
    }
    catch (...) {
        // Handle any potential exception gracefully
        g.setColour(DualTriggerStyle::disabledColour);
        g.setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
        g.drawText("Error accessing sample", getLocalBounds(), juce::Justification::centred);
        return;
    }
    
    // Safety check that we got a valid thumbnail
    if (thumbnailPtr == nullptr) {
        return;
    }
    
    // Draw the waveform
    g.setColour(playingAnimationAlpha > 0.0f ? waveformColour.brighter(0.2f) : waveformColour);
    
    if (thumbnailPtr->getTotalLength() > 0.0)
    {
        thumbnailPtr->drawChannels(g, getLocalBounds().reduced(2),
                              0.0, thumbnailPtr->getTotalLength(),
                              playingAnimationAlpha > 0.0f ? 0.9f : 0.7f); // Brighter when playing
    }
    
    // Draw a title for the current sample - handle exceptions safely
    g.setColour(playingAnimationAlpha > 0.0f ? juce::Colours::white : waveformColour);
    g.setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
    
    juce::String sampleName;
    try {
        sampleName = currentSample->getFileName();
    }
    catch (...) {
        sampleName = "Unknown Sample";
    }
    
    // Add "Playing" text if the sample is currently playing - USING ASCII ONLY
    if (playingAnimationAlpha > 0.0f)
    {
        sampleName = "PLAYING: " + sampleName;  // Changed from Unicode "▶" to ASCII "PLAYING:"
    }
    
    // Draw semi-transparent background for the text
    juce::Rectangle<int> textBounds = getLocalBounds().removeFromTop(DualTriggerStyle::controlHeight);
    g.setColour(DualTriggerStyle::backgroundColour.withAlpha(0.7f));
    g.fillRoundedRectangle(textBounds.toFloat(), DualTriggerStyle::cornerRadius);
    
    // Draw the text
    g.setColour(playingAnimationAlpha > 0.0f ? juce::Colours::white : waveformColour);
    g.drawText(sampleName, textBounds, juce::Justification::centred);
    
    // Draw the playing animation overlay
    if (playingAnimationAlpha > 0.0f)
    {
        g.setColour(waveformColour.withAlpha(playingAnimationAlpha * 0.3f));
        g.fillRoundedRectangle(0.0f, 0.0f, static_cast<float>(getWidth()), static_cast<float>(getHeight()),
                              static_cast<float>(DualTriggerStyle::cornerRadius));
    }
    
    // Draw the start and end markers
    drawMarkers(g);
}

void WaveformDisplay::resized()
{
    // Nothing to do here
}

void WaveformDisplay::mouseDown(const juce::MouseEvent& e)
{
    // No action needed since we're not using markers
    isDraggingStartMarker = false;
    isDraggingEndMarker = false;
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& e)
{
    // No action needed since we're not using markers
}

void WaveformDisplay::mouseUp(const juce::MouseEvent& e)
{
    isDraggingStartMarker = false;
    isDraggingEndMarker = false;
}

void WaveformDisplay::timerCallback()
{
    // Update the playing animation
    if (playingAnimationAlpha > 0.0f)
    {
        // Slower decay for more visible animation
        playingAnimationAlpha -= 0.02f;
        
        if (playingAnimationAlpha < 0.0f)
            playingAnimationAlpha = 0.0f;
        
        repaint();
    }
    else
    {
        // Stop the timer when not animating to save resources
        stopTimer();
    }
}

double WaveformDisplay::pixelToProportionX(int xPos) const
{
    if (getWidth() <= 0)
        return 0.0;
    
    return juce::jlimit(0.0, 1.0, static_cast<double>(xPos) / getWidth());
}

int WaveformDisplay::proportionToPixelX(double proportion) const
{
    return static_cast<int>(proportion * getWidth());
}

void WaveformDisplay::drawMarkers(juce::Graphics& g)
{
    // No markers to draw since we're not using start/end points
    // We always play from the beginning to the end
}

bool WaveformDisplay::isOverMarker(int xPos, double markerPosition) const
{
    // We're not using markers anymore, always return false
    return false;
}

//==============================================================================
// SampleListComponent Implementation
//==============================================================================

SampleListComponent::SampleListComponent()
    : sampleManager(nullptr),
      chainIndex(0),
      themeColour(DualTriggerStyle::textColour),
      itemHeight(30),  // Increased height for play button
      draggedItemIndex(-1),
      draggedYOffset(0),
      currentSampleIndex(-1),
      dropTargetIndex(-1)
{
    // Enable keyboard focus
    setWantsKeyboardFocus(true);
}

SampleListComponent::~SampleListComponent()
{
}

void SampleListComponent::setSampleManager(SampleManager* manager)
{
    sampleManager = manager;
    refresh();
}

void SampleListComponent::setChainIndex(int index)
{
    chainIndex = index;
}

void SampleListComponent::setColour(juce::Colour colour)
{
    themeColour = colour;
    repaint();
}

void SampleListComponent::refresh()
{
    if (sampleManager != nullptr)
    {
        currentSampleIndex = sampleManager->getCurrentSampleIndex();
    }
    
    repaint();
}

void SampleListComponent::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(DualTriggerStyle::backgroundColour);
    
    // Draw a border
    g.setColour(DualTriggerStyle::disabledColour);
    g.drawRect(getLocalBounds(), 1);
    
    if (sampleManager == nullptr || sampleManager->getNumSamples() == 0)
    {
        // Draw a message if no samples are loaded
        g.setColour(DualTriggerStyle::disabledColour);
        g.setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
        g.drawText("Drag & Drop Audio Files", getLocalBounds(), juce::Justification::centred);
        return;
    }
    
    // Draw each item
    int numSamples = sampleManager->getNumSamples();
    int y = 0;
    
    for (int i = 0; i < numSamples; ++i)
    {
        // Skip the dragged item
        if (i == draggedItemIndex && draggedItemIndex >= 0)
        {
            y += itemHeight;
            continue;
        }
        
        // Draw drop target indicator if needed
        if (dropTargetIndex == i && draggedItemIndex >= 0 && i != draggedItemIndex)
        {
            // Draw insert line
            g.setColour(themeColour.withAlpha(0.7f));
            g.fillRect(0, y - 2, getWidth(), 4);
        }
        
        // Draw the item
        bool isCurrentSample = (i == currentSampleIndex);
        drawItem(g, i, y, isCurrentSample);
        
        // Move to the next item
        y += itemHeight;
    }
    
    // Draw drop indicator at the end if needed
    if (dropTargetIndex == numSamples && draggedItemIndex >= 0)
    {
        g.setColour(themeColour.withAlpha(0.7f));
        g.fillRect(0, y - 2, getWidth(), 4);
    }
    
    // Draw the dragged item last (on top) with a shadow effect
    if (draggedItemIndex >= 0)
    {
        // Calculate position for the dragged item
        int mouseY = getMouseXYRelative().getY();
        int draggedY = mouseY - draggedYOffset;
        
        // Create shadow effect
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        juce::Rectangle<int> shadowBounds(2, draggedY + 2, getWidth() - 4, itemHeight - 2);
        g.fillRoundedRectangle(shadowBounds.toFloat(), 3.0f);
        
        // Draw the dragged item with highlight
        drawItem(g, draggedItemIndex, draggedY, true);
        
        // Add a border to show it's being dragged
        g.setColour(themeColour);
        juce::Rectangle<int> highlightBounds(0, draggedY, getWidth(), itemHeight);
        g.drawRoundedRectangle(highlightBounds.toFloat(), 3.0f, 2.0f);
    }
}

void SampleListComponent::resized()
{
    // Nothing to do here
}

void SampleListComponent::mouseDown(const juce::MouseEvent& e)
{
    // Check if the click is over a remove button
    int removeIndex = isOverRemoveButton(e.x, e.y);
    
    if (removeIndex >= 0)
    {
        // Remove the sample
        if (sampleManager != nullptr)
        {
            sampleManager->removeSample(removeIndex);
            refresh();
        }
        
        return;
    }
    
    // Check if the click is over a play button
    int playIndex = isOverPlayButton(e.x, e.y);
    
    if (playIndex >= 0)
    {
        // Set as current and trigger the sample
        if (sampleManager != nullptr)
        {
            DBG("SampleListComponent::mouseDown() - Play button clicked for sample " + juce::String(playIndex));
            
            // FIRST: Stop any currently playing samples
            sampleManager->stopPlayback();
            
            // SECOND: Update the current index immediately
            sampleManager->setCurrentSampleIndex(playIndex);
            currentSampleIndex = playIndex;
            
            // THIRD: Notify the parent to update the waveform display
            juce::Component* parent = getParentComponent();
            if (auto* chainControl = dynamic_cast<ChainControlComponent*>(parent))
            {
                // Update display first
                chainControl->updateDisplay();
                
                // Then trigger sample immediately
                chainControl->triggerSample();
            }
            
            repaint();
        }
        
        return;
    }
    
    // Check if the click is over an item
    int itemIndex = getItemIndexFromY(e.y);
    
    if (itemIndex >= 0 && sampleManager != nullptr)
    {
        // Set the current sample
        sampleManager->setCurrentSampleIndex(itemIndex);
        currentSampleIndex = itemIndex;
        
        // Don't start dragging immediately - wait for actual drag to begin
        // Just store the initial position data
        draggedItemIndex = itemIndex;
        draggedYOffset = e.y - (itemIndex * itemHeight);
        
        repaint();
    }
}

void SampleListComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggedItemIndex >= 0)
    {
        // Calculate visual position of dragged item based on mouse
        int mouseY = e.y;
        
        // Require minimum movement before starting actual drag (reduces accidental drags)
        if (std::abs(mouseY - (draggedItemIndex * itemHeight + draggedYOffset)) < 5)
        {
            // Not enough movement - just repaint and return
            repaint();
            return;
        }
        
        // Update drag offset
        draggedYOffset = mouseY - (draggedItemIndex * itemHeight);
        
        // Get position the sample would go if dropped now
        int targetIndex = getItemIndexFromY(mouseY);
        
        // Bound the target index to valid values
        int maxIndex = (sampleManager != nullptr) ? sampleManager->getNumSamples() - 1 : 0;
        targetIndex = juce::jlimit(0, maxIndex, targetIndex);
        
        // Update drop target for visual indicator
        dropTargetIndex = targetIndex;
        
        // Only move the sample when it clearly moves to a new position
        // This creates a more stable drag experience
        if (targetIndex >= 0 && targetIndex != draggedItemIndex && sampleManager != nullptr
            && std::abs(mouseY - (targetIndex * itemHeight + itemHeight/2)) < itemHeight/3)
        {
            // Move the item in the sample manager
            if (sampleManager->moveSample(draggedItemIndex, targetIndex))
            {
                // Update the dragged index to its new position
                draggedItemIndex = targetIndex;
                currentSampleIndex = sampleManager->getCurrentSampleIndex();
            }
        }
        
        repaint();
    }
}

void SampleListComponent::mouseUp(const juce::MouseEvent& e)
{
    // End dragging
    draggedItemIndex = -1;
    dropTargetIndex = -1;
    repaint();
}

bool SampleListComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Check if any of the files are audio files
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    for (const auto& file : files)
    {
        juce::File f(file);
        
        if (formatManager.findFormatForFileExtension(f.getFileExtension()) != nullptr)
        {
            return true;
        }
    }
    
    return false;
}

void SampleListComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    // Add the dropped files to the sample manager
    if (sampleManager != nullptr)
    {
        sampleManager->addSamplesFromDroppedFiles(files);
        refresh();
    }
}

void SampleListComponent::actionListenerCallback(const juce::String& message)
{
    // Check if this is a chain triggered message
    if (message.startsWith("ChainTriggered_") && message.getTrailingIntValue() == chainIndex)
    {
        // Update the current sample index
        if (sampleManager != nullptr)
        {
            currentSampleIndex = sampleManager->getCurrentSampleIndex();
            
            // Notify the parent to update the waveform display
            juce::Component* parent = getParentComponent();
            if (auto* chainControl = dynamic_cast<ChainControlComponent*>(parent))
            {
                // Update the display to show the currently playing sample
                chainControl->updateDisplay();
            }
            
            repaint();
        }
    }
}

int SampleListComponent::getItemIndexFromY(int y) const
{
    if (sampleManager == nullptr)
        return -1;
    
    int index = y / itemHeight;
    
    if (index >= 0 && index < sampleManager->getNumSamples())
    {
        return index;
    }
    
    return -1;
}

void SampleListComponent::drawItem(juce::Graphics& g, int index, int y, bool isHighlighted)
{
    if (sampleManager == nullptr || index < 0 || index >= sampleManager->getNumSamples())
        return;
    
    // Get the sample
    AudioSample* sample = sampleManager->getSample(index);
    
    if (sample == nullptr)
        return;
    
    // Calculate the item bounds
    juce::Rectangle<int> itemBounds(0, y, getWidth(), itemHeight);
    
    // Check if this is the current sample that's playing
    bool isCurrentSample = (index == currentSampleIndex);
    
    // Draw the background
    if (isCurrentSample)
    {
        // Currently playing sample gets a more noticeable highlight
        g.setColour(themeColour.withAlpha(0.5f));
        g.fillRect(itemBounds);
        
        // Add a pulsing border for the current sample
        g.setColour(themeColour);
        g.drawRect(itemBounds, 2);
    }
    else if (isHighlighted)
    {
        // Regular highlight for selected but not currently playing
        g.setColour(themeColour.withAlpha(0.3f));
        g.fillRect(itemBounds);
    }
    
    // Draw the border (if not already drawn for current sample)
    if (!isCurrentSample)
    {
        g.setColour(DualTriggerStyle::disabledColour);
        g.drawRect(itemBounds, 1);
    }
    
    // Draw the item number
    g.setColour(isCurrentSample ? juce::Colours::white : DualTriggerStyle::textColour);
    g.setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeSmall)));
    g.drawText(juce::String(index + 1), itemBounds.withWidth(20), juce::Justification::centred);
    
    // Draw the filename
    g.setColour(isCurrentSample ? juce::Colours::white : (isHighlighted ? themeColour : DualTriggerStyle::textColour));
    
    // Create font with appropriate size and bold if it's the current sample
    float fontSize = isCurrentSample ? DualTriggerStyle::fontSizeMedium + 1.0f : DualTriggerStyle::fontSizeMedium;
    juce::Font fileNameFont(juce::FontOptions(fontSize));
    if (isCurrentSample)
        fileNameFont.setStyle(juce::Font::bold); // Use setStyle for options
    
    g.setFont(fileNameFont);
    g.drawText(sample->getFileName(), itemBounds.withTrimmedLeft(25).withTrimmedRight(50), juce::Justification::centredLeft);
    
    // Draw the play button - USING ASCII TRIANGLE
    g.setColour(isCurrentSample ? themeColour.brighter(0.2f) : themeColour.withAlpha(0.7f));
    juce::Rectangle<int> playButtonBounds(getWidth() - 48, y + 5, 20, itemHeight - 10);
    g.fillRoundedRectangle(playButtonBounds.toFloat(), 2.0f);
    
    // Draw play icon using ASCII ">" instead of Unicode triangle
    g.setColour(DualTriggerStyle::textColour);
    g.setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium).withStyle(juce::Font::bold)));
    g.drawText(">", playButtonBounds, juce::Justification::centred);  // ASCII ">" instead of Unicode triangle
    
    // Draw the remove button
    g.setColour(DualTriggerStyle::disabledColour);
    juce::Rectangle<int> removeButtonBounds(getWidth() - 24, y + 5, 20, itemHeight - 10);
    g.drawRoundedRectangle(removeButtonBounds.toFloat(), 2.0f, 1.0f);
    
    g.setColour(DualTriggerStyle::textColour);
    g.setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
    g.drawText("X", removeButtonBounds, juce::Justification::centred);
}

int SampleListComponent::isOverRemoveButton(int x, int y) const
{
    // Calculate which item contains this position
    int itemIndex = getItemIndexFromY(y);
    
    if (itemIndex >= 0 && x > getWidth() - 24 && x < getWidth() - 4)
    {
        return itemIndex;
    }
    
    return -1;
}

int SampleListComponent::isOverPlayButton(int x, int y) const
{
    // Calculate which item contains this position
    int itemIndex = getItemIndexFromY(y);
    
    if (itemIndex >= 0 && x > getWidth() - 48 && x < getWidth() - 28)
    {
        return itemIndex;
    }
    
    return -1;
}

//==============================================================================
// CustomLookAndFeel Implementation
//==============================================================================

CustomLookAndFeel::CustomLookAndFeel()
    : accentColour(DualTriggerStyle::highlightColour) // Initialize with the new highlight colour
{
    // Set up the default colors using the new DualTriggerStyle
    setColour(juce::ResizableWindow::backgroundColourId, DualTriggerStyle::backgroundColour);
    setColour(juce::Label::textColourId, DualTriggerStyle::textColour);
    setColour(juce::TextButton::buttonColourId, DualTriggerStyle::controlBackgroundColour); // Normal button background
    setColour(juce::TextButton::buttonOnColourId, DualTriggerStyle::highlightColour); // Button toggled on background
    setColour(juce::TextButton::textColourOffId, DualTriggerStyle::textColour);
    setColour(juce::TextButton::textColourOnId, DualTriggerStyle::backgroundColour); // Text for toggled on button (e.g. dark text on light highlight)

    setColour(juce::Slider::backgroundColourId, DualTriggerStyle::controlBackgroundColour.darker(0.5f)); // Darker groove for slider
    setColour(juce::Slider::trackColourId, DualTriggerStyle::highlightColour); // Filled part of the track
    setColour(juce::Slider::thumbColourId, DualTriggerStyle::highlightColour.brighter(0.2f)); // Thumb a bit brighter

    setColour(juce::ComboBox::backgroundColourId, DualTriggerStyle::controlBackgroundColour);
    setColour(juce::ComboBox::textColourId, DualTriggerStyle::textColour);
    setColour(juce::ComboBox::arrowColourId, DualTriggerStyle::textColour.withAlpha(0.7f));
    setColour(juce::ComboBox::outlineColourId, DualTriggerStyle::disabledColour.withAlpha(0.5f));

    setColour(juce::PopupMenu::backgroundColourId, DualTriggerStyle::controlBackgroundColour.brighter(0.1f));
    setColour(juce::PopupMenu::textColourId, DualTriggerStyle::textColour);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, DualTriggerStyle::highlightColour);
    setColour(juce::PopupMenu::highlightedTextColourId, DualTriggerStyle::backgroundColour); // Text on highlighted menu item
    setColour(juce::PopupMenu::headerTextColourId, DualTriggerStyle::textColour.brighter(0.2f));

    setColour(juce::ScrollBar::thumbColourId, DualTriggerStyle::disabledColour.brighter(0.3f));
    setColour(juce::ScrollBar::backgroundColourId, DualTriggerStyle::backgroundColour.darker(0.3f));

    setColour(juce::TextEditor::backgroundColourId, DualTriggerStyle::controlBackgroundColour.darker(0.2f));
    setColour(juce::TextEditor::textColourId, DualTriggerStyle::textColour);
    setColour(juce::TextEditor::highlightColourId, DualTriggerStyle::highlightColour.withAlpha(0.5f));
    setColour(juce::TextEditor::outlineColourId, DualTriggerStyle::disabledColour.withAlpha(0.5f));
    setColour(juce::TextEditor::focusedOutlineColourId, DualTriggerStyle::highlightColour.withAlpha(0.8f));

    // Set a default sans-serif font if desired, though JUCE handles this reasonably well.
    // setDefaultSansSerifTypefaceName("Your Bundled Sans Serif Font Name"); // Example
}

void CustomLookAndFeel::setAccentColour(juce::Colour colour)
{
    accentColour = colour; // This is the main highlight color from DualTriggerStyle
    // Update specific components if their accent isn't directly from DualTriggerStyle::highlightColour
    setColour(juce::Slider::trackColourId, accentColour);
    setColour(juce::Slider::thumbColourId, accentColour.brighter(0.2f));
    setColour(juce::TextButton::buttonOnColourId, accentColour);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accentColour);
    // Potentially update other colours that should derive from this accent
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                      juce::Slider& slider)
{
    auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
    auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);

    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmin(8.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;

    // Background track
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(),
                                bounds.getCentreY(),
                                arcRadius,
                                arcRadius,
                                0.0f,
                                rotaryStartAngle,
                                rotaryEndAngle,
                                true);

    g.setColour(DualTriggerStyle::controlBackgroundColour.darker(0.7f)); // Darker track
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    // Filled portion (value)
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(),
                               bounds.getCentreY(),
                               arcRadius,
                               arcRadius,
                               0.0f,
                               rotaryStartAngle,
                               toAngle,
                               true);

        g.setColour(accentColour); // Use the modernized accentColour
        g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }

    // Thumb (simple dot or line)
    auto thumbWidth = lineW * 1.2f;
    juce::Point<float> thumbPoint(bounds.getCentreX() + arcRadius * std::cos(toAngle - juce::MathConstants<float>::halfPi),
                                  bounds.getCentreY() + arcRadius * std::sin(toAngle - juce::MathConstants<float>::halfPi));

    g.setColour(slider.findColour(juce::Slider::thumbColourId));
    // g.fillEllipse(juce::Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));
    // Draw a line as a pointer instead of a full thumb for a cleaner look
    g.drawLine(bounds.getCentreX(), bounds.getCentreY(), thumbPoint.getX(), thumbPoint.getY(), lineW * 0.6f);
}

void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                     float sliderPos, float minSliderPos, float maxSliderPos,
                                     const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    // Use new styling constants
    const float trackWidth = 4.0f; // Thinner track for a cleaner look
    const float thumbRadius = (float)getSliderThumbRadius(slider); // Dynamic thumb radius

    juce::Rectangle<float> trackRect;
    juce::Rectangle<float> thumbRect(thumbRadius * 2.0f, thumbRadius * 2.0f);

    // Background track
    g.setColour(DualTriggerStyle::controlBackgroundColour.darker(0.7f)); // Darker, less prominent track
    if (style == juce::Slider::LinearHorizontal)
    {
        trackRect = { (float)x, y + height * 0.5f - trackWidth * 0.5f, (float)width, trackWidth };
        g.fillRoundedRectangle(trackRect, trackWidth * 0.5f);
        thumbRect.setCentre(sliderPos, trackRect.getCentreY());
    }
    else // LinearVertical
    {
        trackRect = { x + width * 0.5f - trackWidth * 0.5f, (float)y, trackWidth, (float)height };
        g.fillRoundedRectangle(trackRect, trackWidth * 0.5f);
        thumbRect.setCentre(trackRect.getCentreX(), sliderPos);
    }

    // Filled portion
    g.setColour(accentColour); // Use the modernized accentColour
    if (style == juce::Slider::LinearHorizontal)
    {
        g.fillRoundedRectangle(trackRect.withWidth(thumbRect.getCentreX() - trackRect.getX()), trackWidth * 0.5f);
    }
    else // LinearVertical
    {
        g.fillRoundedRectangle(trackRect.withTop(thumbRect.getCentreY()), trackWidth * 0.5f);
    }

    // Thumb
    g.setColour(slider.findColour(juce::Slider::thumbColourId));
    g.fillEllipse(thumbRect); // Simple filled ellipse for thumb
    
    // Outline for thumb for better definition
    g.setColour(DualTriggerStyle::backgroundColour.brighter(0.2f)); // Subtle outline
    g.drawEllipse(thumbRect, 0.5f);
}


void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                         const juce::Colour& backgroundColour, // This is TextButton::buttonColourId
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown)
{
    auto cornerRadius = (float)DualTriggerStyle::cornerRadius;
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f); // For border

    auto baseColour = button.getToggleState() ? accentColour // Use accent for toggled ON state
                                             : DualTriggerStyle::controlBackgroundColour; // Normal background

    if (button.isMouseOver() && button.isEnabled())
        baseColour = baseColour.brighter(0.2f);
    if (shouldDrawButtonAsDown && button.isEnabled())
        baseColour = baseColour.darker(0.15f);
    
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Subtle border
    g.setColour(DualTriggerStyle::disabledColour.withAlpha(0.5f));
    if (button.getToggleState() || (shouldDrawButtonAsDown && button.isEnabled()))
        g.setColour(accentColour.darker(0.3f)); // Darker border for active/pressed states

    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                 int buttonX, int buttonY, int buttonW, int buttonH,
                                 juce::ComboBox& box)
{
    auto cornerRadius = (float)DualTriggerStyle::cornerRadius;
    juce::Rectangle<int> boxBounds(0, 0, width, height);

    // Background
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(boxBounds.toFloat(), cornerRadius);

    // Outline
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    if (box.isMouseOver() || box.isKeyboardFocusOwner())
        g.setColour(accentColour.withAlpha(0.7f));
    g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f), cornerRadius, 1.0f);

    // Arrow
    juce::Path arrowPath; // Renamed to avoid conflict with member if any
    arrowPath.startNewSubPath(buttonX + buttonW * 0.3f, buttonY + buttonH * 0.35f);
    arrowPath.lineTo(buttonX + buttonW * 0.5f, buttonY + buttonH * 0.65f);
    arrowPath.lineTo(buttonX + buttonW * 0.7f, buttonY + buttonH * 0.35f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.strokePath(arrowPath, juce::PathStrokeType(1.5f));
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                      bool isSeparator, bool isActive, bool isHighlighted,
                                      bool isTicked, bool hasSubMenu, const juce::String& text,
                                      const juce::String& shortcutKeyText, const juce::Drawable* icon,
                                      const juce::Colour* /*textColourToUse*/) // textColourToUse is often nullptr
{
    juce::Colour textColour = findColour(juce::PopupMenu::textColourId);

    if (isSeparator)
    {
        auto r = area.reduced(5, 0);
        r.removeFromTop(r.getHeight() / 2 - 1);
        g.setColour(DualTriggerStyle::disabledColour.withAlpha(0.3f));
        g.fillRect(r.removeFromTop(1));
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour(findColour(juce::PopupMenu::highlightedBackgroundColourId));
        g.fillRect(area);
        textColour = findColour(juce::PopupMenu::highlightedTextColourId);
    }
    else if (isActive)
    {
        // No specific background for active but not highlighted, or use a very subtle one
        // g.setColour(findColour(juce::PopupMenu::backgroundColourId));
        // g.fillRect(area);
    }
    else
    {
        g.setColour(DualTriggerStyle::disabledColour.darker(0.5f)); // For disabled menu items
        // g.fillRect(area); // Don't fill, just change text color
    }
    
    g.setColour(isActive ? textColour : DualTriggerStyle::disabledColour); // Text color update for disabled

    auto r = area.reduced(1); // Padding inside item

    if (isTicked)
    {
        // Simple ASCII checkmark
        g.setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium * 0.9f)));
        g.drawText("*", r.removeFromLeft(DualTriggerStyle::controlHeight -4), juce::Justification::centred);
    }
    
    if (icon != nullptr)
    {
        icon->drawWithin(g, r.removeFromLeft(DualTriggerStyle::controlHeight -4).toFloat(),
                         juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
    }

    g.setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium))); // Use new font size
    auto textBounds = r;
    if (hasSubMenu)
    {
        auto arrowZone = r.removeFromRight(DualTriggerStyle::controlHeight / 2);
        // Simple ASCII arrow for submenu
        g.setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeSmall)));
        g.drawText(">", arrowZone, juce::Justification::centred);
    }
    g.drawText(text, textBounds, juce::Justification::centredLeft, true);

    if (shortcutKeyText.isNotEmpty())
    {
        g.setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeSmall)));
        g.drawText(shortcutKeyText, r.removeFromRight(area.getWidth()/3), juce::Justification::centredRight, true);
    }
}


juce::Font CustomLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    // Use the new font sizes from DualTriggerStyle
    return juce::Font(juce::FontOptions(juce::jmin((float)buttonHeight * 0.7f, DualTriggerStyle::fontSizeMedium)));
}

int CustomLookAndFeel::getSliderThumbRadius(juce::Slider& slider)
{
    // Adjust thumb radius for a modern look, perhaps slightly larger for easier interaction
    if (slider.getSliderStyle() == juce::Slider::LinearHorizontal ||
        slider.getSliderStyle() == juce::Slider::LinearVertical)
        return 8; // Slightly larger thumb for linear sliders

    return 6; // Default for rotary or other styles
}

//==============================================================================
// ChainControlComponent Implementation
//==============================================================================

ChainControlComponent::ChainControlComponent()
    : sampleManager(nullptr),
      chainManager(nullptr),
      chainIndex(0),
      themeColour(DualTriggerStyle::textColour)
{
    // Create the look and feel
    lookAndFeel = std::make_unique<CustomLookAndFeel>();
    setLookAndFeel(lookAndFeel.get());
    
    // Create the title label
    titleLabel = std::make_unique<juce::Label>("titleLabel", "Chain 1");
    titleLabel->setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeHeader).withStyle(juce::Font::bold)));
    titleLabel->setJustificationType(juce::Justification::centred);
    titleLabel->setColour(juce::Label::textColourId, themeColour);
    addAndMakeVisible(titleLabel.get());
    
    // Create the trigger note label
    triggerNoteLabel = std::make_unique<juce::Label>("triggerNoteLabel", "Trigger Note:");
    triggerNoteLabel->setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
    triggerNoteLabel->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(triggerNoteLabel.get());
    
    // Create the trigger note combo box
    triggerNoteComboBox = std::make_unique<juce::ComboBox>("triggerNoteComboBox");
    initializeTriggerNoteComboBox();
    triggerNoteComboBox->addListener(this);
    addAndMakeVisible(triggerNoteComboBox.get());
    
    // Create the volume label
    volumeLabel = std::make_unique<juce::Label>("volumeLabel", "Volume:");
    volumeLabel->setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
    volumeLabel->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(volumeLabel.get());
    
    // Create the volume slider
    volumeSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    volumeSlider->setRange(0.0, 1.0, 0.01);
    volumeSlider->setValue(1.0);
    volumeSlider->addListener(this);
    addAndMakeVisible(volumeSlider.get());
    
    // Create the sample list
    sampleList = std::make_unique<SampleListComponent>();
    sampleList->setChainIndex(chainIndex);
    sampleList->setColour(themeColour);
    addAndMakeVisible(sampleList.get());
    
    // Create the clear button
    clearButton = std::make_unique<juce::TextButton>("clearButton", "Clear All");
    clearButton->addListener(this);
    addAndMakeVisible(clearButton.get());
    
    // Create the velocity sensitive button
    velocitySensitiveButton = std::make_unique<juce::ToggleButton>("velocitySensitiveButton");
    velocitySensitiveButton->setButtonText("Velocity Sensitive");
    velocitySensitiveButton->setToggleState(true, juce::dontSendNotification);
    velocitySensitiveButton->addListener(this);
    addAndMakeVisible(velocitySensitiveButton.get());
    
    // Create the velocity threshold slider
    velocityThresholdSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow);
    velocityThresholdSlider->setRange(1, 127, 1);
    velocityThresholdSlider->setValue(1);
    velocityThresholdSlider->setTextValueSuffix("");
    velocityThresholdSlider->setDoubleClickReturnValue(true, 1);
    velocityThresholdSlider->addListener(this);
    addAndMakeVisible(velocityThresholdSlider.get());
    
    // Create the velocity threshold label
    velocityThresholdLabel = std::make_unique<juce::Label>("velocityThresholdLabel", "Velocity Threshold:");
    velocityThresholdLabel->setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
    velocityThresholdLabel->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(velocityThresholdLabel.get());
    
    // Create the pitch shift slider
    pitchShiftSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow);
    pitchShiftSlider->setRange(-12.0, 12.0, 0.1);
    pitchShiftSlider->setValue(0.0);
    pitchShiftSlider->setTextValueSuffix(" st");
    pitchShiftSlider->setDoubleClickReturnValue(true, 0.0); // Double-click resets to 0
    pitchShiftSlider->setTooltip("Changes pitch and speed (higher pitch = faster playback)");
    pitchShiftSlider->addListener(this);
    addAndMakeVisible(pitchShiftSlider.get());
    
    // Create the pitch shift label
    pitchShiftLabel = std::make_unique<juce::Label>("pitchShiftLabel", "Pitch Shift:");
    pitchShiftLabel->setFont(juce::Font(juce::FontOptions(DualTriggerStyle::fontSizeMedium)));
    pitchShiftLabel->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(pitchShiftLabel.get());
    
    // Create the waveform display
    waveformDisplay = std::make_unique<WaveformDisplay>();
    waveformDisplay->setWaveformColour(themeColour);
    addAndMakeVisible(waveformDisplay.get());
    
    // Create the trigger button
    triggerButton = std::make_unique<TriggerButton>("Play");
    triggerButton->setColour(themeColour);
    triggerButton->addListener(this);
    addAndMakeVisible(triggerButton.get());
    
    // Create the trigger indicator
    triggerIndicator = std::make_unique<TriggerIndicator>();
    triggerIndicator->setColour(themeColour);
    addAndMakeVisible(triggerIndicator.get());
}

ChainControlComponent::~ChainControlComponent()
{
    setLookAndFeel(nullptr);
}

void ChainControlComponent::setSampleManager(SampleManager* manager)
{
    sampleManager = manager;
    sampleList->setSampleManager(manager);
    
    if (sampleManager != nullptr)
    {
        // Update the pitch shift slider to match the sample manager's value
        pitchShiftSlider->setValue(sampleManager->getPitchShift(), juce::dontSendNotification);
        
        // Set up waveform display if there's a sample loaded
        if (sampleManager->getCurrentSample() != nullptr)
        {
            waveformDisplay->setSample(sampleManager->getCurrentSample());
        }
        else
        {
            waveformDisplay->setSample(nullptr);
        }
    }
    else
    {
        waveformDisplay->setSample(nullptr);
    }
    
    updateDisplay();
}

void ChainControlComponent::setChainIndex(int index)
{
    chainIndex = index;
    sampleList->setChainIndex(index);
    
    // Update the title
    titleLabel->setText("Chain " + juce::String(index + 1), juce::dontSendNotification);
    
    updateDisplay();
}

void ChainControlComponent::setChainManager(ChainManager* manager)
{
    chainManager = manager;
    
    if (chainManager != nullptr)
    {
        // Add this component as a listener to the chain manager
        chainManager->addListener(this);
        
        // Add the sample list as a listener to the chain manager
        chainManager->addListener(sampleList.get());
        
        // Update the trigger note combo box
        triggerNoteComboBox->setSelectedId(chainManager->getTriggerNote(chainIndex) + 1, juce::dontSendNotification);
        
        // Update the volume slider
        volumeSlider->setValue(chainManager->getChainVolume(chainIndex), juce::dontSendNotification);
    }
}

void ChainControlComponent::setColour(juce::Colour colour)
{
    themeColour = colour;
    
    // Update the title label color
    titleLabel->setColour(juce::Label::textColourId, themeColour);
    
    // Update the sample list color
    sampleList->setColour(themeColour);
    
    // Update the waveform display color
    waveformDisplay->setWaveformColour(themeColour);
    
    // Update the trigger button color
    triggerButton->setColour(themeColour);
    
    // Update the trigger indicator color
    triggerIndicator->setColour(themeColour);
    
    // Update the look and feel
    lookAndFeel->setAccentColour(themeColour);
    
    repaint();
}

void ChainControlComponent::setVelocitySensitivityEnabled(bool enabled)
{
    velocitySensitiveButton->setToggleState(enabled, juce::dontSendNotification);
    
    if (sampleManager != nullptr)
    {
        sampleManager->setVelocitySensitive(enabled);
    }
}

void ChainControlComponent::setVelocityThreshold(int threshold)
{
    velocityThresholdSlider->setValue(threshold, juce::dontSendNotification);
    
    if (sampleManager != nullptr)
    {
        sampleManager->setVelocityThreshold(threshold);
    }
}

void ChainControlComponent::setPitchShift(float semitones)
{
    pitchShiftSlider->setValue(semitones, juce::dontSendNotification);
    
    if (sampleManager != nullptr)
    {
        sampleManager->setPitchShift(semitones);
    }
}

void ChainControlComponent::updateDisplay()
{
    // Update the waveform display
    if (sampleManager != nullptr && sampleManager->getCurrentSample() != nullptr)
    {
        AudioSample* currentSample = sampleManager->getCurrentSample();
        waveformDisplay->setSample(currentSample);
        
        // Update the pitch slider to match the current sample
        pitchShiftSlider->setValue(currentSample->getPitchShift(), juce::dontSendNotification);
    }
    else
    {
        waveformDisplay->setSample(nullptr);
    }
    
    // Update the sample list
    sampleList->refresh();
}

void ChainControlComponent::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(DualTriggerStyle::backgroundColour);
    
    // Draw a border
    g.setColour(themeColour.withAlpha(0.3f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), static_cast<float>(DualTriggerStyle::cornerRadius), 2.0f);
}

void ChainControlComponent::resized()
{
    // Calculate the layout using new DualTriggerStyle constants
    const int margin = DualTriggerStyle::padding; // New padding
    const int currentControlHeight = DualTriggerStyle::controlHeight; // New control height
    const int buttonWidth = 120; // Keep or adjust as needed
    int y = margin;
    
    // Position the title label
    titleLabel->setBounds(margin, y, getWidth() - margin * 2, DualTriggerStyle::headerHeight); // New header height
    y += DualTriggerStyle::headerHeight + margin;
    
    // Position the trigger note controls
    int labelWidth = 100; // Example width, adjust as needed
    triggerNoteLabel->setBounds(margin, y, labelWidth, currentControlHeight);
    
    int comboBoxWidth = 150; // Example width
    triggerNoteComboBox->setBounds(margin + labelWidth + margin, y, comboBoxWidth, currentControlHeight);
    
    int indicatorWidth = 30;
    triggerIndicator->setBounds(triggerNoteComboBox->getRight() + margin, y, indicatorWidth, currentControlHeight);
    
    // Position the trigger button on the right, aligned with triggerNoteComboBox
    int triggerButtonWidth = 80; // Example width
    triggerButton->setBounds(getWidth() - margin - triggerButtonWidth, y, triggerButtonWidth, currentControlHeight);
    
    y += currentControlHeight + margin;
    
    // Position the volume controls
    int volLabelWidth = 60;
    volumeLabel->setBounds(margin, y, volLabelWidth, currentControlHeight);
    volumeSlider->setBounds(margin + volLabelWidth + margin, y, getWidth() - (margin * 3) - volLabelWidth, currentControlHeight);
    y += currentControlHeight + margin;
    
    // Position the velocity sensitivity button
    velocitySensitiveButton->setBounds(margin, y, 200, currentControlHeight); // Width can be adjusted
    y += currentControlHeight + margin;

    // Position the velocity threshold controls (Rotary)
    int velThresholdLabelWidth = 130;
    velocityThresholdLabel->setBounds(margin, y, velThresholdLabelWidth, currentControlHeight);
    // Rotary slider with text box below might need more height.
    // Slider itself square, text box adds to height.
    int rotaryDiameter = 70; // Example diameter
    int velThreshSliderHeight = rotaryDiameter + 20; // Approx height for slider + text box
    velocityThresholdSlider->setBounds(margin + velThresholdLabelWidth + margin, y, rotaryDiameter, velThreshSliderHeight);
    y += std::max(currentControlHeight, velThreshSliderHeight) + margin;

    // Position the pitch shift controls (Rotary)
    int pitchShiftLabelWidth = 80;
    pitchShiftLabel->setBounds(margin, y, pitchShiftLabelWidth, currentControlHeight);
    int pitchShiftSliderHeight = rotaryDiameter + 20; // Approx height for slider + text box
    pitchShiftSlider->setBounds(margin + pitchShiftLabelWidth + margin, y, rotaryDiameter, pitchShiftSliderHeight);
    y += std::max(currentControlHeight, pitchShiftSliderHeight) + margin;
    
    // Position the waveform display
    int waveformHeight = 100; // Example height
    waveformDisplay->setBounds(margin, y, getWidth() - (margin * 2), waveformHeight);
    y += waveformHeight + margin;
    
    // Position the sample buttons (Clear All)
    clearButton->setBounds(margin, y, buttonWidth, currentControlHeight);
    y += currentControlHeight + margin;
    
    // Position the sample list (takes remaining space)
    sampleList->setBounds(margin, y, getWidth() - (margin * 2), getHeight() - y - margin);
}

void ChainControlComponent::actionListenerCallback(const juce::String& message)
{
    // Check if this is a chain triggered message
    if (message.startsWith("ChainTriggered_") && message.getTrailingIntValue() == chainIndex)
    {
        // Trigger visual feedback
        waveformDisplay->triggerPlayingAnimation();
        triggerIndicator->triggerIndicator();
        
        // Update the display
        updateDisplay();
    }
}

void ChainControlComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == volumeSlider.get() && chainManager != nullptr)
    {
        // Update the chain volume
        chainManager->setChainVolume(chainIndex, static_cast<float>(volumeSlider->getValue()));
    }
    else if (slider == velocityThresholdSlider.get() && sampleManager != nullptr)
    {
        // Update the velocity threshold
        sampleManager->setVelocityThreshold(static_cast<int>(velocityThresholdSlider->getValue()));
    }
    else if (slider == pitchShiftSlider.get() && sampleManager != nullptr)
    {
        // Update the pitch shift
        float pitchValue = static_cast<float>(pitchShiftSlider->getValue());
        sampleManager->setPitchShift(pitchValue);
        
        // Update the display to reflect changes
        updateDisplay();
    }
}

void ChainControlComponent::buttonClicked(juce::Button* button)
{
    if (button == clearButton.get() && sampleManager != nullptr)
    {
        // First set waveform display to null (so it doesn't try to access samples being deleted)
        waveformDisplay->setSample(nullptr);
        
        // Small delay to ensure UI components finish using any sample references
        juce::Thread::yield();
        
        // Now clear all samples
        sampleManager->clearAllSamples();
        
        // Refresh the list display
        sampleList->refresh();
    }
    else if (button == velocitySensitiveButton.get() && sampleManager != nullptr)
    {
        // Update velocity sensitivity
        sampleManager->setVelocitySensitive(velocitySensitiveButton->getToggleState());
    }
    else if (button == triggerButton.get())
    {
        if (sampleManager != nullptr && chainManager != nullptr && sampleManager->getNumSamples() > 0)
        {
            // Stop current playback before starting new playback
            sampleManager->stopPlayback();
            
            // Get the current sample index before advancing
            int currentIndex = sampleManager->getCurrentSampleIndex();
            
            // Calculate the next sample index (handle wraparound)
            int nextIndex = (currentIndex + 1) % sampleManager->getNumSamples();
            if (nextIndex < 0 && sampleManager->getNumSamples() > 0)
                nextIndex = 0;
            
            // Set the current sample directly
            sampleManager->setCurrentSampleIndex(nextIndex);
            
            // Update the UI to reflect the new current sample
            updateDisplay();
            
            // Trigger the newly selected sample immediately
            if (chainManager != nullptr)
            {
                chainManager->triggerChainSample(chainIndex, 100); // Use medium velocity
            }
        }
    }
}

void ChainControlComponent::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == triggerNoteComboBox.get() && chainManager != nullptr)
    {
        // Update the trigger note
        int noteNumber = triggerNoteComboBox->getSelectedId() - 1;
        chainManager->setTriggerNote(chainIndex, noteNumber);
    }
}

bool ChainControlComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Check if any of the files are audio files
    for (const auto& file : files)
    {
        juce::File f(file);
        juce::String extension = f.getFileExtension().toLowerCase();
        
        if (extension == ".wav" || extension == ".mp3" ||
            extension == ".aiff" || extension == ".aif" ||
            extension == ".flac")
        {
            return true;
        }
    }
    
    return false;
}

void ChainControlComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    // Load audio files
    if (sampleManager != nullptr)
    {
        // Filter out only audio files
        juce::StringArray audioFiles;
        
        for (const auto& file : files)
        {
            juce::File f(file);
            juce::String extension = f.getFileExtension().toLowerCase();
            
            if (extension == ".wav" || extension == ".mp3" ||
                extension == ".aiff" || extension == ".aif" ||
                extension == ".flac")
            {
                audioFiles.add(file);
            }
        }
        
        if (audioFiles.size() > 0)
        {
            sampleManager->addSamplesFromDroppedFiles(audioFiles);
            updateDisplay();
        }
    }
}

void ChainControlComponent::loadSamples()
{
    // Create a file chooser
    juce::FileChooser chooser("Select Audio Files",
                             juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                             "*.wav;*.mp3;*.aiff;*.aif;*.flac");
    
    // Show the dialog asynchronously
    chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectMultipleItems,
                      [this](const juce::FileChooser& fc)
    {
        // Check if any files were selected
        if (fc.getResults().size() > 0 && sampleManager != nullptr)
        {
            // Get the selected files
            juce::Array<juce::File> files = fc.getResults();
            
            // Add each file to the sample manager
            for (const auto& file : files)
            {
                sampleManager->addSample(file);
            }
            
            // Update the display
            updateDisplay();
        }
    });
}

void ChainControlComponent::triggerSample()
{
    if (sampleManager != nullptr && chainManager != nullptr)
    {
        // Trigger the current sample immediately with a default velocity of 100
        if (chainManager->triggerChainSample(chainIndex, 100))
        {
            // Trigger visual feedback
            waveformDisplay->triggerPlayingAnimation();
            triggerIndicator->triggerIndicator();
            
            // Update the display
            updateDisplay();
        }
    }
}

void ChainControlComponent::initializeTriggerNoteComboBox()
{
    // Clear any existing items
    triggerNoteComboBox->clear(juce::dontSendNotification);
    
    // Add all MIDI notes
    for (int i = 0; i < 128; ++i)
    {
        triggerNoteComboBox->addItem(getMidiNoteName(i), i + 1);
    }
    
    // Select the default note for this chain
    int defaultNote = (chainIndex == 0) ? 60 : 62; // C4 or D4
    triggerNoteComboBox->setSelectedId(defaultNote + 1, juce::dontSendNotification);
}

juce::String ChainControlComponent::getMidiNoteName(int noteNumber)
{
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    
    // Calculate the octave and note
    int octave = noteNumber / 12 - 1;
    int note = noteNumber % 12;
    
    // Format the note name
    return juce::String(noteNames[note]) + juce::String(octave);
}

void ChainControlComponent::setChainTitle(const juce::String& newTitle)
{
    titleLabel->setText(newTitle, juce::dontSendNotification);
}

juce::String ChainControlComponent::getTitleText() const
{
    return titleLabel->getText();
}

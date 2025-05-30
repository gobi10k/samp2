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
    g.setFont(DualTriggerStyle::fontSizeMedium);
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
        g.setFont(DualTriggerStyle::fontSizeMedium);
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
        g.setFont(DualTriggerStyle::fontSizeMedium);
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
    g.setFont(DualTriggerStyle::fontSizeMedium);
    
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
        g.setFont(DualTriggerStyle::fontSizeMedium);
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
    g.setFont(juce::Font(DualTriggerStyle::fontSizeSmall));
    g.drawText(juce::String(index + 1), itemBounds.withWidth(20), juce::Justification::centred);
    
    // Draw the filename
    g.setColour(isCurrentSample ? juce::Colours::white : (isHighlighted ? themeColour : DualTriggerStyle::textColour));
    
    // Create font with appropriate size and bold if it's the current sample
    juce::Font fileNameFont(isCurrentSample ? DualTriggerStyle::fontSizeMedium + 1.0f : DualTriggerStyle::fontSizeMedium);
    if (isCurrentSample)
        fileNameFont = fileNameFont.boldened();
    
    g.setFont(fileNameFont);
    g.drawText(sample->getFileName(), itemBounds.withTrimmedLeft(25).withTrimmedRight(50), juce::Justification::centredLeft);
    
    // Draw the play button - USING ASCII TRIANGLE
    g.setColour(isCurrentSample ? themeColour.brighter(0.2f) : themeColour.withAlpha(0.7f));
    juce::Rectangle<int> playButtonBounds(getWidth() - 48, y + 5, 20, itemHeight - 10);
    g.fillRoundedRectangle(playButtonBounds.toFloat(), 2.0f);
    
    // Draw play icon using ASCII ">" instead of Unicode triangle
    g.setColour(DualTriggerStyle::textColour);
    g.setFont(juce::Font(DualTriggerStyle::fontSizeMedium).boldened());
    g.drawText(">", playButtonBounds, juce::Justification::centred);  // ASCII ">" instead of Unicode triangle
    
    // Draw the remove button
    g.setColour(DualTriggerStyle::disabledColour);
    juce::Rectangle<int> removeButtonBounds(getWidth() - 24, y + 5, 20, itemHeight - 10);
    g.drawRoundedRectangle(removeButtonBounds.toFloat(), 2.0f, 1.0f);
    
    g.setColour(DualTriggerStyle::textColour);
    g.setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
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
    : accentColour(DualTriggerStyle::highlightColour)
{
    // Set up the default colors
    setColour(juce::ResizableWindow::backgroundColourId, DualTriggerStyle::backgroundColour);
    setColour(juce::Label::textColourId, DualTriggerStyle::textColour);
    setColour(juce::TextButton::buttonColourId, DualTriggerStyle::controlBackgroundColour);
    setColour(juce::TextButton::textColourOffId, DualTriggerStyle::textColour);
    setColour(juce::TextButton::textColourOnId, DualTriggerStyle::highlightColour);
    setColour(juce::Slider::backgroundColourId, DualTriggerStyle::controlBackgroundColour);
    setColour(juce::Slider::trackColourId, DualTriggerStyle::disabledColour);
    setColour(juce::Slider::thumbColourId, accentColour);
    setColour(juce::ComboBox::backgroundColourId, DualTriggerStyle::controlBackgroundColour);
    setColour(juce::ComboBox::textColourId, DualTriggerStyle::textColour);
    setColour(juce::ComboBox::arrowColourId, DualTriggerStyle::textColour);
    setColour(juce::PopupMenu::backgroundColourId, DualTriggerStyle::controlBackgroundColour);
    setColour(juce::PopupMenu::textColourId, DualTriggerStyle::textColour);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accentColour);
    setColour(juce::PopupMenu::highlightedTextColourId, DualTriggerStyle::controlBackgroundColour);
}

void CustomLookAndFeel::setAccentColour(juce::Colour colour)
{
    accentColour = colour;
    setColour(juce::Slider::thumbColourId, accentColour);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accentColour);
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                      juce::Slider& slider)
{
    // Calculate the bounds and angles
    const float radius = juce::jmin(width / 2.0f, height / 2.0f) - 2.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    // Draw the background circle
    g.setColour(DualTriggerStyle::controlBackgroundColour);
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);
    
    // Draw the outer ring
    g.setColour(DualTriggerStyle::disabledColour);
    g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 1.0f);
    
    // Draw the filled arc
    juce::Path arcPath;
    arcPath.addArc(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f,
                  rotaryStartAngle, angle, true);
    g.setColour(accentColour);
    g.strokePath(arcPath, juce::PathStrokeType(2.0f));
    
    // Draw the pointer
    juce::Path pointerPath;
    const float pointerLength = radius * 0.7f;
    const float pointerThickness = 2.0f;
    
    pointerPath.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
    pointerPath.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    
    g.setColour(DualTriggerStyle::textColour);
    g.fillPath(pointerPath);
    
    // Draw the center dot
    g.setColour(accentColour);
    g.fillEllipse(centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);
}

void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                     float sliderPos, float minSliderPos, float maxSliderPos,
                                     const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    // Draw the background track
    g.setColour(DualTriggerStyle::controlBackgroundColour);
    
    if (style == juce::Slider::LinearHorizontal)
    {
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y + height / 2 - 2), static_cast<float>(width), 4.0f, 2.0f);
    }
    else if (style == juce::Slider::LinearVertical)
    {
        g.fillRoundedRectangle(static_cast<float>(x + width / 2 - 2), static_cast<float>(y), 4.0f, static_cast<float>(height), 2.0f);
    }
    
    // Draw the filled portion of the track
    g.setColour(accentColour);
    
    if (style == juce::Slider::LinearHorizontal)
    {
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y + height / 2 - 2), sliderPos - static_cast<float>(x), 4.0f, 2.0f);
    }
    else if (style == juce::Slider::LinearVertical)
    {
        float filledHeight = static_cast<float>(y + height) - sliderPos;
        g.fillRoundedRectangle(static_cast<float>(x + width / 2 - 2), sliderPos, 4.0f, filledHeight, 2.0f);
    }
    
    // Draw the thumb
    g.setColour(accentColour);
    
    if (style == juce::Slider::LinearHorizontal)
    {
        g.fillEllipse(sliderPos - 5.0f, static_cast<float>(y + height / 2 - 5), 10.0f, 10.0f);
    }
    else if (style == juce::Slider::LinearVertical)
    {
        g.fillEllipse(static_cast<float>(x + width / 2 - 5), sliderPos - 5.0f, 10.0f, 10.0f);
    }
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                         const juce::Colour& backgroundColour,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown)
{
    // Calculate the bounds
    juce::Rectangle<float> bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    
    // Choose the color based on the button state
    juce::Colour baseColour = backgroundColour;
    
    if (shouldDrawButtonAsDown)
    {
        baseColour = accentColour;
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        baseColour = backgroundColour.brighter(0.2f);
    }
    
    // Draw the button background
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Draw the border
    g.setColour(button.findColour(juce::TextButton::textColourOffId).withAlpha(0.4f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                 int buttonX, int buttonY, int buttonW, int buttonH,
                                 juce::ComboBox& box)
{
    // Calculate the bounds
    juce::Rectangle<float> bounds(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    
    // Draw the background
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Draw the border
    g.setColour(box.findColour(juce::ComboBox::textColourId).withAlpha(0.4f));
    g.drawRoundedRectangle(bounds.reduced(0.5f, 0.5f), 4.0f, 1.0f);
    
    // Draw the arrow using ASCII "v" instead of Unicode
    juce::Rectangle<float> arrowBounds(static_cast<float>(buttonX), static_cast<float>(buttonY),
                                     static_cast<float>(buttonW), static_cast<float>(buttonH));
    
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
    g.drawText("v", arrowBounds.toNearestInt(), juce::Justification::centred);  // ASCII "v" instead of Unicode arrow
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                      bool isSeparator, bool isActive, bool isHighlighted,
                                      bool isTicked, bool hasSubMenu, const juce::String& text,
                                      const juce::String& shortcutKeyText, const juce::Drawable* icon,
                                      const juce::Colour* textColour)
{
    // Draw the separator
    if (isSeparator)
    {
        juce::Rectangle<int> r(area.reduced(4, 0));
        r.removeFromTop(r.getHeight() / 2 - 1);
        
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.fillRect(r.removeFromTop(1));
        
        return;
    }
    
    // Draw the highlighted background
    if (isHighlighted && isActive)
    {
        g.setColour(findColour(juce::PopupMenu::highlightedBackgroundColourId));
        g.fillRect(area);
    }
    
    // Calculate the text bounds
    juce::Rectangle<int> textBounds = area.reduced(8, 0);
    
    // Draw the check mark using ASCII instead of Unicode
    if (isTicked)
    {
        g.setColour(findColour(juce::PopupMenu::textColourId));
        g.setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
        g.drawText("*", juce::Rectangle<int>(textBounds.getX(), textBounds.getY(), 20, textBounds.getHeight()),
                  juce::Justification::centred);  // ASCII "*" instead of Unicode checkmark
        
        textBounds.removeFromLeft(20);
    }
    
    // Draw the text
    juce::Colour textColor = (textColour != nullptr) ? *textColour :
                         findColour(isHighlighted ? juce::PopupMenu::highlightedTextColourId
                                           : juce::PopupMenu::textColourId);
    
    g.setColour(textColor);
    g.setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
    g.drawText(text, textBounds, juce::Justification::centredLeft);
    
    // Draw the shortcut text
    if (shortcutKeyText.isNotEmpty())
    {
        g.setFont(juce::Font(DualTriggerStyle::fontSizeSmall));
        g.drawText(shortcutKeyText, textBounds, juce::Justification::centredRight);
    }
}

juce::Font CustomLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(juce::jmin(buttonHeight * 0.8f, DualTriggerStyle::fontSizeMedium));
}

int CustomLookAndFeel::getSliderThumbRadius(juce::Slider& slider)
{
    return 7; // Standard thumb radius
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
    titleLabel->setFont(juce::Font(DualTriggerStyle::fontSizeHeader).boldened());
    titleLabel->setJustificationType(juce::Justification::centred);
    titleLabel->setColour(juce::Label::textColourId, themeColour);
    addAndMakeVisible(titleLabel.get());
    
    // Create the trigger note label
    triggerNoteLabel = std::make_unique<juce::Label>("triggerNoteLabel", "Trigger Note:");
    triggerNoteLabel->setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
    triggerNoteLabel->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(triggerNoteLabel.get());
    
    // Create the trigger note combo box
    triggerNoteComboBox = std::make_unique<juce::ComboBox>("triggerNoteComboBox");
    initializeTriggerNoteComboBox();
    triggerNoteComboBox->addListener(this);
    addAndMakeVisible(triggerNoteComboBox.get());
    
    // Create the volume label
    volumeLabel = std::make_unique<juce::Label>("volumeLabel", "Volume:");
    volumeLabel->setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
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
    velocityThresholdSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    velocityThresholdSlider->setRange(1, 127, 1);
    velocityThresholdSlider->setValue(1);
    velocityThresholdSlider->setTextValueSuffix("");
    velocityThresholdSlider->addListener(this);
    addAndMakeVisible(velocityThresholdSlider.get());
    
    // Create the velocity threshold label
    velocityThresholdLabel = std::make_unique<juce::Label>("velocityThresholdLabel", "Velocity Threshold:");
    velocityThresholdLabel->setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
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
    pitchShiftLabel->setFont(juce::Font(DualTriggerStyle::fontSizeMedium));
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
    // Calculate the layout
    const int margin = DualTriggerStyle::padding;
    const int controlHeight = DualTriggerStyle::controlHeight;
    const int buttonWidth = 120;
    int y = margin;
    
    // Position the title label
    titleLabel->setBounds(margin, y, getWidth() - margin * 2, DualTriggerStyle::headerHeight);
    y += DualTriggerStyle::headerHeight + margin;
    
    // Position the trigger note controls
    triggerNoteLabel->setBounds(margin, y, 100, controlHeight);
    
    // Fix the overlap issue - ensure the combo box doesn't overlap with the indicator
    int comboBoxWidth = 150;
    triggerNoteComboBox->setBounds(margin + 100, y, comboBoxWidth, controlHeight);
    
    // Position the MIDI indicator with proper spacing
    int indicatorWidth = 30;
    int indicatorSpacing = 10; // Space between combo box and indicator
    triggerIndicator->setBounds(margin + 100 + comboBoxWidth + indicatorSpacing, y, indicatorWidth, controlHeight);
    
    // Position the trigger button on the right
    int rightSideX = getWidth() - margin - buttonWidth;
    triggerButton->setBounds(rightSideX, y, buttonWidth, controlHeight);
    
    y += controlHeight + margin;
    
    // Position the volume controls
    volumeLabel->setBounds(margin, y, 60, controlHeight);
    volumeSlider->setBounds(margin + 60, y, getWidth() - margin * 2 - 60, controlHeight);
    y += controlHeight + margin;
    
    // Position the velocity sensitivity button
    velocitySensitiveButton->setBounds(margin, y, 200, controlHeight);
    y += controlHeight + margin;
    
    // Position the velocity threshold controls
    velocityThresholdLabel->setBounds(margin, y, 140, controlHeight);
    velocityThresholdSlider->setBounds(margin + 140, y, getWidth() - margin * 2 - 140, controlHeight);
    y += controlHeight + margin;
    
    // Position the pitch shift controls
    int knobWidth = 120; // Make the rotary knob a good size
    pitchShiftLabel->setBounds(margin, y, 80, controlHeight);
    pitchShiftSlider->setBounds(margin + 80, y, knobWidth, controlHeight * 2);
    
    y += controlHeight * 2 + margin; // Give extra height for the rotary knobs + text boxes
    
    // Position the waveform display
    waveformDisplay->setBounds(margin, y, getWidth() - margin * 2, 100);
    y += 100 + margin;
    
    // Position the sample buttons
    clearButton->setBounds(margin, y, buttonWidth, controlHeight); // Adjusted clearButton position
    y += controlHeight + margin;
    
    // Position the sample list
    sampleList->setBounds(margin, y, getWidth() - margin * 2, getHeight() - y - margin);
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

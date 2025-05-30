#pragma once

#include <JuceHeader.h>
#include "SampleManager.h"

// Forward declarations
class ChainManager;

//==============================================================================
// Common Styling Constants
//==============================================================================

namespace DualTriggerStyle
{
    // Colours
    const juce::Colour backgroundColour = juce::Colour(0xFF1E1E1E);
    const juce::Colour chain1Colour = juce::Colour(0xFF42A5F5);    // Blue
    const juce::Colour chain2Colour = juce::Colour(0xFFEF5350);    // Red
    const juce::Colour textColour = juce::Colour(0xFFE0E0E0);
    const juce::Colour highlightColour = juce::Colour(0xFFFFB300); // Amber
    const juce::Colour disabledColour = juce::Colour(0xFF757575);  // Grey
    const juce::Colour controlBackgroundColour = juce::Colour(0xFF2D2D2D);
    const juce::Colour overlayColour = juce::Colour(0x80000000);
    
    // Fonts
    const float fontSizeSmall = 12.0f;
    const float fontSizeMedium = 14.0f;
    const float fontSizeLarge = 16.0f;
    const float fontSizeHeader = 18.0f;
    
    // Dimensions
    const int padding = 8;
    const int cornerRadius = 5;
    const int controlHeight = 24;
    const int sliderHeight = 36;
    const int headerHeight = 28;
}

//==============================================================================
/**
 * @class TriggerIndicator
 * @brief A visual indicator that lights up when a sample is triggered.
 *
 * This component shows a momentary visual indicator when a sample is triggered
 * by MIDI or mouse click, providing visual feedback of trigger events.
 */
class TriggerIndicator : public juce::Component,
                         public juce::Timer
{
public:
    TriggerIndicator();
    ~TriggerIndicator() override;
    
    /**
     * Activates the indicator to show a trigger event
     */
    void triggerIndicator();
    
    /**
     * Set the color of the indicator
     *
     * @param colour The color to use when lit
     */
    void setColour(juce::Colour colour);
    
    //==============================================================================
    // juce::Component overrides
    void paint(juce::Graphics& g) override;
    
    //==============================================================================
    // juce::Timer overrides
    void timerCallback() override;
    
private:
    juce::Colour indicatorColour;
    float intensity;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggerIndicator)
};

//==============================================================================
/**
 * @class WaveformDisplay
 * @brief Component for displaying and editing audio waveforms.
 *
 * Displays the waveform of an audio sample and allows the user to set the
 * start and end points for playback.
 */
class WaveformDisplay : public juce::Component,
                       public juce::Timer
{
public:
    WaveformDisplay();
    ~WaveformDisplay() override;
    
    /**
     * Set the audio sample to display
     *
     * @param sample Pointer to the audio sample
     */
    void setSample(AudioSample* sample);
    
    /**
     * Set the color for the waveform
     *
     * @param colour The color to use
     */
    void setWaveformColour(juce::Colour colour);
    
    /**
     * Trigger a visual indication that the sample is playing
     */
    void triggerPlayingAnimation();

    //==============================================================================
    // juce::Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    
    //==============================================================================
    // juce::Timer overrides
    void timerCallback() override;
    
private:
    AudioSample* currentSample;
    juce::Colour waveformColour;
    float playingAnimationAlpha;
    bool isDraggingStartMarker;
    bool isDraggingEndMarker;
    
    /**
     * Convert a position in pixels to a proportion of the waveform
     *
     * @param xPos X position in pixels
     * @return Proportion of the waveform (0.0 to 1.0)
     */
    double pixelToProportionX(int xPos) const;
    
    /**
     * Convert a proportion of the waveform to a position in pixels
     *
     * @param proportion Proportion of the waveform (0.0 to 1.0)
     * @return X position in pixels
     */
    int proportionToPixelX(double proportion) const;
    
    /**
     * Draw the start and end markers
     *
     * @param g Graphics context
     */
    void drawMarkers(juce::Graphics& g);
    
    /**
     * Check if a mouse position is over a marker
     *
     * @param xPos X position in pixels
     * @param markerPosition Marker position in proportion (0.0 to 1.0)
     * @return True if the mouse is over the marker
     */
    bool isOverMarker(int xPos, double markerPosition) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};

//==============================================================================
/**
 * @class SampleListComponent
 * @brief Component for displaying and managing a list of audio samples.
 *
 * Displays a list of audio samples with controls to remove samples and
 * supports drag-and-drop reordering.
 */
class SampleListComponent : public juce::Component,
                           public juce::FileDragAndDropTarget,
                           public juce::ActionListener
{
public:
    SampleListComponent();
    ~SampleListComponent() override;
    
    /**
     * Set the sample manager to use
     *
     * @param manager Pointer to the sample manager
     */
    void setSampleManager(SampleManager* manager);
    
    /**
     * Set the chain index for this list
     *
     * @param index The chain index (0 or 1)
     */
    void setChainIndex(int index);
    
    /**
     * Set the color theme for this component
     *
     * @param colour The main color to use
     */
    void setColour(juce::Colour colour);
    
    /**
     * Refresh the list to show any changes in the sample manager
     */
    void refresh();

    //==============================================================================
    // juce::Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    
    //==============================================================================
    // juce::FileDragAndDropTarget overrides
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    
    //==============================================================================
    // juce::ActionListener overrides
    void actionListenerCallback(const juce::String& message) override;
    
private:
    SampleManager* sampleManager;
    int chainIndex;
    juce::Colour themeColour;
    int itemHeight;
    int draggedItemIndex;
    int draggedYOffset;
    int currentSampleIndex;
    int dropTargetIndex; // New variable to track where sample will be dropped
    
    /**
     * Get the index of the item at the given Y position
     *
     * @param y Y position in pixels
     * @return Item index or -1 if none
     */
    int getItemIndexFromY(int y) const;
    
    /**
     * Draw an item in the list
     *
     * @param g Graphics context
     * @param index Item index
     * @param y Y position to start drawing
     * @param isHighlighted True if the item should be highlighted
     */
    void drawItem(juce::Graphics& g, int index, int y, bool isHighlighted);
    
    /**
     * Check if a position is over the remove button for an item
     *
     * @param x X position in pixels
     * @param y Y position in pixels
     * @return Item index or -1 if not over a remove button
     */
    int isOverRemoveButton(int x, int y) const;
    
    /**
     * Check if a position is over the play button for an item
     *
     * @param x X position in pixels
     * @param y Y position in pixels
     * @return Item index or -1 if not over a play button
     */
    int isOverPlayButton(int x, int y) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleListComponent)
};

//==============================================================================
/**
 * @class CustomLookAndFeel
 * @brief Custom look and feel for the plugin UI.
 *
 * Defines the appearance of sliders, buttons, and other UI elements to create
 * a modern, professional look.
 */
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();
    
    /**
     * Set the accent color for controls
     *
     * @param colour The accent color to use
     */
    void setAccentColour(juce::Colour colour);
    
    //==============================================================================
    // juce::LookAndFeel overrides
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;
    
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override;
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                             const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown) override;
    
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                     int buttonX, int buttonY, int buttonW, int buttonH,
                     juce::ComboBox& box) override;
    
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                          bool isSeparator, bool isActive, bool isHighlighted,
                          bool isTicked, bool hasSubMenu, const juce::String& text,
                          const juce::String& shortcutKeyText, const juce::Drawable* icon,
                          const juce::Colour* textColour) override;
    
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    
    int getSliderThumbRadius(juce::Slider& slider) override;
    
private:
    juce::Colour accentColour;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLookAndFeel)
};

//==============================================================================
/**
 * @class TriggerButton
 * @brief Button to manually trigger samples
 *
 * This button provides a visual way to trigger samples with mouse clicks.
 */
class TriggerButton : public juce::Button
{
public:
    TriggerButton(const juce::String& name);
    ~TriggerButton() override = default;
    
    /**
     * Set the color for the button
     *
     * @param colour The main color to use
     */
    void setColour(juce::Colour colour);
    
protected:
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
private:
    juce::Colour buttonColour;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggerButton)
};

//==============================================================================
/**
 * @class ChainControlComponent
 * @brief Component for controlling a single sample chain.
 *
 * Provides controls for a single sample chain including MIDI note selection,
 * volume control, and sample list management.
 */
class ChainControlComponent : public juce::Component,
                             public juce::ActionListener,
                             public juce::Slider::Listener,
                             public juce::Button::Listener,
                             public juce::ComboBox::Listener,
                             public juce::FileDragAndDropTarget
{
public:
    ChainControlComponent();
    ~ChainControlComponent() override;
    
    /**
     * Set the sample manager to use
     *
     * @param manager Pointer to the sample manager
     */
    void setSampleManager(SampleManager* manager);
    
    /**
     * Get the current sample manager
     *
     * @return Pointer to the current sample manager or nullptr if none
     */
    SampleManager* getSampleManager() { return sampleManager; }
    
    /**
     * Set the chain index for this component
     *
     * @param index The chain index (0 or 1)
     */
    void setChainIndex(int index);
    
    /**
     * Get the chain index for this component
     *
     * @return The chain index (0 or 1)
     */
    int getChainIndex() const { return chainIndex; }
    
    /**
     * Set the chain manager to use
     *
     * @param manager Pointer to the chain manager
     */
    void setChainManager(ChainManager* manager);
    
    /**
     * Get the current chain manager
     *
     * @return Pointer to the current chain manager or nullptr if none
     */
    ChainManager* getChainManager() { return chainManager; }
    
    /**
     * Set the color theme for this component
     *
     * @param colour The main color to use
     */
    void setColour(juce::Colour colour);
    
    /**
     * Set whether velocity sensitivity is enabled
     *
     * @param enabled True to enable velocity sensitivity
     */
    void setVelocitySensitivityEnabled(bool enabled);
    
    /**
     * Set the velocity threshold
     *
     * @param threshold The velocity threshold (1-127)
     */
    void setVelocityThreshold(int threshold);
    
    /**
     * Set the pitch shift amount in semitones
     *
     * @param semitones Pitch shift amount (-12 to +12 semitones)
     */
    void setPitchShift(float semitones);
    
    /**
     * Update the display to reflect current state
     */
    void updateDisplay();
    
    /**
     * Set the title of the chain control component.
     * @param newTitle The new title to set.
     */
    void setChainTitle(const juce::String& newTitle);

    /**
     * Get the current title of the chain control component.
     * @return The current title string.
     */
    juce::String getTitleText() const;

    //==============================================================================
    // juce::Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    //==============================================================================
    // juce::ActionListener overrides
    void actionListenerCallback(const juce::String& message) override;
    
    //==============================================================================
    // juce::Slider::Listener overrides
    void sliderValueChanged(juce::Slider* slider) override;
    
    //==============================================================================
    // juce::Button::Listener overrides
    void buttonClicked(juce::Button* button) override;
    
    //==============================================================================
    // juce::ComboBox::Listener overrides
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    
    //==============================================================================
    // juce::FileDragAndDropTarget overrides
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    
    /**
     * Load samples from files
     */
    void loadSamples();
    
    /**
     * Trigger a sample manually
     */
    void triggerSample();
    
private:
    SampleManager* sampleManager;
    ChainManager* chainManager;
    int chainIndex;
    juce::Colour themeColour;
    
    // UI Components
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::Label> triggerNoteLabel;
    std::unique_ptr<juce::ComboBox> triggerNoteComboBox;
    std::unique_ptr<juce::Label> volumeLabel;
    std::unique_ptr<juce::Slider> volumeSlider;
    std::unique_ptr<SampleListComponent> sampleList;
    std::unique_ptr<juce::TextButton> clearButton;
    std::unique_ptr<juce::ToggleButton> velocitySensitiveButton;
    std::unique_ptr<juce::Slider> velocityThresholdSlider;
    std::unique_ptr<juce::Label> velocityThresholdLabel;
    std::unique_ptr<juce::Slider> pitchShiftSlider;
    std::unique_ptr<juce::Label> pitchShiftLabel;
    
    std::unique_ptr<WaveformDisplay> waveformDisplay;
    std::unique_ptr<CustomLookAndFeel> lookAndFeel;
    std::unique_ptr<TriggerButton> triggerButton;
    std::unique_ptr<TriggerIndicator> triggerIndicator;
    
    /**
     * Initialize the trigger note combo box with MIDI note names
     */
    void initializeTriggerNoteComboBox();
    
    /**
     * Get the name for a MIDI note number
     *
     * @param noteNumber The MIDI note number (0-127)
     * @return The name of the note
     */
    juce::String getMidiNoteName(int noteNumber);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainControlComponent)
};

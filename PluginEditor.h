#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UIComponents.h" // For CustomLookAndFeel
// #include "PianoRollComponent.h" // Included via TabContentComponent.h if needed there
#include "TabContentComponent.h" // Include the new TabContentComponent header


/**
 * @class DualChainSampleTriggerEditor
 * @brief Editor class for the Dual-Chain Sample Trigger plugin.
 *
 * This editor class creates the user interface for the plugin.
 */
class DualChainSampleTriggerEditor : public juce::AudioProcessorEditor,
                                     public juce::Timer,
                                     public juce::TextEditor::Listener,
                                     public juce::Button::Listener,
                                     public juce::TabbedComponent::Listener
{
public:
    //==============================================================================
    /**
     * Constructor
     *
     * @param processor Reference to the processor
     * @param parameters Reference to the parameters
     */
    DualChainSampleTriggerEditor(DualChainSampleTriggerProcessor& processor, juce::AudioProcessorValueTreeState& parameters);
    
    /**
     * Destructor
     */
    ~DualChainSampleTriggerEditor() override;

    //==============================================================================
    // juce::Component overrides
    void paint(juce::Graphics&) override;
    void resized() override;
    
    //==============================================================================
    // Listener overrides
    // void sliderValueChanged(juce::Slider* slider) override; // Moved
    
    void timerCallback() override; // Will be simplified
    
    // void pianoNoteSelected(int chainIndex, int midiNoteNumber) override; // Moved

    void textEditorTextChanged(juce::TextEditor& editor) override; // Handles sessionTitleEditor

    void buttonClicked(juce::Button* button) override; // Handles global buttons

    // juce::TabbedComponent::Listener override
    void currentTabChanged(int newCurrentTabIndex, const juce::String& newCurrentTabName) override;

    // juce::AudioProcessorValueTreeState::Listener override (if kept for other params)
    // void parameterChanged(const juce::String& parameterID, float newValue) override; // Removed

private:
    // Reference to the processor
    DualChainSampleTriggerProcessor& audioProcessor;
    
    // Reference to the parameters
    juce::AudioProcessorValueTreeState& parameters;
    
    // UI Components
    juce::TabbedComponent tabbedComponent {juce::TabbedButtonBar::TabsAtTop}; // Initialize with orientation
    // std::vector<TabContentComponent*> tabPages; // Optional: if direct management of pages is needed

    // Global UI Elements (not part of individual tabs)
    std::unique_ptr<juce::Label> titleLabel; // Main session title display (might be replaced by sessionTitleEditor)
    std::unique_ptr<juce::TextEditor> sessionTitleEditor; // For editing overall session/active tab title

    std::unique_ptr<juce::TextButton> saveStateButton;
    std::unique_ptr<juce::TextButton> loadStateButton;
    std::unique_ptr<juce::TextButton> resetStateButton;
    std::unique_ptr<juce::TextButton> addNewTabButton; // New button

    std::unique_ptr<juce::TextButton> saveActiveTabButton;
    std::unique_ptr<juce::TextButton> loadActiveTabButton; // Loads into current active tab
    std::unique_ptr<juce::TextButton> loadTabAsNewButton;  // Loads file into a new tab
    
    // Parameter attachments for global controls if any (e.g. if main volume or blend were outside tabs)
    // For now, blend and main volume are inside TabContentComponent, using global APVTS parameters.

    // Custom look and feel
    std::unique_ptr<CustomLookAndFeel> lookAndFeel;

    // Update UI method (will be simplified or its logic moved)
    void updateUI();
    void buildTabsFromProcessorState(); // New method to populate/update tabs

    // FileChooser member
    std::unique_ptr<juce::FileChooser> chooser;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DualChainSampleTriggerEditor)
};

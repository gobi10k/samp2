#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UIComponents.h"
#include "PianoRollComponent.h"

/**
 * @class DualChainSampleTriggerEditor
 * @brief Editor class for the Dual-Chain Sample Trigger plugin.
 *
 * This editor class creates the user interface for the plugin.
 */
class DualChainSampleTriggerEditor : public juce::AudioProcessorEditor,
                                     public juce::Slider::Listener,
                                     public juce::Timer,
                                     public PianoRollComponent::Listener,
                                     public juce::TextEditor::Listener,
                                     public juce::Button::Listener // Ensure Button::Listener is present
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
    // juce::Slider::Listener overrides
    void sliderValueChanged(juce::Slider* slider) override;
    
    //==============================================================================
    // juce::Timer overrides
    void timerCallback() override;
    
    //==============================================================================
    /**
     * Update the UI to reflect the processor state
     */
    void updateUI();

    void pianoNoteSelected(int chainIndex, int midiNoteNumber) override;

    //==============================================================================
    // juce::TextEditor::Listener overrides
    void textEditorTextChanged(juce::TextEditor& editor) override;

    // juce::Button::Listener overrides
    void buttonClicked(juce::Button* button) override; // Ensure this is declared

    // juce::AudioProcessorValueTreeState::Listener override (if kept for other params)
    // void parameterChanged(const juce::String& parameterID, float newValue) override; // Removed

private:
    // Reference to the processor
    DualChainSampleTriggerProcessor& audioProcessor;
    
    // Reference to the parameters
    juce::AudioProcessorValueTreeState& parameters;
    
    // UI Components
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<ChainControlComponent> chain1Control;
    std::unique_ptr<ChainControlComponent> chain2Control;
    std::unique_ptr<juce::Slider> blendSlider;
    std::unique_ptr<juce::Label> blendLabel;
    std::unique_ptr<juce::Slider> mainVolumeSlider;
    std::unique_ptr<juce::Label> mainVolumeLabel;
    std::unique_ptr<PianoRollComponent> pianoRollComponent_;

    // Chain Title Editors
    std::unique_ptr<juce::Label> chain1TitleLabelEditorLabel;
    std::unique_ptr<juce::TextEditor> chain1TitleEditor;
    std::unique_ptr<juce::Label> chain2TitleLabelEditorLabel;
    std::unique_ptr<juce::TextEditor> chain2TitleEditor;

    // Session Title Editor
    std::unique_ptr<juce::TextEditor> sessionTitleEditor;

    // Save State Button
    std::unique_ptr<juce::TextButton> saveStateButton;

    // Load State Button
    std::unique_ptr<juce::TextButton> loadStateButton;

    // Reset State Button
    std::unique_ptr<juce::TextButton> resetStateButton;
    
    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mainVolumeAttachment;

    // TextEditor Attachments for titles are removed
    // std::unique_ptr<juce::AudioProcessorValueTreeState::TextEditorAttachment> sessionTitleAttachment;
    // std::unique_ptr<juce::AudioProcessorValueTreeState::TextEditorAttachment> chain1TitleAttachment;
    // std::unique_ptr<juce::AudioProcessorValueTreeState::TextEditorAttachment> chain2TitleAttachment;
    
    // Custom look and feel
    std::unique_ptr<CustomLookAndFeel> lookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DualChainSampleTriggerEditor)
};

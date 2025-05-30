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
                                     public PianoRollComponent::Listener
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
    
    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mainVolumeAttachment;
    
    // Custom look and feel
    std::unique_ptr<CustomLookAndFeel> lookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DualChainSampleTriggerEditor)
};

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h" // For accessing TabState, ChainManager etc.
#include "UIComponents.h"    // For ChainControlComponent etc.
#include "PianoRollComponent.h"

class TabContentComponent : public juce::Component,
                            public juce::Slider::Listener,
                            public juce::Timer,
                            public PianoRollComponent::Listener,
                            public juce::TextEditor::Listener,
                            public juce::Button::Listener
{
public:
    TabContentComponent(DualChainSampleTriggerProcessor& processor, juce::AudioProcessorValueTreeState& parameters, int associatedTabIndex);
    ~TabContentComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void sliderValueChanged(juce::Slider* slider) override;
    void timerCallback() override;
    void pianoNoteSelected(int chainIndex, int midiNoteNumber) override;
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void buttonClicked(juce::Button* button) override;

    void updateUIForTab(); // Call this when tab becomes active or its data changes
    int getAssociatedTabIndex() const { return tabIndex; }

private:
    DualChainSampleTriggerProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& valueTreeState; // APVTS reference
    int tabIndex; // Index of the tab this component represents

    // UI Elements (mirrors DualChainSampleTriggerEditor)
    std::unique_ptr<ChainControlComponent> chain1Control;
    std::unique_ptr<ChainControlComponent> chain2Control;
    std::unique_ptr<juce::Slider> blendSlider;
    std::unique_ptr<juce::Label> blendLabel;
    std::unique_ptr<juce::Slider> mainVolumeSlider;
    std::unique_ptr<juce::Label> mainVolumeLabel;
    std::unique_ptr<PianoRollComponent> pianoRollComponent_;
    std::unique_ptr<juce::TextEditor> chain1TitleEditor;
    std::unique_ptr<juce::TextEditor> chain2TitleEditor;
    // Note: Session title editor will be part of the main PluginEditor's tab bar, not inside each tab page.

    // Parameter attachments (these will now be managed more carefully, potentially re-attaching on tab switch or being tab-specific if params are tab-specific)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mainVolumeAttachment;
    // Chain-specific parameter attachments will be handled by ChainControlComponent itself.

    std::unique_ptr<CustomLookAndFeel> lookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabContentComponent)
};

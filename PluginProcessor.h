#pragma once

#include <JuceHeader.h>
#include "ChainManager.h"

/**
 * @class DualChainSampleTriggerProcessor
 * @brief Main processor class for the Dual-Chain Sample Trigger plugin.
 *
 * This processor handles audio processing, MIDI input, parameter management,
 * and plugin state serialization.
 */
class DualChainSampleTriggerProcessor : public juce::AudioProcessor,
                                        public juce::AudioProcessorValueTreeState::Listener,
                                        public juce::ValueTree::Listener
{
public:
    //==============================================================================
    DualChainSampleTriggerProcessor();
    ~DualChainSampleTriggerProcessor() override;

    //==============================================================================
    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    
    const juce::String getName() const override;
    
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    //==============================================================================
    // AudioProcessorValueTreeState::Listener overrides
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    
    //==============================================================================
    // ValueTree::Listener overrides
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                  const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parentTree,
                            juce::ValueTree& childWhichHasBeenAdded) override {};
    void valueTreeChildRemoved(juce::ValueTree& parentTree,
                              juce::ValueTree& childWhichHasBeenRemoved,
                              int indexFromWhichChildWasRemoved) override {};
    void valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved,
                                   int oldIndex, int newIndex) override {};
    void valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged) override {};
    
    //==============================================================================
    /**
     * Get the chain manager
     *
     * @return Pointer to the chain manager
     */
    ChainManager* getChainManager() { return chainManager.get(); }
    
    /**
     * Get a parameter by its ID
     *
     * @param paramId The parameter ID
     * @return Pointer to the parameter or nullptr if not found
     */
    juce::RangedAudioParameter* getParameterById(const juce::String& paramId);
    
    /**
     * Load samples into a chain
     *
     * @param chainIndex The chain index (0 or 1)
     * @param files The audio files to load
     * @return True if loading succeeded
     */
    bool loadSamplesIntoChain(int chainIndex, const juce::Array<juce::File>& files);
    
    /**
     * Clear all samples from a chain
     *
     * @param chainIndex The chain index (0 or 1)
     */
    void clearSamplesFromChain(int chainIndex);
    
    /**
     * Get the number of samples in a chain
     *
     * @param chainIndex The chain index (0 or 1)
     * @return The number of samples
     */
    int getNumSamplesInChain(int chainIndex);
    
    /**
     * Get the filename of a sample in a chain
     *
     * @param chainIndex The chain index (0 or 1)
     * @param sampleIndex The sample index
     * @return The filename or an empty string if not found
     */
    juce::String getSampleFilename(int chainIndex, int sampleIndex);
    
    /**
     * Get the ValueTree that holds the plugin state
     *
     * @return Reference to the state ValueTree
     */
    juce::ValueTree& getState() { return state; }

private:
    //==============================================================================
    // Parameters
    std::unique_ptr<juce::AudioProcessorValueTreeState> parameters;
    
    // Parameter IDs - making public for editor access
public:
    static const juce::String PARAM_BLEND;
    static const juce::String PARAM_MAIN_VOLUME;
    static const juce::String PARAM_CHAIN1_VOLUME;
    static const juce::String PARAM_CHAIN2_VOLUME;
    static const juce::String PARAM_CHAIN1_NOTE;
    static const juce::String PARAM_CHAIN2_NOTE;
    static const juce::String PARAM_CHAIN1_VELOCITY_SENSITIVE;
    static const juce::String PARAM_CHAIN2_VELOCITY_SENSITIVE;
    static const juce::String PARAM_CHAIN1_VELOCITY_THRESHOLD;
    static const juce::String PARAM_CHAIN2_VELOCITY_THRESHOLD;
    static const juce::String PARAM_CHAIN1_PITCH_SHIFT;
    static const juce::String PARAM_CHAIN2_PITCH_SHIFT;
    
private:
    // Create all parameters
    void createParameters();
    
    // Chain manager
    std::unique_ptr<ChainManager> chainManager;
    
    // State ValueTree for saving/loading plugin state
    juce::ValueTree state;
    
    // Initialize state from parameters
    void initializeState();
    
    // Update parameters from state
    void updateParametersFromState();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DualChainSampleTriggerProcessor)
};

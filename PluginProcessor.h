#pragma once

#include <JuceHeader.h>
#include "ChainManager.h"

// Forward declare ChainManager if its full definition isn't already included by PluginProcessor.h
// class ChainManager; // (It is included via PluginProcessor.h -> ChainManager.h)

class TabState // Not using ReferenceCountedObject for now, PluginProcessor will own unique_ptrs
{
public:
    juce::String tabTitle;
    juce::String chain1Title;
    juce::String chain2Title;
    std::unique_ptr<ChainManager> chainManager;

    // Constructor
    TabState(const juce::String& newTabTitle,
             const juce::String& c1Title = "Chain 1",
             const juce::String& c2Title = "Chain 2")
        : tabTitle(newTabTitle), chain1Title(c1Title), chain2Title(c2Title)
    {
        chainManager = std::make_unique<ChainManager>();
    }

    // Methods to save/load ChainManager's state and titles for this tab
    void saveStateToXmlElement(juce::XmlElement& xmlParentForThisTab) const
    {
        xmlParentForThisTab.setAttribute("tabTitle", tabTitle);
        xmlParentForThisTab.setAttribute("chain1Title", chain1Title);
        xmlParentForThisTab.setAttribute("chain2Title", chain2Title);
        // Let ChainManager save its own state as a child element
        if (chainManager) // Check if chainManager is valid
        {
             // ChainManager::saveToXml() returns a unique_ptr<XmlElement>
             // whose tag is "CHAINMANAGER". This should be added as a child.
             xmlParentForThisTab.addChildElement(chainManager->saveToXml().release());
        }
    }

    void loadStateFromXmlElement(const juce::XmlElement& xmlForThisTab)
    {
        tabTitle = xmlForThisTab.getStringAttribute("tabTitle", "Untitled Tab");
        chain1Title = xmlForThisTab.getStringAttribute("chain1Title", "Chain 1");
        chain2Title = xmlForThisTab.getStringAttribute("chain2Title", "Chain 2");

        if (!chainManager) // If chainManager wasn't created or is null
        {
             chainManager = std::make_unique<ChainManager>();
        }

        if (auto* cmXml = xmlForThisTab.getChildByName("CHAINMANAGER"))
        {
            chainManager->restoreFromXml(cmXml);
        }
        else
        {
            // If no CHAINMANAGER element, reset it to default (or log warning)
            chainManager = std::make_unique<ChainManager>();
            DBG("TabState::loadStateFromXmlElement - No CHAINMANAGER element found for tab: " + tabTitle);
        }
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabState)
};

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
    // XML State Management
    void getCurrentStateAsXml(juce::XmlElement& xml); // Helper for both XML save and host save
    void restoreStateFromXml(const juce::XmlElement& xml); // Helper for both XML load and host load
    void saveStateToXml(const juce::File& outputFile);
    void loadStateFromXml(const juce::File& inputFile);

    // Tab Management
    void addNewTab(const juce::String& title = "Untitled Tab");
    void removeTab(int tabIndex);
    void setActiveTab(int tabIndex);
    TabState* getActiveTabState();
    TabState* getTabState(int tabIndex);
    int getNumTabs() const;
    juce::String getTabTitle(int tabIndex) const;
    void setTabTitle(int tabIndex, const juce::String& newTitle);
    void setChainTitleForActiveTab(int chainIndex, const juce::String& newTitle);
    juce::String getChainTitleForActiveTab(int chainIndex) const;
    juce::String getSessionTitleForActiveTab() const;
    void setSessionTitleForActiveTab(const juce::String& title);

    // Access to active ChainManager
    ChainManager* getActiveChainManager();

    void resetToDefaultState();

    // Individual Tab State Management
    void saveSingleTabStateToFile(int tabIndex, const juce::File& file);
    void loadSingleTabStateFromFile(int tabIndex, const juce::File& file);
    void loadTabAsNewFromFile(const juce::File& file);

    // Getter for active tab index
    int getActiveTabIndex() const { return activeTabIndex; }
    
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
    // Existing float/int/bool parameters // Keep these
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

    // APVTS Title Parameter IDs are removed
    // static const juce::StringRef PARAM_SESSION_TITLE;
    // static const juce::StringRef PARAM_CHAIN1_TITLE;
    // static const juce::StringRef PARAM_CHAIN2_TITLE;
    
private:
    // Create all parameters
    void createParameters();
    
    // Chain manager
    // std::unique_ptr<ChainManager> chainManager; // Removed
    
    std::vector<std::unique_ptr<TabState>> tabStates;
    int activeTabIndex = 0;
    double currentSampleRate = 44100.0; // Store these for when new tabs are made
    int currentBlockSize = 512;      // or when restoring state before prepareToPlay

    // State ValueTree for saving/loading plugin state
    juce::ValueTree state;

    // Re-introduce String Members for titles
    // juce::String currentSessionTitle; // Removed
    // juce::String currentChain1Title; // Removed
    // juce::String currentChain2Title; // Removed
    
    // Initialize state from parameters
    void initializeState();
    
    // Update parameters from state
    void updateParametersFromState();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DualChainSampleTriggerProcessor)
};

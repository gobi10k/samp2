#pragma once

#include <JuceHeader.h>
#include "SampleManager.h"

/**
 * @class ChainManager
 * @brief Manages dual sample chains with IMMEDIATE MIDI triggering and crossfade functionality.
 *
 * This class orchestrates the two independent sample chains, handling MIDI input,
 * sample triggering, and mixing between the two chains.
 * Modified for ZERO LATENCY MIDI triggering - processes MIDI immediately in audio thread.
 */
class ChainManager : public juce::ActionBroadcaster
{
public:
    ChainManager();
    ~ChainManager();
    
    void setTriggerNote(int chainIndex, int midiNote);
    int getTriggerNote(int chainIndex) const;
    void setChainVolume(int chainIndex, float newVolume);
    float getChainVolume(int chainIndex) const;
    void setBlendValue(float newBlend);
    float getBlendValue() const;
    void setMainVolume(float newVolume);
    float getMainVolume() const;
    SampleManager* getSampleManager(int chainIndex);
    
    /**
     * Process incoming MIDI messages - NOW WITH ZERO LATENCY
     * Triggers samples IMMEDIATELY in the audio thread
     */
    void processMidiMessages(const juce::MidiBuffer& midiMessages);
    
    /**
     * Manually trigger a sample on a specific chain (thread-safe from UI)
     */
    bool triggerChainSample(int chainIndex, int velocity);
    
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples);
    void prepareToPlay(double sampleRate, int blockSize);
    void releaseResources();
    std::unique_ptr<juce::XmlElement> saveToXml() const;
    bool restoreFromXml(const juce::XmlElement* xml);
    bool isChainPlaying(int chainIndex) const;
    void addListener(juce::ActionListener* listener);
    void removeListener(juce::ActionListener* listener);

private:
    static constexpr int NUM_CHAINS = 2;
    static constexpr int DEFAULT_TRIGGER_NOTES[NUM_CHAINS] = { 60, 62 }; // Middle C and D
    
    std::unique_ptr<SampleManager> sampleManagers[NUM_CHAINS];
    int triggerNotes[NUM_CHAINS];
    float chainVolumes[NUM_CHAINS];
    float blendValue;
    float mainVolume;
    bool chainIsPlaying[NUM_CHAINS];
    
    juce::AudioBuffer<float> chainBuffers[NUM_CHAINS];
    double currentSampleRate;
    int currentBlockSize;
    
    juce::ActionBroadcaster triggerBroadcaster;
    static constexpr const char* CHAIN_TRIGGERED_PREFIX = "ChainTriggered_";
    
    void sendChainTriggeredMessage(int chainIndex);
    bool triggerChainSampleImmediate(int chainIndex, int velocity);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainManager)
};

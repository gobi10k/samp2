#include "ChainManager.h"

ChainManager::ChainManager()
    : blendValue(0.5f),
      mainVolume(1.0f),
      currentSampleRate(44100.0),
      currentBlockSize(512)
{
    for (int i = 0; i < NUM_CHAINS; ++i)
    {
        sampleManagers[i] = std::make_unique<SampleManager>();
        triggerNotes[i] = DEFAULT_TRIGGER_NOTES[i];
        chainVolumes[i] = 1.0f;
        chainIsPlaying[i] = false;
    }
}

ChainManager::~ChainManager()
{
    for (int i = 0; i < NUM_CHAINS; ++i)
    {
        sampleManagers[i].reset();
    }
}

void ChainManager::setTriggerNote(int chainIndex, int midiNote)
{
    if (chainIndex >= 0 && chainIndex < NUM_CHAINS)
        triggerNotes[chainIndex] = juce::jlimit(0, 127, midiNote);
}

int ChainManager::getTriggerNote(int chainIndex) const
{
    return (chainIndex >= 0 && chainIndex < NUM_CHAINS) ? triggerNotes[chainIndex] : -1;
}

void ChainManager::setChainVolume(int chainIndex, float newVolume)
{
    if (chainIndex >= 0 && chainIndex < NUM_CHAINS)
        chainVolumes[chainIndex] = juce::jlimit(0.0f, 1.0f, newVolume);
}

float ChainManager::getChainVolume(int chainIndex) const
{
    return (chainIndex >= 0 && chainIndex < NUM_CHAINS) ? chainVolumes[chainIndex] : 0.0f;
}

void ChainManager::setBlendValue(float newBlend)
{
    blendValue = juce::jlimit(0.0f, 1.0f, newBlend);
}

float ChainManager::getBlendValue() const
{
    return blendValue;
}

void ChainManager::setMainVolume(float newVolume)
{
    mainVolume = juce::jlimit(0.0f, 1.0f, newVolume);
}

float ChainManager::getMainVolume() const
{
    return mainVolume;
}

SampleManager* ChainManager::getSampleManager(int chainIndex)
{
    return (chainIndex >= 0 && chainIndex < NUM_CHAINS) ? sampleManagers[chainIndex].get() : nullptr;
}

void ChainManager::processMidiMessages(const juce::MidiBuffer& midiMessages)
{
    DBG("[MIDI] Processing MIDI messages - count: " << midiMessages.getNumEvents());
    
    // Process all MIDI messages IMMEDIATELY for lowest latency
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            int velocity = message.getVelocity();
            
            DBG("[MIDI] Note On received: " << noteNumber << ", velocity: " << velocity);
            
            for (int i = 0; i < NUM_CHAINS; ++i)
            {
                if (noteNumber == triggerNotes[i])
                {
                    DBG("[MIDI] Note matches chain " << i << " - triggering IMMEDIATELY");
                    triggerChainSampleImmediate(i, velocity);
                }
            }
        }
    }
}

bool ChainManager::triggerChainSample(int chainIndex, int velocity)
{
    return triggerChainSampleImmediate(chainIndex, velocity);
}

bool ChainManager::triggerChainSampleImmediate(int chainIndex, int velocity)
{
    DBG("[TRIGGER] triggerChainSampleImmediate called for chain " << chainIndex << " with velocity " << velocity);
    
    if (chainIndex < 0 || chainIndex >= NUM_CHAINS)
    {
        DBG("[TRIGGER] Invalid chain index: " << chainIndex);
        return false;
    }
    
    SampleManager* manager = sampleManagers[chainIndex].get();
    if (manager == nullptr)
    {
        DBG("[TRIGGER] Sample manager is null");
        return false;
    }
    
    // Stop any currently playing sample first.
    // This also handles the case where chainIsPlaying[chainIndex] might be true
    // but the sample finished on its own.
    manager->stopPlayback();
    chainIsPlaying[chainIndex] = false; // Reflect that playback is stopped before new decision

    int numSamples = manager->getNumSamples();
    if (numSamples == 0)
    {
        DBG("[TRIGGER] No samples in chain " << chainIndex << ", cannot trigger.");
        return false; // No samples to play
    }

    int currentIndex = manager->getCurrentSampleIndex();
    // If currentIndex is -1 (no sample ever selected) or invalid (e.g. samples were cleared), start from 0.
    // Otherwise, advance. The modulo handles wrap-around.
    int nextIndex = (currentIndex < 0 || currentIndex >= numSamples) ? 0 : (currentIndex + 1) % numSamples;
    
    manager->setCurrentSampleIndex(nextIndex);
    DBG("[TRIGGER] Advanced sample index in chain " << chainIndex << " from " << currentIndex << " to " << nextIndex);

    // Now trigger the new current sample
    DBG("[TRIGGER] About to call triggerCurrentSample for the new index " << nextIndex);
    if (manager->triggerCurrentSample(velocity)) // triggerCurrentSample uses the new current index internally
    {
        DBG("[TRIGGER] Sample triggered successfully for new index " << nextIndex);
        chainIsPlaying[chainIndex] = true;
        sendChainTriggeredMessage(chainIndex); // Notify UI about the new sample
        return true;
    }
    else
    {
        DBG("[TRIGGER] Sample trigger FAILED for new index " << nextIndex);
    }
    
    return false;
}

void ChainManager::processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Ensure the chain buffers are the right size
    for (int i = 0; i < NUM_CHAINS; ++i)
    {
        chainBuffers[i].setSize(buffer.getNumChannels(), numSamples, false, false, true);
        chainBuffers[i].clear();
    }
    
    // Process each chain
    for (int i = 0; i < NUM_CHAINS; ++i)
    {
        sampleManagers[i]->processBlock(chainBuffers[i], numSamples);
        
        bool wasPlaying = chainIsPlaying[i];
        chainIsPlaying[i] = sampleManagers[i]->isPlaying();
        
        if (wasPlaying && !chainIsPlaying[i])
        {
            sendChainTriggeredMessage(i);
        }
        
        if (chainVolumes[i] != 1.0f)
        {
            chainBuffers[i].applyGain(chainVolumes[i]);
        }
    }
    
    buffer.clear();
    
    float chain1Gain = 1.0f - blendValue;
    float chain2Gain = blendValue;
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        buffer.addFrom(channel, 0, chainBuffers[0], channel, 0, numSamples, chain1Gain);
        buffer.addFrom(channel, 0, chainBuffers[1], channel, 0, numSamples, chain2Gain);
    }
    
    if (mainVolume != 1.0f)
    {
        buffer.applyGain(mainVolume);
    }
}

void ChainManager::prepareToPlay(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    currentBlockSize = blockSize;
    
    for (int i = 0; i < NUM_CHAINS; ++i)
    {
        sampleManagers[i]->prepareToPlay(sampleRate, blockSize);
    }
    
    for (int i = 0; i < NUM_CHAINS; ++i)
    {
        chainBuffers[i].setSize(2, blockSize);
        chainBuffers[i].clear();
    }
}

void ChainManager::releaseResources()
{
    for (int i = 0; i < NUM_CHAINS; ++i)
    {
        sampleManagers[i]->releaseResources();
        chainIsPlaying[i] = false;
    }
}

std::unique_ptr<juce::XmlElement> ChainManager::saveToXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("CHAINMANAGER");
    xml->setAttribute("blendValue", blendValue);
    xml->setAttribute("mainVolume", mainVolume);
    
    for (int i = 0; i < NUM_CHAINS; ++i)
    {
        auto chainXml = xml->createNewChildElement("CHAIN");
        chainXml->setAttribute("index", i);
        chainXml->setAttribute("triggerNote", triggerNotes[i]);
        chainXml->setAttribute("volume", chainVolumes[i]);
        chainXml->addChildElement(sampleManagers[i]->saveToXml().release());
    }
    
    return xml;
}

bool ChainManager::restoreFromXml(const juce::XmlElement* xml)
{
    if (xml == nullptr || xml->getTagName() != "CHAINMANAGER")
        return false;
    
    blendValue = static_cast<float>(xml->getDoubleAttribute("blendValue", 0.5));
    mainVolume = static_cast<float>(xml->getDoubleAttribute("mainVolume", 1.0));
    
    for (auto* chainXml : xml->getChildWithTagNameIterator("CHAIN"))
    {
        int index = chainXml->getIntAttribute("index", -1);
        
        if (index >= 0 && index < NUM_CHAINS)
        {
            triggerNotes[index] = chainXml->getIntAttribute("triggerNote", DEFAULT_TRIGGER_NOTES[index]);
            chainVolumes[index] = static_cast<float>(chainXml->getDoubleAttribute("volume", 1.0));
            
            auto* sampleManagerXml = chainXml->getChildByName("SAMPLEMANAGER");
            if (sampleManagerXml != nullptr)
            {
                sampleManagers[index]->restoreFromXml(sampleManagerXml);
            }
        }
    }
    
    return true;
}

bool ChainManager::isChainPlaying(int chainIndex) const
{
    return (chainIndex >= 0 && chainIndex < NUM_CHAINS) ? chainIsPlaying[chainIndex] : false;
}

void ChainManager::addListener(juce::ActionListener* listener)
{
    triggerBroadcaster.addActionListener(listener);
}

void ChainManager::removeListener(juce::ActionListener* listener)
{
    triggerBroadcaster.removeActionListener(listener);
}

void ChainManager::sendChainTriggeredMessage(int chainIndex)
{
    // Use an AsyncUpdater to safely notify from audio thread
    class AsyncTriggerNotifier : public juce::AsyncUpdater
    {
    public:
        AsyncTriggerNotifier(juce::ActionBroadcaster& broadcaster, const juce::String& msg)
            : triggerBroadcaster(broadcaster), message(msg) {}
        
        void handleAsyncUpdate() override
        {
            triggerBroadcaster.sendActionMessage(message);
            delete this;
        }
        
    private:
        juce::ActionBroadcaster& triggerBroadcaster;
        juce::String message;
    };
    
    juce::String messageId = CHAIN_TRIGGERED_PREFIX + juce::String(chainIndex);
    (new AsyncTriggerNotifier(triggerBroadcaster, messageId))->triggerAsyncUpdate();
}

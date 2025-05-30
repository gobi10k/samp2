#pragma once

#include <JuceHeader.h>
#include "AudioSample.h"

/**
 * @class SampleManager
 * @brief Manages a collection of audio samples for playback.
 *
 * This class handles loading, unloading, and sequencing through multiple audio samples,
 * optimized for low-latency sample triggering.
 */
class SampleManager
{
public:
    /**
     * Constructor
     */
    SampleManager()
        : currentSampleIndex(-1),
          volume(1.0f),
          velocitySensitive(true),
          velocityThreshold(1),
          pitchShift(0.0f),
          currentSampleRate(44100.0),
          currentBlockSize(512),
          isPrepared(false)
    {
    }
    
    /**
     * Destructor
     */
    ~SampleManager()
    {
        clearAllSamples();
    }
    
    /**
     * Add a sample from file
     *
     * @param file The audio file
     * @return True if successful
     */
    bool addSample(const juce::File& file)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        // Create a new sample
        std::unique_ptr<AudioSample> newSample = std::make_unique<AudioSample>();
        
        // Try to load the file
        if (!newSample->loadFromFile(file))
            return false;
        
        // Apply current pitch shift to the new sample
        newSample->setPitchShift(pitchShift);
        
        // CRITICAL: Prepare the sample immediately if we know the playback parameters
        if (isPrepared && currentSampleRate > 0.0)
        {
            newSample->prepareToPlay(currentBlockSize, currentSampleRate);
            DBG("SampleManager::addSample() - Sample prepared immediately for low-latency playback");
        }
        
        // Add the sample to our collection
        samples.push_back(std::move(newSample));
        
        // If this is the first sample, make it the current one
        if (currentSampleIndex < 0 && !samples.empty())
            currentSampleIndex = 0;
        
        DBG("SampleManager::addSample() - Added sample: " + file.getFileName());
        return true;
    }
    
    /**
     * Add a sample from URL (more flexible)
     *
     * @param url The audio URL
     * @return True if successful
     */
    bool addSampleFromURL(const juce::URL& url)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        // Create a new sample
        std::unique_ptr<AudioSample> newSample = std::make_unique<AudioSample>();
        
        // Try to load from the URL
        if (!newSample->loadFromURL(url))
            return false;
        
        // Apply current pitch shift to the new sample
        newSample->setPitchShift(pitchShift);
        
        // CRITICAL: Prepare the sample immediately if we know the playback parameters
        if (isPrepared && currentSampleRate > 0.0)
        {
            newSample->prepareToPlay(currentBlockSize, currentSampleRate);
        }
        
        // Add the sample to our collection
        samples.push_back(std::move(newSample));
        
        // If this is the first sample, make it the current one
        if (currentSampleIndex < 0 && !samples.empty())
            currentSampleIndex = 0;
        
        return true;
    }
    
    /**
     * Add samples from files that were dropped
     *
     * @param droppedFiles The files
     * @return Number of successfully loaded samples
     */
    int addSamplesFromDroppedFiles(const juce::StringArray& droppedFiles)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        int successCount = 0;
        
        for (const auto& filePath : droppedFiles)
        {
            juce::File file(filePath);
            
            if (addSample(file))
                ++successCount;
        }
        
        return successCount;
    }
    
    /**
     * Remove a sample
     *
     * @param index The sample index
     * @return True if successful
     */
    bool removeSample(int index)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        if (index < 0 || index >= samples.size())
            return false;
        
        // Stop the sample if it's playing
        if (index == currentSampleIndex && isPlaying())
            stopPlayback();
        
        // Remove the sample
        samples.erase(samples.begin() + index);
        
        // Update the current index if necessary
        if (currentSampleIndex >= samples.size())
            currentSampleIndex = samples.empty() ? -1 : static_cast<int>(samples.size()) - 1;
        
        return true;
    }
    
    /**
     * Clear all samples
     */
    void clearAllSamples()
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        // Stop playback first
        stopPlayback();
        
        // Clear the samples
        samples.clear();
        currentSampleIndex = -1;
    }
    
    /**
     * Get the number of samples
     *
     * @return The number of samples
     */
    int getNumSamples() const
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        return static_cast<int>(samples.size());
    }
    
    /**
     * Get a sample
     *
     * @param index The sample index
     * @return Pointer to the sample or nullptr if invalid
     */
    AudioSample* getSample(int index)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        if (index < 0 || index >= samples.size())
            return nullptr;
        
        return samples[index].get();
    }
    
    /**
     * Move a sample
     *
     * @param currentIndex The current index
     * @param newIndex The new index
     * @return True if successful
     */
    bool moveSample(int currentIndex, int newIndex)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        if (currentIndex < 0 || currentIndex >= samples.size() ||
            newIndex < 0 || newIndex >= samples.size() ||
            currentIndex == newIndex)
            return false;
        
        // Store whether the moved sample was the current one
        bool wasCurrentSample = (currentIndex == currentSampleIndex);
        
        // Move the sample
        auto movedSample = std::move(samples[currentIndex]);
        samples.erase(samples.begin() + currentIndex);
        samples.insert(samples.begin() + newIndex, std::move(movedSample));
        
        // Update the current index if necessary
        if (wasCurrentSample)
            currentSampleIndex = newIndex;
        else if (currentSampleIndex > currentIndex && currentSampleIndex <= newIndex)
            --currentSampleIndex;
        else if (currentSampleIndex < currentIndex && currentSampleIndex >= newIndex)
            ++currentSampleIndex;
        
        return true;
    }
    
    /**
     * Trigger playback of the current sample - OPTIMIZED FOR LOW LATENCY
     *
     * @param velocity The MIDI velocity (0-127)
     * @return True if successful
     */
    bool triggerCurrentSample(int velocity)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        DBG("[TRIGGER] Entry - Velocity: " << velocity);
        
        // Check if we have a current sample
        if (currentSampleIndex < 0 || currentSampleIndex >= samples.size())
        {
            DBG("[TRIGGER] FAIL: No current sample");
            return false;
        }
        
        // Check the velocity threshold
        if (velocity < velocityThreshold)
        {
            DBG("[TRIGGER] Velocity too low: " << velocity << " < " << velocityThreshold);
            return false;
        }
        
        // Get the current sample
        AudioSample* currentSample = samples[currentSampleIndex].get();
        if (currentSample == nullptr)
        {
            DBG("[TRIGGER] FAIL: Current sample is null");
            return false;
        }
        
        // CRITICAL: Check if sample is prepared for low-latency playback
        if (!currentSample->isPreparedForPlayback())
        {
            DBG("[TRIGGER] WARNING: Sample not prepared - this will cause delay!");
            // Try to prepare it now (but this might still cause delay)
            if (isPrepared && currentSampleRate > 0.0)
            {
                currentSample->prepareToPlay(currentBlockSize, currentSampleRate);
            }
        }
        
        DBG("[TRIGGER] About to stop playback");
        
        // Stop any currently playing sample immediately
        stopPlayback();
        
        DBG("[TRIGGER] Calculating gain");
        
        // Calculate gain based on velocity if enabled
        float gain = volume;
        
        if (velocitySensitive)
            gain *= static_cast<float>(velocity) / 127.0f;
        
        DBG("[TRIGGER] About to start sample " << currentSampleIndex << " with gain " << gain);
        
        // Start playback immediately - should now be truly immediate!
        currentSample->start(gain);
        
        DBG("[TRIGGER] SUCCESS - Low-latency trigger complete");
        
        return true;
    }
    
    /**
     * Stop playback
     */
    void stopPlayback()
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        // Stop all samples immediately
        for (auto& sample : samples)
        {
            if (sample != nullptr)
            {
                sample->stop();
            }
        }
    }
    
    /**
     * Check if a sample is playing
     *
     * @return True if playing
     */
    bool isPlaying() const
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        if (currentSampleIndex >= 0 && currentSampleIndex < samples.size())
        {
            // Additional check for the sample pointer itself before calling a method on it
            AudioSample* currentSamplePtr = samples[currentSampleIndex].get();
            if (currentSamplePtr != nullptr)
                return currentSamplePtr->isPlaying();
        }
        return false;
    }
    
    /**
     * Get the current playback position as a proportion
     *
     * @return Position as proportion (0-1) or -1 if no sample is playing
     */
    double getCurrentPosition() const
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        if (currentSampleIndex >= 0 && currentSampleIndex < samples.size())
        {
            // Additional check for the sample pointer itself
            AudioSample* currentSamplePtr = samples[currentSampleIndex].get();
            if (currentSamplePtr != nullptr && currentSamplePtr->isPlaying()) // isPlaying already checks if loaded
                return currentSamplePtr->getCurrentPositionProportion();
        }
        return -1.0;
    }
    
    /**
     * Process audio - OPTIMIZED FOR LOW LATENCY
     *
     * @param buffer The buffer to fill
     * @param numSamples The number of samples to process
     */
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        // Clear the buffer first
        buffer.clear();
        
        // If no sample is current, we're done
        if (currentSampleIndex < 0 || currentSampleIndex >= samples.size())
            return;
        
        AudioSample* currentSamplePtr = samples[currentSampleIndex].get();
        if (currentSamplePtr == nullptr)
            return;
        
        // Process audio through the current sample - this should now be low-latency
        currentSamplePtr->processBlock(buffer, numSamples);
    }
    
    /**
     * Process audio (compatibility with AudioSource)
     *
     * @param bufferToFill The buffer to fill
     */
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        // Clear the buffer first
        bufferToFill.clearActiveBufferRegion();
        
        // If no sample is current, we're done
        if (currentSampleIndex < 0 || currentSampleIndex >= samples.size())
            return;
            
        AudioSample* currentSamplePtr = samples[currentSampleIndex].get();
        if (currentSamplePtr == nullptr)
            return;

        // Process audio through the current sample
        currentSamplePtr->getNextAudioBlock(bufferToFill);
    }
    
    /**
     * Advance to the next sample
     *
     * @return The new sample index
     */
    int advanceToNextSample()
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        if (samples.empty())
        {
            currentSampleIndex = -1;
            return -1;
        }
        
        // Move to the next sample or wrap around
        currentSampleIndex = (currentSampleIndex + 1) % samples.size();
        
        return currentSampleIndex;
    }
    
    /**
     * Set the current sample
     *
     * @param index The sample index
     * @return True if successful
     */
    bool setCurrentSampleIndex(int index)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        if (index < 0 || index >= samples.size())
            return false;
        
        currentSampleIndex = index;
        return true;
    }
    
    /**
     * Get the current sample index
     *
     * @return The current sample index
     */
    int getCurrentSampleIndex() const
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        return currentSampleIndex;
    }
    
    /**
     * Get the current sample
     *
     * @return Pointer to the current sample or nullptr if none
     */
    AudioSample* getCurrentSample()
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        if (currentSampleIndex < 0 || currentSampleIndex >= samples.size())
            return nullptr;
        
        return samples[currentSampleIndex].get();
    }
    
    /**
     * Set the volume
     *
     * @param newVolume The volume (0.0 to 1.0)
     */
    void setVolume(float newVolume)
    {
        volume = juce::jlimit(0.0f, 1.0f, newVolume);
    }
    
    /**
     * Get the volume
     *
     * @return The volume
     */
    float getVolume() const
    {
        return volume;
    }
    
    /**
     * Set velocity sensitivity
     *
     * @param shouldBeVelocitySensitive True to enable
     */
    void setVelocitySensitive(bool shouldBeVelocitySensitive)
    {
        velocitySensitive = shouldBeVelocitySensitive;
    }
    
    /**
     * Check if velocity sensitivity is enabled
     *
     * @return True if enabled
     */
    bool isVelocitySensitive() const
    {
        return velocitySensitive;
    }
    
    /**
     * Set the velocity threshold
     *
     * @param newThreshold The threshold (1-127)
     */
    void setVelocityThreshold(int newThreshold)
    {
        velocityThreshold = juce::jlimit(1, 127, newThreshold);
    }
    
    /**
     * Get the velocity threshold
     *
     * @return The threshold
     */
    int getVelocityThreshold() const
    {
        return velocityThreshold;
    }
    
    /**
     * Set the pitch shift (semitones)
     *
     * @param semitones The pitch shift in semitones
     */
    void setPitchShift(float semitones)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        // Store the pitch shift value
        pitchShift = juce::jlimit(-12.0f, 12.0f, semitones);
        
        // Apply pitch shift to all loaded samples
        for (auto& sample : samples)
        {
            if (sample != nullptr) // Check for null before calling method
                sample->setPitchShift(pitchShift);
        }
    }
    
    
    /**
     * Get the pitch shift
     *
     * @return The pitch shift in semitones
     */
    float getPitchShift() const
    {
        return pitchShift;
    }
    
    /**
     * Set the time stretch ratio for all samples
     * This is kept for compatibility but does nothing
     *
     * @param ratio The time stretch ratio (ignored)
     */
    void setTimeStretchForAll(float ratio)
    {
        // Do nothing - time stretching is disabled
    }
    
    /**
     * Prepare for playback - CRITICAL FOR LOW LATENCY
     *
     * @param sampleRate The sample rate
     * @param blockSize The block size
     */
    void prepareToPlay(double sampleRate, int blockSize)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        currentSampleRate = sampleRate;
        currentBlockSize = blockSize;
        isPrepared = true;
        
        DBG("SampleManager::prepareToPlay() - Preparing all samples for low-latency playback");
        DBG("Sample rate: " << sampleRate << ", Block size: " << blockSize);
        
        // Prepare each sample for low-latency playback
        for (auto& sample : samples)
        {
            if (sample != nullptr) // Check for null before calling method
                sample->prepareToPlay(blockSize, sampleRate);
        }
        
        DBG("SampleManager::prepareToPlay() - All samples prepared");
    }
    
    /**
     * Release resources
     */
    void releaseResources()
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        DBG("SampleManager::releaseResources() - Starting");
        
        // Release resources for each sample
        for (auto& sample : samples)
        {
            if (sample != nullptr) // Check for null before calling method
                 sample->releaseResources();
        }
        
        isPrepared = false;
        
        DBG("SampleManager::releaseResources() - Complete");
    }
    
    /**
     * Save to XML
     *
     * @return The XML element
     */
    std::unique_ptr<juce::XmlElement> saveToXml() const
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        auto xml = std::make_unique<juce::XmlElement>("SAMPLEMANAGER");
        
        // Save general settings
        xml->setAttribute("currentSampleIndex", currentSampleIndex);
        xml->setAttribute("volume", volume);
        xml->setAttribute("velocitySensitive", velocitySensitive);
        xml->setAttribute("velocityThreshold", velocityThreshold);
        xml->setAttribute("pitchShift", pitchShift);
        
        // Save each sample
        auto samplesXml = xml->createNewChildElement("SAMPLES");
        
        for (const auto& sample : samples)
        {
            if (sample != nullptr) // Check for null before calling method
                samplesXml->addChildElement(sample->saveToXml().release());
        }
        return xml;
    }
    
    /**
     * Restore from XML
     *
     * @param xml The XML element
     * @return True if successful
     */
    bool restoreFromXml(const juce::XmlElement* xml)
    {
        const juce::ScopedLock sl(sampleAccessLock_);
        if (xml == nullptr || xml->getTagName() != "SAMPLEMANAGER")
            return false;
        
        // Clear existing samples
        clearAllSamples();
        
        // Load general settings
        currentSampleIndex = xml->getIntAttribute("currentSampleIndex", -1);
        volume = static_cast<float>(xml->getDoubleAttribute("volume", 1.0));
        velocitySensitive = xml->getBoolAttribute("velocitySensitive", true);
        velocityThreshold = xml->getIntAttribute("velocityThreshold", 1);
        pitchShift = static_cast<float>(xml->getDoubleAttribute("pitchShift", 0.0));
        
        // Load samples
        auto samplesXml = xml->getChildByName("SAMPLES");
        
        if (samplesXml != nullptr)
        {
            for (auto* sampleXml : samplesXml->getChildIterator())
            {
                std::unique_ptr<AudioSample> sample = std::make_unique<AudioSample>();
                
                if (sample->restoreFromXml(sampleXml))
                {
                    // Prepare for playback if sample rate is set
                    if (isPrepared && currentSampleRate > 0.0)
                        sample->prepareToPlay(currentBlockSize, currentSampleRate);
                        
                    samples.push_back(std::move(sample));
                }
            }
        }
        
        // Validate current sample index
        if (currentSampleIndex >= samples.size())
        {
            currentSampleIndex = samples.empty() ? -1 : 0;
        }
        
        return true;
    }

private:
    std::vector<std::unique_ptr<AudioSample>> samples;
    int currentSampleIndex;
    float volume;
    bool velocitySensitive;
    int velocityThreshold;
    float pitchShift;
    double currentSampleRate;
    int currentBlockSize;
    bool isPrepared;
    
    juce::CriticalSection sampleAccessLock_; // Added lock

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleManager)
};

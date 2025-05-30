#pragma once

#include <JuceHeader.h>

/**
 * @class AudioSample
 * @brief Enhanced audio sample with high-quality pitch shifting that maintains duration.
 *
 * This implementation uses a PSOLA-style pitch shifter with Hann windowing
 * and proper interpolation for high-quality audio with maintained duration.
 */
class AudioSample : public juce::ChangeListener
{
public:
    AudioSample()
        : isLoaded(false),
          isCurrentlyPlaying(false),
          currentPosition(0.0),
          gain(1.0f),
          pitchShiftSemitones(0.0f),
          pitchRatio(1.0f),
          grainSize(2048),
          hopSize(512),
          fadeLength(128),
          inputReadPosition(0.0),
          outputWritePosition(0.0),
          grainPhase(0.0)
    {
        formatManager.registerBasicFormats();
        thumbnailCache = std::make_unique<juce::AudioThumbnailCache>(10);
        thumbnail = std::make_unique<juce::AudioThumbnail>(512, formatManager, *thumbnailCache);
        thumbnail->addChangeListener(this);
        
        // Initialize pitch shift buffers
        initializePitchShift();
        
        // Pre-calculate Hann window
        calculateHannWindow();
    }
    
    ~AudioSample()
    {
        if (thumbnail != nullptr)
            thumbnail->removeChangeListener(this);
    }
    
    bool loadFromFile(const juce::File& file)
    {
        if (!file.existsAsFile())
            return false;
        
        sourceFile = file;
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        
        if (reader == nullptr)
            return false;
        
        auto numChannels = reader->numChannels;
        auto lengthInSamples = reader->lengthInSamples;
        sampleRate = reader->sampleRate;
        
        DBG("AudioSample::loadFromFile() - Loading: " + file.getFileName());
        DBG("Channels: " << static_cast<int>(numChannels) << ", Length: " << static_cast<int>(lengthInSamples) << ", SR: " << sampleRate);
        
        // Load entire file into memory
        audioBuffer.setSize(static_cast<int>(numChannels), static_cast<int>(lengthInSamples));
        reader->read(&audioBuffer, 0, static_cast<int>(lengthInSamples), 0, true, true);
        
        // Set up thumbnail
        thumbnail->clear();
        thumbnail->setSource(new juce::FileInputSource(file));
        
        // Reset playback state
        isCurrentlyPlaying = false;
        currentPosition = 0.0;
        gain = 1.0f;
        
        // Initialize pitch shift for this sample
        initializePitchShiftForSample();
        
        isLoaded = true;
        
        DBG("AudioSample::loadFromFile() - SUCCESS! File loaded into memory buffer");
        return true;
    }
    
    bool loadFromURL(const juce::URL& url)
    {
        if (url.isLocalFile())
        {
            return loadFromFile(url.getLocalFile());
        }
        
        DBG("AudioSample::loadFromURL() - URL loading not fully implemented");
        return false;
    }
    
    void start(float newGain)
    {
        if (!isLoaded)
        {
            DBG("AudioSample::start() - Sample not loaded");
            return;
        }
        
        gain = newGain;
        currentPosition = 0.0;
        isCurrentlyPlaying = true;
        
        // Reset pitch shift state for clean start
        resetPitchShiftState();
        
        DBG("AudioSample::start() - IMMEDIATE START! Gain: " + juce::String(newGain));
    }
    
    void stop()
    {
        isCurrentlyPlaying = false;
        currentPosition = 0.0;
        DBG("AudioSample::stop() - Stopped");
    }
    
    bool isPlaying() const
    {
        return isCurrentlyPlaying && currentPosition < audioBuffer.getNumSamples();
    }
    
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        if (!isLoaded || !isCurrentlyPlaying)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }
        
        bufferToFill.clearActiveBufferRegion();
        
        if (std::abs(pitchRatio - 1.0f) < 0.001f)
        {
            // No pitch shift - direct copy for lowest latency
            processDirectCopy(bufferToFill);
        }
        else
        {
            // Apply high-quality pitch shift while maintaining duration
            processHighQualityPitchShift(bufferToFill);
        }
    }
    
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
    {
        juce::AudioSourceChannelInfo info(&buffer, 0, numSamples);
        getNextAudioBlock(info);
    }
    
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate)
    {
        // Update grain size based on expected block size for optimal performance
        if (samplesPerBlockExpected > 0)
        {
            // Use larger grains for better quality, smaller hops for smoother playback
            grainSize = juce::jmax(samplesPerBlockExpected * 8, 4096);
            hopSize = grainSize / 8;  // Smaller hop for better quality
            fadeLength = hopSize / 2;
            initializePitchShift();
            calculateHannWindow();
        }
        DBG("AudioSample::prepareToPlay() - Grain size: " << grainSize);
    }
    
    void releaseResources()
    {
        stop();
    }
    
    void setPitchShift(float semitones)
    {
        pitchShiftSemitones = juce::jlimit(-12.0f, 12.0f, semitones);
        // Convert semitones to pitch ratio: 2^(semitones/12)
        pitchRatio = std::pow(2.0f, pitchShiftSemitones / 12.0f);
        DBG("Pitch shift set to " << pitchShiftSemitones << " semitones, ratio: " << pitchRatio);
    }
    
    float getPitchShift() const { return pitchShiftSemitones; }
    
    juce::String getFileName() const { return sourceFile.getFileName(); }
    juce::String getFilePath() const { return sourceFile.getFullPathName(); }
    juce::AudioThumbnail& getThumbnail() { return *thumbnail; }
    double getLengthInSeconds() const { return isLoaded && sampleRate > 0.0 ? audioBuffer.getNumSamples() / sampleRate : 0.0; }
    int getLengthInSamples() const { return isLoaded ? audioBuffer.getNumSamples() : 0; }
    double getCurrentPosition() const { return isLoaded && sampleRate > 0.0 ? currentPosition / sampleRate : 0.0; }
    double getCurrentPositionProportion() const { return isLoaded && audioBuffer.getNumSamples() > 0 ? currentPosition / audioBuffer.getNumSamples() : 0.0; }
    bool isValidAndLoaded() const { return isLoaded; }
    
    std::unique_ptr<juce::XmlElement> saveToXml() const
    {
        auto xml = std::make_unique<juce::XmlElement>("AUDIOSAMPLE");
        xml->setAttribute("filepath", sourceFile.getFullPathName());
        xml->setAttribute("pitchshift", pitchShiftSemitones);
        return xml;
    }
    
    bool restoreFromXml(const juce::XmlElement* xml)
    {
        if (xml == nullptr || xml->getTagName() != "AUDIOSAMPLE")
            return false;
        
        juce::String filePath = xml->getStringAttribute("filepath");
        juce::File file(filePath);
        
        if (!loadFromFile(file))
            return false;
        
        float pitchShift = static_cast<float>(xml->getDoubleAttribute("pitchshift", 0.0f));
        setPitchShift(pitchShift);
        
        return true;
    }
    
    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if (source == thumbnail.get()) { /* Thumbnail updated */ }
    }
    
    // Compatibility methods
    void setStartPoint(double proportionOfLength) { }
    double getStartPoint() const { return 0.0; }
    void setEndPoint(double proportionOfLength) { }
    double getEndPoint() const { return 1.0; }
    float getTimeStretch() const { return 1.0f; }
    void setTimeStretch(float ratio) { }
    void setPositionProportion(double proportionOfLength) { }
    bool isPreparedForPlayback() const { return isLoaded; }

private:
    juce::File sourceFile;
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioThumbnailCache> thumbnailCache;
    std::unique_ptr<juce::AudioThumbnail> thumbnail;
    
    juce::AudioBuffer<float> audioBuffer;
    
    bool isLoaded;
    bool isCurrentlyPlaying;
    double currentPosition;
    float gain;
    double sampleRate;
    
    float pitchShiftSemitones;
    float pitchRatio;
    
    // Enhanced pitch shift processing state
    juce::AudioBuffer<float> grainBuffer;
    juce::AudioBuffer<float> overlapBuffer;
    juce::AudioBuffer<float> outputBuffer;
    std::vector<float> hannWindow;
    
    int grainSize;
    int hopSize;
    int fadeLength;
    
    double inputReadPosition;
    double outputWritePosition;
    double grainPhase;
    int grainCounter;
    
    void initializePitchShift()
    {
        grainBuffer.setSize(2, grainSize * 2);
        overlapBuffer.setSize(2, grainSize * 2);
        outputBuffer.setSize(2, grainSize * 2);
        grainBuffer.clear();
        overlapBuffer.clear();
        outputBuffer.clear();
        resetPitchShiftState();
    }
    
    void initializePitchShiftForSample()
    {
        if (audioBuffer.getNumChannels() > 0)
        {
            grainBuffer.setSize(audioBuffer.getNumChannels(), grainSize * 2);
            overlapBuffer.setSize(audioBuffer.getNumChannels(), grainSize * 2);
            outputBuffer.setSize(audioBuffer.getNumChannels(), grainSize * 2);
            grainBuffer.clear();
            overlapBuffer.clear();
            outputBuffer.clear();
        }
    }
    
    void resetPitchShiftState()
    {
        inputReadPosition = 0.0;
        outputWritePosition = 0.0;
        grainPhase = 0.0;
        grainCounter = 0;
        grainBuffer.clear();
        overlapBuffer.clear();
        outputBuffer.clear();
    }
    
    void calculateHannWindow()
    {
        hannWindow.resize(grainSize);
        for (int i = 0; i < grainSize; ++i)
        {
            hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (grainSize - 1)));
        }
    }
    
    void processDirectCopy(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        int samplesAvailable = audioBuffer.getNumSamples() - static_cast<int>(currentPosition);
        int samplesToCopy = juce::jmin(bufferToFill.numSamples, samplesAvailable);
        
        if (samplesToCopy <= 0)
        {
            isCurrentlyPlaying = false;
            return;
        }
        
        for (int channel = 0; channel < juce::jmin(bufferToFill.buffer->getNumChannels(), audioBuffer.getNumChannels()); ++channel)
        {
            bufferToFill.buffer->copyFrom(
                channel,
                bufferToFill.startSample,
                audioBuffer,
                channel,
                static_cast<int>(currentPosition),
                samplesToCopy
            );
            
            if (gain != 1.0f)
            {
                bufferToFill.buffer->applyGain(channel, bufferToFill.startSample, samplesToCopy, gain);
            }
        }
        
        currentPosition += samplesToCopy;
        
        if (currentPosition >= audioBuffer.getNumSamples())
        {
            isCurrentlyPlaying = false;
        }
    }
    
    void processHighQualityPitchShift(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        auto* outputBuffer = bufferToFill.buffer;
        const int numChannels = juce::jmin(outputBuffer->getNumChannels(), audioBuffer.getNumChannels());
        const int numSamples = bufferToFill.numSamples;
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Process grain-based pitch shifting
            if (grainPhase >= hopSize)
            {
                processGrain(numChannels);
                grainPhase = 0.0;
                grainCounter++;
            }
            
            // Output samples from overlap buffer
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float outputSample = 0.0f;
                
                // Get sample from overlap buffer with bounds checking
                int overlapIndex = static_cast<int>(outputWritePosition) % (grainSize * 2);
                if (overlapIndex >= 0 && overlapIndex < overlapBuffer.getNumSamples())
                {
                    outputSample = overlapBuffer.getSample(ch, overlapIndex);
                    // Clear the sample after reading to avoid repeated playback
                    overlapBuffer.setSample(ch, overlapIndex, 0.0f);
                }
                
                // Apply gain and write to output
                outputBuffer->addSample(ch, bufferToFill.startSample + sample, outputSample * gain);
            }
            
            // Advance positions
            grainPhase += 1.0;
            outputWritePosition += 1.0;
            
            // Advance current position at normal speed (maintains duration)
            currentPosition += 1.0;
            
            // Check if we've reached the end
            if (currentPosition >= audioBuffer.getNumSamples())
            {
                isCurrentlyPlaying = false;
                return;
            }
        }
    }
    
    void processGrain(int numChannels)
    {
        // Calculate input position for this grain
        double actualInputPos = inputReadPosition;
        
        // Extract grain from input with pitch-dependent resampling
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < grainSize; ++i)
            {
                // Calculate source position with pitch ratio applied
                double sourcePos = actualInputPos + (i * pitchRatio);
                
                // Bounds checking
                if (sourcePos >= audioBuffer.getNumSamples() - 1)
                {
                    break;
                }
                
                // Cubic (Catmull-Rom) interpolation for fractional sample positions
                int intPos = static_cast<int>(sourcePos);
                float frac = static_cast<float>(sourcePos - intPos);

                float y0, y1, y2, y3;
                int lastSampleIndex = audioBuffer.getNumSamples() - 1;

                // Ensure indices are within bounds
                y0 = audioBuffer.getSample(ch, juce::jmax(0, intPos - 1));
                y1 = audioBuffer.getSample(ch, juce::jmax(0, intPos)); // Clamp intPos if it's somehow negative, though sourcePos should be positive
                y2 = audioBuffer.getSample(ch, juce::jmin(lastSampleIndex, intPos + 1));
                y3 = audioBuffer.getSample(ch, juce::jmin(lastSampleIndex, intPos + 2));

                // Catmull-Rom formula:
                // out = P1 + 0.5 * t * ((P2 - P0) + t * ((2 * P0 - 5 * P1 + 4 * P2 - P3) + t * (-P0 + 3 * P1 - 3 * P2 + P3)))
                // Here, y1 is P1, y2 is P2, y0 is P0, y3 is P3, and frac is t.
                float interpolatedSample = y1 + 0.5f * frac * (
                                                (y2 - y0) + frac * (
                                                    (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3) + frac * (
                                                        -y0 + 3.0f * y1 - 3.0f * y2 + y3
                                                    )
                                                )
                                            );
                
                // Apply Hann window for smooth grain boundaries
                float windowedSample = interpolatedSample * hannWindow[i];
                
                // Store in grain buffer
                grainBuffer.setSample(ch, i, windowedSample);
            }
        }
        
        // Add grain to overlap buffer with proper positioning
        int overlapStart = static_cast<int>(outputWritePosition) % (grainSize * 2);
        
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < grainSize; ++i)
            {
                int overlapIndex = (overlapStart + i) % (grainSize * 2);
                if (overlapIndex >= 0 && overlapIndex < overlapBuffer.getNumSamples())
                {
                    float existingSample = overlapBuffer.getSample(ch, overlapIndex);
                    float newSample = grainBuffer.getSample(ch, i);
                    overlapBuffer.setSample(ch, overlapIndex, existingSample + newSample);
                }
            }
        }
        
        // Advance input position at normal speed (maintains duration)
        inputReadPosition += hopSize;
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSample)
};

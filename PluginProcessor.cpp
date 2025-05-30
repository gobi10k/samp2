#include "PluginProcessor.h"
#include "PluginEditor.h"

// Define parameter IDs
const juce::String DualChainSampleTriggerProcessor::PARAM_BLEND = "blend";
const juce::String DualChainSampleTriggerProcessor::PARAM_MAIN_VOLUME = "main_volume";
const juce::String DualChainSampleTriggerProcessor::PARAM_CHAIN1_VOLUME = "chain1_volume";
const juce::String DualChainSampleTriggerProcessor::PARAM_CHAIN2_VOLUME = "chain2_volume";
const juce::String DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE = "chain1_note";
const juce::String DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE = "chain2_note";
const juce::String DualChainSampleTriggerProcessor::PARAM_CHAIN1_VELOCITY_SENSITIVE = "chain1_velocity_sensitive";
const juce::String DualChainSampleTriggerProcessor::PARAM_CHAIN2_VELOCITY_SENSITIVE = "chain2_velocity_sensitive";
const juce::String DualChainSampleTriggerProcessor::PARAM_CHAIN1_VELOCITY_THRESHOLD = "chain1_velocity_threshold";
const juce::String DualChainSampleTriggerProcessor::PARAM_CHAIN2_VELOCITY_THRESHOLD = "chain2_velocity_threshold";
const juce::String DualChainSampleTriggerProcessor::PARAM_CHAIN1_PITCH_SHIFT = "chain1_pitch_shift";
const juce::String DualChainSampleTriggerProcessor::PARAM_CHAIN2_PITCH_SHIFT = "chain2_pitch_shift";

// APVTS Title Parameter ID definitions are removed
// const juce::StringRef DualChainSampleTriggerProcessor::PARAM_SESSION_TITLE = "sessionTitle";
// const juce::StringRef DualChainSampleTriggerProcessor::PARAM_CHAIN1_TITLE = "chain1Title";
// const juce::StringRef DualChainSampleTriggerProcessor::PARAM_CHAIN2_TITLE = "chain2Title";

//==============================================================================
DualChainSampleTriggerProcessor::DualChainSampleTriggerProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state("DualChainSampleTrigger")
{
    // Create the chain manager
    chainManager = std::make_unique<ChainManager>();
    
    // Create parameters
    createParameters();

    // Initialize String Members for titles
    currentSessionTitle = "sample keyboard";
    currentChain1Title = "Chain 1";
    currentChain2Title = "Chain 2";
    
    // Initialize state
    initializeState();
    
    // Add this as a listener to the value tree (for state changes)
    state.addListener(this);
    
    // Add parameter change callback
    parameters->addParameterListener(PARAM_BLEND, this);
    parameters->addParameterListener(PARAM_MAIN_VOLUME, this);
    parameters->addParameterListener(PARAM_CHAIN1_VOLUME, this);
    parameters->addParameterListener(PARAM_CHAIN2_VOLUME, this);
    parameters->addParameterListener(PARAM_CHAIN1_NOTE, this);
    parameters->addParameterListener(PARAM_CHAIN2_NOTE, this);
    parameters->addParameterListener(PARAM_CHAIN1_VELOCITY_SENSITIVE, this);
    parameters->addParameterListener(PARAM_CHAIN2_VELOCITY_SENSITIVE, this);
    parameters->addParameterListener(PARAM_CHAIN1_VELOCITY_THRESHOLD, this);
    parameters->addParameterListener(PARAM_CHAIN2_VELOCITY_THRESHOLD, this);
    parameters->addParameterListener(PARAM_CHAIN1_PITCH_SHIFT, this);
    parameters->addParameterListener(PARAM_CHAIN2_PITCH_SHIFT, this);
    // Remove APVTS listeners for titles
    // parameters->addParameterListener(PARAM_SESSION_TITLE.toString(), this);
    // parameters->addParameterListener(PARAM_CHAIN1_TITLE.toString(), this);
    // parameters->addParameterListener(PARAM_CHAIN2_TITLE.toString(), this);
}

DualChainSampleTriggerProcessor::~DualChainSampleTriggerProcessor()
{
    // Remove parameter listeners
    parameters->removeParameterListener(PARAM_BLEND, this);
    parameters->removeParameterListener(PARAM_MAIN_VOLUME, this);
    parameters->removeParameterListener(PARAM_CHAIN1_VOLUME, this);
    parameters->removeParameterListener(PARAM_CHAIN2_VOLUME, this);
    parameters->removeParameterListener(PARAM_CHAIN1_NOTE, this);
    parameters->removeParameterListener(PARAM_CHAIN2_NOTE, this);
    parameters->removeParameterListener(PARAM_CHAIN1_VELOCITY_SENSITIVE, this);
    parameters->removeParameterListener(PARAM_CHAIN2_VELOCITY_SENSITIVE, this);
    parameters->removeParameterListener(PARAM_CHAIN1_VELOCITY_THRESHOLD, this);
    parameters->removeParameterListener(PARAM_CHAIN2_VELOCITY_THRESHOLD, this);
    parameters->removeParameterListener(PARAM_CHAIN1_PITCH_SHIFT, this);
    parameters->removeParameterListener(PARAM_CHAIN2_PITCH_SHIFT, this);
    // Remove APVTS listeners for titles
    // parameters->removeParameterListener(PARAM_SESSION_TITLE.toString(), this);
    // parameters->removeParameterListener(PARAM_CHAIN1_TITLE.toString(), this);
    // parameters->removeParameterListener(PARAM_CHAIN2_TITLE.toString(), this);
    
    // Remove this as a listener from the state
    state.removeListener(this);
}

//==============================================================================
void DualChainSampleTriggerProcessor::createParameters()
{
    // Create the parameters object
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Add blend parameter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        PARAM_BLEND, "Blend", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value * 100.0f)) + "%"; },
        [](const juce::String& text) { return text.getFloatValue() / 100.0f; }
    ));
    
    // Add main volume parameter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        PARAM_MAIN_VOLUME, "Main Volume", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value * 100.0f)) + "%"; },
        [](const juce::String& text) { return text.getFloatValue() / 100.0f; }
    ));
    
    // Add chain 1 volume parameter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        PARAM_CHAIN1_VOLUME, "Chain 1 Volume", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value * 100.0f)) + "%"; },
        [](const juce::String& text) { return text.getFloatValue() / 100.0f; }
    ));
    
    // Add chain 2 volume parameter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        PARAM_CHAIN2_VOLUME, "Chain 2 Volume", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value * 100.0f)) + "%"; },
        [](const juce::String& text) { return text.getFloatValue() / 100.0f; }
    ));
    
    // Add chain 1 MIDI note parameter
    layout.add(std::make_unique<juce::AudioParameterInt>(
        PARAM_CHAIN1_NOTE, "Chain 1 Note", 0, 127, 60, // Default to middle C
        juce::String(),
        [](int value, int) {
            static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            int octave = value / 12 - 1;
            int note = value % 12;
            return juce::String(noteNames[note]) + juce::String(octave) + " (" + juce::String(value) + ")";
        }
    ));
    
    // Add chain 2 MIDI note parameter
    layout.add(std::make_unique<juce::AudioParameterInt>(
        PARAM_CHAIN2_NOTE, "Chain 2 Note", 0, 127, 62, // Default to D
        juce::String(),
        [](int value, int) {
            static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            int octave = value / 12 - 1;
            int note = value % 12;
            return juce::String(noteNames[note]) + juce::String(octave) + " (" + juce::String(value) + ")";
        }
    ));
    
    // Add chain 1 velocity sensitive parameter
    layout.add(std::make_unique<juce::AudioParameterBool>(
        PARAM_CHAIN1_VELOCITY_SENSITIVE, "Chain 1 Velocity Sensitive", true
    ));
    
    // Add chain 2 velocity sensitive parameter
    layout.add(std::make_unique<juce::AudioParameterBool>(
        PARAM_CHAIN2_VELOCITY_SENSITIVE, "Chain 2 Velocity Sensitive", true
    ));
    
    // Add chain 1 velocity threshold parameter
    layout.add(std::make_unique<juce::AudioParameterInt>(
        PARAM_CHAIN1_VELOCITY_THRESHOLD, "Chain 1 Velocity Threshold", 1, 127, 1
    ));
    
    // Add chain 2 velocity threshold parameter
    layout.add(std::make_unique<juce::AudioParameterInt>(
        PARAM_CHAIN2_VELOCITY_THRESHOLD, "Chain 2 Velocity Threshold", 1, 127, 1
    ));
    
    // Add chain 1 pitch shift parameter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        PARAM_CHAIN1_PITCH_SHIFT, "Chain 1 Pitch Shift", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) {
            // Format the value with + sign for positive values
            return (value > 0.0f ? "+" : "") + juce::String(value, 1) + " st";
        }
    ));
    
    // Add chain 2 pitch shift parameter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        PARAM_CHAIN2_PITCH_SHIFT, "Chain 2 Pitch Shift", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) {
            return (value > 0.0f ? "+" : "") + juce::String(value, 1) + " st";
        }
    ));

    // Remove AudioParameterString for titles from layout
    // layout.add(std::make_unique<juce::AudioParameterString>(...));
    // layout.add(std::make_unique<juce::AudioParameterString>(...));
    // layout.add(std::make_unique<juce::AudioParameterString>(...));
    
    // Create the parameters object
    parameters = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "Parameters", std::move(layout));
}


void DualChainSampleTriggerProcessor::initializeState()
{
    // Set initial values for the chain manager from parameters
    chainManager->setBlendValue(parameters->getParameterAsValue(PARAM_BLEND).getValue());
    chainManager->setMainVolume(parameters->getParameterAsValue(PARAM_MAIN_VOLUME).getValue());
    chainManager->setChainVolume(0, parameters->getParameterAsValue(PARAM_CHAIN1_VOLUME).getValue());
    chainManager->setChainVolume(1, parameters->getParameterAsValue(PARAM_CHAIN2_VOLUME).getValue());
    chainManager->setTriggerNote(0, parameters->getParameterAsValue(PARAM_CHAIN1_NOTE).getValue());
    chainManager->setTriggerNote(1, parameters->getParameterAsValue(PARAM_CHAIN2_NOTE).getValue());
    
    // Set velocity sensitivity, threshold, and pitch shift for both chains
    for (int i = 0; i < 2; ++i)
    {
        SampleManager* sampleManager = chainManager->getSampleManager(i);
        
        if (sampleManager != nullptr)
        {
            sampleManager->setVelocitySensitive(i == 0 ?
                parameters->getParameterAsValue(PARAM_CHAIN1_VELOCITY_SENSITIVE).getValue() :
                parameters->getParameterAsValue(PARAM_CHAIN2_VELOCITY_SENSITIVE).getValue());
            
            sampleManager->setVelocityThreshold(i == 0 ?
                parameters->getParameterAsValue(PARAM_CHAIN1_VELOCITY_THRESHOLD).getValue() :
                parameters->getParameterAsValue(PARAM_CHAIN2_VELOCITY_THRESHOLD).getValue());
                
            sampleManager->setPitchShift(i == 0 ?
                parameters->getParameterAsValue(PARAM_CHAIN1_PITCH_SHIFT).getValue() :
                parameters->getParameterAsValue(PARAM_CHAIN2_PITCH_SHIFT).getValue());
        }
    }
    
    // Initialize the state ValueTree
    state.setProperty("version", 1, nullptr);
}

void DualChainSampleTriggerProcessor::updateParametersFromState()
{
    // Update parameters from state
    parameters->getParameterAsValue(PARAM_BLEND) = state.getProperty("blend", 0.5f);
    parameters->getParameterAsValue(PARAM_MAIN_VOLUME) = state.getProperty("mainVolume", 1.0f);
    parameters->getParameterAsValue(PARAM_CHAIN1_VOLUME) = state.getProperty("chain1Volume", 1.0f);
    parameters->getParameterAsValue(PARAM_CHAIN2_VOLUME) = state.getProperty("chain2Volume", 1.0f);
    parameters->getParameterAsValue(PARAM_CHAIN1_NOTE) = state.getProperty("chain1Note", 60);
    parameters->getParameterAsValue(PARAM_CHAIN2_NOTE) = state.getProperty("chain2Note", 62);
    parameters->getParameterAsValue(PARAM_CHAIN1_VELOCITY_SENSITIVE) = state.getProperty("chain1VelocitySensitive", true);
    parameters->getParameterAsValue(PARAM_CHAIN2_VELOCITY_SENSITIVE) = state.getProperty("chain2VelocitySensitive", true);
    parameters->getParameterAsValue(PARAM_CHAIN1_VELOCITY_THRESHOLD) = state.getProperty("chain1VelocityThreshold", 1);
    parameters->getParameterAsValue(PARAM_CHAIN2_VELOCITY_THRESHOLD) = state.getProperty("chain2VelocityThreshold", 1);
    parameters->getParameterAsValue(PARAM_CHAIN1_PITCH_SHIFT) = state.getProperty("chain1PitchShift", 0.0f);
    parameters->getParameterAsValue(PARAM_CHAIN2_PITCH_SHIFT) = state.getProperty("chain2PitchShift", 0.0f);
}

// Parameter change handling
void DualChainSampleTriggerProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // Update the chain manager based on parameter ID
    if (parameterID == PARAM_BLEND)
    {
        chainManager->setBlendValue(newValue);
        state.setProperty("blend", newValue, nullptr); // Still update old state for backward compatibility if needed
    }
    else if (parameterID == PARAM_MAIN_VOLUME)
    {
        chainManager->setMainVolume(newValue);
        state.setProperty("mainVolume", newValue, nullptr);
    }
    else if (parameterID == PARAM_CHAIN1_VOLUME)
    {
        chainManager->setChainVolume(0, newValue);
        state.setProperty("chain1Volume", newValue, nullptr);
    }
    else if (parameterID == PARAM_CHAIN2_VOLUME)
    {
        chainManager->setChainVolume(1, newValue);
        state.setProperty("chain2Volume", newValue, nullptr);
    }
    else if (parameterID == PARAM_CHAIN1_NOTE)
    {
        chainManager->setTriggerNote(0, static_cast<int>(newValue));
        state.setProperty("chain1Note", static_cast<int>(newValue), nullptr);
    }
    else if (parameterID == PARAM_CHAIN2_NOTE)
    {
        chainManager->setTriggerNote(1, static_cast<int>(newValue));
        state.setProperty("chain2Note", static_cast<int>(newValue), nullptr);
    }
    else if (parameterID == PARAM_CHAIN1_VELOCITY_SENSITIVE)
    {
        SampleManager* sampleManager = chainManager->getSampleManager(0);
        if (sampleManager != nullptr)
        {
            sampleManager->setVelocitySensitive(newValue >= 0.5f);
        }
        state.setProperty("chain1VelocitySensitive", newValue >= 0.5f, nullptr);
    }
    else if (parameterID == PARAM_CHAIN2_VELOCITY_SENSITIVE)
    {
        SampleManager* sampleManager = chainManager->getSampleManager(1);
        if (sampleManager != nullptr)
        {
            sampleManager->setVelocitySensitive(newValue >= 0.5f);
        }
        state.setProperty("chain2VelocitySensitive", newValue >= 0.5f, nullptr);
    }
    else if (parameterID == PARAM_CHAIN1_VELOCITY_THRESHOLD)
    {
        SampleManager* sampleManager = chainManager->getSampleManager(0);
        if (sampleManager != nullptr)
        {
            sampleManager->setVelocityThreshold(static_cast<int>(newValue));
            DBG("[PARAM] Chain 1 velocity threshold set to: " << static_cast<int>(newValue));
        }
        state.setProperty("chain1VelocityThreshold", static_cast<int>(newValue), nullptr);
    }
    else if (parameterID == PARAM_CHAIN2_VELOCITY_THRESHOLD)
    {
        SampleManager* sampleManager = chainManager->getSampleManager(1);
        if (sampleManager != nullptr)
        {
            sampleManager->setVelocityThreshold(static_cast<int>(newValue));
            DBG("[PARAM] Chain 2 velocity threshold set to: " << static_cast<int>(newValue));
        }
        state.setProperty("chain2VelocityThreshold", static_cast<int>(newValue), nullptr);
    }
    else if (parameterID == PARAM_CHAIN1_PITCH_SHIFT)
    {
        SampleManager* sampleManager = chainManager->getSampleManager(0);
        if (sampleManager != nullptr)
        {
            sampleManager->setPitchShift(newValue);
        }
        state.setProperty("chain1PitchShift", newValue, nullptr);
    }
    else if (parameterID == PARAM_CHAIN2_PITCH_SHIFT)
    {
        SampleManager* sampleManager = chainManager->getSampleManager(1);
        if (sampleManager != nullptr)
        {
            sampleManager->setPitchShift(newValue);
        }
        state.setProperty("chain2PitchShift", newValue, nullptr);
    }
    // Remove handling for APVTS title parameters
    // else if (parameterID == PARAM_SESSION_TITLE) { ... }
    // else if (parameterID == PARAM_CHAIN1_TITLE) { ... }
    // else if (parameterID == PARAM_CHAIN2_TITLE) { ... }
}

//==============================================================================
// Renamed XML helper (This is the one to keep for XML logic)
void DualChainSampleTriggerProcessor::getCurrentStateAsXml(juce::XmlElement& xml)
{
    // Ensure the root element is named appropriately (this method receives the root)
    // We assume xml is already the correct root element, e.g., "DualChainSampleTriggerState"
    // So, we just add attributes and children to it.

    // Save main plugin parameters (using raw values from APVTS for now)
    if (auto* blendParam = parameters->getRawParameterValue(PARAM_BLEND))
        xml.setAttribute("blend", blendParam->load());
    if (auto* mainVolParam = parameters->getRawParameterValue(PARAM_MAIN_VOLUME))
        xml.setAttribute("mainVolume", mainVolParam->load());

    // Save titles using the string member variables
    xml.setAttribute("sessionTitle", currentSessionTitle);
    xml.setAttribute("chain1Title", currentChain1Title);
    xml.setAttribute("chain2Title", currentChain2Title);

    // Save chain-specific parameters
    for (int i = 0; i < 2; ++i)
    {
        juce::XmlElement* chainXml = new juce::XmlElement("Chain" + juce::String(i + 1));
        
        // Trigger Note
        if (auto* noteParam = parameters->getRawParameterValue(i == 0 ? PARAM_CHAIN1_NOTE : PARAM_CHAIN2_NOTE))
            chainXml->setAttribute("triggerNote", (int)noteParam->load());
        
        // Chain Volume
        if (chainManager) 
            chainXml->setAttribute("volume", chainManager->getChainVolume(i));

        // Pitch Shift, Velocity Sensitivity, Velocity Threshold from SampleManager
        if (chainManager && chainManager->getSampleManager(i))
        {
            SampleManager* sm = chainManager->getSampleManager(i);
            chainXml->setAttribute("pitchShift", sm->getPitchShift());
            chainXml->setAttribute("velocitySensitive", sm->isVelocitySensitive());
            chainXml->setAttribute("velocityThreshold", sm->getVelocityThreshold());

            // Save sample paths
            juce::XmlElement* samplesXml = new juce::XmlElement("Samples");
            for (int j = 0; j < sm->getNumSamples(); ++j)
            {
                if (auto* sample = sm->getSample(j))
                {
                    juce::XmlElement* sampleXml = new juce::XmlElement("Sample");
                    sampleXml->setAttribute("path", sample->getFilePath());
                    samplesXml->addChildElement(sampleXml);
                }
            }
            chainXml->addChildElement(samplesXml);
        }
        xml.addChildElement(chainXml);
    }
}

void DualChainSampleTriggerProcessor::saveStateToXml(const juce::File& outputFile)
{
    std::unique_ptr<juce::XmlElement> rootXml = std::make_unique<juce::XmlElement>("DualChainSampleTriggerState");
    getCurrentStateAsXml(*rootXml); // Populate the XML structure

    if (rootXml->getNumAttributes() > 0 || rootXml->getNumChildElements() > 0) // Check if it's not empty
    {
        if (!rootXml->writeTo(outputFile))
        {
            DBG("Failed to write XML state to file: " + outputFile.getFullPathName());
        }
    }
    else
    {
        DBG("XML state is empty, not writing to file.");
    }
}

void DualChainSampleTriggerProcessor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                                        const juce::Identifier& property)
{
    // Only update if the property is one we care about
    juce::Identifier treeID = treeWhosePropertyHasChanged.getType();
    juce::Identifier expectedID("DualChainSampleTrigger");
    
    if (treeID == expectedID)
    {
        // Check if this is a property we need to update parameters from
        juce::String propName = property.toString();
        if (propName == "blend" || propName == "mainVolume" ||
            propName == "chain1Volume" || propName == "chain2Volume" ||
            propName == "chain1Note" || propName == "chain2Note" ||
            propName == "chain1VelocitySensitive" || propName == "chain2VelocitySensitive" ||
            propName == "chain1VelocityThreshold" || propName == "chain2VelocityThreshold" ||
            propName == "chain1PitchShift" || propName == "chain2PitchShift")
        {
            updateParametersFromState();
        }
    }
}

juce::RangedAudioParameter* DualChainSampleTriggerProcessor::getParameterById(const juce::String& paramId)
{
    return parameters->getParameter(paramId);
}

bool DualChainSampleTriggerProcessor::loadSamplesIntoChain(int chainIndex, const juce::Array<juce::File>& files)
{
    if (chainIndex < 0 || chainIndex > 1)
    {
        return false;
    }
    
    SampleManager* sampleManager = chainManager->getSampleManager(chainIndex);
    
    if (sampleManager == nullptr)
    {
        return false;
    }
    
    bool success = false;
    
    for (const auto& file : files)
    {
        if (sampleManager->addSample(file))
        {
            success = true;
        }
    }
    
    return success;
}

void DualChainSampleTriggerProcessor::clearSamplesFromChain(int chainIndex)
{
    if (chainIndex < 0 || chainIndex > 1)
    {
        return;
    }
    
    SampleManager* sampleManager = chainManager->getSampleManager(chainIndex);
    
    if (sampleManager != nullptr)
    {
        sampleManager->clearAllSamples();
    }
}

int DualChainSampleTriggerProcessor::getNumSamplesInChain(int chainIndex)
{
    if (chainIndex < 0 || chainIndex > 1)
    {
        return 0;
    }
    
    SampleManager* sampleManager = chainManager->getSampleManager(chainIndex);
    
    if (sampleManager == nullptr)
    {
        return 0;
    }
    
    return sampleManager->getNumSamples();
}

juce::String DualChainSampleTriggerProcessor::getSampleFilename(int chainIndex, int sampleIndex)
{
    if (chainIndex < 0 || chainIndex > 1)
    {
        return {};
    }
    
    SampleManager* sampleManager = chainManager->getSampleManager(chainIndex);
    
    if (sampleManager == nullptr)
    {
        return {};
    }
    
    AudioSample* sample = sampleManager->getSample(sampleIndex);
    
    if (sample == nullptr)
    {
        return {};
    }
    
    return sample->getFileName();
}

//==============================================================================
void DualChainSampleTriggerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare the chain manager
    chainManager->prepareToPlay(sampleRate, samplesPerBlock);
}

void DualChainSampleTriggerProcessor::releaseResources()
{
    // Release resources in the chain manager
    chainManager->releaseResources();
}

bool DualChainSampleTriggerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // We only support stereo or mono in and stereo out
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo() &&
        layouts.getMainInputChannelSet() != juce::AudioChannelSet::mono())
        return false;
    
    return true;
}

void DualChainSampleTriggerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Debug MIDI reception at processor level
    if (!midiMessages.isEmpty())
    {
        DBG("[PROCESSOR] MIDI buffer has " << midiMessages.getNumEvents() << " events");
        for (const auto metadata : midiMessages)
        {
            const auto message = metadata.getMessage();
            if (message.isNoteOn())
            {
                DBG("[PROCESSOR] MIDI Note On: " << message.getNoteNumber() << ", Velocity: " << message.getVelocity());
            }
        }
    }
    
    // Check if chainManager exists
    if (chainManager == nullptr)
    {
        DBG("[PROCESSOR] ERROR: chainManager is NULL!");
    }
    else
    {
        DBG("[PROCESSOR] About to call chainManager->processMidiMessages()");
        // Process MIDI messages
        chainManager->processMidiMessages(midiMessages);
        DBG("[PROCESSOR] Returned from chainManager->processMidiMessages()");
    }
    
    // Process audio
    if (chainManager != nullptr)
    {
        chainManager->processBlock(buffer, buffer.getNumSamples());
    }
}

//==============================================================================
juce::AudioProcessorEditor* DualChainSampleTriggerProcessor::createEditor()
{
    return new DualChainSampleTriggerEditor(*this, *parameters);
}

bool DualChainSampleTriggerProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
const juce::String DualChainSampleTriggerProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DualChainSampleTriggerProcessor::acceptsMidi() const
{
    return true;
}

bool DualChainSampleTriggerProcessor::producesMidi() const
{
    return false;
}

bool DualChainSampleTriggerProcessor::isMidiEffect() const
{
    return false;
}

double DualChainSampleTriggerProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================
int DualChainSampleTriggerProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DualChainSampleTriggerProcessor::getCurrentProgram()
{
    return 0;
}

void DualChainSampleTriggerProcessor::setCurrentProgram(int index)
{
    // This plugin doesn't use programs
}

const juce::String DualChainSampleTriggerProcessor::getProgramName(int index)
{
    return {};
}

void DualChainSampleTriggerProcessor::changeProgramName(int index, const juce::String& newName)
{
    // This plugin doesn't use programs
}

//==============================================================================
// Removed duplicate getStateInformation(juce::MemoryBlock& destData)
// The primary one (that calls getCurrentStateAsXml) is kept.

void DualChainSampleTriggerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes)); // Assumes getXmlFromBinary is a utility in JUCE or defined elsewhere

    if (xmlState != nullptr)
    {
        // Extract the chain manager state
        auto* chainManagerXml = xmlState->getChildByName("CHAINMANAGER"); // User specified "CHAINMANAGER"

        if (chainManagerXml != nullptr)
        {
            // Load the chain manager state
            // This assumes chainManager has a 'restoreFromXml' method.
            if (chainManager) // Ensure chainManager itself is not null
                chainManager->restoreFromXml(*chainManagerXml); 

            // Remove the chain manager XML from the state XML
            xmlState->removeChildElement(chainManagerXml, true); // true to delete the element
        }

        // Restore the parameters from the remaining XML
        // This assumes 'parameters' is the AudioProcessorValueTreeState instance
        if (parameters) // Ensure parameters is not null
            parameters->replaceState(juce::ValueTree::fromXml(*xmlState));

        initializeState(); 
    }
}

// Removed old setStateInformation(const juce::XmlElement& xml) definition.
// Its logic is now in restoreStateFromXml.

void DualChainSampleTriggerProcessor::loadStateFromXml(const juce::File& inputFile)
{
    std::unique_ptr<juce::XmlElement> xmlState = juce::parseXML(inputFile);
    if (xmlState == nullptr)
    {
        DBG("Failed to parse XML from file: " + inputFile.getFullPathName());
        return;
    }
    setStateInformation(*xmlState);
    
    // It's good practice to ensure all parts of the processor are updated.
    // Calling initializeState() or a similar method might be useful if parameterChanged callbacks
    // don't cover all aspects updated by setStateInformation.
    // For now, assuming parameterChanged and subsequent editor updates are sufficient.
}

// Renamed XML helper (This is the one to keep for XML logic)
void DualChainSampleTriggerProcessor::restoreStateFromXml(const juce::XmlElement& xml)
{
    if (!xml.hasTagName("DualChainSampleTriggerState"))
    {
        DBG("XML root tag name mismatch on load.");
        return; 
    }

    if (xml.hasAttribute("blend"))
        parameters->getParameterAsValue(PARAM_BLEND) = xml.getDoubleAttribute("blend");
    if (xml.hasAttribute("mainVolume"))
        parameters->getParameterAsValue(PARAM_MAIN_VOLUME) = xml.getDoubleAttribute("mainVolume");

    // Load titles into string member variables
    currentSessionTitle = xml.getStringAttribute("sessionTitle", "sample keyboard");
    currentChain1Title = xml.getStringAttribute("chain1Title", "Chain 1");
    currentChain2Title = xml.getStringAttribute("chain2Title", "Chain 2");
    
    int chainIdx = 0;
    forEachXmlChildElement(xml, chainXml)
    {
        if (chainXml->hasTagName("Chain1") || chainXml->hasTagName("Chain2"))
        {
            int currentProcessingChainIndex = chainXml->hasTagName("Chain1") ? 0 : 1;

            juce::String noteParamName = currentProcessingChainIndex == 0 ? PARAM_CHAIN1_NOTE : PARAM_CHAIN2_NOTE;
            if (parameters->getRawParameterValue(noteParamName) != nullptr)
            {
                 auto noteValue = chainXml->getIntAttribute("triggerNote", currentProcessingChainIndex == 0 ? 60 : 62);
                 parameters->getParameterAsValue(noteParamName) = noteValue;
            }

            if (chainManager)
            {
                chainManager->setChainVolume(currentProcessingChainIndex, (float)chainXml->getDoubleAttribute("volume", 1.0));
                
                SampleManager* sm = chainManager->getSampleManager(currentProcessingChainIndex);
                if (sm)
                {
                    sm->setPitchShift((float)chainXml->getDoubleAttribute("pitchShift", 0.0));
                    sm->setVelocitySensitive(chainXml->getBoolAttribute("velocitySensitive", true));
                    sm->setVelocityThreshold(chainXml->getIntAttribute("velocityThreshold", 1));

                    sm->clearAllSamples(); 
                    if (auto* samplesXmlElement = chainXml->getChildByName("Samples"))
                    {
                        forEachXmlChildElement(*samplesXmlElement, sampleXml)
                        {
                            if (sampleXml->hasTagName("Sample"))
                            {
                                juce::File sampleFile(sampleXml->getStringAttribute("path"));
                                if (sampleFile.existsAsFile())
                                {
                                    sm->addSample(sampleFile);
                                }
                                else
                                {
                                    DBG("Sample file not found on load: " + sampleFile.getFullPathName());
                                }
                            }
                        }
                    }
                }
            }
            chainIdx++; // This was not used, but kept from plan. Could be removed.
        }
    }
    // Parameter changes should trigger parameterChanged and update ChainManager/SampleManager instances.
    // The editor will manually update its components after this.
}

void DualChainSampleTriggerProcessor::loadStateFromXml(const juce::File& inputFile)
{
    juce::XmlDocument xmlDoc(inputFile);
    std::unique_ptr<juce::XmlElement> xmlState = xmlDoc.getDocumentElement();

    if (xmlState == nullptr)
    {
        DBG("Failed to parse XML from file: " + inputFile.getFullPathName() + " - Error: " + xmlDoc.getLastParseError());
        return;
    }
    if (!xmlDoc.getLastParseError().isEmpty())
    {
        DBG("XML parsing error for file: " + inputFile.getFullPathName() + " - Error: " + xmlDoc.getLastParseError());
        // Optionally return or proceed with caution if xmlState is not null but errors occurred
    }

    // Only proceed if xmlState is valid (it might be non-null even with minor parse errors)
    // For robustness, ensure it has the expected tag if we proceed.
    // The restoreStateFromXml method already checks the tag name.
    restoreStateFromXml(*xmlState); // Use renamed helper
    
    // It's good practice to ensure all parts of the processor are updated.
    // Calling initializeState() or a similar method might be useful if parameterChanged callbacks
    // don't cover all aspects updated by setStateInformation.
    // For now, assuming parameterChanged and subsequent editor updates are sufficient.
}

void DualChainSampleTriggerProcessor::resetToDefaultState()
{
    // Reset APVTS parameters to their known hardcoded defaults
    // Using operator-> on std::unique_ptr to access AudioProcessorValueTreeState members
    // And then assigning directly to the juce::var returned by getParameterAsValue()

    parameters->getParameterAsValue(PARAM_BLEND) = 0.5f;
    parameters->getParameterAsValue(PARAM_MAIN_VOLUME) = 1.0f;
    parameters->getParameterAsValue(PARAM_CHAIN1_VOLUME) = 1.0f;
    parameters->getParameterAsValue(PARAM_CHAIN2_VOLUME) = 1.0f;
    parameters->getParameterAsValue(PARAM_CHAIN1_NOTE) = 60.0f; // juce::var handles float to int if underlying is int
    parameters->getParameterAsValue(PARAM_CHAIN2_NOTE) = 62.0f; // juce::var handles float to int
    parameters->getParameterAsValue(PARAM_CHAIN1_VELOCITY_SENSITIVE) = true;
    parameters->getParameterAsValue(PARAM_CHAIN2_VELOCITY_SENSITIVE) = true;
    parameters->getParameterAsValue(PARAM_CHAIN1_VELOCITY_THRESHOLD) = 1.0f; // juce::var handles float to int
    parameters->getParameterAsValue(PARAM_CHAIN2_VELOCITY_THRESHOLD) = 1.0f; // juce::var handles float to int
    parameters->getParameterAsValue(PARAM_CHAIN1_PITCH_SHIFT) = 0.0f;
    parameters->getParameterAsValue(PARAM_CHAIN2_PITCH_SHIFT) = 0.0f;
    
    // Note: The DBG messages for "param not found" are removed as getParameterAsValue()
    // would return a void juce::var if the parameter doesn't exist, and assigning to it
    // would be a benign operation (or potentially an error depending on JUCE version,
    // but parameters should exist if defined in createParameters).
    // If a parameter ID is incorrect, it's better to catch that during development.

    // Reset titles (temporary processor variables)
    currentSessionTitle = "sample keyboard"; 
    currentChain1Title = "Chain 1";
    currentChain2Title = "Chain 2";

    // Reset chain-specific settings in ChainManager and SampleManager
    if (chainManager)
    {
        for (int i = 0; i < 2; ++i)
        {
            // APVTS parameters (like trigger notes, chain volumes if they become APVTS)
            // are reset above. If chain volume is not APVTS, reset directly:
            // chainManager->setChainVolume(i, 1.0f); 
            // For now, assuming chain volumes are part of APVTS or their reset is covered
            // by APVTS parameter reset callbacks.

            SampleManager* sm = chainManager->getSampleManager(i);
            if (sm)
            {
                sm->setPitchShift(0.0f);        // Default pitch shift
                sm->setVelocitySensitive(true); // Default velocity sensitivity
                sm->setVelocityThreshold(1);    // Default velocity threshold
                sm->clearAllSamples();          // Clear all samples
            }
        }
    }
    // Parameter changes should trigger parameterChanged and update ChainManager/SampleManager instances.
    // The editor will manually update its components after this.
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DualChainSampleTriggerProcessor();
}

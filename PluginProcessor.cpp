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
        state.setProperty("blend", newValue, nullptr);
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
void DualChainSampleTriggerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save the parameters
    auto stateXml = parameters->copyState().createXml();
    
    // Save the chain manager state
    auto chainManagerXml = chainManager->saveToXml();
    stateXml->addChildElement(chainManagerXml.release());
    
    // Copy into the memory block
    copyXmlToBinary(*stateXml, destData);
}

void DualChainSampleTriggerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Parse the XML
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState != nullptr)
    {
        // Extract the chain manager state
        auto* chainManagerXml = xmlState->getChildByName("CHAINMANAGER");
        
        if (chainManagerXml != nullptr)
        {
            // Load the chain manager state
            chainManager->restoreFromXml(chainManagerXml);
            
            // Remove the chain manager XML from the state XML
            xmlState->removeChildElement(chainManagerXml, true);
        }
        
        // Restore the parameters
        parameters->replaceState(juce::ValueTree::fromXml(*xmlState));
        
        // Update the state
        initializeState();
    }
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DualChainSampleTriggerProcessor();
}

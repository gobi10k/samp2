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
      // activeTabIndex, currentSampleRate, currentBlockSize are initialized in PluginProcessor.h by default
{
    // Create parameters
    createParameters();

    // Initialize activeTabIndex and add a default tab
    activeTabIndex = 0;
    if (tabStates.empty()) {
        addNewTab("Default Tab");
    }
    // initializeState() is very simple, can be called.
    // Parameters will be applied to the new tab via parameterChanged if necessary,
    // or when UI is built/refreshed.
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
    // This function's logic is now primarily to ensure the global ValueTree 'state'
    // has any necessary top-level properties if not managed by APVTS state directly.
    // Most specific initializations will occur when a new tab is created or state is loaded.
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
    ChainManager* activeCM = getActiveChainManager();
    if (!activeCM) return;

    // Update the active chain manager based on parameter ID
    if (parameterID == PARAM_BLEND) { activeCM->setBlendValue(newValue); }
    else if (parameterID == PARAM_MAIN_VOLUME) { activeCM->setMainVolume(newValue); }
    else if (parameterID == PARAM_CHAIN1_VOLUME) { activeCM->setChainVolume(0, newValue); }
    else if (parameterID == PARAM_CHAIN2_VOLUME) { activeCM->setChainVolume(1, newValue); }
    else if (parameterID == PARAM_CHAIN1_NOTE) { activeCM->setTriggerNote(0, static_cast<int>(newValue)); }
    else if (parameterID == PARAM_CHAIN2_NOTE) { activeCM->setTriggerNote(1, static_cast<int>(newValue)); }
    else if (parameterID == PARAM_CHAIN1_VELOCITY_SENSITIVE) { if (SampleManager* sm = activeCM->getSampleManager(0)) sm->setVelocitySensitive(newValue >= 0.5f); }
    else if (parameterID == PARAM_CHAIN2_VELOCITY_SENSITIVE) { if (SampleManager* sm = activeCM->getSampleManager(1)) sm->setVelocitySensitive(newValue >= 0.5f); }
    else if (parameterID == PARAM_CHAIN1_VELOCITY_THRESHOLD) { if (SampleManager* sm = activeCM->getSampleManager(0)) sm->setVelocityThreshold(static_cast<int>(newValue)); }
    else if (parameterID == PARAM_CHAIN2_VELOCITY_THRESHOLD) { if (SampleManager* sm = activeCM->getSampleManager(1)) sm->setVelocityThreshold(static_cast<int>(newValue)); }
    else if (parameterID == PARAM_CHAIN1_PITCH_SHIFT) { if (SampleManager* sm = activeCM->getSampleManager(0)) sm->setPitchShift(newValue); }
    else if (parameterID == PARAM_CHAIN2_PITCH_SHIFT) { if (SampleManager* sm = activeCM->getSampleManager(1)) sm->setPitchShift(newValue); }

    // The state.setProperty calls are removed as APVTS handles ValueTree updates,
    // and custom XML saving will now iterate through tabs.
}

//==============================================================================
// Tab Management Methods (Implementation - getActiveTabState, getTabState, getActiveChainManager)
// These are fundamental helpers for the tab system.
// Other tab methods (addNewTab, removeTab, etc.) will be implemented in a subsequent step.
//==============================================================================

TabState* DualChainSampleTriggerProcessor::getActiveTabState() {
    if (activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabStates.size())) {
        return tabStates[static_cast<size_t>(activeTabIndex)].get();
    }
    return nullptr;
}

TabState* DualChainSampleTriggerProcessor::getTabState(int tabIndex) {
   if (tabIndex >= 0 && tabIndex < static_cast<int>(tabStates.size())) {
       return tabStates[static_cast<size_t>(tabIndex)].get();
   }
   return nullptr;
}

ChainManager* DualChainSampleTriggerProcessor::getActiveChainManager()
{
    TabState* activeState = getActiveTabState();
    if (activeState && activeState->chainManager) {
        return activeState->chainManager.get();
    }
    return nullptr;
}

void DualChainSampleTriggerProcessor::addNewTab(const juce::String& title) {
    tabStates.push_back(std::make_unique<TabState>(title));
    // If this is the very first tab, set it active.
    if (tabStates.size() == 1) {
        activeTabIndex = 0;
    }
    // If prepareToPlay has already been called (currentSampleRate and currentBlockSize are valid),
    // then prepare the new ChainManager.
    if (currentSampleRate > 0.001 && currentBlockSize > 0 && tabStates.back()->chainManager) {
       tabStates.back()->chainManager->prepareToPlay(currentSampleRate, currentBlockSize);
    }
    // TODO: Initialize new tab's parameters from current global APVTS settings if needed,
    // or ensure parameterChanged is robust enough if active tab switches to this new one.
}

void DualChainSampleTriggerProcessor::removeTab(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= static_cast<int>(tabStates.size())) return;
    // Optional: Prevent removing the last tab. For now, allowing it.
    // if (tabStates.size() == 1) { return; }

    bool removingActiveTab = (tabIndex == activeTabIndex);

    tabStates.erase(tabStates.begin() + tabIndex);

    if (tabStates.empty()) {
        // If all tabs are removed, potentially add a new default one or handle empty state.
        // For now, let's add a default tab back.
        addNewTab("Default Tab"); // This will set activeTabIndex = 0
        return;
    }

    // Adjust activeTabIndex if necessary
    if (removingActiveTab) {
        // If the active tab was removed, try to set the new active tab to the same index,
        // or the last tab if the index is now out of bounds.
        if (activeTabIndex >= static_cast<int>(tabStates.size())) {
            activeTabIndex = static_cast<int>(tabStates.size()) - 1;
        }
        // If activeTabIndex became -1 (e.g. if it was 0 and tab 0 was removed, and size became 0 temporarily)
        if (activeTabIndex < 0) { // Should be covered by tabStates.empty() case, but defensive
            activeTabIndex = 0;
        }
    } else if (tabIndex < activeTabIndex) {
        // If a tab before the active one was removed, decrement activeTabIndex.
        activeTabIndex--;
    }
    // No change to activeTabIndex if a tab after the active one is removed.
    // Ensure activeTabIndex is always valid if tabStates is not empty.
    if (activeTabIndex < 0 && !tabStates.empty()) { // Should not happen with above logic
        activeTabIndex = 0;
    } else if (activeTabIndex >= static_cast<int>(tabStates.size())) { // Clamp if somehow out of bounds
         activeTabIndex = static_cast<int>(tabStates.size()) - 1;
    }
}

void DualChainSampleTriggerProcessor::setActiveTab(int tabIndex) {
    if (tabIndex >= 0 && tabIndex < static_cast<int>(tabStates.size())) {
        if (activeTabIndex != tabIndex) {
            activeTabIndex = tabIndex;
            // TODO: Notify the editor that the active tab has changed so it can update.
            // This might involve triggering parameter updates for all parameters
            // to reflect the state of the new active tab's ChainManager.
        }
    }
}

int DualChainSampleTriggerProcessor::getNumTabs() const {
    return static_cast<int>(tabStates.size());
}

juce::String DualChainSampleTriggerProcessor::getTabTitle(int tabIndex) const {
    if (tabIndex >= 0 && tabIndex < static_cast<int>(tabStates.size()) && tabStates[static_cast<size_t>(tabIndex)]) {
        return tabStates[static_cast<size_t>(tabIndex)]->tabTitle;
    }
    return "Invalid Tab";
}

void DualChainSampleTriggerProcessor::setTabTitle(int tabIndex, const juce::String& newTitle) {
    if (tabIndex >= 0 && tabIndex < static_cast<int>(tabStates.size()) && tabStates[static_cast<size_t>(tabIndex)]) {
        tabStates[static_cast<size_t>(tabIndex)]->tabTitle = newTitle;
    }
}

void DualChainSampleTriggerProcessor::setChainTitleForActiveTab(int chainIdx, const juce::String& newTitle) {
    if (TabState* activeState = getActiveTabState()) {
        if (chainIdx == 0) activeState->chain1Title = newTitle;
        else if (chainIdx == 1) activeState->chain2Title = newTitle;
    }
}

juce::String DualChainSampleTriggerProcessor::getChainTitleForActiveTab(int chainIdx) const {
    // To be const correct, ensure getActiveTabState could be const, or access tabStates directly.
    if (activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabStates.size())) {
        const TabState* activeState = tabStates[static_cast<size_t>(activeTabIndex)].get(); // Use .get() on unique_ptr
        if (activeState) {
             if (chainIdx == 0) return activeState->chain1Title;
             else if (chainIdx == 1) return activeState->chain2Title;
        }
    }
    return "N/A";
}

juce::String DualChainSampleTriggerProcessor::getSessionTitleForActiveTab() const {
    if (activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabStates.size())) {
        const TabState* activeState = tabStates[static_cast<size_t>(activeTabIndex)].get();
        if (activeState) { return activeState->tabTitle; }
    }
    return "N/A";
}

void DualChainSampleTriggerProcessor::setSessionTitleForActiveTab(const juce::String& title) {
    if (TabState* activeState = getActiveTabState()) {
        activeState->tabTitle = title;
    }
}

//==============================================================================
// Renamed XML helper (This is the one to keep for XML logic)
void DualChainSampleTriggerProcessor::getCurrentStateAsXml(juce::XmlElement& xml)
{
    // This custom XML is primarily for our tab structure and their specific states.
    // Global parameters like blend/main volume are part of APVTS and saved by the host.
    // If there were other non-APVTS global settings, they would be saved here.

    xml.setAttribute("activeTabIndex", activeTabIndex);
    auto* tabsXmlElement = xml.createNewChildElement("TABS");
    for (const auto& tabState : tabStates) {
        if (tabState) {
            auto* tabStateXmlElement = tabsXmlElement->createNewChildElement("TAB_STATE");
            tabState->saveStateToXmlElement(*tabStateXmlElement);
        }
    }
}

void DualChainSampleTriggerProcessor::saveStateToXml(const juce::File& outputFile)
{
    std::unique_ptr<juce::XmlElement> rootXml = std::make_unique<juce::XmlElement>("DualChainSampleTriggerSession");
    getCurrentStateAsXml(*rootXml);

    if (rootXml->getNumAttributes() > 0 || rootXml->getNumChildElements() > 0)
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
    this->currentSampleRate = sampleRate;
    this->currentBlockSize = samplesPerBlock;
    for (auto& tabState : tabStates) {
        if (tabState && tabState->chainManager) {
            tabState->chainManager->prepareToPlay(sampleRate, samplesPerBlock);
        }
    }
}

void DualChainSampleTriggerProcessor::releaseResources()
{
    for (auto& tabState : tabStates) {
        if (tabState && tabState->chainManager) {
            tabState->chainManager->releaseResources();
        }
    }
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
    
    ChainManager* activeCM = getActiveChainManager();
    if (activeCM != nullptr)
    {
        // Debug MIDI reception (optional)
        // if (!midiMessages.isEmpty()) {
        //     DBG("[PROCESSOR] MIDI for active tab " << activeTabIndex << ": " << midiMessages.getNumEvents() << " events");
        // }
        activeCM->processMidiMessages(midiMessages);
        activeCM->processBlock(buffer, buffer.getNumSamples());
    }
    else
    {
        // If no active chain manager, clear the buffer to avoid outputting garbage
        buffer.clear();
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

void DualChainSampleTriggerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Create an XML element to hold the state
    std::unique_ptr<juce::XmlElement> xml(new juce::XmlElement("DualChainSampleTriggerState"));

    // std::unique_ptr<juce::XmlElement> xml(new juce::XmlElement("DualChainSampleTriggerState")); // Old root name
    std::unique_ptr<juce::XmlElement> xml(new juce::XmlElement("DualChainSampleTriggerSession")); // New root name for session
    getCurrentStateAsXml(*xml); // This now saves tabs and global params if any
    copyXmlToBinary(*xml, destData);
}

void DualChainSampleTriggerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr) {
        // Allow loading from old root tag "DualChainSampleTriggerState" for backward compatibility
        if (xmlState->hasTagName("DualChainSampleTriggerSession") || xmlState->hasTagName("DualChainSampleTriggerState")) {
            restoreStateFromXml(*xmlState);
            // APVTS parameters are typically restored by the host when it calls setStateInformation.
            // If your custom XML also contains state that should affect APVTS parameters
            // (e.g., global settings not tied to a specific tab's ChainManager),
            // you might need to update them here or ensure restoreStateFromXml does.
            // For now, assuming APVTS parameters for global settings are handled by host state restoration
            // and parameters for tab-specific things are handled within TabState/ChainManager restoration.
        }
    }
}

// Removed old setStateInformation(const juce::XmlElement& xml) definition.
// Its logic is now in restoreStateFromXml.

// The older loadStateFromXml definition (which called setStateInformation(XmlElement&)) is removed.
// The correct one using XmlDocument and calling restoreStateFromXml is kept (defined later in the file).

// Renamed XML helper (This is the one to keep for XML logic)
void DualChainSampleTriggerProcessor::restoreStateFromXml(const juce::XmlElement& xml)
{
    // Check for old root tag "DualChainSampleTriggerState" or new "DualChainSampleTriggerSession"
    if (!xml.hasTagName("DualChainSampleTriggerSession") && !xml.hasTagName("DualChainSampleTriggerState"))
    {
        DBG("XML root tag name mismatch on load. Expected DualChainSampleTriggerSession or DualChainSampleTriggerState.");
        return; 
    }

    // Global parameters not part of APVTS would be restored here if saved in getCurrentStateAsXml.
    // currentSessionTitle, etc. are now part of TabState.

    tabStates.clear();
    activeTabIndex = xml.getIntAttribute("activeTabIndex", 0);
    
    if (auto* tabsXmlElement = xml.getChildByName("TABS")) {
        for (auto* tabStateXmlElement : tabsXmlElement->findAllSubElementsWithTagName("TAB_STATE")) {
            // Create a new TabState. Its constructor gives it a default ChainManager.
            auto newTabState = std::make_unique<TabState>("Loading..."); // Temp title, will be overwritten by loadStateFromXmlElement
            newTabState->loadStateFromXmlElement(*tabStateXmlElement);   // Populate TabState and its ChainManager
            tabStates.push_back(std::move(newTabState));
        }
    }

    // Ensure there's at least one tab and activeTabIndex is valid.
    if (tabStates.empty()) {
        addNewTab("Default Tab"); // Creates a tab and sets activeTabIndex = 0 by addNewTab logic.
    } else {
        if (activeTabIndex < 0 || activeTabIndex >= static_cast<int>(tabStates.size())) {
            activeTabIndex = 0;
        }
    }

    // Prepare ChainManagers for all loaded/created tabs if prepareToPlay has already run.
    if (currentSampleRate > 0.001 && currentBlockSize > 0) {
        for (auto& tabState : tabStates) {
            if (tabState && tabState->chainManager) {
                tabState->chainManager->prepareToPlay(currentSampleRate, currentBlockSize);
            }
        }
    }

    // After loading all tabs and determining the active tab,
    // update the global APVTS parameters to reflect the state of this active tab.
    // This is crucial for UI elements that are bound to these global parameters.
    if (TabState* activeTab = getActiveTabState()) {
        if (ChainManager* activeCM = activeTab->chainManager.get()) {
            // Use setValueNotifyingHost to ensure UI and host are updated if parameters changed.
            parameters->getParameterAsValue(PARAM_BLEND).setValueNotifyingHost(activeCM->getBlendValue());
            parameters->getParameterAsValue(PARAM_MAIN_VOLUME).setValueNotifyingHost(activeCM->getMainVolume());
            parameters->getParameterAsValue(PARAM_CHAIN1_VOLUME).setValueNotifyingHost(activeCM->getChainVolume(0));
            parameters->getParameterAsValue(PARAM_CHAIN2_VOLUME).setValueNotifyingHost(activeCM->getChainVolume(1));
            parameters->getParameterAsValue(PARAM_CHAIN1_NOTE).setValueNotifyingHost(activeCM->getTriggerNote(0));
            parameters->getParameterAsValue(PARAM_CHAIN2_NOTE).setValueNotifyingHost(activeCM->getTriggerNote(1));

            if (SampleManager* sm0 = activeCM->getSampleManager(0)) {
                parameters->getParameterAsValue(PARAM_CHAIN1_VELOCITY_SENSITIVE).setValueNotifyingHost(sm0->isVelocitySensitive());
                parameters->getParameterAsValue(PARAM_CHAIN1_VELOCITY_THRESHOLD).setValueNotifyingHost(sm0->getVelocityThreshold());
                parameters->getParameterAsValue(PARAM_CHAIN1_PITCH_SHIFT).setValueNotifyingHost(sm0->getPitchShift());
            }
            if (SampleManager* sm1 = activeCM->getSampleManager(1)) {
                parameters->getParameterAsValue(PARAM_CHAIN2_VELOCITY_SENSITIVE).setValueNotifyingHost(sm1->isVelocitySensitive());
                parameters->getParameterAsValue(PARAM_CHAIN2_VELOCITY_THRESHOLD).setValueNotifyingHost(sm1->getVelocityThreshold());
                parameters->getParameterAsValue(PARAM_CHAIN2_PITCH_SHIFT).setValueNotifyingHost(sm1->getPitchShift());
            }
        }
    }
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

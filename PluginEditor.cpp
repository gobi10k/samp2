#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DualChainSampleTriggerEditor::DualChainSampleTriggerEditor(DualChainSampleTriggerProcessor& p, juce::AudioProcessorValueTreeState& params)
    : AudioProcessorEditor(&p), audioProcessor(p), parameters(params), tabbedComponent(juce::TabbedButtonBar::TabsAtTop)
{
    // Create the look and feel
    lookAndFeel = std::make_unique<CustomLookAndFeel>();
    setLookAndFeel(lookAndFeel.get());

    // Session Title Editor (Global)
    sessionTitleEditor = std::make_unique<juce::TextEditor>("sessionTitleEditor");
    sessionTitleEditor->setFont(juce::Font(DualTriggerStyle::fontSizeHeader * 1.2f).boldened());
    sessionTitleEditor->setJustification(juce::Justification::centred);
    sessionTitleEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    sessionTitleEditor->setColour(juce::TextEditor::textColourId, DualTriggerStyle::textColour);
    sessionTitleEditor->addListener(this);
    addAndMakeVisible(sessionTitleEditor.get());
    // titleLabel might be redundant now, or used as a static label if sessionTitleEditor is hidden/shown.
    // For now, let's assume sessionTitleEditor is always visible for editing the active tab's name.
    titleLabel = std::make_unique<juce::Label>("titleLabel", ""); // Not making visible for now
    titleLabel->setVisible(false);


    // Tabbed Component
    addAndMakeVisible(tabbedComponent);
    tabbedComponent.setTabBarDepth(30); // Example depth
    tabbedComponent.addListener(this);

    // Global Buttons
    saveStateButton = std::make_unique<juce::TextButton>("Save Session");
    saveStateButton->addListener(this);
    addAndMakeVisible(saveStateButton.get());

    loadStateButton = std::make_unique<juce::TextButton>("Load Session");
    loadStateButton->addListener(this);
    addAndMakeVisible(loadStateButton.get());

    resetStateButton = std::make_unique<juce::TextButton>("Reset Session");
    resetStateButton->addListener(this);
    addAndMakeVisible(resetStateButton.get());

    addNewTabButton = std::make_unique<juce::TextButton>("+ Add Tab");
    addNewTabButton->addListener(this);
    addAndMakeVisible(addNewTabButton.get());

    saveActiveTabButton = std::make_unique<juce::TextButton>("Save Active Tab");
    saveActiveTabButton->addListener(this);
    addAndMakeVisible(saveActiveTabButton.get());

    loadActiveTabButton = std::make_unique<juce::TextButton>("Load to Active Tab");
    loadActiveTabButton->addListener(this);
    addAndMakeVisible(loadActiveTabButton.get());

    loadTabAsNewButton = std::make_unique<juce::TextButton>("Load as New Tab");
    loadTabAsNewButton->addListener(this);
    addAndMakeVisible(loadTabAsNewButton.get());

    // Populate tabs from processor state
    buildTabsFromProcessorState();

    // Set initial component values (global ones)
    updateUI(); // This will update sessionTitleEditor among other things

    // Start the timer
    startTimerHz(30); // 30 fps
    
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(800, 600);
    
    // Make the editor resizable
    setResizable(true, true);
    setResizeLimits(800, 600, 1200, 800);
}

DualChainSampleTriggerEditor::~DualChainSampleTriggerEditor()
{
    stopTimer();
    tabbedComponent.removeListener(this); // Remove listener for tabbedComponent
    setLookAndFeel(nullptr);

    // Global buttons remove their own listeners implicitly if unique_ptr owns them.
    // No need to manually remove listeners for unique_ptrs that are about to be destroyed.
    // sessionTitleEditor->removeListener(this); // Not needed if this is the only listener and it's being destroyed
}

//==============================================================================
void DualChainSampleTriggerEditor::paint(juce::Graphics& g)
{
    g.fillAll(DualTriggerStyle::backgroundColour);
    // No border here, TabbedComponent will fill the area.
}

void DualChainSampleTriggerEditor::resized()
{
    const int padding = DualTriggerStyle::padding; // Use new padding
    const int topBarActualHeight = DualTriggerStyle::headerHeight; // Use new headerHeight for the controls in the top bar
    const int bottomBarActualHeight = DualTriggerStyle::controlHeight; // Use new controlHeight for bottom buttons
    
    juce::Rectangle<int> localBounds = getLocalBounds(); // Full editor area

    // Top bar area (session title, tab action buttons)
    juce::Rectangle<int> topBarArea = localBounds.removeFromTop(topBarActualHeight + 2 * padding); // Add padding above and below
    topBarArea.reduce(padding, padding); // Reduce horizontally for side padding

    int buttonClusterWidth = addNewTabButton->getWidth() + saveActiveTabButton->getWidth() + loadActiveTabButton->getWidth() + loadTabAsNewButton->getWidth() + (3 * padding);
    sessionTitleEditor->setBounds(topBarArea.removeFromLeft(topBarArea.getWidth() - buttonClusterWidth - padding));

    topBarArea.removeFromLeft(padding); // Space between title editor and first button
    addNewTabButton->setBounds(topBarArea.removeFromLeft(100));
    topBarArea.removeFromLeft(padding);
    saveActiveTabButton->setBounds(topBarArea.removeFromLeft(120));
    topBarArea.removeFromLeft(padding);
    loadActiveTabButton->setBounds(topBarArea.removeFromLeft(120));
    topBarArea.removeFromLeft(padding);
    loadTabAsNewButton->setBounds(topBarArea.removeFromLeft(120));


    // Bottom bar area (session action buttons)
    juce::Rectangle<int> bottomBarArea = localBounds.removeFromBottom(bottomBarActualHeight + padding);
    bottomBarArea.reduce(padding, padding / 2); // Horizontal padding, less vertical padding for bottom

    int globalButtonWidth = 100; // Assuming these are roughly this width
    resetStateButton->setBounds(bottomBarArea.removeFromRight(globalButtonWidth));
    bottomBarArea.removeFromRight(padding);
    loadStateButton->setBounds(bottomBarArea.removeFromRight(globalButtonWidth));
    bottomBarArea.removeFromRight(padding);
    saveStateButton->setBounds(bottomBarArea.removeFromRight(globalButtonWidth));
    // This right-to-left layout is simple. A FlexBox or manual calculation from left could also be used.

    // Tabbed component takes up the remaining middle area
    tabbedComponent.setBounds(localBounds); // localBounds is already reduced by top and bottom bars
}


void DualChainSampleTriggerEditor::buildTabsFromProcessorState() {
    tabbedComponent.clearTabs();
    // tabPages.clear(); // If using a vector of direct pointers
    for (int i = 0; i < audioProcessor.getNumTabs(); ++i) {
        TabContentComponent* tabPage = new TabContentComponent(audioProcessor, parameters, i);
        // tabPages.push_back(tabPage); // If using a vector
        tabbedComponent.addTab(audioProcessor.getTabTitle(i), DualTriggerStyle::backgroundColour.darker(0.2f), tabPage, true, i);
    }
    // Ensure the active tab in processor is reflected in UI
    if (audioProcessor.getNumTabs() > 0) {
        int processorActiveIndex = audioProcessor.activeTabIndex;
        if (processorActiveIndex < 0 || processorActiveIndex >= tabbedComponent.getNumTabs()) {
            processorActiveIndex = 0; // Fallback if index is invalid
            audioProcessor.setActiveTab(processorActiveIndex); // Correct processor state
        }
        tabbedComponent.setCurrentTabIndex(processorActiveIndex, false); // false = don't send change message
        sessionTitleEditor->setText(audioProcessor.getSessionTitleForActiveTab(), juce::dontSendNotification);
    } else {
        sessionTitleEditor->setText("No Tabs", juce::dontSendNotification);
    }
}

void DualChainSampleTriggerEditor::currentTabChanged(int newCurrentTabIndex, const juce::String& newCurrentTabName) {
    audioProcessor.setActiveTab(newCurrentTabIndex);
    sessionTitleEditor->setText(audioProcessor.getSessionTitleForActiveTab(), juce::dontSendNotification);
    
    // When tab changes, ensure the APVTS parameters reflect the state of the newly active tab's ChainManager
    // This is crucial for UI components (like ChainControlComponent) that use APVTS attachments
    // to correctly display and control the parameters of the active ChainManager.
    if (TabState* activeTabState = audioProcessor.getActiveTabState()) {
        if (ChainManager* activeCM = activeTabState->chainManager.get()) {
            // Manually update global parameters based on the new active ChainManager's state.
            // This ensures that UI elements attached to these global parameters reflect the active tab.
            parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_BLEND).setValueNotifyingHost(activeCM->getBlendValue());
            parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_MAIN_VOLUME).setValueNotifyingHost(activeCM->getMainVolume());
            parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN1_VOLUME).setValueNotifyingHost(activeCM->getChainVolume(0));
            parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN2_VOLUME).setValueNotifyingHost(activeCM->getChainVolume(1));
            parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN1_NOTE).setValueNotifyingHost(activeCM->getTriggerNote(0));
            parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN2_NOTE).setValueNotifyingHost(activeCM->getTriggerNote(1));

            if (SampleManager* sm0 = activeCM->getSampleManager(0)) {
                parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN1_VELOCITY_SENSITIVE).setValueNotifyingHost(sm0->isVelocitySensitive());
                parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN1_VELOCITY_THRESHOLD).setValueNotifyingHost(sm0->getVelocityThreshold());
                parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN1_PITCH_SHIFT).setValueNotifyingHost(sm0->getPitchShift());
            }
            if (SampleManager* sm1 = activeCM->getSampleManager(1)) {
                parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN2_VELOCITY_SENSITIVE).setValueNotifyingHost(sm1->isVelocitySensitive());
                parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN2_VELOCITY_THRESHOLD).setValueNotifyingHost(sm1->getVelocityThreshold());
                parameters.getParameterAsValue(DualChainSampleTriggerProcessor::PARAM_CHAIN2_PITCH_SHIFT).setValueNotifyingHost(sm1->getPitchShift());
            }
        }
    }

    if (auto* activeTabPage = dynamic_cast<TabContentComponent*>(tabbedComponent.getTabContentComponent(newCurrentTabIndex))) {
        activeTabPage->updateUIForTab();
    }
}


// void DualChainSampleTriggerEditor::sliderValueChanged(juce::Slider* slider)
// {
//     // This is now handled by TabContentComponent for its sliders,
//     // or directly by APVTS attachments for global sliders if any were outside tabs.
// }

void DualChainSampleTriggerEditor::timerCallback()
{
    // Check if the number of tabs in processor matches UI; rebuild if not.
    if (audioProcessor.getNumTabs() != tabbedComponent.getNumTabs()) {
       buildTabsFromProcessorState();
    }
    // Check if active tab title changed in processor (e.g. by host)
    juce::String procActiveTabTitle = audioProcessor.getSessionTitleForActiveTab();
    if (sessionTitleEditor->getText() != procActiveTabTitle) {
       sessionTitleEditor->setText(procActiveTabTitle, juce::dontSendNotification);
       if (audioProcessor.activeTabIndex >= 0 && audioProcessor.activeTabIndex < tabbedComponent.getNumTabs()) {
           tabbedComponent.setTabName(audioProcessor.activeTabIndex, procActiveTabTitle);
       }
    }

    // The active TabContentComponent's timer will handle its own updates.
    // No need to call updateUI() on it from here explicitly if its timer is running.
}

void DualChainSampleTriggerEditor::updateUI()
{
    // This method used to update all parts of the UI.
    // Now, most UI elements are in TabContentComponent, which has its own updateUIForTab.
    // This main updateUI should only handle global elements.
    sessionTitleEditor->setText(audioProcessor.getSessionTitleForActiveTab(), juce::dontSendNotification);
    if (audioProcessor.activeTabIndex >=0 && audioProcessor.activeTabIndex < tabbedComponent.getNumTabs()) {
         tabbedComponent.setTabName(audioProcessor.activeTabIndex, audioProcessor.getSessionTitleForActiveTab());
    }

    // If there's an active tab, maybe tell it to update too,
    // though its own timer should be doing this.
    // if (auto* activeTabPage = dynamic_cast<TabContentComponent*>(tabbedComponent.getCurrentTabComponent())) {
    //     activeTabPage->updateUIForTab();
    // }
}

// void DualChainSampleTriggerEditor::pianoNoteSelected(int chainIndex, int midiNoteNumber)
// {
//     // Moved to TabContentComponent
// }

void DualChainSampleTriggerEditor::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == sessionTitleEditor.get())
    {
        audioProcessor.setSessionTitleForActiveTab(sessionTitleEditor->getText());
        if (audioProcessor.activeTabIndex >= 0 && audioProcessor.activeTabIndex < tabbedComponent.getNumTabs()) {
            tabbedComponent.setTabName(audioProcessor.activeTabIndex, sessionTitleEditor->getText());
        }
    }
    // Chain title editors are within TabContentComponent
}

void DualChainSampleTriggerEditor::buttonClicked(juce::Button* button)
{
    if (button == saveStateButton.get())
    {
        // Session title is now handled by active tab logic, but ensure it's saved if processor holds it temporarily
        // audioProcessor.setSessionTitleForSaving(sessionTitleEditor->getText()); // Old way

        chooser = std::make_unique<juce::FileChooser>(
            "Save Session as XML",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.xml", // Ensure this matches what processor expects if it checks extension
            true,
            false,
            this);
        
        auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;

        chooser->launchAsync(flags, [this] (const juce::FileChooser& fc)
        {
            juce::File file = fc.getResult();
            if (file != juce::File{})
            {
                if (!file.hasFileExtension(".xml") && !file.hasFileExtension(".XML")) // Processor might do this too
                    file = file.withFileExtension(".xml");
                audioProcessor.saveStateToXml(file); // Processor now saves based on its tab structure
            }
        });
    }
    else if (button == loadStateButton.get())
    {
        chooser = std::make_unique<juce::FileChooser>(
            "Load Session from XML",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.xml",
            true,
            false,
            this);

        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(flags, [this] (const juce::FileChooser& fc)
        {
            juce::File file = fc.getResult();
            if (file != juce::File{})
            {
                audioProcessor.loadStateFromXml(file);
                buildTabsFromProcessorState(); // Rebuild UI based on new processor state
                updateUI(); // Update global UI elements like session title
            }
        }); 
    }
    else if (button == resetStateButton.get())
    {
        // Ask for confirmation before resetting
        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::WarningIcon,
            "Reset Session",
            "Are you sure you want to reset the entire session to its default state? This will clear all tabs and their content.",
            "Reset",
            "Cancel",
            nullptr,
            juce::ModalCallbackFunction::create([this](int result) {
                if (result == 1) // OK
                {
                    audioProcessor.resetToDefaultState();
                    buildTabsFromProcessorState(); // Rebuild UI based on new processor state
                    updateUI(); // Update global UI elements
                }
            }));
    }
    else if (button == addNewTabButton.get())
    {
        audioProcessor.addNewTab("New Tab " + juce::String(audioProcessor.getNumTabs() + 1));
        buildTabsFromProcessorState(); // Rebuild UI
        if (audioProcessor.getNumTabs() > 0) {
             tabbedComponent.setCurrentTabIndex(audioProcessor.getNumTabs() - 1, true); // Switch to the new tab and notify listeners
        }
    }
    else if (button == saveActiveTabButton.get()) {
        if (audioProcessor.activeTabIndex < 0) return;
        chooser = std::make_unique<juce::FileChooser>(
            "Save Active Tab State",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.tabstate;*.xml", true, false, this);
        
        auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;
        chooser->launchAsync(flags, [this] (const juce::FileChooser& fc) {
            juce::File file = fc.getResult();
            if (file != juce::File{}) {
                if (!file.hasFileExtension(".tabstate") && !file.hasFileExtension(".xml"))
                    file = file.withFileExtension(".tabstate");
                audioProcessor.saveSingleTabStateToFile(audioProcessor.activeTabIndex, file);
            }
        });
    }
    else if (button == loadActiveTabButton.get()) {
        if (audioProcessor.activeTabIndex < 0) return;
        chooser = std::make_unique<juce::FileChooser>(
            "Load State into Active Tab",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.tabstate;*.xml", true, false, this);

        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        chooser->launchAsync(flags, [this] (const juce::FileChooser& fc) {
            juce::File file = fc.getResult();
            if (file != juce::File{}) {
                audioProcessor.loadSingleTabStateFromFile(audioProcessor.activeTabIndex, file);
                if (auto* activeTabComp = dynamic_cast<TabContentComponent*>(tabbedComponent.getTabContentComponent(audioProcessor.activeTabIndex))) {
                    activeTabComp->updateUIForTab();
                }
                tabbedComponent.setTabName(audioProcessor.activeTabIndex, audioProcessor.getTabTitle(audioProcessor.activeTabIndex));
                sessionTitleEditor->setText(audioProcessor.getSessionTitleForActiveTab(), juce::dontSendNotification);
            }
        });
    }
    else if (button == loadTabAsNewButton.get()) {
        chooser = std::make_unique<juce::FileChooser>(
            "Load Tab State as New Tab",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.tabstate;*.xml", true, false, this);

        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        chooser->launchAsync(flags, [this] (const juce::FileChooser& fc) {
            juce::File file = fc.getResult();
            if (file != juce::File{}) {
                audioProcessor.loadTabAsNewFromFile(file);
                buildTabsFromProcessorState();
                if (audioProcessor.getNumTabs() > 0) {
                    tabbedComponent.setCurrentTabIndex(audioProcessor.getNumTabs() - 1, true);
                }
            }
        });
    }
}

// The parameterChanged method definition has been removed as it is not declared in PluginEditor.h
// and the class no longer inherits from juce::AudioProcessorValueTreeState::Listener.
// Listeners for individual controls are now primarily in TabContentComponent or handled by APVTS.

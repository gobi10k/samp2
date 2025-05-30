#include "PianoRollComponent.h"

PianoRollComponent::PianoRollComponent()
    : selectedNoteChain1_(-1),
      selectedNoteChain2_(-1),
      highlightColourChain1_(juce::Colours::blue),
      highlightColourChain2_(juce::Colours::orange),
      selectForChain1Button_("Set Ch1"),
      selectForChain2Button_("Set Ch2"),
      currentSelectionTarget_(0),
      whiteKeyWidth_(0.0f),
      blackKeyWidth_(0.0f)
{
    addAndMakeVisible(selectForChain1Button_);
    addAndMakeVisible(selectForChain2Button_);

    selectForChain1Button_.setRadioGroupId(1);
    selectForChain2Button_.setRadioGroupId(1);
    selectForChain1Button_.setToggleState(true, juce::dontSendNotification);

    selectForChain1Button_.addListener(this);
    selectForChain2Button_.addListener(this);
}

PianoRollComponent::~PianoRollComponent()
{
    selectForChain1Button_.removeListener(this);
    selectForChain2Button_.removeListener(this);
}

void PianoRollComponent::setInitialNotes(int noteChain1, int noteChain2)
{
    selectedNoteChain1_ = noteChain1;
    selectedNoteChain2_ = noteChain2;
    repaint();
}

void PianoRollComponent::setChainHighlightColour(int chainIndex, juce::Colour colour)
{
    if (chainIndex == 0)
        highlightColourChain1_ = colour;
    else if (chainIndex == 1)
        highlightColourChain2_ = colour;
    repaint();
}

void PianoRollComponent::addListener(Listener* l) { listeners_.add(l); }
void PianoRollComponent::removeListener(Listener* l) { listeners_.remove(l); }

void PianoRollComponent::buttonClicked(juce::Button* button)
{
    if (button == &selectForChain1Button_)
    {
        currentSelectionTarget_ = 0;
        DBG("PianoRoll: Selection target is Chain 1");
    }
    else if (button == &selectForChain2Button_)
    {
        currentSelectionTarget_ = 1;
        DBG("PianoRoll: Selection target is Chain 2");
    }
}

void PianoRollComponent::resized()
{
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.items.add(juce::FlexItem(selectForChain1Button_).withFlex(1));
    fb.items.add(juce::FlexItem(selectForChain2Button_).withFlex(1));
    
    juce::Rectangle<int> topArea = getLocalBounds().removeFromTop(30);
    fb.performLayout(topArea);

    // Calculate key widths based on the remaining area
    int numWhiteKeysInDisplay = 0;
    for (int i = 0; i < NUM_NOTES; ++i) {
        if (!isBlackKey(START_NOTE + i)) {
            numWhiteKeysInDisplay++;
        }
    }

    if (numWhiteKeysInDisplay > 0) {
        float pianoAreaWidth = (float)getWidth();
        whiteKeyWidth_ = pianoAreaWidth / numWhiteKeysInDisplay;
        blackKeyWidth_ = whiteKeyWidth_ * 0.6f;
    } else {
        whiteKeyWidth_ = 0;
        blackKeyWidth_ = 0;
    }
}

bool PianoRollComponent::isBlackKey(int midiNote) {
    int noteInOctave = midiNote % 12;
    return noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10;
}

// getKeyRectangle is not strictly needed if calculations are done directly in paint and mouse handling,
// but it was in the header, so providing a basic implementation.
// For accurate drawing, especially for black keys, direct calculation in paint might be better.
juce::Rectangle<float> PianoRollComponent::getKeyRectangle(int midiNote)
{
    if (midiNote < START_NOTE || midiNote >= START_NOTE + NUM_NOTES)
        return {};

    float pianoAreaY = (float)selectForChain1Button_.getBottom();
    float pianoHeight = (float)getHeight() - pianoAreaY;
    
    int whiteKeyIndex = 0; // Number of white keys before this current midiNote
    for (int i = START_NOTE; i < midiNote; ++i) {
        if (!isBlackKey(i)) {
            whiteKeyIndex++;
        }
    }

    if (!isBlackKey(midiNote)) // White key
    {
        return juce::Rectangle<float>(whiteKeyIndex * whiteKeyWidth_, pianoAreaY, whiteKeyWidth_, pianoHeight);
    }
    else // Black key
    {
        // Black keys are positioned relative to the white key immediately to their left (or the one they "emerge" from).
        // Their X position is the right edge of that white key, minus half a black key's width.
        float xPos = (whiteKeyIndex * whiteKeyWidth_) - (blackKeyWidth_ / 2.0f);
        float blackKeyHeight = pianoHeight * blackKeyHeightRatio_;
        return juce::Rectangle<float>(xPos, pianoAreaY, blackKeyWidth_, blackKeyHeight);
    }
}


void PianoRollComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId)); // Background

    float pianoAreaY = (float)selectForChain1Button_.getBottom();
    float pianoHeight = (float)getHeight() - pianoAreaY;

    if (whiteKeyWidth_ <= 0) return; // Avoid drawing if not resized properly

    // Draw white keys first
    int currentWhiteKeyIndex = 0;
    for (int i = 0; i < NUM_NOTES; ++i)
    {
        int midiNote = START_NOTE + i;
        if (!isBlackKey(midiNote))
        {
            juce::Rectangle<float> r(currentWhiteKeyIndex * whiteKeyWidth_, pianoAreaY, whiteKeyWidth_, pianoHeight);
            g.setColour(juce::Colours::white);
            g.fillRect(r);
            g.setColour(juce::Colours::black);
            g.drawRect(r, 0.5f);

            if (midiNote == selectedNoteChain1_)
            {
                g.setColour(highlightColourChain1_);
                g.fillRect(r.reduced(2.0f));
            }
            if (midiNote == selectedNoteChain2_)
            {
                g.setColour( (midiNote == selectedNoteChain1_) ? highlightColourChain2_.interpolatedWith(highlightColourChain1_, 0.5f) : highlightColourChain2_ );
                g.fillRect( (midiNote == selectedNoteChain1_) ? r.reduced(4.0f) : r.reduced(2.0f) );
            }
            currentWhiteKeyIndex++;
        }
    }

    // Draw black keys
    // For black keys, their x position depends on the white key to their logical left.
    int whiteKeysPassed = 0;
    for (int i = 0; i < NUM_NOTES; ++i)
    {
        int midiNote = START_NOTE + i;
        if (!isBlackKey(midiNote)) {
             whiteKeysPassed++;
        } else {
            // The x position is the right edge of the 'whiteKeysPassed -1'th white key, minus half black key width.
            // Or, more simply, the left edge of the 'whiteKeysPassed'th white key, minus half black key width.
            float xPos = (whiteKeysPassed * whiteKeyWidth_) - (blackKeyWidth_ / 2.0f);
            
            juce::Rectangle<float> r(xPos, pianoAreaY, blackKeyWidth_, pianoHeight * blackKeyHeightRatio_);
            g.setColour(juce::Colours::black);
            g.fillRect(r);

            if (midiNote == selectedNoteChain1_)
            {
                g.setColour(highlightColourChain1_.brighter(0.4f));
                g.fillRect(r.reduced(1.0f));
                g.setColour(juce::Colours::white);
                g.drawRect(r.reduced(1.0f), 0.5f);
            }
            if (midiNote == selectedNoteChain2_)
            {
                g.setColour( (midiNote == selectedNoteChain1_) ? highlightColourChain2_.brighter(0.4f).interpolatedWith(highlightColourChain1_.brighter(0.4f),0.5f) : highlightColourChain2_.brighter(0.4f) );
                g.fillRect( (midiNote == selectedNoteChain1_) ? r.reduced(2.0f) : r.reduced(1.0f) );
                g.setColour(juce::Colours::white);
                g.drawRect((midiNote == selectedNoteChain1_) ? r.reduced(2.0f) : r.reduced(1.0f), 0.5f);
            }
        }
    }
}


int PianoRollComponent::getMidiNoteFromPoint(const juce::Point<int>& point)
{
    float pianoAreaY = (float)selectForChain1Button_.getBottom();
    if (point.y < pianoAreaY) return -1; // Click was on buttons

    float pianoHeight = (float)getHeight() - pianoAreaY;
    float blackKeyH = pianoHeight * blackKeyHeightRatio_;

    // Prioritize black keys due to Z-order (they are "on top")
    int whiteKeysPassed = 0;
    for (int i = 0; i < NUM_NOTES; ++i)
    {
        int midiNote = START_NOTE + i;
        if (isBlackKey(midiNote))
        {
            float xPos = (whiteKeysPassed * whiteKeyWidth_) - (blackKeyWidth_ / 2.0f);
            juce::Rectangle<float> blackKeyRect(xPos, pianoAreaY, blackKeyWidth_, blackKeyH);
            if (blackKeyRect.contains(point.toFloat()))
                return midiNote;
        } else {
            whiteKeysPassed++;
        }
    }
    
    // Check white keys if no black key was hit
    if (whiteKeyWidth_ > 0) { // Avoid division by zero if not resized
        int whiteKeyIndexAtClick = static_cast<int>(point.x / whiteKeyWidth_);
        
        int currentWhiteKeyScanned = 0;
        for (int i = 0; i < NUM_NOTES; ++i) {
            int midiNote = START_NOTE + i;
            if (!isBlackKey(midiNote)) {
                if (currentWhiteKeyScanned == whiteKeyIndexAtClick) {
                    // Verify y-coordinate as well, though black keys already checked
                    juce::Rectangle<float> whiteKeyRect(whiteKeyIndexAtClick * whiteKeyWidth_, pianoAreaY, whiteKeyWidth_, pianoHeight);
                    if(whiteKeyRect.contains(point.toFloat())) return midiNote;
                    break;
                }
                currentWhiteKeyScanned++;
            }
        }
    }
    return -1; // No key hit
}

void PianoRollComponent::mouseDown(const juce::MouseEvent& event)
{
    int midiNote = getMidiNoteFromPoint(event.getPosition());
    DBG("PianoRoll: Mouse down, detected MIDI note: " << midiNote);

    if (midiNote != -1)
    {
        if (currentSelectionTarget_ == 0) // Selecting for Chain 1
        {
            // Allow deselecting if clicking the same note, or select new note
            selectedNoteChain1_ = (selectedNoteChain1_ == midiNote) ? -1 : midiNote;
            DBG("PianoRoll: Selected Note for Chain 1: " << selectedNoteChain1_);
            listeners_.call(&Listener::pianoNoteSelected, 0, selectedNoteChain1_);
        }
        else // Selecting for Chain 2
        {
            selectedNoteChain2_ = (selectedNoteChain2_ == midiNote) ? -1 : midiNote;
            DBG("PianoRoll: Selected Note for Chain 2: " << selectedNoteChain2_);
            listeners_.call(&Listener::pianoNoteSelected, 1, selectedNoteChain2_);
        }
        repaint();
    }
}

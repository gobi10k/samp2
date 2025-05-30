#pragma once
#include <JuceHeader.h>

class PianoRollComponent : public juce::Component,
                           public juce::Button::Listener
{
public:
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void pianoNoteSelected(int chainIndex, int midiNoteNumber) = 0;
    };

    PianoRollComponent();
    ~PianoRollComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void buttonClicked(juce::Button* button) override;

    void addListener(Listener* l);
    void removeListener(Listener* l);

    void setInitialNotes(int noteChain1, int noteChain2);
    void setChainHighlightColour(int chainIndex, juce::Colour colour);

private:
    int getMidiNoteFromPoint(const juce::Point<int>& point);
    juce::Rectangle<float> getKeyRectangle(int midiNote);
    bool isBlackKey(int midiNote);

    static constexpr int START_NOTE = 48; // C3
    static constexpr int NUM_NOTES = 36;  // 3 octaves

    int selectedNoteChain1_;
    int selectedNoteChain2_;
    juce::Colour highlightColourChain1_;
    juce::Colour highlightColourChain2_;

    juce::ToggleButton selectForChain1Button_;
    juce::ToggleButton selectForChain2Button_;
    int currentSelectionTarget_; // 0 for chain 1, 1 for chain 2

    juce::ListenerList<Listener> listeners_;

    // White keys in 3 octaves: 3 * 7 = 21
    // Black keys in 3 octaves: 3 * 5 = 15
    // Total physical keys to draw differently: 21 white, 15 black
    // We'll calculate their rects on the fly in resized/paint.
    float whiteKeyWidth_;
    float blackKeyWidth_;
    float blackKeyHeightRatio_ = 0.6f; // Black keys are shorter

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};

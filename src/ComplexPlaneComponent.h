#pragma once
#include "DFVComponent.h"
#include "ComplexPlaneEditor.h"

class ComplexPlaneComponent final
    :public DFVComponent
    , private juce::ValueTree::Listener
{
public:
    ComplexPlaneComponent(AudioPluginAudioProcessor* p);
    void resized(void) override;
    void paint(juce::Graphics& g) override;

private:
    static constexpr float innerCornerSize = 9.f;
    static constexpr int buttonsHeight = 20;
    static constexpr int buttonsWidth = 45;

    AudioPluginAudioProcessor& processorRef;
    ComplexPlaneEditor editor;

    // interaction mode toggles
    juce::TextButton defaultInteractionButton;
    juce::TextButton constantMagnitudeInteractionButton;
    juce::TextButton constantAngleInteractionButton;
    juce::TextButton addRootButton;

    static constexpr int InteractionToggleGroupID = 0x00867068;
};

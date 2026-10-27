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
    ComplexPlaneEditor editor;
};

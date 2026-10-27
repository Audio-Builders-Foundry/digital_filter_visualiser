#include "ComplexPlaneComponent.h"

ComplexPlaneComponent::
ComplexPlaneComponent(AudioPluginAudioProcessor* p)
    :editor(p)
{
    setName("PLOT");
    addAndMakeVisible(editor);
}

void ComplexPlaneComponent::
resized(void)
{
    PROFILE_FUNCTION();

    DFVComponent::resized();
    int reduce = padding + (int)std::sqrt(innerCornerSize) + 1;
    auto editorRect = reducedBounds.reduced(reduce, reduce);
    editor.setBounds(editorRect);
}

void ComplexPlaneComponent::
paint(juce::Graphics& g)
{
    PROFILE_FUNCTION();

    g.setColour(findColour(DFVLookAndFeel::blackBackgroundColourId));
    g.fillRoundedRectangle(reducedBounds.toFloat(), innerCornerSize);
    g.setColour(findColour(DFVLookAndFeel::controlOutlineColourId));
    g.drawRoundedRectangle(reducedBounds.toFloat(), innerCornerSize, 0.25f);

    auto innerRect = reducedBounds.reduced(padding, padding);
    g.setColour(findColour(DFVLookAndFeel::plotBackgroundColourId));
    g.fillRoundedRectangle(innerRect.toFloat(), innerCornerSize);
    g.setColour(findColour(DFVLookAndFeel::plotAxesColourId));
    g.drawRoundedRectangle(innerRect.toFloat(), innerCornerSize, 0.25f);
    DFVComponent::paint(g);
}

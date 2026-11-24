#include "ComplexPlaneComponent.h"

ComplexPlaneComponent::
ComplexPlaneComponent(AudioPluginAudioProcessor* p)
    :editor(p)
    , defaultInteractionButton("DEF")
    , constantMagnitudeInteractionButton("MAG")
    , constantAngleInteractionButton("ARG")
{
    setName("PLOT");
    
    // TODO(ry): some generatable looking code here

    defaultInteractionButton.setTooltip("Roots can be moved in any direction");
    constantMagnitudeInteractionButton.setTooltip("Roots can be moved preserving their magnitudes");
    constantAngleInteractionButton.setTooltip("Roots can be moved preserving their angles");

    defaultInteractionButton.addMouseListener(this, false);
    constantMagnitudeInteractionButton.addMouseListener(this, false);
    constantAngleInteractionButton.addMouseListener(this, false);

    defaultInteractionButton.setClickingTogglesState(true);
    constantMagnitudeInteractionButton.setClickingTogglesState(true);
    constantAngleInteractionButton.setClickingTogglesState(true);

    defaultInteractionButton.setRadioGroupId(InteractionToggleGroupID);
    constantMagnitudeInteractionButton.setRadioGroupId(InteractionToggleGroupID);
    constantAngleInteractionButton.setRadioGroupId(InteractionToggleGroupID);

    defaultInteractionButton.onClick = [this]() {
        if (defaultInteractionButton.getToggleState())
        {
            editor.setInteractionMode(0, true);
        }
        };
    constantMagnitudeInteractionButton.onClick = [this]() {
        if (constantMagnitudeInteractionButton.getToggleState())
        {
            editor.setInteractionMode(editor.InteractionFlag_constantMagnitude, true);
        }
        };
    constantAngleInteractionButton.onClick = [this]() {
        if (constantAngleInteractionButton.getToggleState())
        {
            editor.setInteractionMode(editor.InteractionFlag_constantAngle, true);
        }
        };

    addAndMakeVisible(defaultInteractionButton);
    addAndMakeVisible(constantMagnitudeInteractionButton);
    addAndMakeVisible(constantAngleInteractionButton);

    editor.onInteractionModeChanged = [this](const bool constantMagnitude, const bool constantAngle)
        {
            constantMagnitudeInteractionButton
                .setToggleState(constantMagnitude, juce::dontSendNotification);
            constantAngleInteractionButton
                .setToggleState(constantAngle, juce::dontSendNotification);
            defaultInteractionButton
                .setToggleState(!constantMagnitude && !constantAngle,
           		juce::dontSendNotification);
        };

    defaultInteractionButton.setToggleState(true, juce::sendNotification);

    addAndMakeVisible(editor);
}

void ComplexPlaneComponent::
resized(void)
{
    PROFILE_FUNCTION();

    DFVComponent::resized();

    
    int y = reducedBounds.getY();
    defaultInteractionButton.setBounds(reducedBounds.getX(), y, plotButtonsWidth, buttonsHeight);
    constantMagnitudeInteractionButton.setBounds(defaultInteractionButton.getRight() + padding, y, plotButtonsWidth, buttonsHeight);
    constantAngleInteractionButton.setBounds(constantMagnitudeInteractionButton.getRight() + padding, y, plotButtonsWidth, buttonsHeight);

    reducedBounds.removeFromTop(constantMagnitudeInteractionButton.getHeight() + padding);
    int reduce = padding + (int)std::sqrt(innerCornerSize) + 1;
    auto editorRect = reducedBounds.reduced(reduce, reduce);
    editor.setBounds(editorRect);

    //{
//    auto thirdWidth = area.getWidth() / 3;
//}
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

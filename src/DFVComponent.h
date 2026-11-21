#pragma once
#include "DFVLookAndFeel.h"
#include "Accessibility.h"

class DFVComponent
    : public juce::Component
{
public:
    DFVComponent(const Group& g, const juce::String& componentName = "")
        : juce::Component(componentName)
    {
        setTitle(g.title);
        setDescription(g.description);
        setHelpText(g.helpText); 
        setFocusContainerType(FocusContainerType::focusContainer);
        setExplicitFocusOrder(g.focusOrder);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        g.setColour(findColour(DFVLookAndFeel::controlOutlineColourId));
        g.drawRoundedRectangle(bounds.toFloat(), outerFrameCornerSize, 1.5f);
        bounds = bounds.reduced(padding, padding);
        const auto name = getName();
        if (name.isNotEmpty())
        {
            g.setColour(findColour(DFVLookAndFeel::controlHeaderBackgroundColourId));
            g.fillRoundedRectangle(headerRect, 10.f);
            g.setColour(findColour(DFVLookAndFeel::controlHeaderOutlineColourId));
            g.drawRoundedRectangle(headerRect, 10.f, 1.5f);
            g.setColour(findColour(DFVLookAndFeel::controlHeaderTextColourId));
            g.setFont(juce::FontOptions(std::min(headerRect.getHeight() - 2.f, 18.f), juce::Font::bold));
            g.drawFittedText(name, headerRect.toNearestInt(), juce::Justification::centred, 1);
        }
    }

    void resized() override
    {
        reducedBounds = getLocalBounds().reduced(padding, padding);
        if (getName().isNotEmpty())
        {
            headerRect = reducedBounds.removeFromTop(headerHeight).toFloat();
            reducedBounds.removeFromTop(padding);
        }
    }

protected:
    const int padding = 7;
    const float outerFrameCornerSize = 15.f;
    juce::Rectangle<int> reducedBounds;
    juce::Rectangle<float> headerRect;

private:
    const int headerHeight = 30;
};
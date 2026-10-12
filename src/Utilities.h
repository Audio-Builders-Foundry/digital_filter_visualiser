#pragma once
#include <juce_graphics/juce_graphics.h>
#include "DFVLookAndFeel.h"

class Utilities
{
public:
    static void setButtonImage(juce::ImageButton* button, juce::Image& image)
    {
        button->setImages(
            false, true, true,
            image, 1.0f, juce::Colours::transparentWhite,
            image, 0.8f, juce::Colours::transparentWhite,
            image, 0.5f, juce::Colours::transparentWhite);
    }

    static void updateButtonColours(juce::ImageButton* button)
    {
        button->setImages(
            false, true, true,
            button->getNormalImage(), 1.f, button->getLookAndFeel().findColour(DFVLookAndFeel::ColourIds::imageButtonNormalColourId),
            button->getOverImage(), 1.f, button->getLookAndFeel().findColour(DFVLookAndFeel::ColourIds::imageButtonOverColourId),
            button->getDownImage(), 1.f, button->getLookAndFeel().findColour(DFVLookAndFeel::ColourIds::imageButtonDownColourId));
    }
};
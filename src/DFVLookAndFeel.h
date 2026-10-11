#pragma once
class DFVLookAndFeel : public juce::LookAndFeel_V4
{
public:
    enum ColourIds
    {
        controlFrameColourId = 0x2000001,
        controlBackgroundColourId,
        titleTextColourId,
        controlHeaderBackgroundColourId,
        controlHeaderFrameColourId,
        controlHeaderTextColourId,
        plotBackgroundColourId,
        plotAxesColourId,
        plotGridColourId,
        plotChartColourId,
        equationTextColourId,
        tableSelectedRowBackgroundId,
        blackBackgroundColourId,
    };

    DFVLookAndFeel()
    {
        const auto axesAndEquationColour = juce::Colour(0xfffbec64);
        const auto backgroundColour = juce::Colour(0xff302d27);
        const auto outlineColour = juce::Colour(0xff100b07);
        const auto tableColour = juce::Colour(0xfff7e862);
        const auto buttonAndTableHeaderColour = juce::Colour(0xffcabc91);
        const auto buttonBackgroundColour = juce::Colour(0xff1a1713);
        const auto almostBlackColour = juce::Colour(0xff0e0e0e);

        setColour(juce::ResizableWindow::ColourIds::backgroundColourId, juce::Colour(0xff151411));
        setColour(juce::ScrollBar::ColourIds::thumbColourId, juce::Colour(0xffa88860));
        setColour(juce::ListBox::ColourIds::backgroundColourId, backgroundColour);
        setColour(juce::ListBox::ColourIds::textColourId, tableColour);
        setColour(juce::ListBox::ColourIds::outlineColourId, tableColour);
        setColour(juce::TableListBox::ColourIds::backgroundColourId, backgroundColour);
        setColour(juce::TableListBox::ColourIds::textColourId, tableColour);
        setColour(juce::TableListBox::ColourIds::outlineColourId, tableColour);
        setColour(juce::TableHeaderComponent::backgroundColourId, backgroundColour);
        setColour(juce::TableHeaderComponent::outlineColourId, tableColour);
        setColour(juce::TableHeaderComponent::textColourId, buttonAndTableHeaderColour);
        setColour(juce::TextButton::ColourIds::buttonColourId, buttonBackgroundColour);
        setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colour(0xff9d4000));
        setColour(juce::TextButton::ColourIds::textColourOffId, buttonAndTableHeaderColour);
        setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colour(0xfffff141));
        setColour(juce::Label::textColourId, tableColour);
        setColour(juce::Slider::thumbColourId, juce::Colour(0xffa44d01));
        setColour(juce::Slider::trackColourId, juce::Colour(0xfffcc559));
        setColour(juce::Slider::backgroundColourId, almostBlackColour);
        setColour(juce::Slider::textBoxBackgroundColourId, buttonBackgroundColour);
        setColour(juce::Slider::textBoxOutlineColourId, outlineColour);
        setColour(juce::Slider::textBoxTextColourId, tableColour);
        setColour(juce::Slider::textBoxHighlightColourId, buttonBackgroundColour);

        setColour(ColourIds::controlHeaderBackgroundColourId, juce::Colour(0xffa08058));
        setColour(ColourIds::controlHeaderFrameColourId, outlineColour);
        setColour(ColourIds::controlHeaderTextColourId, juce::Colour(0xff151515));
        setColour(ColourIds::controlFrameColourId, juce::Colour(0xffc5b49f));
        setColour(ColourIds::controlBackgroundColourId, backgroundColour);
        setColour(ColourIds::titleTextColourId, juce::Colour(0xffc9af92));
        setColour(ColourIds::plotBackgroundColourId, juce::Colour(0xff121a10));
        setColour(ColourIds::plotAxesColourId, axesAndEquationColour);
        setColour(ColourIds::plotGridColourId, juce::Colour(0xff738755));
        setColour(ColourIds::plotChartColourId, juce::Colour(0xffe8e29a));
        setColour(ColourIds::equationTextColourId, axesAndEquationColour);
        setColour(ColourIds::tableSelectedRowBackgroundId, juce::Colour(0xffa08058));
        setColour(ColourIds::blackBackgroundColourId, almostBlackColour);
    }

    void drawButtonBackground(
        juce::Graphics& g,
        juce::Button& button,
        const juce::Colour& backgroundColour,
        bool isMouseOverButton,
        bool isButtonDown) override
    {
        if (button.getToggleState())
        {
            auto cornerSize = 6.0f;
            auto intBounds = button.getLocalBounds();
            if (intBounds.getWidth() == 0 || intBounds.getHeight() == 0)
                return;
            auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
            auto centre = bounds.getCentre();
            const float scaleCoeff = bounds.getHeight() / bounds.getWidth();

            auto gradient = juce::ColourGradient(
                juce::Colour(findColour(juce::TextButton::ColourIds::buttonOnColourId)),
                centre,
                juce::Colour(findColour(juce::TextButton::ColourIds::buttonColourId)),
                bounds.getTopLeft(),
                true);
            
            auto transform = juce::AffineTransform::scale(
                1.0f, scaleCoeff, centre.x, centre.y);
            auto fill = juce::FillType(gradient).transformed(transform);
            g.setFillType(fill);
            g.fillRoundedRectangle(bounds, cornerSize);

            g.setColour(button.findColour(juce::ComboBox::outlineColourId));
            g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
        }
        else
            LookAndFeel_V4::drawButtonBackground(g, button, backgroundColour, isMouseOverButton, isButtonDown);
    }
};
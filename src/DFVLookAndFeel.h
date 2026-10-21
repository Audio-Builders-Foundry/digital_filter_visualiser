#pragma once
class DFVLookAndFeel : public juce::LookAndFeel_V4
{
public:
    enum ColourIds
    {
        controlOutlineColourId = 0x2000001,
        controlBackgroundColourId,
        titleTextColourId,
        controlHeaderBackgroundColourId,
        controlHeaderOutlineColourId,
        controlHeaderTextColourId,
        plotBackgroundColourId,
        plotAxesColourId,
        plotGridColourId,
        plotChartColourId,
        equationTextColourId,
        tableSelectedRowBackgroundId,
        blackBackgroundColourId,
        imageButtonOutlineColourId,
        imageButtonNormalColourId,
        imageButtonOverColourId,
        imageButtonDownColourId,
        sliderThumbOutlineColourId,
        sliderBackgroundMinColourId,
        sliderBackgroundMaxColourId,
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
        const auto controlHeaderAndImageButtonBackgroundColour = juce::Colour(0xffa08058);

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
        setColour(juce::Label::textWhenEditingColourId, tableColour);
        setColour(juce::Slider::thumbColourId, juce::Colour(0xffa44d01));
        setColour(juce::Slider::trackColourId, juce::Colour(0xfffcc559));
        setColour(juce::Slider::backgroundColourId, almostBlackColour);
        setColour(juce::Slider::textBoxBackgroundColourId, buttonBackgroundColour);
        setColour(juce::Slider::textBoxOutlineColourId, outlineColour);
        setColour(juce::Slider::textBoxTextColourId, tableColour);
        setColour(juce::Slider::textBoxHighlightColourId, buttonBackgroundColour);

        setColour(ColourIds::controlHeaderBackgroundColourId, controlHeaderAndImageButtonBackgroundColour);
        setColour(ColourIds::controlHeaderOutlineColourId, outlineColour);
        setColour(ColourIds::controlHeaderTextColourId, juce::Colour(0xff151515));
        setColour(ColourIds::controlOutlineColourId, juce::Colour(0xffc5b49f));
        setColour(ColourIds::controlBackgroundColourId, backgroundColour);
        setColour(ColourIds::titleTextColourId, juce::Colour(0xffc9af92));
        setColour(ColourIds::plotBackgroundColourId, juce::Colour(0xff121a10));
        setColour(ColourIds::plotAxesColourId, axesAndEquationColour);
        setColour(ColourIds::plotGridColourId, juce::Colour(0xff738755));
        setColour(ColourIds::plotChartColourId, juce::Colour(0xffe8e29a));
        setColour(ColourIds::equationTextColourId, axesAndEquationColour);
        setColour(ColourIds::tableSelectedRowBackgroundId, juce::Colour(0xffa08058));
        setColour(ColourIds::blackBackgroundColourId, almostBlackColour);
        setColour(ColourIds::imageButtonOutlineColourId, outlineColour);
        setColour(ColourIds::imageButtonNormalColourId, juce::Colours::black);
        setColour(ColourIds::imageButtonOverColourId, juce::Colours::darkgrey);
        setColour(ColourIds::imageButtonDownColourId, juce::Colours::dimgrey);
        setColour(ColourIds::sliderThumbOutlineColourId, juce::Colour(0xff9f5e01));
        setColour(ColourIds::sliderBackgroundMinColourId, juce::Colour(0xfffcc559));
        setColour(ColourIds::sliderBackgroundMaxColourId, juce::Colour(0xffde9300));
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
            auto bounds = button.getLocalBounds().toFloat();
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

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        if (dynamic_cast<juce::Slider*>(label.getParentComponent()) == nullptr)
            juce::LookAndFeel_V4::drawLabel(g, label);
        else
        {
            auto cornerSize = 6.0f;
            auto bounds = label.getLocalBounds().toFloat();
            g.setColour(findColour(juce::Slider::backgroundColourId));
            g.fillRoundedRectangle(bounds, cornerSize);
            g.setColour(findColour(juce::Slider::textBoxOutlineColourId));
            g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
            g.setColour(findColour(juce::Slider::textBoxTextColourId));
            g.drawFittedText(
                label.getText(),
                label.getLocalBounds(),
                label.getJustificationType(),
                1);
        }
    }
    void fillTextEditorBackground(
        juce::Graphics& g,
        int /*width*/,
        int /*height*/,
        juce::TextEditor& textEditor) override
    {
        auto cornerSize = 6.0f;
        auto bounds = textEditor.getLocalBounds().toFloat();
        g.setColour(findColour(juce::Slider::backgroundColourId));
        g.fillRoundedRectangle(bounds, cornerSize);
    }

    void drawTextEditorOutline(
        juce::Graphics& g,
        int width,
        int height,
        juce::TextEditor& textEditor) override
    {
        auto cornerSize = 6.0f;
        auto bounds = textEditor.getLocalBounds().toFloat();
        g.setColour(findColour(juce::Slider::textBoxOutlineColourId));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }

    void drawImageButton(
        juce::Graphics& g,
        juce::Image* image,
        int imageX, int imageY,
        int imageW, int imageH,
        const juce::Colour& overlayColour,
        float imageOpacity,
        juce::ImageButton& button) override
    {
        const float cornerSize = 9.f;
        const int sub = 4.f;
        auto bounds = button.getLocalBounds().toFloat();
        g.setColour(findColour(ColourIds::controlHeaderBackgroundColourId));
        g.fillRoundedRectangle(bounds, cornerSize);
        g.setColour(findColour(juce::Slider::textBoxOutlineColourId));
        g.drawRoundedRectangle(bounds, cornerSize, 1.5f);
        juce::LookAndFeel_V2::drawImageButton(
            g,
            image,
            imageX + sub,
            imageY + sub,
            imageW - 2 * sub,
            imageH - 2 * sub,
            overlayColour,
            imageOpacity,
            button);
    }

    void drawLinearSlider(
        juce::Graphics& g,
        int x, int y,
        int width, int height,
        float sliderPos,
        float minSliderPos,
        float maxSliderPos,
        const juce::Slider::SliderStyle style,
        juce::Slider& slider) override
    {
        auto trackWidth = juce::jmin(6.0f, (float)height * 0.25f);
        juce::Point<float> startPoint((float)x, (float)y + (float)height * 0.5f);
        juce::Point<float> endPoint((float)(width + x), startPoint.y);

        const float d = 4.f;
        auto outerRect = juce::Rectangle<float>(
            startPoint.getX() - d,
            startPoint.getY() - 0.5f * trackWidth - d,
            endPoint.getX() - startPoint.getX() + 2.f * d,
            trackWidth + 2.f * d);
        g.setColour(findColour(ColourIds::controlOutlineColourId));
        g.drawRoundedRectangle(outerRect, d, 0.5f);

        auto trackRect = juce::Rectangle<float>(
            startPoint.getX(),
            startPoint.getY() - 0.5f * trackWidth,
            endPoint.getX() - startPoint.getX(),
            trackWidth);
        auto gradient = juce::ColourGradient(
            juce::Colour(findColour(ColourIds::sliderBackgroundMinColourId)),
            startPoint,
            juce::Colour(findColour(ColourIds::sliderBackgroundMaxColourId)),
            endPoint,
            false);
        auto fill = juce::FillType(gradient);
        g.setFillType(fill);
        g.fillRoundedRectangle(trackRect, 0.5f * trackWidth);

        // thumb
        juce::Point<float> thumbPoint(sliderPos, (float)y + (float)height * 0.5f);
        auto thumbWidth = getSliderThumbRadius(slider);
        auto thumbRect = juce::Rectangle<float>(
            static_cast<float>(thumbWidth) * 0.5f, static_cast<float>(trackWidth + 2.f * d))
            .withCentre(thumbPoint);
        auto corner = static_cast<float>(thumbWidth) * 0.25f;
        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        g.fillRoundedRectangle(thumbRect, corner);
        g.setColour(slider.findColour(ColourIds::sliderThumbOutlineColourId));
        g.drawRoundedRectangle(thumbRect, corner, 1.f);
    }
};
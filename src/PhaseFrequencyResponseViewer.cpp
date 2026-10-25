#include "PhaseFrequencyResponseViewer.h"
#include "PhaseFrequencyResponseCalculator.h"

PhaseFrequencyResponseViewer::PhaseFrequencyResponseViewer(AudioPluginAudioProcessor* p) :
    processor(p),
    ampDb(minAmpDb * 4),
    sampleRate(processor->getSampleRate()),
    zoomInButton("+"),
    zoomOutButton("-"),
    linearScaleButton("LINEAR"),
    logScaleButton("LOG"),
    freqButton("FREQ."),
    phaseButton("PHASE"),
    bothButton("BOTH")
{
    setName("RESPONSE");
    this->processor->addChangeListener(this);
    this->processor->filterState->um->addChangeListener(this);

    freqButton.onClick = [this] { changePlotsSet(); };
    freqButton.setClickingTogglesState(true);
    freqButton.setRadioGroupId(1);
    freqButton.setTooltip("Show only frequency response plot");

    phaseButton.onClick = [this] { changePlotsSet(); };
    phaseButton.setClickingTogglesState(true);
    phaseButton.setRadioGroupId(1);
    phaseButton.setTooltip("Show only phase response plot");

    bothButton.onClick = [this] { changePlotsSet(); };
    bothButton.setClickingTogglesState(true);
    bothButton.setRadioGroupId(1);
    bothButton.setToggleState(true, juce::dontSendNotification);
    bothButton.setTooltip("Show frequency and phase response plots");

    zoomInButton.onClick = [this]
        {
            if (ampDb > minAmpDb)
            {
                ampDb /= 2.f;
                repaint();
            }
        };
    zoomInButton.setTooltip("Zoom in");

    zoomOutButton.onClick = [this]
        {
            if (ampDb < maxAmpDb)
            {
                ampDb *= 2.f;
                repaint();
            }
        };
    zoomOutButton.setTooltip("Zoom out");

    logScaleButton.onClick = [this] { repaint(); };
    logScaleButton.setClickingTogglesState(true);
    logScaleButton.setRadioGroupId(2);
    logScaleButton.setEnabled(sampleRate > 0);
    logScaleButton.setTooltip("Logarithmic X scale");

    linearScaleButton.onClick = [this] { repaint(); };
    linearScaleButton.setClickingTogglesState(true);
    linearScaleButton.setRadioGroupId(2);
    linearScaleButton.setEnabled(sampleRate > 0);
    linearScaleButton.setTooltip("Linear X scale");
    linearScaleButton.setToggleState(true, juce::sendNotificationSync);

    addAndMakeVisible(freqButton);
    addAndMakeVisible(phaseButton);
    addAndMakeVisible(bothButton);
    addAndMakeVisible(zoomInButton);
    addAndMakeVisible(zoomOutButton);
    addAndMakeVisible(linearScaleButton);
    addAndMakeVisible(logScaleButton);
}

PhaseFrequencyResponseViewer::~PhaseFrequencyResponseViewer()
{
    processor->filterState->um->removeChangeListener(this);
    processor->removeChangeListener(this);
}

void PhaseFrequencyResponseViewer::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == processor->filterState->um)
        repaint();
    else if (source == processor)
    {
        sampleRate = processor->getSampleRate();
        logScaleButton.setEnabled(sampleRate > 0);
        repaint();
    }
}

void PhaseFrequencyResponseViewer::resized()
{
    //PROFILE_FUNCTION();
    DFVComponent::resized();
    int y = reducedBounds.getY();
    freqButton.setBounds(reducedBounds.getX(), y, plotButtonsWidth, buttonsHeight);
    phaseButton.setBounds(freqButton.getRight() + padding, y, plotButtonsWidth, buttonsHeight);
    bothButton.setBounds(phaseButton.getRight() + padding, y, plotButtonsWidth, buttonsHeight);

    logScaleButton.setBounds(reducedBounds.getRight() - padding - scaleButtonsWidth, y, scaleButtonsWidth, buttonsHeight);
    linearScaleButton.setBounds(logScaleButton.getX() - padding - scaleButtonsWidth, y, scaleButtonsWidth, buttonsHeight);
    setZoomButtonsBounds();

    repaint();
}

void PhaseFrequencyResponseViewer::paint(juce::Graphics& g)
{
    //PROFILE_FUNCTION();
    DFVComponent::paint(g);

    const int plotsWidth = singleRect.getWidth() - plotPaddingLeft - plotPaddingRight;

    if (plotsWidth <= 0 ||
        frequencyRect.getHeight() <= plotTitleHeight ||
        phaseRect.getHeight() <= plotTitleHeight)
        return;

    // angles, amplitudes & phases
    const bool isLogScale = logScaleButton.getToggleState();
    std::vector<double> angles, amplitudes, phases;
    PhaseFrequencyResponseCalculator::calculate(
        processor->filterState.get(),
        minFreq,
        sampleRate,
        isLogScale, true, plotsWidth, angles, amplitudes, phases);

    if (!phaseButton.getToggleState())
        paintPlot(g, amplitudes, isLogScale, ampDb, frequencyRect, false);
    if (!freqButton.getToggleState())
        paintPlot(g, phases, isLogScale, 180., phaseRect, true);
}

void PhaseFrequencyResponseViewer::changePlotsSet()
{
    setZoomButtonsBounds();
    zoomInButton.setVisible(!phaseButton.getToggleState());
    zoomOutButton.setVisible(!phaseButton.getToggleState());
    repaint();
}

void PhaseFrequencyResponseViewer::paintPlot(
    juce::Graphics& g,
    std::vector<double>& yValues,
    bool isLogScale,
    float yAmplitude,
    juce::Rectangle<int> rect,
    bool isPhase)
{
    const juce::String title = isPhase ? "PHASE RESPONSE" : "FREQUENCY RESPONSE";
    const juce::String unitText = isPhase ? " deg." : " dB";

    const auto axesColour = findColour(DFVLookAndFeel::plotAxesColourId);
    const auto gridColour = findColour(DFVLookAndFeel::plotGridColourId);
    const auto chartColour = findColour(DFVLookAndFeel::plotChartColourId);

    g.setColour(findColour(DFVLookAndFeel::blackBackgroundColourId));
    g.fillRoundedRectangle(rect.toFloat(), 10.f);
    g.setColour(findColour(DFVLookAndFeel::controlOutlineColourId));
    g.drawRoundedRectangle(rect.toFloat(), 10.f, 0.25f);

    g.setColour(findColour(juce::TableHeaderComponent::textColourId));
    g.setFont(juce::FontOptions(std::min(plotTitleHeight - 2.f, 16.f), juce::Font::plain));
    g.drawFittedText(title, rect.getX() + padding, rect.getY(), rect.getWidth() - 2 * padding, plotTitleHeight, juce::Justification::centredLeft, 1);

    auto innerRect = rect;
    innerRect.removeFromTop(plotTitleHeight - padding);
    innerRect.reduce(padding, padding);

    g.setColour(findColour(DFVLookAndFeel::plotBackgroundColourId));
    g.fillRoundedRectangle(innerRect.toFloat(), 10.f);
    g.setColour(findColour(DFVLookAndFeel::plotAxesColourId));
    g.drawRoundedRectangle(innerRect.toFloat(), 10.f, 0.25f);

    juce::Rectangle<int> plotRect(
        innerRect.getX() + plotPaddingLeft,
        innerRect.getY() + plotPaddingTop,
        innerRect.getWidth() - plotPaddingLeft - plotPaddingRight,
        innerRect.getHeight() - plotPaddingTop - plotPaddingBottom);
    juce::Rectangle<float> plotRectFloat = plotRect.toFloat();

    g.setFont(juce::FontOptions(15.f, juce::Font::plain));

    // Y grid
    g.setColour(axesColour);
    g.drawText(
        juce::String(static_cast<int>(yAmplitude)) + unitText,
        padding,
        plotRect.getY() - textHeight / 2,
        plotRect.getX() - 2 * padding,
        textHeight,
        juce::Justification::centredRight);
    g.drawText(
        "0",
        padding,
        plotRect.getY() + (plotRect.getHeight() - textHeight) / 2,
        plotRect.getX() - 2 * padding,
        textHeight,
        juce::Justification::centredRight);
    g.drawText(
        juce::String(static_cast<int>(-yAmplitude)) + unitText,
        padding,
        plotRect.getY() + plotRect.getHeight() - textHeight / 2,
        plotRect.getX() - 2 * padding,
        textHeight,
        juce::Justification::centredRight);
    g.drawLine(plotRectFloat.getX(), plotRectFloat.getBottom(), plotRectFloat.getRight(), plotRectFloat.getBottom());
    g.setColour(gridColour);
    g.drawLine(plotRectFloat.getX(), plotRectFloat.getCentreY(), plotRectFloat.getRight(), plotRectFloat.getCentreY());

    // X grid
    const int hzTextY = static_cast<int>(plotRectFloat.getBottom()) + padding;
    g.setColour(axesColour);
    g.drawLine(plotRectFloat.getX(), plotRectFloat.getY(), plotRectFloat.getX(), plotRectFloat.getBottom());
    if (isLogScale)
    {
        g.setColour(axesColour);
        g.drawText(
            juce::String(static_cast<int>(minFreq)),
            plotRect.getX() - textWidth / 2,
            hzTextY,
            textWidth,
            textHeight,
            juce::Justification::centred);
        g.drawText(
            juce::String(static_cast<int>(sampleRate / 2)),
            plotRect.getRight() - textWidth / 2,
            hzTextY,
            textWidth,
            textHeight,
            juce::Justification::centred);
        g.drawText(
            " Hz",
            plotRect.getRight() + textWidth / 2,
            hzTextY,
            textWidth,
            textHeight,
            juce::Justification::centredLeft);

        const float maxFreq = static_cast<float>(sampleRate / 2.0);
        const float logMaxFreq = std::log10(maxFreq);
        const float logMinFreq = std::log10(minFreq);
        const float freqCoeff = plotRect.getWidth() / (logMaxFreq - logMinFreq);

        int freq = static_cast<int>(minFreq * 10);
        while (freq < maxFreq)
        {
            float x = freqCoeff * (std::log10(freq) - logMinFreq) + plotRectFloat.getX();
            g.setColour(gridColour);
            g.drawLine(x, plotRectFloat.getY(), x, plotRectFloat.getBottom());
            if (plotRectFloat.getRight() - x > static_cast<float>(textWidth))
            {
                g.setColour(axesColour);
                g.drawText(
                    juce::String(freq),
                    static_cast<int>(x) - textWidth / 2,
                    hzTextY,
                    textWidth,
                    textHeight,
                    juce::Justification::centred);
            }
            freq *= 10;
        }
    }
    else
    {
        g.setColour(axesColour);
        g.drawText(
            "0",
            plotRect.getX() - textWidth / 2,
            hzTextY,
            textWidth,
            textHeight,
            juce::Justification::centred);
        g.drawText(
            juce::exactlyEqual(sampleRate, 0.0) ? "pi/2" : juce::String(static_cast<int>(sampleRate / 4)),
            plotRect.getX() + (plotRect.getWidth() - textWidth) / 2,
            hzTextY,
            textWidth,
            textHeight,
            juce::Justification::centred);
        g.drawText(
            juce::exactlyEqual(sampleRate, 0.0) ? "pi" : juce::String(static_cast<int>(sampleRate / 2)),
            plotRect.getRight() - textWidth / 2,
            hzTextY,
            textWidth,
            textHeight,
            juce::Justification::centred);
        if (!juce::exactlyEqual(sampleRate, 0.0))
            g.drawText(
                " Hz",
                plotRect.getRight() + textWidth / 2,
                hzTextY,
                textWidth,
                textHeight,
                juce::Justification::centredLeft);

        g.setColour(gridColour);
        g.drawLine(plotRectFloat.getCentreX(), plotRectFloat.getY(), plotRectFloat.getCentreX(), plotRectFloat.getBottom());
    }

    // drawing values
    {
        // clipping inside curly brackets
        const juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(plotRect);

        const float period = 2 * yAmplitude;
        juce::Path path;
        auto x = plotRectFloat.getX();

        auto wrapPhase = [&](double value)
            {
                double result = std::fmod(value + yAmplitude, period);
                return result < 0.0 ? result + yAmplitude : result - yAmplitude;
            };

        auto mapY = [&](double value)
            {
                return juce::jmap(static_cast<float>(value), -yAmplitude, yAmplitude, plotRectFloat.getBottom(), plotRectFloat.getY());
            };

        auto drawPhaseSegment = [&](float x0, float x1, double y0, double y1)
            {
                const double delta = y1 - y0;
                if (juce::approximatelyEqual(delta, 0.0))
                {
                    path.lineTo(x1, mapY(wrapPhase(y1)));
                    return;
                }

                if (delta > 0.0)
                {
                    auto boundary = std::ceil((y0 - yAmplitude) / period) * period + yAmplitude;
                    while (boundary < y1)
                    {
                        const auto t = (boundary - y0) / delta;
                        const auto crossingX = x0 + (x1 - x0) * static_cast<float>(t);
                        path.lineTo(crossingX, mapY(yAmplitude));
                        path.startNewSubPath(crossingX, mapY(-yAmplitude));
                        boundary += period;
                    }
                }
                else
                {
                    auto boundary = std::floor((y0 + yAmplitude) / period) * period - yAmplitude;
                    while (boundary > y1)
                    {
                        const auto t = (boundary - y0) / delta;
                        const auto crossingX = x0 + (x1 - x0) * static_cast<float>(t);
                        path.lineTo(crossingX, mapY(-yAmplitude));
                        path.startNewSubPath(crossingX, mapY(yAmplitude));
                        boundary -= period;
                    }
                }
                path.lineTo(x1, mapY(wrapPhase(y1)));
            };

        g.setColour(chartColour);
        if (isPhase)
        {
            path.startNewSubPath(x, mapY(wrapPhase(yValues[0])));
            for (int i = 1; i < plotRect.getWidth(); i++)
            {
                drawPhaseSegment(x, x + 1, yValues[static_cast<size_t>(i - 1)], yValues[static_cast<size_t>(i)]);
                x++;
            }
        }
        else
        {
            juce::ColourGradient gradient(
                juce::Colours::red.withAlpha(0.75f), 0.0f, plotRect.getY(),
                juce::Colours::green.withAlpha(0.75f), 0.0f, plotRect.getBottom(),
                false);
            gradient.addColour(0.5, juce::Colours::yellow.withAlpha(0.75f));
            g.setGradientFill(gradient);

            auto y = mapY(yValues[0]);
            g.drawLine(x, plotRect.getCentreY(), x, y);
            path.startNewSubPath(x, y);
            for (int i = 1; i < plotRect.getWidth(); i++)
            {
                x++;
                y = mapY(yValues[static_cast<size_t>(i)]);
                g.drawLine(x, plotRect.getCentreY(), x, y);
                path.lineTo(x, y);
            }
        }
        g.setColour(chartColour);
        g.strokePath(path, strokeType);
    }
}

void PhaseFrequencyResponseViewer::setZoomButtonsBounds()
{
    singleRect = reducedBounds;
    singleRect.removeFromTop(buttonsHeight + padding);
    topRect = singleRect;
    bottomRect = topRect.removeFromBottom((singleRect.getHeight() - distanceBetweenRects)/ 2);
    topRect.removeFromBottom(distanceBetweenRects);

    const bool areBoth = bothButton.getToggleState();
    frequencyRect = areBoth ? topRect : singleRect;
    phaseRect = areBoth ? bottomRect : singleRect;

    const int freqPlotRectTop = frequencyRect.getY() + plotTitleHeight + plotPaddingTop;
    const int freqPlotRectBottom = frequencyRect.getBottom() - padding - plotPaddingBottom;
    const int freqPlotCentreY = (freqPlotRectTop + freqPlotRectBottom) / 2;

    zoomOutButton.setBounds(30, freqPlotCentreY + distanceBetweenRects / 2, buttonsHeight, buttonsHeight);
    zoomInButton.setBounds(30, freqPlotCentreY - distanceBetweenRects / 2 - buttonsHeight, buttonsHeight, buttonsHeight);
}

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */

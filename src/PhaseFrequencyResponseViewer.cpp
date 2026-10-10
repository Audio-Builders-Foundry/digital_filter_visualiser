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
    freqButton.setBounds(padding, padding, plotButtonsWidth, zoomButtonsSize);
    phaseButton.setBounds(freqButton.getRight() + padding, padding, plotButtonsWidth, zoomButtonsSize);
    bothButton.setBounds(phaseButton.getRight() + padding, padding, plotButtonsWidth, zoomButtonsSize);

    logScaleButton.setBounds(getWidth() - padding - scaleButtonsWidth, padding, scaleButtonsWidth, zoomButtonsSize);
    linearScaleButton.setBounds(logScaleButton.getX() - padding - scaleButtonsWidth, padding, scaleButtonsWidth, zoomButtonsSize);
    setZoomButtonsBounds();

    repaint();
}

void PhaseFrequencyResponseViewer::paint(juce::Graphics& g)
{
    //PROFILE_FUNCTION();

    g.fillAll(findColour(DFVLookAndFeel::plotBackgroundColourId));

    const auto width = getWidth() - plotPaddingLeft - plotPaddingRight;
    const auto height = getHeight() - plotPaddingTop - plotPaddingBottom;

    int topFreq = plotPaddingTop;
    int topPhase = topFreq;
    int bottomFreq = plotPaddingTop + height;
    int bottomPhase = bottomFreq;

    if (bothButton.getToggleState())
    {
        bottomFreq = plotPaddingTop + (height - distanceBetweenPlots) / 2;
        topPhase = plotPaddingTop + (height + distanceBetweenPlots) / 2;
    }

    if (width <= 0 || bottomFreq <= topFreq || bottomPhase <= topPhase)
        return;

    // angles, amplitudes & phases
    const bool isLogScale = logScaleButton.getToggleState();
    std::vector<double> angles, amplitudes, phases;
    PhaseFrequencyResponseCalculator::calculate(
        processor->filterState.get(),
        minFreq,
        sampleRate,
        isLogScale, true, width, angles, amplitudes, phases);

    if (!phaseButton.getToggleState())
        paintPlot(g, amplitudes, isLogScale, ampDb, " dB", topFreq, bottomFreq, false);
    if (!freqButton.getToggleState())
        paintPlot(g, phases, isLogScale, 180., " deg.", topPhase, bottomPhase, true);
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
    juce::String unitText,
    int top,
    int bottom,
    bool areValuesAngular)
{
    const auto axesColour = findColour(DFVLookAndFeel::plotAxesColourId);
    const auto gridColour = findColour(DFVLookAndFeel::plotGridColourId);
    const auto chartColour = findColour(DFVLookAndFeel::plotChartColourId);

    const auto width = getWidth() - plotPaddingLeft - plotPaddingRight;
    const auto height = bottom - top;

    const float xLeft = static_cast<float>(plotPaddingLeft);
    const float xRight = static_cast<float>(xLeft + width);
    const float yTop = static_cast<float>(top);
    const float yBottom = static_cast<float>(yTop + height);

    juce::Rectangle<int> rect(plotPaddingLeft, top, width, height);
    //g.setColour(gridColour);
    //g.drawRect(rect, 1);

    // Y grid
    g.setColour(axesColour);
    g.drawText(
        juce::String(static_cast<int>(yAmplitude)) + unitText,
        padding,
        top - textHeight / 2,
        plotPaddingLeft - 3 * padding,
        textHeight,
        juce::Justification::centredRight);
    g.drawText(
        "0",
        padding,
        top + (height - textHeight) / 2,
        plotPaddingLeft - 3 * padding,
        textHeight,
        juce::Justification::centredRight);
    g.drawText(
        juce::String(static_cast<int>(-yAmplitude)) + unitText,
        padding,
        top + height - textHeight / 2,
        plotPaddingLeft - 3 * padding,
        textHeight,
        juce::Justification::centredRight);
    g.drawLine(xLeft, bottom, xRight, bottom);
    g.setColour(gridColour);
    int y = top + height / 2;
    g.drawLine(xLeft, y, xRight, y);

    // X grid
    const int hzTextY = static_cast<int>(yBottom) + padding;
    g.setColour(axesColour);
    g.drawLine(plotPaddingLeft, top, plotPaddingLeft, top + height);
    if (isLogScale)
    {
        g.setColour(axesColour);
        g.drawText(
            juce::String(static_cast<int>(minFreq)),
            plotPaddingLeft - textWidth / 2,
            hzTextY,
            textWidth,
            textHeight,
            juce::Justification::centred);
        g.drawText(
            juce::String(static_cast<int>(sampleRate / 2)),
            static_cast<int>(xRight) - textWidth / 2,
            hzTextY,
            textWidth,
            textHeight,
            juce::Justification::centred);
        g.drawText(
            " Hz",
            static_cast<int>(xRight) + textWidth / 2,
            hzTextY,
            textWidth,
            textHeight,
            juce::Justification::centredLeft);

        const float maxFreq = static_cast<float>(sampleRate / 2.0);
        const float logMaxFreq = std::log10(maxFreq);
        const float logMinFreq = std::log10(minFreq);
        const float freqCoeff = width / (logMaxFreq - logMinFreq);

        int freq = static_cast<int>(minFreq * 10);
        while (freq < maxFreq)
        {
            int x = static_cast<int>(freqCoeff * (std::log10(freq) - logMinFreq) + plotPaddingLeft);
            g.setColour(gridColour);
            g.drawLine(x, yTop, x, yBottom);
            if (xRight - x > textWidth)
            {
                g.setColour(axesColour);
                g.drawText(
                    juce::String(freq),
                    x - textWidth / 2,
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
            plotPaddingLeft - textWidth / 2,
            hzTextY,
            textWidth,
            textHeight,
            juce::Justification::centred);
        g.drawText(
            juce::exactlyEqual(sampleRate, 0.0) ? "pi/2" : juce::String(static_cast<int>(sampleRate / 4)),
            plotPaddingLeft + (width - textWidth) / 2,
            hzTextY,
            textWidth,
            textHeight,
            juce::Justification::centred);
        g.drawText(
            juce::exactlyEqual(sampleRate, 0.0) ? "pi" : juce::String(static_cast<int>(sampleRate / 2)),
            static_cast<int>(xRight) - textWidth / 2,
            hzTextY,
            textWidth,
            textHeight,
            juce::Justification::centred);
        if (!juce::exactlyEqual(sampleRate, 0.0))
            g.drawText(
                " Hz",
                static_cast<int>(xRight) + textWidth / 2,
                hzTextY,
                textWidth,
                textHeight,
                juce::Justification::centredLeft);

        g.setColour(gridColour);
        int x = plotPaddingLeft + width / 2;
        g.drawLine(x, yTop, x, yBottom);
    }

    // drawing values
    {
        // clipping inside curly brackets
        const juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(rect);

        const float period = 2 * yAmplitude;
        juce::Path path;
        auto x = xLeft;

        auto wrapPhase = [&](double value)
            {
                double result = std::fmod(value + yAmplitude, period);
                return result < 0.0 ? result + yAmplitude : result - yAmplitude;
            };

        auto mapY = [&](double value)
            {
                return juce::jmap(static_cast<float>(value), -yAmplitude, yAmplitude, yBottom, yTop);
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
        if (areValuesAngular)
        {
            path.startNewSubPath(x, mapY(wrapPhase(yValues[0])));
            for (int i = 1; i < width; i++)
            {
                drawPhaseSegment(x, x + 1, yValues[static_cast<size_t>(i - 1)], yValues[static_cast<size_t>(i)]);
                x++;
            }
        }
        else
        {
            juce::ColourGradient gradient(
                juce::Colours::red.withAlpha(0.75f), 0.0f, rect.getY(),
                juce::Colours::green.withAlpha(0.75f), 0.0f, rect.getBottom(),
                false);
            gradient.addColour(0.5, juce::Colours::yellow.withAlpha(0.75f));
            g.setGradientFill(gradient);

            auto y = mapY(yValues[0]);
            g.drawLine(x, rect.getCentreY(), x, y);
            path.startNewSubPath(x, y);
            for (int i = 1; i < width; i++)
            {
                x++;
                y = mapY(yValues[static_cast<size_t>(i)]);
                g.drawLine(x, rect.getCentreY(), x, y);
                path.lineTo(x, y);
            }
        }
        g.setColour(chartColour);
        g.strokePath(path, strokeType);
        //g.restoreState();
    }
}

void PhaseFrequencyResponseViewer::setZoomButtonsBounds()
{
    const auto height = getHeight() - plotPaddingTop - plotPaddingBottom;
    const int bottomFreq =
        bothButton.getToggleState() ? 
        plotPaddingTop + (height - distanceBetweenPlots) / 2 :
        plotPaddingTop + height;
    const int plotHeight = bottomFreq - plotPaddingTop;
    zoomOutButton.setBounds(20, plotPaddingTop + (plotHeight + padding) / 2, zoomButtonsSize, zoomButtonsSize);
    zoomInButton.setBounds(20, plotPaddingTop + (plotHeight - padding) / 2 - zoomButtonsSize, zoomButtonsSize, zoomButtonsSize);
}

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */

#pragma once

class PhaseFrequencyResponseViewer final :
    public DFVComponent,
    juce::ChangeListener
{
public:
    PhaseFrequencyResponseViewer(AudioPluginAudioProcessor* p);
    ~PhaseFrequencyResponseViewer();
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    void changePlotsSet();
    void paintPlot(
        juce::Graphics& g,
        std::vector<double>& yValues,
        bool isLogScale,
        float yAmplitude,
        juce::Rectangle<int> rect,
        bool isPhase);
    void setZoomButtonsBounds();

    const juce::PathStrokeType strokeType{ 3.f };
    const int
        distanceBetweenRects = 4,
        plotPaddingLeft = 60,
        plotPaddingRight = 40,
        plotPaddingTop = 10,
        plotPaddingBottom = 25,
        buttonsHeight = 20,
        plotButtonsWidth = 45,
        scaleButtonsWidth = 45,
        textWidth = 40,
        textHeight = 10,
        plotTitleHeight = 25;
    const float
        minAmpDb = 6.f,
        maxAmpDb = 96.f,
        minFreq = 20.f;

    AudioPluginAudioProcessor* processor;

    float ampDb;
    double sampleRate;

    juce::Rectangle<int> singleRect, topRect, bottomRect, frequencyRect, phaseRect;
    juce::TextButton
        zoomInButton, zoomOutButton,
        linearScaleButton, logScaleButton,
        freqButton, phaseButton, bothButton;
};

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */

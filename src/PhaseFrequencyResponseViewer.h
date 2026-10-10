#pragma once

class PhaseFrequencyResponseViewer final :
    public juce::Component,
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
        juce::String unitText,
        int top,
        int bottom,
        bool areValuesAngular);
    void setZoomButtonsBounds();

    const juce::PathStrokeType strokeType{ 3.f };
    const int
        padding = 5,
        distanceBetweenPlots = 30,
        plotPaddingLeft = 72,
        plotPaddingRight = 45,
        plotPaddingTop = 65,
        plotPaddingBottom = 20;
    const int
        zoomButtonsSize = 20,
        plotButtonsWidth = 45,
        scaleButtonsWidth = 45,
        textWidth = 40,
        textHeight = 10;
    const float
        minAmpDb = 6.f,
        maxAmpDb = 96.f,
        minFreq = 20.f;

	AudioPluginAudioProcessor* processor;

    float ampDb;
    double sampleRate;

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

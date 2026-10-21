#include "PluginEditor.h"
#include "ComplexPlaneEditor.cpp"
#include "ComplexPlaneComponent.cpp"
#include "PhaseFrequencyResponseCalculator.cpp"
#include "PhaseFrequencyResponseViewer.cpp"
#include "CoeffComponents.cpp"
#include "StateSerializer.cpp"
#include "PlayerComponent.cpp"
#include "ButtonPanel.cpp"
#include "EquationViewer.cpp"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
  :AudioProcessorEditor (&p)
  ,complexPlaneComponent(&p)
  ,phaseFrequencyResponseViewer(&p)
  ,coefficients(&p)
  ,buttonPanel(p)
  ,equationViewer(&p)
  ,tooltipWindow(this)
{
  juce::LookAndFeel::setDefaultLookAndFeel(&lnf);
  buttonPanel.updateColours();

  addAndMakeVisible(complexPlaneComponent);
  addAndMakeVisible(coefficients);
  addAndMakeVisible(phaseFrequencyResponseViewer);
  addAndMakeVisible(buttonPanel);
  addAndMakeVisible(equationViewer);

  setSize(960, 640);
  setResizable(true, true);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
  const float cornerSize = 20.f;
  const float rectCornerSize = 3.f;
  const float lineThickness = 1.5f;
  auto bounds = getLocalBounds().toFloat();
  const float w = bounds.getWidth();
  const float h = bounds.getHeight();
  g.fillAll(findColour(juce::ResizableWindow::ColourIds::backgroundColourId));

  bounds.reduce(0.01f * w, 0.01f * h);
  g.setColour(findColour(DFVLookAndFeel::controlBackgroundColourId));
  g.fillRoundedRectangle(bounds, cornerSize);
  g.setColour(findColour(DFVLookAndFeel::controlOutlineColourId));
  g.drawRoundedRectangle(bounds, cornerSize, lineThickness);

  bounds.reduce(0.01f * w, 0);
  bounds.removeFromBottom(0.02f * h);
  const auto titleBounds = bounds.removeFromTop(0.06f * h);
  g.setColour(findColour(DFVLookAndFeel::titleTextColourId));
  g.setFont(std::clamp(titleBounds.getHeight() - 2.f, 2.f, 18.f));
  g.drawFittedText("DIGITAL FILTER VISUALIZER", titleBounds.toNearestInt(), juce::Justification::centred, 1);

  const auto rect1 = bounds.removeFromTop(0.03f * h);
  const auto rect2 = bounds.removeFromBottom(0.03f * h).reduced(10.f, 0.f);
  g.setColour(findColour(DFVLookAndFeel::controlHeaderBackgroundColourId));
  g.fillRoundedRectangle(rect1, rectCornerSize);
  g.fillRoundedRectangle(rect2, rectCornerSize);
  g.setColour(findColour(DFVLookAndFeel::controlHeaderOutlineColourId));
  g.drawRoundedRectangle(rect1, rectCornerSize, lineThickness);
  g.drawRoundedRectangle(rect2, rectCornerSize, lineThickness);
}

void AudioPluginAudioProcessorEditor::resized()
{
  buttonPanel.setBoundsRelative(0.02f, 0.12f, 0.96f, 0.07f);
  equationViewer.setBoundsRelative(0.02f, 0.205f, 0.96f, 0.165f);
  coefficients.setBoundsRelative(0.02f, 0.385f, 0.26f, 0.535f);
  complexPlaneComponent.setBoundsRelative(0.29f, 0.385f, 0.37f, 0.535f);
  phaseFrequencyResponseViewer.setBoundsRelative(0.67f, 0.385f, 0.31f, 0.535f);
}

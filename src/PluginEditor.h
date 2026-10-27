#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "DFVLookAndFeel.h"
#include "FilterState.h"

#include "ComplexPlaneComponent.h"
#include "PhaseFrequencyResponseViewer.h"
#include "CoeffComponents.h"
#include "ButtonPanel.h"
#include "EquationViewer.h"
#include "StateSerializer.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
  explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
  ~AudioPluginAudioProcessorEditor() override;

  //==============================================================================
  void paint (juce::Graphics&) override;
  void resized() override;

private:
  ComplexPlaneComponent complexPlaneComponent;
  PhaseFrequencyResponseViewer phaseFrequencyResponseViewer;
  CoefficientsComponent coefficients;
  ButtonPanel buttonPanel;
  EquationViewer equationViewer;
  juce::TooltipWindow tooltipWindow;
  DFVLookAndFeel lnf;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};

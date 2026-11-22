#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "DFVLookAndFeel.h"
#include "FilterState.h"

#include "ComplexPlaneComponent.h"
#include "FrequencyResponseViewer.h"
#include "CoeffComponents.h"
#include "ButtonPanel.h"
#include "EquationViewer.h"
#include "StateSerializer.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final 
	: public juce::AudioProcessorEditor
	, private juce::KeyListener

{
public:
  explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
  ~AudioPluginAudioProcessorEditor() override;

  //==============================================================================
  void paint (juce::Graphics&) override;
  void resized() override;
  void parentHierarchyChanged() override;

private:
  ComplexPlaneComponent complexPlaneComponent;
  FrequencyResponseViewer frequencyResponseViewer;
  CoefficientsComponent coefficients;
  ButtonPanel buttonPanel;
  EquationViewer equationViewer;
  juce::TooltipWindow tooltipWindow;
  DFVLookAndFeel lnf;

  // parent component of AudioPluginAudioProcessorEditor
  juce::Component::SafePointer<juce::Component> topLevelComponent;
  
  bool keyPressed(const juce::KeyPress& key, juce::Component*) override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};

#pragma once
#include "PluginProcessor.h"
#include "PlayerComponent.h"

class ButtonPanel final
  :public juce::Component
  ,private juce::ValueTree::Listener
{
public:
    ButtonPanel(AudioPluginAudioProcessor& p);
    ~ButtonPanel();
    void resized() override;
    void updateSliderTextBoxColours();

private:
    void valueTreePropertyChanged(juce::ValueTree &node, juce::Identifier const &property) override;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;
    juce::PopupMenu exportPopupMenu;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image saveImage, undoImage, redoImage, addImage, delImage;
    juce::ImageButton exportButton;
    juce::ImageButton undoButton;
    juce::ImageButton redoButton;
    juce::ImageButton addRootButton;
    juce::ImageButton delRootButton;
    PlayerComponent player;
    juce::Slider gainSlider;

    void chooseFileAndSave(std::shared_ptr<juce::XmlElement> xml);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonPanel)
};

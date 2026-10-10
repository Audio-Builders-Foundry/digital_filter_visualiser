#include "ButtonPanel.h"
#include <juce_graphics/juce_graphics.h>
#include <BinaryData.h>
#include "Utilities.h"
#include "StateSerializer.h"

ButtonPanel::ButtonPanel(AudioPluginAudioProcessor& p)
  :processorRef(p)
  ,player(p)
{
    saveImage = juce::ImageCache::getFromMemory(BinaryData::Save_png, BinaryData::Save_pngSize);
    undoImage = juce::ImageCache::getFromMemory(BinaryData::Undo_png, BinaryData::Undo_pngSize);
    redoImage = juce::ImageCache::getFromMemory(BinaryData::Redo_png, BinaryData::Redo_pngSize);
    addImage = juce::ImageCache::getFromMemory(BinaryData::Add_png, BinaryData::Add_pngSize);
    delImage = juce::ImageCache::getFromMemory(BinaryData::Delete_png, BinaryData::Delete_pngSize);

    Utilities::setButtonImage(&exportButton, saveImage);
    Utilities::setButtonImage(&undoButton, undoImage);
    Utilities::setButtonImage(&redoButton, redoImage);
    Utilities::setButtonImage(&addRootButton, addImage);
    Utilities::setButtonImage(&delRootButton, delImage);

    exportButton.setTooltip("Export");
    undoButton.setTooltip("Undo");
    redoButton.setTooltip("Redo");
    addRootButton.setTooltip("Add root");
    delRootButton.setTooltip("Delete root");

    exportPopupMenu.addItem("Filter coefficients", [this]
        {
            chooseFileAndSave(StateSerializer::exportCoefficients(this->processorRef.filterState.get()));
        });
    exportPopupMenu.addItem("Processor chain parameters", [this]
        {
            chooseFileAndSave(StateSerializer::exportProcessorChainParameters(this->processorRef.activeState.load()));
        });
    exportButton.onClick = [this] { exportPopupMenu.showMenuAsync({}); };

    undoButton.onClick = [this]{
      processorRef.filterState->um->undo();
      jassert(processorRef.filterState->totalOrder >= processorRef.filterState->finiteZerosOrder);
    };
    redoButton.onClick = [this]{
      processorRef.filterState->um->redo();
      jassert(processorRef.filterState->totalOrder >= processorRef.filterState->finiteZerosOrder);
    };
    addRootButton.onClick = [this]{
      processorRef.filterState->um->beginNewTransaction();
      processorRef.filterState->add(1);
    };

    // NOTE(ry): gain slider setup
    gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gainSlider.setRange(-90.f, 6.f, 0.01f);
    gainSlider.setTextValueSuffix(" dB");
    gainSlider.onValueChange = [this]{
      auto const dB = gainSlider.getValue();
      auto const amp = juce::Decibels::decibelsToGain(dB);
      processorRef.filterState->gain = amp;
    };

    addAndMakeVisible(exportButton);
    addAndMakeVisible(undoButton);
    addAndMakeVisible(redoButton);
    addAndMakeVisible(addRootButton);
    addAndMakeVisible(delRootButton);
    addAndMakeVisible(gainSlider);

    if (processorRef.wrapperType == juce::AudioProcessor::WrapperType::wrapperType_Standalone)
        addAndMakeVisible(player);

    processorRef.filterState->addListener(this);
    processorRef.filterState->syncListener(this);
}

ButtonPanel::~ButtonPanel()
{
    processorRef.filterState->removeListener(this);
}

void ButtonPanel::resized()
{
    constexpr int padding = 5;
    constexpr int maxButtonSize = 30;
    const bool needPlayer =
        processorRef.wrapperType == juce::AudioProcessor::WrapperType::wrapperType_Standalone;
    auto area = getLocalBounds();
    const int height = area.getHeight();
    const int width = area.getWidth();
    const int buttonGroupWidth =
        needPlayer ? (width - 4 * padding) / 3 : (width - 3 * padding) / 2;
    const int buttonSize = 
        std::min((buttonGroupWidth - 4 * padding) / 5, std::min(height, maxButtonSize));
    const int buttonY = (height - buttonSize) / 2;

  //auto constexpr buttonWidth_PercentOfParent = 0.1;
  //auto const buttonWidth = getWidth() * buttonWidth_PercentOfParent;
  undoButton.setBounds(padding, buttonY, buttonSize, buttonSize);
  redoButton.setBounds(2 * padding + buttonSize, buttonY, buttonSize, buttonSize);
  exportButton.setBounds(3 * padding + 2 * buttonSize, buttonY, buttonSize, buttonSize);
  addRootButton.setBounds(4 * padding + 3 * buttonSize, buttonY, buttonSize, buttonSize);
  delRootButton.setBounds(5 * padding + 4 * buttonSize, buttonY, buttonSize, buttonSize);
  
  //undoButton.setBounds(area.removeFromLeft(height).reduced(padding));
  //redoButton.setBounds(area.removeFromLeft(height).reduced(padding));
  //exportButton.setBounds(area.removeFromLeft(height).reduced(padding));
  //addRootButton.setBounds(area.removeFromLeft(height).reduced(padding));
  //delRootButton.setBounds(area.removeFromLeft(height).reduced(padding));
  if (needPlayer)
  {
      player.setBounds(2 * padding + buttonGroupWidth, buttonY, buttonGroupWidth, buttonSize);
      gainSlider.setBounds(3 * padding + 2 * buttonGroupWidth, 0, buttonGroupWidth, height);
  //  area.removeFromLeft(height);
    //player.setBounds(area.removeFromLeft(player.buttonsCount * (height - padding) + padding).reduced(padding));
  //  area.removeFromLeft(height);
  }
  else
    gainSlider.setBounds(2 * padding + buttonGroupWidth, 0, buttonGroupWidth, height);
}

void ButtonPanel::updateSliderTextBoxColours()
{
    gainSlider.setColour(juce::Slider::textBoxBackgroundColourId, findColour(juce::Slider::textBoxBackgroundColourId));
    gainSlider.setColour(juce::Slider::textBoxOutlineColourId, findColour(juce::Slider::textBoxOutlineColourId));
    gainSlider.setColour(juce::Slider::textBoxTextColourId, findColour(juce::Slider::textBoxTextColourId));
    gainSlider.setColour(juce::Slider::textBoxHighlightColourId, findColour(juce::Slider::textBoxHighlightColourId));

}

void ButtonPanel::chooseFileAndSave(std::shared_ptr<juce::XmlElement> xml)
{
    chooser = std::make_unique<juce::FileChooser>(
        "Save to file...", juce::File{}, "*.xml");
    const auto chooserFlags =
        juce::FileBrowserComponent::FileChooserFlags::saveMode |
        juce::FileBrowserComponent::FileChooserFlags::warnAboutOverwriting;
    std::function<void(const juce::FileChooser& fc)> function =
        [this, xml](const juce::FileChooser& fc)
        {
		    juce::ignoreUnused(this);
            try
            {
                juce::File file = fc.getResult();
                if (file != juce::File{})
                    xml->writeTo(file, {});
            }
            catch (const std::exception& e)
            {
                juce::NativeMessageBox::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon, "Error", e.what());
            }
        };
    chooser->launchAsync(chooserFlags, function);
}

void ButtonPanel::valueTreePropertyChanged(juce::ValueTree &node, juce::Identifier const &property)
{
  if(property == IDs::Gain)
  {
    auto const amp = r64(node.getProperty(IDs::Gain));
    auto const dB = juce::Decibels::gainToDecibels(amp);
    gainSlider.setValue(dB, juce::dontSendNotification);
  }
}

#include "ButtonPanel.h"
#include <juce_graphics/juce_graphics.h>
#include <BinaryData.h>
#include "Utilities.h"
#include "StateSerializer.h"

ButtonPanel::ButtonPanel(AudioPluginAudioProcessor& p)
  :processorRef(p)
  ,DFVComponent(Focus::bp) 
  ,player(p)
{
    
    saveImage = juce::ImageCache::getFromMemory(BinaryData::Save_png, BinaryData::Save_pngSize);
    undoImage = juce::ImageCache::getFromMemory(BinaryData::Undo_png, BinaryData::Undo_pngSize);
    redoImage = juce::ImageCache::getFromMemory(BinaryData::Redo_png, BinaryData::Redo_pngSize);
    addImage = juce::ImageCache::getFromMemory(BinaryData::Add_png, BinaryData::Add_pngSize);

    Utilities::setButtonImage(&exportButton, saveImage);
    Utilities::setButtonImage(&undoButton, undoImage);
    Utilities::setButtonImage(&redoButton, redoImage);
    Utilities::setButtonImage(&addRootButton, addImage);

    exportButton.setTooltip("Export");
    undoButton.setTooltip("Undo");
    redoButton.setTooltip("Redo");
    addRootButton.setTooltip("Add root");

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
    addAndMakeVisible(gainSlider);

    if (processorRef.wrapperType == juce::AudioProcessor::WrapperType::wrapperType_Standalone)
        addAndMakeVisible(player);

    processorRef.filterState->addListener(this);
    processorRef.filterState->syncListener(this);

    // add keyboard focus on buttons
    exportButton.setWantsKeyboardFocus(true);
    exportButton.setHasFocusOutline(true);
    undoButton.setWantsKeyboardFocus(true);
    undoButton.setHasFocusOutline(true);
    redoButton.setWantsKeyboardFocus(true);
    redoButton.setHasFocusOutline(true);
    addRootButton.setWantsKeyboardFocus(true);
    addRootButton.setHasFocusOutline(true);

    gainSlider.setWantsKeyboardFocus(true);
    gainSlider.setHasFocusOutline(true);

    undoButton.setTitle(Focus::buttonPanel::undo.title);
    undoButton.setDescription(Focus::buttonPanel::undo.description);
    undoButton.setHelpText(Focus::buttonPanel::undo.helpText); // Should receive the current state? 
    undoButton.setExplicitFocusOrder(Focus::buttonPanel::undo.focusOrder);

    redoButton.setTitle(Focus::buttonPanel::redo.title);
    redoButton.setDescription(Focus::buttonPanel::redo.description);
    redoButton.setHelpText(Focus::buttonPanel::redo.helpText);
    redoButton.setExplicitFocusOrder(Focus::buttonPanel::redo.focusOrder);

    exportButton.setTitle(Focus::buttonPanel::exportb.title);
    exportButton.setDescription(Focus::buttonPanel::exportb.description);
    exportButton.setHelpText(Focus::buttonPanel::exportb.helpText);
    exportButton.setExplicitFocusOrder(Focus::buttonPanel::exportb.focusOrder);

    addRootButton.setTitle(Focus::buttonPanel::addroot.title);
    addRootButton.setDescription(Focus::buttonPanel::addroot.description);
    addRootButton.setHelpText(Focus::buttonPanel::addroot.helpText);
    addRootButton.setExplicitFocusOrder(Focus::buttonPanel::addroot.focusOrder);

    gainSlider.setTitle(Focus::buttonPanel::level.title);
    gainSlider.setDescription(Focus::buttonPanel::level.description);
    gainSlider.setHelpText(Focus::buttonPanel::level.helpText);
    gainSlider.setExplicitFocusOrder(Focus::buttonPanel::level.focusOrder);

}

ButtonPanel::~ButtonPanel()
{
    processorRef.filterState->removeListener(this);
}

void ButtonPanel::resized()
{
    DFVComponent::resized();
    reducedBounds = reducedBounds.reduced(static_cast<int>(outerFrameCornerSize), 0);

    constexpr int padding = 5;
    constexpr int maxButtonSize = 30;
    const bool needPlayer =
        processorRef.wrapperType == juce::AudioProcessor::WrapperType::wrapperType_Standalone;
    const int height = reducedBounds.getHeight();
    const int width = reducedBounds.getWidth();
    const int buttonGroupWidth =
        needPlayer ? (width - 4 * padding) / 3 : (width - 3 * padding) / 2;
    const int buttonSize = 
        std::min((buttonGroupWidth - 4 * padding) / 5, std::min(height, maxButtonSize));
    const int buttonY = reducedBounds.getY() + (height - buttonSize) / 2;

  undoButton.setBounds(reducedBounds.getX(), buttonY, buttonSize, buttonSize);
  redoButton.setBounds(undoButton.getRight() + padding, buttonY, buttonSize, buttonSize);
  addRootButton.setBounds(redoButton.getRight() + padding, buttonY, buttonSize, buttonSize);
  exportButton.setBounds(addRootButton.getRight() + padding, buttonY, buttonSize, buttonSize);

  if (needPlayer)
  {
      player.setBounds(2 * padding + buttonGroupWidth, buttonY, buttonGroupWidth, buttonSize);
      gainSlider.setBounds(3 * padding + 2 * buttonGroupWidth, buttonY, buttonGroupWidth, buttonSize);
  }
  else
    gainSlider.setBounds(2 * padding + buttonGroupWidth, buttonY, buttonGroupWidth, buttonSize);
}

void ButtonPanel::updateColours()
{
    Utilities::updateButtonColours(&exportButton);
    Utilities::updateButtonColours(&undoButton);
    Utilities::updateButtonColours(&redoButton);
    Utilities::updateButtonColours(&addRootButton);
    
    player.updateButtonColours();
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

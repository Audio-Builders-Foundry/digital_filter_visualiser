#include "PlayerComponent.h"
#include <juce_graphics/juce_graphics.h>
#include <BinaryData.h>
#include "Utilities.h"

PlayerComponent::PlayerComponent(AudioPluginAudioProcessor& p)
    : processor(p)
{
    openImage = juce::ImageCache::getFromMemory(BinaryData::Load_png, BinaryData::Load_pngSize);
    playImage = juce::ImageCache::getFromMemory(BinaryData::Play_png, BinaryData::Play_pngSize);
    pauseImage = juce::ImageCache::getFromMemory(BinaryData::Pause_png, BinaryData::Pause_pngSize);
    stopImage = juce::ImageCache::getFromMemory(BinaryData::Stop_png, BinaryData::Stop_pngSize);
    loopImage = juce::ImageCache::getFromMemory(BinaryData::Loop_png, BinaryData::Loop_pngSize);

    addAndMakeVisible(&openButton);
    Utilities::setButtonImage(&openButton, openImage);
    openButton.onClick = [this] { openFile(); };

    addAndMakeVisible(&playButton);
    playButton.onClick = [this] { playPause(); };

    addAndMakeVisible(&stopButton);
    Utilities::setButtonImage(&stopButton, stopImage);
    stopButton.onClick = [this] { stop(); };

    addAndMakeVisible(&loopButton);
    Utilities::setButtonImage(&loopButton, loopImage);
    loopButton.setClickingTogglesState(true);
    loopButton.onClick = [this] { updateLoopStatus(); };

    this->processor.playerState.addChangeListener(this);
    changeListenerCallback(&this->processor.playerState);
}

PlayerComponent::~PlayerComponent()
{
    this->processor.playerState.removeChangeListener(this);
}

void PlayerComponent::resized()
{
    constexpr int padding = 5;
    const int height = getHeight();
    const int width = getWidth();
    const int buttonSize = std::clamp((width - 5 * padding) / 4, 1, height);
    const int x = (width - 3 * padding - 4 * buttonSize) / 2;
    const int y = (height - buttonSize) / 2;

    openButton.setBounds(x, y, buttonSize, buttonSize);
    playButton.setBounds(x + buttonSize + padding, y, buttonSize, buttonSize);
    stopButton.setBounds(x + 2 * (buttonSize + padding), y, buttonSize, buttonSize);
    loopButton.setBounds(x + 3 * (buttonSize + padding), y, buttonSize, buttonSize);
}

void PlayerComponent::updateButtons(PlayerState playerState)
{
    openButton.setEnabled(playerState == PlayerState::Empty || playerState == PlayerState::Stopped);
    playButton.setEnabled(playerState != PlayerState::Empty);
    stopButton.setEnabled(playerState == PlayerState::Playing || playerState == PlayerState::Paused);
    loopButton.setEnabled(true);
    Utilities::setButtonImage(&playButton, playerState == PlayerState::Playing ? pauseImage : playImage);
}

void PlayerComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &processor.playerState)
        updateButtons(processor.playerState.get());
}

void PlayerComponent::openFile()
{
    chooser = std::make_unique<juce::FileChooser>("Open", juce::File{}, filePatterns);
    auto chooserFlags = 
        juce::FileBrowserComponent::FileChooserFlags::openMode |
        juce::FileBrowserComponent::FileChooserFlags::canSelectFiles;
    chooser->launchAsync(
        chooserFlags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.exists())
            {
                processor.setTransportSourceFromFile(file);
                updateLoopStatus();
            }
        });
}

void PlayerComponent::playPause()
{
    if (processor.playerState.get() == PlayerState::Playing)
    {
        processor.transportSource.stop();
        processor.playerState.set(PlayerState::Paused);
    }
    else
    {
        processor.transportSource.start();
        processor.playerState.set(PlayerState::Playing);
    }
}

void PlayerComponent::stop()
{
    processor.transportSource.stop();
    processor.transportSource.setPosition(0);
    processor.playerState.set(PlayerState::Stopped);
}

void PlayerComponent::updateLoopStatus()
{
    auto* src = processor.readerSource.get();
    if (src != nullptr)
        src->setLooping(loopButton.getToggleState());
}

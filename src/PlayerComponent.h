#pragma once
#include <juce_core/system/juce_PlatformDefs.h>
#include <juce_events/juce_events.h>
#include "PluginProcessor.h"

class PlayerComponent 
	: public juce::Component
	, public juce::ChangeListener
{
public:
	PlayerComponent(AudioPluginAudioProcessor& p);
	~PlayerComponent();
	void resized() override;
	void updateButtons(PlayerState playerState);
	void changeListenerCallback(juce::ChangeBroadcaster* source) override;
	
private:

#if JUCE_WINDOWS
	const juce::String filePatterns = "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3;*.wma";
#elif JUCE_MAC
	const juce::String filePatterns = "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3;*.m4a";
#else 
	const juce::String filePatterns = "*.wav;*.aif;*.aiff;*.flac;*.ogg";
#endif

	void openFile();
	void playPause();
	void stop();
	void updateLoopStatus();

	juce::ImageButton openButton, playButton, stopButton, loopButton;
	juce::Image openImage, playImage, pauseImage, stopImage, loopImage;

	AudioPluginAudioProcessor& processor;
	std::unique_ptr<juce::FileChooser> chooser;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerComponent)
};

#define IMPLEMENTOR_UNIT
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "FilterState.cpp"
#include "RootsToCoefficients.cpp"
#include "ProcessorChainModifier.cpp"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
  : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                    .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                    .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    apvts(*this, &um),
    activeState(new FullState<SampleType>),
    pendingState(new FullState<SampleType>),
    lastProcessTime(0),
	playerState(PlayerState::Empty)
{
    if (!apvts.state.isValid())
    {
        apvts.state = juce::ValueTree(IDs::FilterState);
    }
    filterState = std::make_unique<FilterState>(apvts.state, &um);
    um.addChangeListener(this);
    transportSource.addChangeListener(this);
    formatManager.registerBasicFormats();
}


AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    um.removeChangeListener(this);
    transportSource.removeChangeListener(this);

    if (auto* activePtr = activeState.exchange(nullptr))
    {
        delete activePtr;
    }

    if (pendingState)
    {
        delete pendingState;
        pendingState = nullptr;
    }

	// NOTE(ry): print each profile site's maximum runtime
	for(u32 siteIdx = 0; siteIdx < Profiler::siteCount; ++siteIdx)
	{
		auto *profSite = Profiler::sites + siteIdx;
		if(!profSite->label.empty())
		{
			r64 siteMaxTscElapsed_us = 1e6 * (r64)profSite->tscElapsedMax / (r64)Profiler::tscFreq;
			DBG(profSite->label.data() << " elapsed: " << siteMaxTscElapsed_us << " us");
		}
	}
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
  return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
  return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
  // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
  return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
  juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
  juce::ignoreUnused (index);
  return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
  juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    crossFadeBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);

    juce::dsp::ProcessSpec newSpec
    {
        sampleRate,
        juce::uint32(samplesPerBlock),
        juce::uint32(1) // one channel per filter!
    };
    spec = newSpec;
    auto numChannels = getTotalNumOutputChannels();

    if (auto activeProc = activeState.load())
        if (pendingState != nullptr)
        {
            activeProc->clear(true);
            pendingState->clear(true);

            for (auto i = 0; i < numChannels; i++)
            {
                auto* activeItem = new ProcessorChain<SampleType>;
                activeItem->prepare(spec);
                activeProc->add(activeItem);
                auto* pendingItem = new ProcessorChain<SampleType>;
                pendingItem->prepare(spec);
                pendingState->add(pendingItem);
            }

            ProcessorChainModifier::rootsToJuceCoeffs(filterState.get(), activeState.load(), spec);
        }

    transportSource.prepareToPlay(samplesPerBlock, sampleRate);
    if (resamplerSource != nullptr && readerSource != nullptr)
    {
        double ratio = readerSource->getAudioFormatReader()->sampleRate / sampleRate;
        resamplerSource->setResamplingRatio(ratio);
        resamplerSource->prepareToPlay(samplesPerBlock, sampleRate);
    }

    isPrepared = true;
    sendChangeMessage();
}

void AudioPluginAudioProcessor::releaseResources()
{
  transportSource.releaseResources();
  if (resamplerSource != nullptr)
    resamplerSource->releaseResources();
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused (layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
      && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<SampleType>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
	PROFILE_FUNCTION();
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    if (wrapperType == wrapperType_Standalone)
    {
        buffer.clear();
        juce::AudioSourceChannelInfo info(buffer);
        if (transportSource.isPlaying())
        {
            if (juce::approximatelyEqual(resamplerSource->getResamplingRatio(), 1.0))
                transportSource.getNextAudioBlock(info);
            else
                resamplerSource->getNextAudioBlock(info);
        }
    }

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(static_cast<int>(i), 0, numSamples);

    if (isPendingStateReady.load())
    {
		PROFILE_SCOPE("new state is ready");

        auto* activeProc = activeState.load();
        juce::dsp::AudioBlock<SampleType> block(buffer);
        juce::dsp::AudioBlock<SampleType> crossFadeBlock(crossFadeBuffer);

        for (int ch = 0; ch < totalNumInputChannels; ch++)
        {
            crossFadeBuffer.copyFrom(ch, 0, buffer.getReadPointer(ch), numSamples);

            auto oldBlock = block.getSingleChannelBlock(static_cast<size_t>(ch));
            juce::dsp::ProcessContextReplacing<SampleType> oldContext(oldBlock);
            activeProc->getUnchecked(ch)->process(oldContext);

            auto newBlock = crossFadeBlock.getSingleChannelBlock(static_cast<size_t>(ch));
            juce::dsp::ProcessContextReplacing<SampleType> newContext(newBlock);
            pendingState->getUnchecked(ch)->process(newContext);

            buffer.applyGainRamp(ch, 0, numSamples, 1.0f, 0.0f);
            buffer.addFromWithRamp(ch, 0, crossFadeBuffer.getReadPointer(ch), numSamples, 0.0f, 1.0f);
        }

        auto* old = activeState.exchange(pendingState);
        pendingState = old;
        isPendingStateReady.store(false);
    }
    else
    {
		PROFILE_SCOPE("new state is not ready");

        auto* proc = activeState.load();
        juce::dsp::AudioBlock<SampleType> block(buffer);

        for (int ch = 0; ch < totalNumInputChannels; ch++)
        {
            auto channelBlock = block.getSingleChannelBlock(static_cast<size_t>(ch));
            juce::dsp::ProcessContextReplacing<SampleType> context(channelBlock);
            proc->getUnchecked(ch)->process(context);
        }
    }

    lastProcessTime.store(juce::Time::getApproximateMillisecondCounter());
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
  return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
  return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (tree.isValid())
    {
        apvts.replaceState(tree);

        filterState = std::make_unique<FilterState>(apvts.state, &um);
		FilterState::listeners.call([&](juce::ValueTree::Listener &l){
			filterState->treeRoot.addListener(&l);
			filterState->syncListener(&l);
		});

        ProcessorChainModifier::process(*this);
    }
}

void AudioPluginAudioProcessor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &um)
    {
        ProcessorChainModifier::process(*this);
    }
    else if (source == &transportSource)
    {
        if (transportSource.hasStreamFinished())
        {
            transportSource.setPosition(0);
            playerState.set(PlayerState::Stopped);
        }
    }
}

void AudioPluginAudioProcessor::setTransportSourceFromFile(juce::File file)
{
    auto* reader = formatManager.createReaderFor(file);
    if (reader != nullptr)
    {
        transportSource.stop();
        transportSource.setSource(nullptr);
        resamplerSource.reset();

        readerSource.reset(new juce::AudioFormatReaderSource(reader, true));
        readerSource->prepareToPlay(getBlockSize(), getSampleRate());

        transportSource.setSource(readerSource.get(), 0, nullptr, reader->sampleRate);

        double ratio = reader->sampleRate / getSampleRate();
        resamplerSource.reset(new juce::ResamplingAudioSource(&transportSource, false, getTotalNumOutputChannels()));
        resamplerSource->setResamplingRatio(ratio);
        resamplerSource->prepareToPlay(getBlockSize(), getSampleRate());

        playerState.set(PlayerState::Stopped);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new AudioPluginAudioProcessor();
}

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* buffer-file-coding-system: undecided-unix */
/* End: */

#include "ProcessorChainModifier.h"
#include "RootsToCoefficients.h"

void ProcessorChainModifier::rootsToJuceCoeffs(
	FilterState* state,
	FullState<float>* processorState,
	juce::dsp::ProcessSpec& spec)
{
	// It is proposed that the invariant "total zero order <= total pole order" is maintained
	// and there cannot be zeros at zero and poles at zero simultaneously
	// as they will be annihilated immedialely.
	// That's why zeros at zero (if exist) will always "find" their poles to form biquad.

	const int channelSize = processorState->size();
	if (channelSize == 0)
		return;

	const auto polesSize = static_cast<std::size_t>(state->poles.size());
	const auto zerosSize = static_cast<std::size_t>(state->zeros.size());

	// 1. Sort non-null poles by priority for pairing with zeros;
	// calculate delay count.
	int delayCount = 0;
	std::vector<PolePeak> polePeaks;
	polePeaks.reserve(polesSize);

	for (std::size_t i = 0; i < polesSize; i++)
	{
		auto* pole = state->poles[static_cast<int>(i)];
		if (!pole->isAtZero())
			polePeaks.push_back(calculatePeak(pole, i));
		delayCount -= pole->order.get() * (pole->isReal() ? 1 : 2);
	}
	for (std::size_t i = 0; i < zerosSize; i++)
	{
		auto* zero = state->zeros[static_cast<int>(i)];
		delayCount -= zero->order.get() * (zero->isReal() ? 1 : 2);
	}
	jassert(delayCount >= 0);

	std::sort(polePeaks.begin(), polePeaks.end(),
		[](const PolePeak a, const PolePeak b)
		{
			return a.gain > b.gain;
		});
	const auto nonNullPolesSize = polePeaks.size();

	// 2. Find the best zero pair for each pole
	// and calculate IIR coefficients
	std::vector<int> usedZeros(zerosSize, false);
	std::vector<juce::dsp::IIR::Coefficients<float>*> iirCoeffs;

	int bestZeroIndex = -1;
	for (std::size_t i = 0; i < nonNullPolesSize; i++)
	{
		auto* pole = state->poles[static_cast<int>(polePeaks[i].index)];
		const int poleOrder = std::abs(pole->order.get());

		for (int j = 0; j < poleOrder; j++)
		{
			bestZeroIndex = findBestZeroIndexPairForPole(
				pole,
				polePeaks[i].frequency,
				state->zeros,
				usedZeros,
				poleOrder - j > 1);

			const auto* zero = bestZeroIndex == -1 ? nullptr : state->zeros[bestZeroIndex];
			const int unusedZeroOrder =
				zero == nullptr ?
				0 :
				zero->order.get() - usedZeros[static_cast<std::size_t>(bestZeroIndex)];
			const bool shouldEqualPoleBeTaken =
				pole->isReal() &&
				poleOrder - j > 1 &&
				(zero == nullptr ||
					!state->zeros[bestZeroIndex]->isReal() ||
					unusedZeroOrder > 1);
			const bool shouldEqualZeroBeTaken =
				zero != nullptr &&
				zero->isReal() &&
				pole->isReal() &&
				poleOrder - j > 1 &&
				unusedZeroOrder > 1;

			iirCoeffs.push_back(calculateIirCoefficients(
				pole,
				state->zeros,
				bestZeroIndex,
				shouldEqualPoleBeTaken,
				shouldEqualZeroBeTaken));

			if (shouldEqualPoleBeTaken)
				j++;
			if (zero != nullptr)
			{
				const int zeroOrder = shouldEqualZeroBeTaken ? 2 : 1;
				usedZeros[static_cast<std::size_t>(bestZeroIndex)] += zeroOrder;
			}
		}
	}
	const auto iirFiltersSize = iirCoeffs.size();

	// 3. Calculate FIR coefficients for non-paired zeros.
	std::vector<double> firCoeffsDblArray =
		RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->zeros, 1, &usedZeros);
	std::size_t firCoeffArraySize = firCoeffsDblArray.size();
	std::vector<float> firCoeffsArray(firCoeffArraySize);
	for (std::size_t i = 0; i < firCoeffArraySize; i++)
		firCoeffsArray[i] = static_cast<float>(firCoeffsDblArray[i]);

	// 4. Set processors parameters
	for (int ch = 0; ch < channelSize; ch++)
	{
		auto* proc = (*processorState)[ch];

		// 4a. Delay
		auto& delay = proc->delay;
		if (delay.getMaximumDelayInSamples() < delayCount)
			delay.setMaximumDelayInSamples(delayCount);
		delay.setDelay(delayCount);
		delay.prepare(spec);

		// 4b. IIR cascade
		auto& iirCascade = proc->iirCascade;
		iirCascade.clear();
		for (std::size_t i = 0; i < iirFiltersSize; i++)
		{
			const std::size_t iirCoeffsIndex = iirFiltersSize - 1 - i; // reverse order
			auto* newFilter = new juce::dsp::IIR::Filter<float>{ iirCoeffs[iirCoeffsIndex] };
			newFilter->prepare(spec);
			iirCascade.add(newFilter);
		}

		// 4c. FIR filter
		// FIR filter should be recreated each time as filter reset() method does not reset pos field.
		proc->firFilter.reset(new juce::dsp::FIR::Filter<float>{ new juce::dsp::FIR::Coefficients<float>{ firCoeffsArray.data(), firCoeffsArray.size() } });
		proc->firFilter->prepare(spec);

		//4d. Gain
		proc->gain.setGainLinear(static_cast<float>(state->gain.get()));
		proc->gain.prepare(spec);
	}
}

void ProcessorChainModifier::process(AudioPluginAudioProcessor& processor)
{
	// NB: It is proposed that this method will be called only from the Message Thread;
	// otherwise, races should be avoided between this method,
	// AudioPluginAudioProcessor::prepareToPlay and AudioPluginAudioProcessor::~AudioPluginAudioProcessor.

	// If playing is not started yet
	// then the changed state goes directly to the activeState.
	// This is to ensure that pendingState will never be used in processBlock
	// without spec preparation,
	// even if a user changes processing parameters before pushing Play button.
	// The second condition is for the case
	// when sound was muted by DAW due to "bad" processor parameters
	// and therefore processBlock method is not called.
	// Before "unmuting" a user can change the parameters and they should be updated.
	if (!processor.isPrepared ||
		juce::Time::getApproximateMillisecondCounter() - processor.lastProcessTime.load() > processBlockMaxPause)
	{
		processor.isPendingStateReady.store(false);
		rootsToJuceCoeffs(processor.filterState.get(), processor.pendingState, processor.spec);
		auto* old = processor.activeState.exchange(processor.pendingState);
		processor.pendingState = old;
	}
	// If the updated state is already loaded
	// but the sound was not fully processed using this state
	// then another update should be cancelled.
	else if (!processor.isPendingStateReady.load())
	{
		rootsToJuceCoeffs(processor.filterState.get(), processor.pendingState, processor.spec);
		processor.isPendingStateReady.store(true);
	}
}

ProcessorChainModifier::PolePeak ProcessorChainModifier::calculatePeak(
	const FilterRoot* pole, const size_t index)
{
	const auto value = pole->value.get();
	const double r = std::abs(value);
	const double edgeFreq = value.real() > 0.0 ? 0.0 : pi;

	if (pole->isReal() && std::abs(pole->order.get()) == 1)
		return { index, 1.0 / (1.0 - r), edgeFreq };

	const double angle = std::arg(value);
	const double peakFreqCos = (1.0 + r * r) / (2.0 * r) * std::cos(angle);
	if (std::abs(peakFreqCos) < 1.0)
		return { index, 1.0 / ((1.0 - r * r) * std::abs(std::sin(angle))), std::acos(peakFreqCos) };

	return { index, 1.0 / std::min(std::norm(1.0 - value), std::norm(1.0 + value)), edgeFreq };
}

int ProcessorChainModifier::findBestZeroIndexPairForPole(
	const FilterRoot* pole,
	const double peakFrequency,
	juce::OwnedArray<FilterRoot>& zeros,
	std::vector<int>& usedZeros,
	bool doesEqualPoleExist)
{
	const auto usedZerosSize = usedZeros.size();
	const auto peakPoint = std::polar(1.0, peakFrequency);
	const auto poleValue = pole->value.get();
	const bool isPoleReal = pole->isReal();
	const bool shouldDenominatorBeFirstOrder =
		isPoleReal && !doesEqualPoleExist;

	auto currentBestIndex = -1;
	double currentMin = 1.0; // only zeros that attenuate at peakFrequency are accepted
	for (std::size_t i = 0; i < usedZerosSize; i++)
	{
		auto* zero = zeros[static_cast<int>(i)];
		// Zero is already used with its order
		if (usedZeros[i] == zero->order.get())
			continue;

		// Check that numerator's order <= denominator's order,
		// otherwise don't get this zero.
		const bool isZeroReal = zero->isReal();
		if (shouldDenominatorBeFirstOrder && !isZeroReal)
			continue;

		const auto zeroValue = zero->value.get();
		double currentGain = std::abs(peakPoint - zeroValue);
		if (!isZeroReal)
			currentGain *= std::abs(peakPoint - std::conj(zeroValue));
		if (currentGain < currentMin)
		{
			currentBestIndex = static_cast<int>(i);
			currentMin = currentGain;
		}
	}

	return currentBestIndex;
}

juce::dsp::IIR::Coefficients<float>* ProcessorChainModifier::calculateIirCoefficients(
	FilterRoot* pole,
	juce::OwnedArray<FilterRoot>& zeros,
	int zeroIndex,
	bool shouldEqualPoleBeTaken,
	bool shouldEqualZeroBeTaken)
{
	const bool isPoleReal = pole->isReal();
	jassert(isPoleReal || !shouldEqualPoleBeTaken);
	float b0, b1, b2, a0, a1, a2;

	calculatePolynomialCoefficients(pole, shouldEqualPoleBeTaken, a0, a1, a2);

	if (zeroIndex != -1)
	{
		calculatePolynomialCoefficients(zeros[zeroIndex], shouldEqualZeroBeTaken, b0, b1, b2);
	}
	else if (juce::exactlyEqual(a0, 0.f)) // no zeros, 1-order pole
	{
		b1 = 1.0;
		b0 = b2 = 0;
	}
	else // no zeros, 2-order pole
	{
		b0 = 1.0;
		b1 = b2 = 0;
	}

	if (juce::exactlyEqual(a0, 0.f)) // 1-order filter
	{
		jassert(juce::exactlyEqual(b0, 0.f) && !juce::exactlyEqual(a1, 0.f));
		return new juce::dsp::IIR::Coefficients<float>{ b1, b2, a1, a2 };
	}
	else // 2-order filter
	{
		// Coefficients should be shifted in order not to decrement delay.
		if (juce::exactlyEqual(b0, 0.f))
		{
			b0 = b1;
			b1 = b2;
			b2 = 0;
		}
		return new juce::dsp::IIR::Coefficients<float>{ b0, b1, b2, a0, a1, a2 };
	}
}

void ProcessorChainModifier::calculatePolynomialCoefficients(
	FilterRoot* root,
	bool shouldBeTakenTwice,
	float& c0, float& c1, float& c2)
{
	if (root == nullptr)
	{
		c2 = 1.0;
		c0 = c1 = 0;
		return;
	}
	const double re = root->value.re.get();
	const double im = root->value.im.get();

	const bool rootIsReal = juce::exactlyEqual(im, 0.0);
	jassert(rootIsReal || !shouldBeTakenTwice);

	if (rootIsReal && !shouldBeTakenTwice)
	{
		c2 = static_cast<float>(-re);
		c1 = 1.0;
		c0 = 0;
		return;
	}

	c2 = static_cast<float>(re * re + im * im);
	c1 = static_cast<float>(-2.0 * re);
	c0 = 1.0;
}

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 8 */
/* c-basic-offset: 8 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */

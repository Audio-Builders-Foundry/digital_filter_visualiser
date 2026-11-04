#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_data_structures/juce_data_structures.h>
#include "TestHelper.h"
#include "../src/PluginProcessor.h"
#include "../src/FrequencyResponseCalculator.h"

class FrequencyResponseTest : public juce::UnitTest
{
public:
    FrequencyResponseTest() : UnitTest("FrequencyResponseTest", "Math")
    {
    }

    void runTest() override
    {
        AudioPluginAudioProcessor processor; // shouldn't be a field of this class as its static object is defined.

        performTest(
            "No roots",
            processor,
            {},
            0.123,
            1,
            0);

        // If a pole is outside 0.99 * unitCircleRadius then pole creating is blocked;
        // therefore, phase-frequency response isn't changed.
        performTest(
            "Pole in the same point (unit circle)",
            processor,
            { {-1, -1 / std::sqrt(2), 1 / std::sqrt(2)} },
            0.75 * pi,
            1,
            0);

        // When adding a single complex zero,
        // a conjugated zero and a 2-order pole in (0; 0) are authomatically added.
        performTest(
            "Zero in the same point",
            processor,
            { {1, -1 / std::sqrt(2), 1 / std::sqrt(2)} },
            0.75 * pi,
            0,
            0); // phase should not be checked as amplitude is 0 dB.

        performTest(
            "1 pole",
            processor,
            { {-1, -1 / std::sqrt(2), 0} },
            0.75 * pi,
            std::sqrt(2),
            -0.5 * pi);

        performTest(
            "1 2-order pole",
            processor,
            { {-2, -1 / std::sqrt(2), 0} },
            0.75 * pi,
            2,
            -pi);

        performTest(
            "1 2-order pole, 1 complex zero",
            processor,
            { {-2, -1 / std::sqrt(2), 0}, {1, 1, 1} },
            0.75 * pi,
            8.3630811007041093, // 2 * std::sqrt(3) * (1 + std::sqrt(2))
            2.5261129449194057); // -pi + 0.75 * pi + pi + std::atan2(1 - 1 / std::sqrt(2), 1 + 1 / std::sqrt(2))
    }

private:
    void performTest(
        const juce::String testName,
        AudioPluginAudioProcessor& processor,
        std::vector<TestRootSpecification> roots,
        double angle,
        double expectedGain,
        double expectedPhase)
    {
        beginTest(testName);
        auto* state = processor.filterState.get();
        TestHelper::makeFilterState(state, roots, 1);
        double amplitude, phase;
        FrequencyResponseCalculator::calculateForAngle(
            state, phaseUnitCoeff, phaseAmplitude, angle, amplitude, phase);

        double amplitudeDb = juce::Decibels::gainToDecibels(expectedGain);
        expectWithinAbsoluteError(amplitude, amplitudeDb, maxRelError * std::abs(amplitudeDb));
        if (expectedGain > 0) // otherwise, phase response is not defined
            expectWithinAbsoluteError(phase, expectedPhase, maxRelError * std::abs(expectedPhase));
    }

    const double pi = juce::MathConstants<double>::pi;
    const double phaseUnitCoeff = 1.0;
    const double phaseAmplitude = pi;
    const float maxRelError = 1e-6f;
};

static FrequencyResponseTest frequencyResponseTest;

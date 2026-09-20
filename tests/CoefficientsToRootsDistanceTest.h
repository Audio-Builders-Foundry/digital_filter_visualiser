#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_data_structures/juce_data_structures.h>
#include "TestHelper.h"
#include "../src/PluginProcessor.h"
#include "../src/CoefficientsToRoots.h"
#include "../src/RootsToCoefficients.h"
#include "generator/root_distance_tests.h"

class CoefficientsToRootsDistanceTest : public juce::UnitTest
{
public:
    CoefficientsToRootsDistanceTest() : UnitTest("CoefficientsToRootsDistanceTest", "Math")
	{
	}

    void runTest() override
    {
		AudioPluginAudioProcessor processor; // shouldn't be a field of this class as its static object is defined.

#define X(name, ...)\
		performTest("QR: " name, processor, QRSolve, __VA_ARGS__);\

		COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_POLES_XLIST;
		COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_ZEROS_XLIST;
#undef X
#define X(name, ...)\
		performTest("Aberth: " name, processor, AberthSolve, __VA_ARGS__);\

		COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_POLES_XLIST;
		COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_ZEROS_XLIST;
#undef X
    }

	static void printReport()
	{
		std::cout<<"CoefficientsToRootsDistanceTest Report:"<<std::endl;
		std::cout<<"Total tests : "<<totalNumAllTests<<std::endl;
		std::cout<<"Total number of warnigns : "<<totalNumWarnings<<"/"<<totalNumAllTests<<std::endl;
		std::cout<<"Mean distance all tests : "<<totalDistanceAllTests/totalNumAllTests<<std::endl;
	}

private:
	void performTest(
		const juce::String testName,
		AudioPluginAudioProcessor& processor,
		CoefficientsToRoots::SolverFn solve,
		std::vector<TestRootSpecification> givenRoots)
	{
		beginTest(testName);

		auto* state = processor.filterState.get();
		TestHelper::makeFilterState(state, givenRoots, 1);

        if (!state->zeros.isEmpty())
        {
            auto ffcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->zeros);
            auto newZeros = solve(ffcoeffs);

			double totalDistance {0}, worstDistance{0};
			auto zero = state->zeros[0]; // these are one-zero tests
			for (auto r : newZeros)
			{
				double distance = std::abs(r.value - std::complex<double>(zero->value.re.get(), zero->value.im.get()));
				totalDistance += distance;
				worstDistance = std::max(worstDistance, distance);
			}
			double meanDistance = totalDistance / newZeros.size();

			std::cout<<"Report Distance (zeros):"<<std::endl;
			std::cout<<"Mean distance :"<<meanDistance<<std::endl;
			std::cout<<"Worst distance :"<<worstDistance<<std::endl;

			if (meanDistance > DistanceTolerance)
			{
				std::cout<<"Warning : computation error { meanDistance > DistanceTolerance  : "<<meanDistance<<" > "<<DistanceTolerance<< " }"<<std::endl;
				totalNumWarnings++;
			}

			totalDistanceAllTests += meanDistance;
			totalNumAllTests++;
			std::cout<<"totalDistanceAllTests:" <<totalDistanceAllTests<<", totalNumAllTests:" <<totalNumAllTests<<std::endl;
        }

		else if (!state->poles.isEmpty()) // else if make these pole  / zero tests mutually exlusive. Cause these are one-root tests.
        {
            auto fbcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->poles);
	    auto newPoles = solve(fbcoeffs);

			double totalDistance {0}, worstDistance{0};
			auto pole = state->poles[0]; // these are one-pole tests
			for (auto r : newPoles)
			{
				double distance = std::abs(r.value - std::complex<double>(pole->value.re.get(), pole->value.im.get()));
				totalDistance += distance;
				worstDistance = std::max(worstDistance, distance);
			}
			double meanDistance = totalDistance / newPoles.size();

			std::cout<<"Report Distance (poles):"<<std::endl;
			std::cout<<"Mean distance :"<<meanDistance<<std::endl;
			std::cout<<"Worst distance :"<<worstDistance<<std::endl;

			if (meanDistance > DistanceTolerance)
			{
				std::cout<<"Warning : computation error { meanDistance > DistanceTolerance  : "<<meanDistance<<" > "<<DistanceTolerance<< " }"<<std::endl;
				totalNumWarnings++;
			}


			totalDistanceAllTests += meanDistance;
			totalNumAllTests++;
			std::cout<<"totalDistanceAllTests:" <<totalDistanceAllTests<<", totalNumAllTests:" <<totalNumAllTests<<std::endl;
        }
	}

	static constexpr double DistanceTolerance {5e-2}; // equal to CoefficientsToRoots::tolerance
	static inline double totalDistanceAllTests {0.0};
    static inline int totalNumAllTests {0}, totalNumWarnings {0};

};

static CoefficientsToRootsDistanceTest coeffsToRootsDistanceTest;

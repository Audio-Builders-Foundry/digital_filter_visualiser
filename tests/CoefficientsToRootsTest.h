#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_data_structures/juce_data_structures.h>
#include "TestHelper.h"
#include "../src/PluginProcessor.h"
#include "../src/CoefficientsToRoots.h"
#include "../src/RootsToCoefficients.h"
#include "generator/root_order_tests.h"


class CoefficientsToRootsTest : public juce::UnitTest
{
public:
    CoefficientsToRootsTest() : UnitTest("CoefficientsToRootsTest", "Math")
	{
	}

    void runTest() override
    {
		AudioPluginAudioProcessor processor; // shouldn't be a field of this class as its static object is defined.

#define X(name, ...)\
		performTest("QR: " name, processor, QRSolve, __VA_ARGS__);\

		COEFFICIENTS_TO_ROOTS_TEST_XLIST;
#undef X
#define X(name, ...)\
		performTest("Aberth: " name, processor, AberthSolve, __VA_ARGS__);\

		COEFFICIENTS_TO_ROOTS_TEST_XLIST;
#undef X
    }

	static void printReport()
	{
		std::cout<<"CoefficientsToRootsTest Report:"<<std::endl;
		std::cout<<"Total tests : "<<totalNumAllTests<<std::endl;
		std::cout<<"Total failing unit-tests : "<<totalFailingUnitTests<<"/"<<totalNumAllTests<<std::endl;
		std::cout<<"Total missed root orders - all tests : "<<totalMissedRootOrdersAllTests<<std::endl;
	}

private:
	void performTest(
		const juce::String testName,
		AudioPluginAudioProcessor& processor,
		CoefficientsToRoots::SolverFn solve,
		std::vector<TestRootSpecification> givenRoots)
	{
		beginTest(testName);

		int zeros_order{0}, poles_order{0};
		int distinctZeroCount{0}, distinctPoleCount{0};
		for (auto r : givenRoots)
		{
		    int order = static_cast<int>(r.order) * (juce::exactlyEqual(r.valIm, 0.0) ? 1 : 2);
		    jassert(std::abs(order) > 0);
		    if (order>0)
		    {
		        zeros_order+=order;
			distinctZeroCount+=1;
		    }
		    else
		    {
		        poles_order+=order;
			distinctPoleCount+=1;
		    }
		}
		// causality
		if (zeros_order > -poles_order)
			poles_order = -zeros_order;

		auto* state = processor.filterState.get();
		TestHelper::makeFilterState(state, givenRoots, 1);

		if (!state->zeros.isEmpty())
		{
		    auto ffcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->zeros);
		    auto newZeros = solve(ffcoeffs);

		    // Expect to have the same order
		    int curr_order{0};
		    for (auto r : newZeros)
		    {
		      curr_order += r.order * (juce::exactlyEqual(r.value.imag(), 0.0) ? 1 : 2);
		    }

		    bool failed{false};

		    const size_t absDiff{static_cast<size_t>(std::abs(static_cast<int>(zeros_order) - curr_order))};
		    expectEquals(absDiff, size_t(0));
		    if (absDiff)
		    {
		        std::cout<<"Error { total order = "<<zeros_order<<" != "<<curr_order<< " by "<<absDiff<<"}"<<std::endl;
		        totalMissedRootOrdersAllTests += absDiff;
		        failed = true;
		    }

		    const auto newZerosCount{static_cast<int>(newZeros.size())};
		    const int countDiff{std::abs(newZerosCount - distinctZeroCount)};
		    if (countDiff)
		    {
		        std::cout<<"Error { got "<<newZerosCount<<" distinct zeros, expected "<<distinctZeroCount<<" }"<<std::endl;
		        failed = true;
		    }

		    if (failed)
		    {
		        ++totalFailingUnitTests;
		    }
		    totalNumAllTests++;
		}

		// TODO : fix this condition. In case that a test contains only zeros, or the order of zeros exceeds order of poles, then default poles at 0,0 will be added.
		if (!state->poles.isEmpty())
		{
		    auto fbcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->poles);
		    auto newPoles = solve(fbcoeffs);

		    // Expect to have the same order
		    int curr_order{0};
		    for (auto r : newPoles)
		    {
		      curr_order -= r.order * (juce::exactlyEqual(r.value.imag(), 0.0) ? 1 : 2); // have to reverse sign, since pole-logic is outside GramSchmidt function
		    }

		    bool failed{false};

		    const size_t absDiff{static_cast<size_t>(std::abs(static_cast<int>(poles_order) - curr_order))};
		    expectEquals(absDiff, size_t(0));
		    if (absDiff)
		    {
		        std::cout<<"Error { total order = "<<poles_order<<" != "<<curr_order<< " by "<<absDiff<<"}"<<std::endl;
		        totalMissedRootOrdersAllTests += absDiff;
			failed = true;
		    }

		    const auto newPolesCount{static_cast<int>(newPoles.size())};
		    const int countDiff{std::abs(newPolesCount - distinctPoleCount)};
		    if (countDiff)
		    {
		        std::cout<<"Error { got "<<newPolesCount<<" distinct poles, expected "<<distinctPoleCount<<" }"<<std::endl;
		        failed = true;
		    }

		    if (failed)
		    {
		        ++totalFailingUnitTests;
		    }
		    totalNumAllTests++;
		}
	}

    static inline int totalMissedRootOrdersAllTests, totalNumAllTests {0}, totalFailingUnitTests {0};
};

static CoefficientsToRootsTest coeffsToRootsTest;

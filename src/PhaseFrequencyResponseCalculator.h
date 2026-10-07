#pragma once
#include "FilterState.h"

class PhaseFrequencyResponseCalculator
{
public:
    static void calculate(
        FilterState* filterState,
        float minFreq,
        double sampleRate,
        bool isLogScale,
        bool isDegrees,
        int intWidth,
        std::vector<double>& angles,
        std::vector<double>& amplitudes,
        std::vector<double>& phases);
    static void calculateForAngle(
        FilterState* filterState,
        double phaseUnitCoeff,
        double phaseAmplitude,
        double angle,
        double& amplitude,
        double& phase);
private:
    inline static double calculateUnwrappedPhase(
        double angle, 
        std::complex<double>& point,
        std::complex<double>& root);
    inline static void calculateCoefficients(
        double angle,
        FilterRoot* root,
        double& ampCoeff,
        double& phaseCoeff);

    static constexpr double eps = std::numeric_limits<double>::epsilon();
    static constexpr double pi = juce::MathConstants<double>::pi;
};
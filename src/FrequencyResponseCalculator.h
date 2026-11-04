#pragma once
#include "FilterState.h"

class FrequencyResponseCalculator
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
        std::vector<double>& magnitudes,
        std::vector<double>& phases);
    static void calculateForAngle(
        FilterState* filterState,
        double phaseUnitCoeff,
        double phaseAmplitude,
        double angle,
        double& magnitude,
        double& phase);
private:
    inline static double calculateUnwrappedPhase(
        double angle, 
        std::complex<double>& point,
        std::complex<double>& root);
    inline static void calculateCoefficients(
        double angle,
        FilterRoot* root,
        double& magnCoeff,
        double& phaseCoeff);

    static constexpr double eps = std::numeric_limits<double>::epsilon();
    static constexpr double pi = juce::MathConstants<double>::pi;
};
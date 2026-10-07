#include "PhaseFrequencyResponseCalculator.h"
#include <complex>
#include <cmath>

void PhaseFrequencyResponseCalculator::calculate(
    FilterState* filterState, 
    float minFreq,
    double sampleRate,
    bool isLogScale,
    bool isDegrees,
    int intWidth,
    std::vector<double>& angles,
    std::vector<double>& amplitudes,
    std::vector<double>& phases)
{
    std::size_t width = static_cast<size_t>(intWidth);
    angles.resize(width);
    amplitudes.resize(width, 1.);
    phases.resize(width, 0.);

    const double maxAngle = pi;
    const double minAngle = pi * minFreq / (sampleRate / 2);
    const double logMaxAngle = std::log10(maxAngle);
    const double logMinAngle = std::log10(minAngle);
    const double angleCoeff =
        isLogScale ?
        (logMaxAngle - logMinAngle) / width :
        maxAngle / width;
    const double phaseUnitCoeff = isDegrees ? 180.0 / pi : 1.0;
    const double phaseAmplitude = isDegrees ? 180.0 : pi;

    for (std::size_t i = 0; i < width; i++)
    {
        angles[i] =
            isLogScale ?
            std::pow(10., angleCoeff * i + logMinAngle) :
            angleCoeff * i;
        calculateForAngle(
            filterState,
            phaseUnitCoeff,
            phaseAmplitude,
            angles[i],
            amplitudes[i],
            phases[i]);
    }
}

void PhaseFrequencyResponseCalculator::calculateForAngle(
    FilterState* filterState,
    double phaseUnitCoeff,
    double phaseAmplitude,
    double angle,
    double& amplitude,
    double& phase)
{
    amplitude = 1;
    phase = 0;
    double ampCoeff, phaseCoeff;
    for (auto pole : filterState->poles)
    {
        int order = std::abs(pole->order.get());
        calculateCoefficients(angle, pole, ampCoeff, phaseCoeff);
        amplitude *= std::pow(ampCoeff, order); // later it turns to 1 / amplitudes[i]
        phase -= phaseCoeff * order;
    }

    amplitude = 1.0 / std::max(amplitude, eps);

    for (auto zero : filterState->zeros)
    {
        int order = zero->order.get();
        calculateCoefficients(angle, zero, ampCoeff, phaseCoeff);
        amplitude *= std::pow(ampCoeff, order);
        phase += phaseCoeff * order;
    }

    amplitude = juce::Decibels::gainToDecibels(amplitude);
    phase *= phaseUnitCoeff;
}

double PhaseFrequencyResponseCalculator::calculateUnwrappedPhase(
    double angle, 
    std::complex<double>& point, 
    std::complex<double>& root)
{ // unwrapped version of std::arg(point - root)
    return 
        std::abs(root) < 1.0 ?
        angle + std::arg(1.0 - root * std::conj(point)) :
        std::arg(-root) + std::arg(1.0 - point / root);
}

void PhaseFrequencyResponseCalculator::calculateCoefficients(
    double angle,
    FilterRoot* root,
    double& ampCoeff,
    double& phaseCoeff)
{
    std::complex<double> rootVal = root->value.get();
    std::complex<double> point(std::cos(angle), std::sin(angle));
    ampCoeff = std::abs(point - rootVal);
    phaseCoeff = calculateUnwrappedPhase(angle, point, rootVal);
    if (!root->isReal())
    {
        std::complex<double> conj = std::conj(rootVal);
        ampCoeff *= std::abs(point - conj);
        phaseCoeff += calculateUnwrappedPhase(angle, point, conj);
    }
}

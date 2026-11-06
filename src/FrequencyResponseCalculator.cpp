#include "FrequencyResponseCalculator.h"
#include <complex>
#include <cmath>

void FrequencyResponseCalculator::calculate(
    FilterState* filterState,
    float minFreq,
    double sampleRate,
    bool isLogScale,
    bool isDegrees,
    int intWidth,
    std::vector<double>& angles,
    std::vector<double>& magnitudes,
    std::vector<double>& phases)
{
    std::size_t width = static_cast<size_t>(intWidth);
    angles.resize(width);
    magnitudes.resize(width, 1.);
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
            magnitudes[i],
            phases[i]);
    }
}

void FrequencyResponseCalculator::calculateForAngle(
    FilterState* filterState,
    double phaseUnitCoeff,
    double phaseAmplitude,
    double angle,
    double& magnitude,
    double& phase)
{
    magnitude = 1;
    phase = 0;
    double ampCoeff, phaseCoeff;
    int totalOrder = 0;
    for (auto pole : filterState->poles)
    {
        int order = std::abs(pole->order.get());
        totalOrder -= order * (pole->isReal() ? 1 : 2);
        calculateCoefficients(angle, pole, ampCoeff, phaseCoeff);
        magnitude *= std::pow(ampCoeff, order); // later it turns to 1 / magnitudes[i]
        phase -= phaseCoeff * order;
    }

    magnitude = 1.0 / std::max(magnitude, eps);

    for (auto zero : filterState->zeros)
    {
        int order = zero->order.get();
        totalOrder += order * (zero->isReal() ? 1 : 2);
        calculateCoefficients(angle, zero, ampCoeff, phaseCoeff);
        magnitude *= std::pow(ampCoeff, order);
        phase += phaseCoeff * order;
    }

    magnitude = juce::Decibels::gainToDecibels(magnitude);
    phase *= phaseUnitCoeff;
}

double FrequencyResponseCalculator::calculateUnwrappedPhase(
    double angle,
    std::complex<double>& point,
    std::complex<double>& root)
{ // unwrapped version of std::arg(point - root)
    return
        std::abs(root) < 1.0 ?
        angle + std::arg(1.0 - root * std::conj(point)) :
        std::arg(-root) + std::arg(1.0 - point / root);
}

void FrequencyResponseCalculator::calculateCoefficients(
    double angle,
    FilterRoot* root,
    double& magnCoeff,
    double& phaseCoeff)
{
    std::complex<double> rootVal = root->value.get();
    std::complex<double> point(std::cos(angle), std::sin(angle));
    magnCoeff = std::abs(point - rootVal);
    phaseCoeff = calculateUnwrappedPhase(angle, point, rootVal);
    if (!root->isReal())
    {
        std::complex<double> conj = std::conj(rootVal);
        magnCoeff *= std::abs(point - conj);
        phaseCoeff += calculateUnwrappedPhase(angle, point, conj);
    }
}

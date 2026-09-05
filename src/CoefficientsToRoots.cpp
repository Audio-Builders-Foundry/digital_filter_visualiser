// #pragma once
// #include <juce_audio_processors/juce_audio_processors.h>
// #include <juce_dsp/juce_dsp.h>

#include "CoefficientsToRoots.h"

#include <algorithm>
#include <cmath>

namespace CoefficientsToRoots
{

  static c128
  evaluatePolynomial(const Coefficients &coeffs, c128 pt)
  {
    c128 result = coeffs[0];
    for(size_t i = 1; i < coeffs.size(); ++i)
    {
      auto coeff = coeffs[i];
      result = pt*result + coeff;
    }

    return result;
  }

  static c128
  evaluateComplexPolynomial(const ComplexCoefficients &coeffs, c128 pt)
  {
    c128 result = coeffs[0];
    for(size_t i = 1; i < coeffs.size(); ++i)
    {
      auto coeff = coeffs[i];
      result = pt*result + coeff;
    }

    return result;
  }

  static c128
  dividePolynomial(const Coefficients &coeffs, c128 pt, ComplexCoefficients &quotient)
  {
    quotient.resize(coeffs.size());

    c128 result = coeffs[0];
    quotient[0] = result;

    for(size_t i = 1; i < coeffs.size(); ++i)
    {
      auto coeff = coeffs[i];
      result = pt*result + coeff;

      if(i < coeffs.size() - 1)
      { quotient[i] = result; }
    }

    return result;
  }

  static c128
  divideComplexPolynomial(const ComplexCoefficients &coeffs, c128 pt, ComplexCoefficients &quotient)
  {
    quotient.resize(coeffs.size());

    c128 result = coeffs[0];
    quotient[0] = result;

    for(size_t i = 1; i < coeffs.size(); ++i)
    {
      auto coeff = coeffs[i];
      result = pt*result + coeff;

      if(i < coeffs.size() - 1)
      { quotient[i] = result; }
    }

    return result;
  }

  static void
  dividePolynomialByRoot(const Coefficients &coeffs, Root pt, ComplexCoefficients &remainders)
  {
    jassert(pt.order <= int(coeffs.size()));
    jassert(pt.order == int(remainders.size()));

    auto remSize = static_cast<int>(remainders.size());

    ComplexCoefficients quotientBacking(coeffs.size() - 1);
    ComplexCoefficients *quotient = &quotientBacking;
    remainders[static_cast<size_t>(remSize - 1)] = dividePolynomial(coeffs, pt.value, *quotient);
    if(pt.order == 1) return;

    ComplexCoefficients ccoeffsBacking(coeffs.size() - 1);
    ComplexCoefficients *ccoeffs= &quotientBacking;
    quotient = &ccoeffsBacking;
    quotient->resize(quotient->size() - 1);

    for(int n = 1; n < pt.order; ++n)
    {
      remainders[static_cast<size_t>(remSize - 1 - n)] = divideComplexPolynomial(*ccoeffs, pt.value, *quotient);
      std::swap(ccoeffs, quotient);
      quotient->resize(quotient->size() - 1);
    }
  }

  static c128
  evaluatePolynomialAtRoot(const Coefficients &coeffs, Root pt)
  {
    // TODO: ensure pt.order is at most coeffs.size()

    ComplexCoefficients quotientBacking(coeffs.size() - 1);
    ComplexCoefficients ccoeffsBacking(coeffs.size() - 1);

    ComplexCoefficients *quotient = &quotientBacking;
    c128 result = dividePolynomial(coeffs, pt.value, *quotient);
    DBG("intermediate remainder: (" << result.real() << ", " << result.imag() << ")");

    ComplexCoefficients *ccoeffs = quotient;
    quotient = &ccoeffsBacking;
    quotient->resize(quotient->size() - 1);

    for(int n = 1; n < pt.order; ++n)
    {
      result = divideComplexPolynomial(*ccoeffs, pt.value, *quotient);
      DBG("intermediate remainder: (" << result.real() << ", " << result.imag() << ")");
      std::swap(ccoeffs, quotient);
      quotient->resize(quotient->size() - 1);
    }

    return result;
  }

  static bool
  rootDividesPolynomial(const Coefficients &coeffs, Root pt)
  {
    if(pt.order > int(coeffs.size()))
    { return false; } // a root can't divide a polynomial with a smaller order

    ComplexCoefficients quotientBacking(coeffs.size() - 1);

    ComplexCoefficients *quotient = &quotientBacking;
    c128 lastRem = dividePolynomial(coeffs, pt.value, *quotient);

    // NOTE(ry): if the order is 1, we can only compute 1 remainder, so we
    // determine divisibility by its magnitude
    // TODO(ry): is the first remainder sufficient by itself?
    if(pt.order == 1)
    {
      double const eps = 1e-4;
      bool result = std::abs(lastRem) < eps;
      return result;
    }

    // NOTE(ry): otherwise, we determine divisibility from the sequence of
    // remainders: if the remainders are decreasing, then the root does not
    // divide the polynomial; if the remainders are increasing, then the root
    // divides the polynomial

    ComplexCoefficients ccoeffsBacking(coeffs.size() - 1);
    ComplexCoefficients *ccoeffs = quotient;
    quotient = &ccoeffsBacking;
    quotient->resize(quotient->size() - 1);

    // TODO(ry): is it sufficient to just check the first two terms, so we don't
    // have a worst-case O(n^2) loop here?
    for(int n = 1; n < pt.order; ++n)
    {
      c128 newRem = divideComplexPolynomial(*ccoeffs, pt.value, *quotient);
      if((std::abs(lastRem) - std::abs(newRem)) > 0)
      { return false; }

      std::swap(ccoeffs, quotient);
      quotient->resize(quotient->size() - 1);
      lastRem = newRem;
    }

    return true;
  }

  // TODO(ry): is this too general? should we always assume pt1.order > pt0.order?
  static const Root &
  betterDivisorOfPolynomial(const Coefficients &coeffs, const Root &pt0, const Root &pt1)
  {
    jassert(std::max(pt0.order, pt1.order) <= int(coeffs.size()));

    jassert(pt0.order > 0);
    jassert(pt1.order > 0);
    ComplexCoefficients remainders0(size_t(pt0.order));
    ComplexCoefficients remainders1(size_t(pt1.order));

    dividePolynomialByRoot(coeffs, pt0, remainders0);
    dividePolynomialByRoot(coeffs, pt1, remainders1);

    // NOTE(ry): score remainders
    // the score is the largest ratio |rem(pt1.value)_k|/|rem(pt2.value)_k|.
    // the idea is if the remaiders of pt1 are not larger than those of pt0,
    // then we may say pt1 is a stronger divisor than pt0 since its order is
    // larger.
    // note that the remainders are stored in reverse order, so they can be used
    // for polynomial evaluation (TODO: is that necessary?), so we have to look
    // at different indices to compare corresponding remainders.

    // TODO(ry): sould we always divide the value with larger order by the value with smaller order?
    int minOrder = std::min(pt0.order, pt1.order);
    int remDiff0, remDiff1;
    if(minOrder == pt0.order)
    {
      remDiff0 = 0;
      remDiff1 = pt1.order - pt0.order;
    }
    else // minOrder == pt1.order
    {
      remDiff0 = pt0.order - pt1.order;
      remDiff1 = 0;
    }
    double const minDiv = 1e-5;
    double score = 0.0;
    // TODO(ry): do we need to look at _all_ the remainders, or is the constant term sufficient?
    for(int i = 0; i < minOrder; ++i)
    {
      // TODO(ry): compare normalized remainders
      double rem0 = std::abs(remainders0[static_cast<size_t>(i + remDiff0)]);
      double rem1 = std::abs(remainders1[static_cast<size_t>(i + remDiff1)]);
      score = std::max(score, rem1/std::max(rem0, minDiv));
    }

    double const tol = 3;
    return (score < tol) ? pt1 : pt0;
  }

  static Root
  mergeRoots(Root oldRoot, Root addedRoot)
  {
    Root newRoot;
    if(juce::exactlyEqual(oldRoot.value.imag(), 0.0) == juce::exactlyEqual(addedRoot.value.imag(), 0.0))
    {
      newRoot.value = (oldRoot.value*c128(oldRoot.order, 0) + addedRoot.value) / c128(oldRoot.order + 1, 0);
      newRoot.order = oldRoot.order + 1;
    }
    else
    {
      c128 reVal = (!juce::exactlyEqual(oldRoot.value.imag(), 0.0)) ? addedRoot.value : oldRoot.value;
      c128 imVal = (!juce::exactlyEqual(oldRoot.value.imag(), 0.0)) ? c128(oldRoot.value.real(), 0) : c128(addedRoot.value.real(), 0);
      int reOrder = (!juce::exactlyEqual(oldRoot.value.imag(), 0.0)) ? addedRoot.order : oldRoot.order;
      int imOrder = (!juce::exactlyEqual(oldRoot.value.imag(), 0.0)) ? oldRoot.order : addedRoot.order;

      newRoot.value = (reVal*c128(reOrder, 0) + c128(2, 0)*imVal*c128(imOrder, 0)) / c128(reOrder + 2*imOrder, 0);
      newRoot.order = reOrder + 2*imOrder;
    }

    return newRoot;
  }

  static c128
  centerOfCircleThroughPoints(c128 z0, c128 z1, c128 z2)
  {
    // TODO: check for colinearity, duplicates

    c128 w = (z2 - z0) / (z1 - z0);
    c128 c = (z1 - z0)*((w - std::norm(w))/(w - std::conj(w))) + z0;
    return c;
  }

#include "CoefficientsToRoots.QR.cpp"
#include "CoefficientsToRoots.Aberth.cpp"

}

#pragma once

#include "FilterState.h"
#include <float.h>
#include <vector>
#include <utility>
#include <limits>

namespace CoefficientsToRoots
{
  struct Root
  {
    Root(void) : value(0), order(0) {}
    Root(c128 v, int o) : value(v), order(o) {}

    c128 value;
    int order;
  };

  using Coefficients = std::vector<double>;
  using ComplexCoefficients = std::vector<c128>;
  using SolutionSet = std::vector<Root>;
  using SolverFn = SolutionSet (*)(Coefficients);

  /** evaluates polynomial with real coefficients at complex point.
      returns result
  */
  [[maybe_unused]] static c128 evaluatePolynomial(const Coefficients &coeffs, c128 pt);

  /** evaluates polynomial with complex coefficients at complex point.
      returns result
  */
  [[maybe_unused]] static c128 evaluateComplexPolynomial(const ComplexCoefficients &coeffs, c128 pt);

  /** divides polynomial with real coefficients by linear term corresponding to
      a complex point.
      fills `quotient` with the complex coefficients of the quotient polynomial.
      returns the remainder.
  */
  [[maybe_unused]] static c128 dividePolynomial(const Coefficients &coeffs, c128 pt, ComplexCoefficients &quotient);

  /** divides polynomial with complex coefficients by linear term corresponding
      to a complex point.
      fills `quotient` with the complex coefficients of the quotient polynomial.
      returns the remainder.
  */
  [[maybe_unused]] static c128 divideComplexPolynomial(const ComplexCoefficients &coeffs, c128 pt, ComplexCoefficients &quotient);

  /** repeatedly divides polynomial with real coefficients by linear terms
      corresponding to the value and multiplicity of `pt`.
      fills `remainders` with the remainder of each division, in reverse order
      so the remainders can be used as coefficients to the taylor approximation
      of the polynomial about `pt.value`.
  */
  [[maybe_unused]] static void dividePolynomialByRoot(const Coefficients &coeffs, Root pt, ComplexCoefficients &remainders);

  /** determines which root is a better divisor of the polynomial whose
      coefficients are given by coeffs.
      returns the root whose sequence of remainders when dividing the polynomial
      has smaller values.
   */
  [[maybe_unused]] static const Root &betterDivisorOfPolynomial(const Coefficients &coeffs, const Root &pt0, const Root &pt1);

  /** returns the weighted average of `oldRoot` and `addedRoot`, adding their
      orders.
      if one is complex and the other is real, will treat the complex root as a
      pair of conjugates being combined with the real root (counts the complex
      order as double).
   */
  [[maybe_unused]] static Root mergeRoots(Root oldRoot, Root addedRoot);

  /** returns the center of the unique circle passing through 3 complex points */
  [[maybe_unused]] static c128 centerOfCircleThroughPoints(c128 z0, c128 z1, c128 z2);

#define SOLVER_DEFINE(Name)\
  }\
  static CoefficientsToRoots::SolverFn constexpr Name##Solve = &CoefficientsToRoots::Name::Solve;\
  namespace CoefficientsToRoots\
  {\

#include "CoefficientsToRoots.QR.h"
#include "CoefficientsToRoots.Aberth.h"

}

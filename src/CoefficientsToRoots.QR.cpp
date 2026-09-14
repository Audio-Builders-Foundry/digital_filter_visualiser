QR::Matrix QR::Q, QR::R;

ComplexCoefficients QR::remaindersCurrent{}, QR::remaindersNew{};

SolutionSet QR::Solve(Coefficients coefs)
{
    PROFILE_FUNCTION();

    // filter out leading zeros, increasing counter until the leading 1.0 is found.
    size_t degree = 0;
    while( degree < coefs.size() && coefs[degree] != 1.0)
        degree++;
    degree = coefs.size() - degree -1; // subtract 1 for the leading 1.0

   // filter out trailing zeros increasing the order of root at Zero (usually for default poles)
    size_t orderAtZero = 0;
    for( int i = static_cast<int>(coefs.size()-1); i >= 0 && std::abs(coefs[(size_t)i]) == 0.0 ; --i)
    {
        ++orderAtZero;
        --degree;
    }

    // append root at zero of "orderAtZero" order
    SolutionSet roots;
    if (orderAtZero>0)
        roots.emplace_back(Root{c128(0.0, 0.0), static_cast<int>(orderAtZero)});

    if (degree == 0 )
        // Returing root at (0,0) if any
        return roots;

    if (degree==1)
    {
        // Returing 1 non-zero root + root at (0,0) if any
        roots.emplace_back(Root{static_cast<c128>(-coefs[coefs.size() - 1]), 1});
        return roots;
    }
    else if (degree == 2)
    {
	// NOTE(ry): solve for roots explicitly via quadratic formula
	auto a = coefs[0];
	auto b = coefs[1];
	auto c = coefs[2];
	auto disc = b*b - 4.0*a*c;
	if (disc < 0)
	{
	    auto sln = c128(-b, std::sqrt(-disc))/(2.0*a);
	    roots.push_back(Root{sln, 1});
	}
	else
	{
	    auto sln0 = c128(-b + std::sqrt(disc), 0)/(2.0*a);
	    auto sln1 = c128(-b - std::sqrt(disc), 0)/(2.0*a);
	    auto root0 = Root{sln0, 1};
	    auto root1 = updateSolutions(roots, coefs, root0, Root{sln1, 1});
	    roots.push_back(root1);
	}

	return roots;
    }

    // build companion Matrix
    std::vector<double> A(degree * degree, 0.0);
    // Build companion matrix (Hessenberg form)
    for (size_t i=0; i< degree; i++)
    {
        size_t j = i+1;
        size_t coef_idx = coefs.size()-1 - orderAtZero -i;
        A[i * degree + (degree-1)] = -coefs[coef_idx];  // fill last column with negative vals of coefs
        if (i!=degree-1)
        {
            A[j * degree + i] = 1.0;  // fill subdiagonal entries
        }
    }

    // QR algorithm

    double const eps = 2.0*std::numeric_limits<double>::epsilon();
    size_t iter {0};
    size_t startIdx{0}, endIdx{degree-1};
    while(endIdx > 1)
    {
        if (++iter > MaxIterations)
	{
	    DBG("hit MaxIterations " << MaxIterations << " for range [" << startIdx << ", " << endIdx << "]");
	    endIdx -= 1;
	    iter = 0;
	    continue;
	}

	// NOTE(ry): compute QR = A, A' = RQ using the default decomposition method
	decompUpdate(A, degree, startIdx, endIdx);

	// NOTE(ry): update unreduced matrix range
	{
	    while(endIdx > 1)
	    {
		double diag0 = A[endIdx*degree + endIdx];
		double diag1 = A[(endIdx-1)*degree + (endIdx-1)];
		double diag2 = A[(endIdx-2)*degree + (endIdx-2)];
		double subdiag0 = A[endIdx*degree + (endIdx-1)];
		double subdiag1 = A[(endIdx-1)*degree + (endIdx-2)];
		if(std::abs(subdiag0) <= eps*(std::abs(diag0) + std::abs(diag1)))
		{
		    DBG("1x1 convergence at index " << endIdx << ", iter = " << iter);
		    A[endIdx*degree + (endIdx-1)] = 0.0;
		    endIdx -= 1;
		    iter = 0;
		}
		else if(std::abs(subdiag1) <= eps*(std::abs(diag1) + std::abs(diag2)))
		{
		    DBG("2x2 convergence at index " << endIdx << ", iter = " << iter);
		    A[(endIdx-1)*degree + (endIdx-2)] = 0.0;
		    endIdx -= 2;
		    iter = 0;
		}
		else
		{
		    break;
		}
	    }

	    if(endIdx > 1)
	    {
		startIdx = endIdx - 2;
		while(startIdx > 0)
		{
		    double diag0 = A[startIdx*degree + startIdx];
		    double diag1 = A[(startIdx-1)*degree + (startIdx-1)];
		    double subdiag = A[startIdx*degree + (startIdx-1)];
		    if(std::abs(subdiag) <= eps*(std::abs(diag0) + std::abs(diag1)))
		    {
			A[startIdx*degree + (startIdx-1)] = 0.0;
			break;
		    }
		    startIdx -= 1;
		}
	    }
	    else
	    {
		break;
	    }
	}
    }

    // Extract roots — read diagonal
    extractRoots(roots, A, degree, coefs);
    return roots;
}

void QR::decompUpdateGramSchmidtExplicit(Matrix &A, size_t degree, size_t shift_idx)
{
    PROFILE_FUNCTION();

    auto shift = shiftRayleigh(A, degree, shift_idx);

    std::vector<double> v(shift_idx); // vector for storing current column of A. Note: only the first {0 to (curr val of shift_idx - 1) } indexes are used per iteration.

    Q.resize(A.size());
    R.resize(A.size());
    std::fill(Q.begin(), Q.end(), 0.0);

    // step 1 - calculate Q : Q = A[col] - projections onto the previous Q[col]
    for (size_t col = 0 ; col < shift_idx; col++)
    {
	// set v with column A[col]
	for (size_t row = 0 ; row< shift_idx; row++)
	    v[row] = A[row * degree + col];

	// subtract projection : v[col] = A[col] - proj onto the previous (<col) orthonormal colums of Q.
	for (size_t curr_col = 0; curr_col < col; curr_col++)
	{
	    // compute dot product ...
	    double dot_product {0.0};
	    for (size_t curr_row = 0; curr_row < shift_idx; ++curr_row)
	    {
		dot_product += Q[curr_row * degree + curr_col] * v[curr_row];
	    }

	    // .. and subtract it from the current column vector
	    for (size_t curr_row=0; curr_row < shift_idx; ++curr_row)
	    {
		v[curr_row] -= dot_product * Q[curr_row * degree + curr_col];
	    }
	}

	// normalize column of q
	double norm = 0.0;
	for (size_t k = 0; k < shift_idx; ++k)
	{
	    norm += v[k] * v[k];
	}

	double normReciprocal = 1.0 / std::sqrt(norm);

	for (size_t k = 0; k < shift_idx; ++k)
	{
	    Q[k * degree + col] = v[k] * normReciprocal;
	}
    }

    // step 2 - calculate R :  R = Q A
    for (size_t row = 0; row < shift_idx; row++)
    {
	for (size_t col = 0; col < shift_idx; col++)
	{
	    double sum {0.0};
	    for (size_t inner = 0; inner < shift_idx; inner++)
	    {
		sum += (Q[inner * degree + row] * A[inner * degree + col]);     // Q transposed
	    }
	    R[row* degree + col] = sum;
	}
    }

    // step 3 - A = R Q
    for (size_t row = 0; row < shift_idx; row++)
    {
	for (size_t col = 0; col < shift_idx; col++)
	{
	    double sum {0.0};
	    for (size_t inner = 0; inner< shift_idx; ++inner)
	    {
		sum += R[row * degree + inner] * Q[inner * degree + col];
	    }
	    A[row* degree + col] = sum; // resetting A[row][col]
	}
    }

    unshiftRayleigh(A, degree, shift_idx, shift);
}

void QR::decompUpdateHouseholderExplicit(Matrix &A, size_t degree, size_t shift_idx)
{
  PROFILE_FUNCTION();

  auto shift = shiftRayleigh(A, degree, shift_idx);

  Q.resize(A.size());
  R.resize(A.size());
  auto N = degree;

  // TODO(ry): many of these loops may be shortened or even eliminated by
  // exploiting the fact that A is initialized upper-hessenberg (and always so over each iteration?)

  // TODO(ry): exploit upper-triangularity of R matrix

  // NOTE(ry): A is iniitialized row-major (TODO: would be less work if it was column-major)

  // NOTE(ry): R is a column-major matrix

  // NOTE(ry): V a column-major matrix which stores v column-vectors used for each Q_k
  auto &V = Q;

  // NOTE(ry): scratch space for intermediate vector calculations
  std::vector<double> scratch(shift_idx);

  // NOTE(ry): initialize R to A
  for(size_t j = 0; j < shift_idx; ++j)
  {
    for(size_t i = 0; i < shift_idx; ++i)
    {
      // TODO(ry): anything to do about this access pattern?
      R[i + j*N] = A[i*N + j];
    }
  }

  // NOTE(ry): compute R via householder reflections.
  // compute sequence of reflections to put R in upper-triangular form
  {
    // TODO(ry): since A is upper-hessenberg, it should be sufficient to only
    // work with 2-component v vectors, which zero out the sole sub-diagonal
    // element
    for(size_t k = 0; k < shift_idx-1; ++k)
    {
      auto &u = scratch;
      auto alpha = 0.0;
      {
	auto *rcol = &R.data()[k + k*N];
	for(size_t i = 0; i < shift_idx-k; ++i)
	{
	  auto x = rcol[i];
	  alpha += x*x;
	  u[i] = x;
	}
      }
      alpha = std::sqrt(alpha);
      u[0] -= alpha; // TODO(ry): don't always subtract here. if u[0] goes to zero then its inverse norm can go to infinity, so make sure we increase magnitude (add if u[0] positive, subtract if negative).

      auto unorm = 0.0;
      for(size_t i = 0; i < shift_idx-k; ++i)
      {
	unorm += u[i]*u[i];
      }
      auto unorminv = 1.0/std::sqrt(unorm);

      auto *vcol = &V.data()[k + k*N];
      for(size_t i = 0; i < shift_idx-k; ++i)
      {
	vcol[i] = u[i]*unorminv;
      }

      // NOTE(ry): update R
      {
	// NOTE(ry): v^T*R
	auto &rowTemp = scratch;
	{
	  auto *vrow = vcol;
	  for(size_t j = 0; j < shift_idx-k; ++j)
	  {
	    rowTemp[j] = 0.0;
	    auto *rcol = &R.data()[k + (k+j)*N];
	    for(size_t i = 0; i < shift_idx-k; ++i)
	    {
	      rowTemp[j] += vrow[i]*rcol[i];
	    }
	  }
	}

	// NOTE(ry): R' = (I - 2*v*v^T)*R
	for(size_t j = 0; j < shift_idx-k; ++j)
	{
	  auto *rcol = &R.data()[k + (k+j)*N];
	  for(size_t i = 0; i < shift_idx-k; ++i)
	  {
	    rcol[i] -= 2.0*vcol[i]*rowTemp[j];
	  }
	}
      }
    }
  }

  // NOTE(ry): update A
  {
    // NOTE(ry): set A to R (TODO: wouldn't be necessary if A was always column-major)
    for(size_t i = 0; i < shift_idx; ++i)
    {
      for(size_t j = 0; j < shift_idx; ++j)
      {
	A[i*N + j] = R[i + j*N];
      }
    }

    for(size_t k = 0; k < shift_idx-1; ++k)
    {
      // NOTE(ry): A*v
      auto *vcol = &V.data()[k + k*N];
      auto &colTemp = scratch;
      for(size_t i = 0; i < shift_idx; ++i)
      {
	colTemp[i] = 0.0;
	auto *arow = &A.data()[i*N + k];
	for(size_t j = 0; j < shift_idx-k; ++j)
	{
	  colTemp[i] += arow[j]*vcol[j];
	}
      }

      // NOTE(ry): A' = A*(I - 2*v*v^T)
      for(size_t i = 0; i < shift_idx; ++i)
      {
	auto *arow = &A.data()[i*N + k];
	auto *vrow = vcol;
	for(size_t j = 0; j < shift_idx-k; ++j)
	{
	  arow[j] -= 2.0*colTemp[i]*vrow[j];
	}
      }
    }
  }

  unshiftRayleigh(A, degree, shift_idx, shift);
}

void QR::decompUpdateHouseholderImplicit(Matrix &A, size_t degree, size_t startIdx, size_t endIdx)
{
  PROFILE_FUNCTION();

  // NOTE(ry): computes a Francis Implicit QR Step
  // adapted from _Matrix Computations_ by Golub and Van Loan, pgs. 356-9

  // NOTE(ry): compute first column of (A - a_1I)(A - a_2I) (a_1, a_2 eigenvalues of lowest 2x2 sub-block)
  //size_t n = shift_idx - 1;
  size_t p = startIdx;
  size_t n = endIdx;
  size_t m = n - 1;

  // NOTE(ry): A is assumed row-major
  double Amm = A[m*degree + m];
  double Ann = A[n*degree + n];
  double Amn = A[m*degree + n];
  double Anm = A[n*degree + m];
  double A00 = A[p*degree + p];
  double A01 = A[p*degree + p+1];
  double A10 = A[(p+1)*degree + p];
  double A11 = A[(p+1)*degree + p+1];
  double A21 = A[(p+2)*degree + p+1];

  double s = Amm + Ann;
  double t = Amm*Ann - Amn*Anm;

  double x = A00*A00 + A01*A10 - s*A00 + t;
  double y = A10*(A00 + A11 - s);
  double z = A10*A21;

  // NOTE(ry): symmetrically update A via 3x3 householder matrices ("bulge chasing").
  // applies all but the last transformation, which is 2x2
  for(size_t k = p; k < n - 1; ++k)
  {
    size_t colIdx = (k == p) ? p : k-1;

    size_t rowIdx = std::min(k+4, n+1);

    // NOTE(ry): compute householder reflection vector.
    // the vector is normalized so the first entry is 1; we store a separate
    // coefficient beta so the reflection matrix is (I - beta*v*v^T)
    double beta, vy, vz;
    {
      auto sigma = y*y + z*z;
      if(juce::exactlyEqual(sigma, 0.0))
      {
	beta = 0;
	vy = 0;
	vz = 0;
      }
      else
      {
	auto mu = std::sqrt(x*x + sigma);
	double vx;
	if(x <= 0)
	{
	  vx = x - mu;
	}
	else
	{
	  vx = -sigma/(x + mu);
	}

	beta = 2.0*vx*vx/(sigma + vx*vx);
	vz = z/vx;
	vy = y/vx;
      }
    }

    // NOTE(ry): multiply on the left (A' = (I - beta*v*v^T)*A)
    {
      auto *arow0 = &A.data()[(k+0)*degree + colIdx];
      auto *arow1 = &A.data()[(k+1)*degree + colIdx];
      auto *arow2 = &A.data()[(k+2)*degree + colIdx];

      for(size_t j = 0; j < n+1 - colIdx; ++j)
      {
	auto sum = arow0[j] + vy*arow1[j] + vz*arow2[j];

	arow0[j] -= beta*sum;
	arow1[j] -= beta*vy*sum;
	arow2[j] -= beta*vz*sum;
      }
    }

    // NOTE(ry): multiply on the right (A'' = A'*(I - beta*v*v^T))
    {
      for(size_t i = p; i < rowIdx; ++i)
      {
	auto *arow = &A.data()[i*degree + k];

	auto sum = arow[0] + vy*arow[1] + vz*arow[2];

	arow[0] -= beta*sum;
	arow[1] -= beta*vy*sum;
	arow[2] -= beta*vz*sum;
      }
    }

    x = A[(k+1)*degree + k];
    y = A[(k+2)*degree + k];
    if(k < n-2)
    { z = A[(k+3)*degree + k]; }
  }

  // NOTE(ry): last bulge chansing transform (2x2)
  {
    // NOTE(ry): householder reflection vector
    double beta, vy;
    {
      auto sigma = y*y;
      if(juce::exactlyEqual(sigma, 0.0))
      {
	beta = 0;
	vy = 0;
      }
      else
      {
	auto mu = std::sqrt(x*x + sigma);
	double vx;
	if(x <= 0)
	{
	  vx = x - mu;
	}
	else
	{
	  vx = -sigma/(x + mu);
	}

	beta = 2.0*vx*vx/(sigma + vx*vx);
	vy = y/vx;
      }
    }

    // NOTE(ry): multiply on the left (A' = (I - beta*v*v^T)*A)
    {
      auto *arow0 = &A.data()[(n-1)*degree + (n-2)];
      auto *arow1 = &A.data()[n*degree + (n-2)];

      for(size_t j = 0; j < 3; ++j)
      {
	auto sum = arow0[j] + vy*arow1[j];

	arow0[j] -= beta*sum;
	arow1[j] -= beta*vy*sum;
      }
    }

    // NOTE(ry): multiply on the right (A'' = A'*(I - beta*v*v^T))
    {
      for(size_t i = p; i < n+1; ++i)
      {
	auto *arow = &A.data()[i*degree + (n-1)];

	auto sum = arow[0] + vy*arow[1];

	arow[0] -= beta*sum;
	arow[1] -= beta*vy*sum;
      }
    }
  }
}

double QR::shiftRayleigh(Matrix &A, size_t degree, size_t shift_idx)
{
  // subtract rayleigh quotient shift
  const double shift = A[(shift_idx - 1) * degree + (shift_idx - 1)];
  for (size_t i = 0; i < shift_idx; ++i)
    A[i * degree + i] -= shift; // Decompose (Ak - sI)

  return shift;
}

void QR::unshiftRayleigh(Matrix &A, size_t degree, size_t shift_idx, double shift)
{
  // add shift back in A and check sub-diagonal entries
  for (size_t i = 0; i < shift_idx; ++i)
    A[i * degree + i] += shift;
}

Root QR::updateSolutions(SolutionSet &solns, const Coefficients &coeffs, Root currentCluster, Root newRoot)
{
  PROFILE_FUNCTION();

  jassert(newRoot.order > 0);

  if(currentCluster.order == 0)
  {
    return newRoot;
  }

  jassert(currentCluster.order > 0);
  jassert(currentCluster.order >= newRoot.order);
  jassert(currentCluster.order + newRoot.order <= int(coeffs.size()));

  Root newCluster = mergeRoots(currentCluster, newRoot);
  DBG("currentCluster = (" << currentCluster.value.real() << ", " << currentCluster.value.imag() << ")^" << currentCluster.order);
  DBG("newRoot = (" << newRoot.value.real() << ", " << newRoot.value.imag() << ")^" << newRoot.order);
  DBG("newCluster = (" << newCluster.value.real() << ", " << newCluster.value.imag() << ")^" << newCluster.order);

  if(remaindersCurrent.size() == 0)
  {
    remaindersCurrent.resize(size_t(currentCluster.order));
    dividePolynomialByRoot(coeffs, currentCluster, remaindersCurrent);
  }

  //ComplexCoefficients remaindersCurrent(size_t(currentCluster.order));
  //ComplexCoefficients remaindersNew(size_t(newCluster.order));
  //dividePolynomialByRoot(coeffs, currentCluster, remaindersCurrent); // TODO(ry): if we kept last call's newCluster, we already computed this, so we should keep `remaindersNew` to save computation
  //dividePolynomialByRoot(coeffs, newCluster, remaindersNew);
  remaindersNew.resize(size_t(newCluster.order));
  dividePolynomialByRoot(coeffs, newCluster, remaindersNew);

  double const divEps = 1e-12;

  // NOTE(ry): compare remainders of old and new clusters to see if new cluster still divides polynomial
  size_t orderDiff = newCluster.order - currentCluster.order;
  double clusterScore = 0.0;
  for(int i = 0; i < currentCluster.order; ++i)
  {
    // TODO(ry): is it correct to compare remainders in the same position but
    // corresponding to different points directly, or is there some
    // normalization necessary to map them to the same space?
    double remCurrent = std::abs(remaindersCurrent[i]);
    double remNew = std::abs(remaindersNew[i + orderDiff]);
    DBG("remCurrent = " << remCurrent);
    DBG("remNew = " << remNew);
    clusterScore = std::max(clusterScore, remNew / (remCurrent + divEps));
    DBG("clusterScore = " << clusterScore);
  }

  double const tolNewCurrent = 1300;
  bool clusterDividesPoly = clusterScore < tolNewCurrent;

  // NOTE(ry): compare higher-order remainders of new cluster to see if new
  // cluster divides polynomial in its full order.
  // it is possible the current cluster and new root are distinct, but their
  // average lies at another true root.
  // this check covers this edge case when the other true root order is less
  // than the order of the new cluster.
  // if the other true root order is at least the new cluster's order, this
  // check will fail and we will overcount that root.
  if(clusterDividesPoly)
  {
    double const tolHiLo = 200000;
    for(int i = 0; clusterDividesPoly && (i < orderDiff); ++i)
    {
      // TODO(ry): is it correct to compare remainders in different positions
      // directly, or is there some normalization necessary to map them to the
      // same space?
      double remHi = std::abs(remaindersNew[i]);
      double remLo = std::abs(remaindersNew[i+1]);
      DBG("remHi = " << remHi);
      DBG("remLo = " << remLo);
      double rat = remHi / (remLo + divEps);
      DBG("rat = " << rat);
      clusterDividesPoly = rat < tolHiLo;
    }
  }

  std::swap(remaindersCurrent, remaindersNew);
  if(clusterDividesPoly)
  {
    return newCluster;
  }
  else
  {
    // TODO(ry): divide out the coefficients to save evaluation iterations down the line?
    solns.push_back(currentCluster);
    remaindersCurrent.resize(0);
    return newRoot;
  }
}

void QR::extractRoots(SolutionSet& roots, const std::vector<double>& M, size_t degree, const Coefficients &coeffs)
{
    PROFILE_FUNCTION();

#if 0
  // DEBUG:
  bool firstrun = 1;
  int clusterCount = 1;
  Root oldSolution{};
  c128 lastrem{};
  ComplexCoefficients remainders(degree);
  remainders.resize(1);

    auto addRoot = [&](c128 newVal) {
      // DEBUG:
      Root newSolution;
      if(firstrun)
      {
	newSolution = Root{newVal, 1};
      }
      else
      {
	newSolution = mergeRoot(oldSolution, Root{newVal, 1});
      }
      if(degree >= 32)
      { int breakme = 1; }

      if(!firstrun)
      {
	const Root &betterSoln = betterDivisorOfPolynomial(coeffs, oldSolution, newSolution);
	if(&betterSoln == &oldSolution)
	{
	  DBG("old solution better, start of new cluster");
	  roots.push_back(oldSolution);
	  newSolution = Root{newVal, 1};
	  ++clusterCount;
	}
	else
	{
	  DBG("new solution better, same cluster (" << newSolution.order << ")");
	}
      }

      c128 rem = evaluatePolynomialAtRoot(coeffs, newSolution);
      lastrem = rem;
      c128 newRem = evaluatePolynomial(coeffs, newVal);
      DBG("poly evaluated at (" << newVal.real() << ", " << newVal.imag() << ")" << " = " << "(" << newRem.real() << ", " << newRem.imag() << ")");

      DBG("(" << newSolution.value.real() << ", " << newSolution.value.imag() << ")^" << newSolution.order << ((rootDividesPolynomial(coeffs, newSolution)) ? " divides" : " does not divide") << " polynomial");

      firstrun = 0;
      oldSolution = newSolution;

#if 0
        for (auto& [val, order] : roots)
        {
#if 0
            double diff_re = std::abs(val.real() - newVal.real());
            double diff_im = std::abs(val.imag() - newVal.imag());
            double scale = std::abs(val) + std::abs(newVal) + 1e-7; // + 1e-7 to avoid division with zero
	    if (diff_re / scale < tolerance && diff_im / scale < tolerance)
            {
                order++;
                val += (newVal - val) / static_cast<double>(order);
                return;
            }
#else
	    if ((juce::exactlyEqual(val.imag(), 0.0)) != (juce::exactlyEqual(newVal.imag(), 0.0)))
	    {
	      // NOTE(ry): comparing real with complex root
	      double x = juce::exactlyEqual(val.imag(), 0.0) ? val.real() : newVal.real(); // real
	      int x_order = juce::exactlyEqual(val.imag(), 0.0) ? order : 1;
	      c128 z = juce::exactlyEqual(val.imag(), 0.0) ? newVal : val; // complex
	      int z_order = juce::exactlyEqual(val.imag(), 0.0) ? 2 : 2*order;

	      double diff = std::abs(z.real() - x);
	      double err = diff / std::abs(val.real());
	      if (err < tolerance && z.imag() < tolerance)
	      {
		// NOTE(ry): merge the complex with the real root, creating real root
		// set order to twice the complex order plus the real order
		// set val to the weighted average of the two roots
		order = x_order + z_order;
		val = c128((x_order*x + z_order*z.real()) / double(order), 0);
		return;
	      }
	    }
	    else
	    {
	      // NOTE(ry): comparing real with real complex with complex
	      double diff_mag = std::abs(val - newVal);
	      double err = diff_mag / (std::abs(val) + 1e-7);
	      if (err < tolerance)
	      {
		// NOTE(ry): merge the roots
		// set val to the weighted average of the two roots
		// increment order by 1
		double val_re = (order*val.real() + newVal.real()) / double(order + 1);
		double val_im = (order*val.imag() + newVal.imag()) / double(order + 1);

		val = c128(val_re, val_im);
		order++;
		return;
	      }
	    }
#endif
        }
        roots.emplace_back(newVal, 1);
#endif
    };
#endif

    Root currentCluster{};
    remaindersCurrent.resize(0);
    //remaindersNew.resize(0);
    size_t i = 0;
    while (i < degree)
    {
        if ( i == degree - 1 || std::abs(M[(i+1) * degree + i]) < Epsilon )
        {
            // Real eigenvalue on diagonal
            c128 newRoot (M[i * degree + i], 0.0);
            //addRoot(newRoot);
	    currentCluster = updateSolutions(roots, coeffs, currentCluster, {newRoot, 1});
            ++i;
        }
        else
        {
            // check if conjugate by solving the equation which instantiates by the 2x2 cell:
            // https://www.physicsforums.com/threads/how-do-i-estimate-complex-eigenvalues.170108/post-1330547
            // https://www.mosismath.com/Eigenvalues/EigenvalsQR.html
            // det(A - λI) = 0
            // =>  det| a-λ    b |
            //        |   c  d-λ | = 0
            // => (a-λ)(d-λ) - bc = 0
            // => λ^2 - (a+d)λ + (ad-bc) = 0
            // => λ^2 - (a+d)λ + detA = 0 --> solve with discriminant
            const double a = M[i * degree + i];
            const double b = M[i * degree + i+1];
            const double c = M[(i+1) * degree + i];
            const double d = M[(i+1) * degree + i+1];
            const double det  = a*d - b*c;
            // solve with discriminant
            const double discriminant = (a+d)*(a+d) - 4.0*det;

            const double halfSum = 0.5 * (a + d);
            const double halfSqrt = 0.5 * std::sqrt(std::abs(discriminant));

            if ( discriminant >= 0.0)
            {
                // two real roots
                //addRoot(c128(halfSum + halfSqrt, 0.0));
                //addRoot(c128(halfSum - halfSqrt, 0.0));
		auto r0 = c128(halfSum + halfSqrt, 0.0);
		auto r1 = c128(halfSum - halfSqrt, 0.0);
		currentCluster = updateSolutions(roots, coeffs, currentCluster, {r0, 1});
		currentCluster = updateSolutions(roots, coeffs, currentCluster, {r1, 1});
            }
            else
            {
                // Complex conjugate pair
                const double re = halfSum;
                const double im = halfSqrt;
		auto r = c128(re, im);
		currentCluster = updateSolutions(roots, coeffs, currentCluster, {r, 1});
                //addRoot(c128(re,im));
                // addRoot(c128(re,-im)); // Note: this is added automatically later using FilterState::add method. Commenting this, removes the bug of overlapping roots.
            }
            i += 2;
        }
    }

    //roots.push_back(oldSolution);
    roots.push_back(currentCluster);
    DBG("counted " << roots.size() << " clusters");
}

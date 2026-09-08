QR::Matrix QR::Q, QR::R;

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

    double state2x2[4]{};
    size_t shift_idx {degree}, iter {0};
    std::vector<double> v(shift_idx); // vector for storing current column of A. Note: only the first {0 to (curr val of shift_idx - 1) } indexes are used per iteration.
    while(shift_idx > 1)
    {
        if (++iter > MaxIterations)
	{
	    DBG("hit MaxIterations " << MaxIterations << " for shift index " << shift_idx);
	    shift_idx -= 1;
	    iter = 0;
	    continue;
	}

	state2x2[0] = A[(shift_idx-2)*degree + shift_idx-2];
	state2x2[1] = A[(shift_idx-2)*degree + shift_idx-1];
	state2x2[2] = A[(shift_idx-1)*degree + shift_idx-2];
	state2x2[3] = A[(shift_idx-1)*degree + shift_idx-1];

        // subtract rayleigh quotient shift
        const double shift = A[(shift_idx - 1) * degree + (shift_idx - 1)];
        for (size_t i = 0; i < shift_idx; ++i)
            A[i * degree + i] -= shift; // Decompose (Ak - sI)

	// NOTE(ry): compute QR = A, A' = RQ using the default decomposition method
	decomp(A, degree, shift_idx);

        // add shift back in A and check sub-diagonal entries
        for (size_t i = 0; i < shift_idx; ++i)
            A[i * degree + i] += shift;

        // check only the last subdiagonal entry (real eigenvalue)
        if (std::abs(A[(shift_idx-1)* degree + (shift_idx-2)]) < Epsilon)
        {
	    if(std::abs(state2x2[1*2 + 0] - A[(shift_idx-1)*degree + (shift_idx-2)]) < Epsilon)
	    {
	        DBG("shift " << shift_idx << " converged after " << iter << " iterations (1x1)");
                shift_idx--;
                iter = 0;
	    }
	}
        // check the entry above the 2x2 block in search of complex conjugate pairs
        else if (shift_idx > 2 && std::abs(A[(shift_idx-2) * degree + (shift_idx-3)]) < Epsilon)
        {
	    DBG("shift " << shift_idx << " converged after " << iter << " iterations (2x2)");
	    shift_idx -= 2;
	    iter = 0;
	}
    }

    // Extract roots — read diagonal
    extractRoots(roots, A, degree, coefs);
    return roots;
}

void QR::decompGramSchmidt(Matrix &A, size_t degree, size_t shift_idx)
{
    PROFILE_FUNCTION();

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
}

void QR::decompHouseholder(Matrix &A, size_t degree, size_t shift_idx)
{
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
}

void QR::extractRoots(SolutionSet& roots, const std::vector<double>& M, size_t degree, const Coefficients &coeffs)
{
    PROFILE_FUNCTION();

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

    size_t i = 0;
    while (i < degree)
    {
        if ( i == degree - 1 || std::abs(M[(i+1) * degree + i]) < Epsilon )
        {
            // Real eigenvalue on diagonal
            c128 newRoot (M[i * degree + i], 0.0);
            addRoot(newRoot);
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
                addRoot(c128(halfSum + halfSqrt, 0.0));
                addRoot(c128(halfSum - halfSqrt, 0.0));
            }
            else
            {
                // Complex conjugate pair
                const double re = halfSum;
                const double im = halfSqrt;
                addRoot(c128(re,im));
                // addRoot(c128(re,-im)); // Note: this is added automatically later using FilterState::add method. Commenting this, removes the bug of overlapping roots.
            }
            i += 2;
        }
    }

    roots.push_back(oldSolution);
    DBG("counted " << clusterCount << " clusters");
#ifdef DEBUG_C2R
    std::cout<<"Calculated Roots: ";
    for (auto root : roots)
        std::cout<<"("<<root.value.real()<<","<<root.value.imag()<<") - "<<root.order<<", ";
    std::cout<<std::endl;
#endif

}

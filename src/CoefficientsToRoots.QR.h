class QR
{
  /*
    Class to convert polynomial coefficients into roots using a QR method.
  */

public:
  /*	Method used to convert polynomial coefficients into roots using a QR method.
	Consists of the following steps:
	- Preprocessing:
	- Filter out leading zeros if any
	- Filter out trailing zeros if any
	- Build companion matrix of Hessenberg form from given coefficients
	- Calculate rayleigh quotient shift
	- apply shifted QR iterations (starting from bottom right) with deflation when subdiagonal entries become ~0
	- Step 1 calculate Q0 via Gram Schmidt orthogonalization method
	- Step 2 calculate R0 :  R0 = Q0 A0
	- Step 3 calculate A1 : A1 = R0 Q0
	- Step 4 add shift back in A1 and check sub-diagonal entries for deflation
	- Extract eigenvalues:
	- merge roots that have smaller scaled difference than tolerance
	- scan subdiagonal:
	- if values on the subdiagonal smaller than Epsilon, then real root
	- else 2 solutions. Solve det(A - λI) = 0 for 2x2 square matrix:
	- if discriminant >= 0, then two real solutions
	- else then root has a complex conjugate (conjugate is not appended on returned vector, as this is done automatically when using FilterState::add to add that root in the Value tree)
	Returns complex roots paired with their corresponding order.
  */
  //static std::vector<std::pair<c128, int>> QR(std::vector<double> coefs);
  static SolutionSet Solve(Coefficients coeffs);
private:

  using Matrix = std::vector<double>;
  static Matrix Q, R;

  using DecompFn = void(*)(Matrix &, size_t, size_t, size_t);
  static void decompUpdateGramSchmidtExplicit(Matrix &A, size_t degree, size_t shift_idx);
  static void decompUpdateHouseholderExplicit(Matrix &A, size_t degree, size_t shift_idx);
  static void decompUpdateHouseholderImplicit(Matrix &A, size_t degree, size_t startIdx, size_t endIdx);
  static constexpr DecompFn decompUpdate = &decompUpdateHouseholderImplicit;

  static double shiftRayleigh(Matrix &A, size_t degree, size_t shift_idx);
  static void unshiftRayleigh(Matrix &A, size_t degree, size_t shift_idx, double shift);

  static bool signedBitSet(double num)
  { return *reinterpret_cast<u64*>(&num) & (1ULL << 63); }

  static void setSignedBit(double &num)
  { *reinterpret_cast<u64*>(&num) |= (1ULL << 63); }

  struct ClusterSolutionsState
  {
    ClusterSolutionsState(const Coefficients &_coeffs, SolutionSet &_roots, SolutionSet &_clusters)
      :coeffs(_coeffs)
      ,roots(_roots)
      ,clusters(_clusters)
    {
      if(clusters.size() == 0)
      {
	clusters.push_back(roots[0]);
	// NOTE(ry): why can't I just get a reference to the imaginary part?
	setSignedBit(reinterpret_cast<double(&)[2]>(roots[0].value)[1]);
	firstUnclusteredIndex = 1;
      }
    }

    const Coefficients &coeffs;
    SolutionSet &roots;
    SolutionSet &clusters;
    size_t firstUnclusteredIndex = 0;
  };

  static void clusterSolutions(ClusterSolutionsState &state);

  static ComplexCoefficients remaindersCurrent, remaindersNew;
  static bool compareRemainders(void);

  // NOTE(ry): tries adding new root to cluster. if it still divides and is
  // better guess, returns updated root; else adds old cluster to solns and
  // returns newRoot as cluster state.
  static Root updateSolutions(SolutionSet &solns, const Coefficients &coeffs, Root currentCluster, Root newRoot);

  // TODO Finetune these parameters

  /*	Threshold for detecting convergence (near-zero) of the subdiagonal elements in QR iteration.*/
  static constexpr double Epsilon = 1e-10;//1e-6;//1e-12;

  /*	Maximum QR iterations per eigenvalue block to prevent infinite loops.*/
  static constexpr size_t MaxIterations = 100;

  /* 	Threshold for considering two roots with negligible diff the same.
	Expressed in % after scaling differences, since zeros may lie outside the unit circle. */
  static constexpr double tolerance = 5e-2;

  /*	Extracts roots from the eigenvalues of the converged quasi-triangular QR matrix and merges duplicates.
	For more details see description of QR method */
  //static void extractRoots(std::vector<std::pair<c128, int>> &, const std::vector<double>&, size_t);
  static void extractRoots(SolutionSet&, const std::vector<double>&, size_t, const Coefficients &coeffs);
};

SOLVER_DEFINE(QR)

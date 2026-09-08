# QR ISSUES:
- 2x2 convergence check only checks zero entries, not whether eigenvalues have converged
- 2x2 fails to converge when 2x2 block is in upper-left (or its the only block cuz it's a 2x2 matrix w/ complex eigs) since there are no entries to the left
- guesses are still not good sometimes even for 1x1 convergence (tune epsilon; separate epsilons for zero check, eigenvalue difference)
  - see 4: 4 real order 1, 5: 5 real order 1 b
- starting with worst guesses (top-left) instead of best guesses (bottom-right); not good for clustering
- sometimes fails to converge then converges 1x1 after shift, so result is 2x2; why doesn't it converge 2x2 on first shift?

# CLUSTERING ISSUES:
- can't distiguish between distinct complex and real roots with same or similar real parts
  - combined root has first remainder 0 (since there is a real root there) but subsequent remainders are large, not taken into account since complex root didn't have those
    - check pt0 remainder clamping; maybe replacing clamp with small offset would make first remainder enough of an indicator w/o having to look at subsequent remainders
    - would probably not be a problem if the complex order was larger than the real order
  - see 5: 1 real order 1 + 2 complex order 1 a, 6: 1 real order 4 + 1 complex order 1 a
- similar to above, can't distinguish between distinct roots whose average is also a root
  - like above, can try to address by incorporating uncompared remainders into scoring check
  - see 7: 7 real order 1
- high-order real roots sometimes have a conjugate-pair component w/ small imag part; when these are added to an order-1 isolated root, the higher-order real part can dominate and the boundary is not distinguished
  - maybe mitigate by adding each element of conjugate pair one at a time.
	- introduces problems of distinguishing between true conjugate pairs and those representing real roots order >= 2, throwing away conjugates
  - can also try incorporating new root being considered into score, either as integrated score of old cluster vs. new cluster or separate score of new cluster vs. new root by itself if new cluster won first test.
  - problem might go away as QR solver gets more accurate
  - see 6: 1 real order 4 + 2 real order 1 a, 6: 3 real order 2

# FAILING TESTS:
-  ~~4: 2 real order 1 + 1 real order 2~~
-  ~~4: 4 real order 1~~
-  ~~5: 5 real order 1 b~~
-  ~~5: 3 real order 1 + 1 complex order 1 b (NEW WITH `QR::updateSolutions`)~~
-  ~~5: 1 real order 1 + 2 complex order 1 a~~
-  ~~6: 1 real order 4 + 1 complex order 1 a~~
-  ~~6: 1 real order 4 + 2 real order 1 a~~
-  ~~6: 1 real order 4 + 2 real order 1 b~~
-  ~~6: 3 real order 2~~
-  ~~7: 1 real order 6 + 1 real order 1~~
-  ~~7: 7 real order 1~~
-  ~~8: 1 real order 7 + 1 real order 1~~
-  ~~8: 8 real order 1~~
-  ~~8: 6 real order 1 + 1 complex order 1~~
-  ~~8: 4 real order 1 + 2 complex order 1~~
-  ~~9: 9 real order 1~~
-  ~~9: 7 real order 1 + 1 complex order 1~~
- ~~10: 1 real order 5 + 1 real order 4 + 1 real order 1~~
- ~~10: 1 real order 4 + 1 real order 3 + 1 real order 2 + 1 real order 1~~
- ~~10: 1 real order 4 + 1 real order 3 + 1 real order 1 + 1 complex order 1~~
- ~~32: 1 real order 32 (NEW WITH HOUSEHOLDER)~~

# THINGS TO WORK ON

## QR
- 2x2 convergence check
- epsilon fine-tuning
- more numerically stable QR decomposition (eg householder reflections, givens rotations)

## CLUSTERING
- adjust denominator clamping in score calculation
- incorporate new, uncoalesced root in scoring calculations
- incorporate remainders that can't be compared in scores
- tune threshold

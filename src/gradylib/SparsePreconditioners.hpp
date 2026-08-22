//
// Created by Grady Schofield on 8/22/26.
//

#ifndef CONTROL_SPARSEPRECONDITIONERS_HPP
#define CONTROL_SPARSEPRECONDITIONERS_HPP

#include"SparseDirect.hpp"
#include"SparseMatrix.hpp"

namespace gradylib {
    class KktPreconditioner {
        FixedSparseMatrix::DiagonalAndArrow AdiagAndArrow;
        std::vector<double> w;
        std::vector<double> v;
        double delta;
        double factor;
        sparse_direct_utilities::CholeskyDecomposition cholesky;
        size_t numDof;
        size_t numConstraints;
    public:

        KktPreconditioner(FixedSparseMatrix const & M, int numDof, int numConstraints)
            : AdiagAndArrow(M.getDiagonalAndArrow(numDof)), numDof(numDof), numConstraints(numConstraints)
        {
            using std::vector;
            using namespace gradylib::sparse_direct_utilities;
            using namespace gradylib::std_vector_operators;
            AdiagAndArrow.makeIntoPreconditioner();
            auto Dinv = M.diagonal();
            Dinv.resize(numDof);
            for (double & x : Dinv) {
                x = fabs(x);
                if (x == 0) {
                    x = 1;
                }
                x = 1.0 / x;
            }
            auto B = M.submatrix(numDof, 0, numDof+numConstraints, numDof);
            auto BBt = B.matrixTimesSelfTranspose(Dinv, true);
            cholesky = CholeskyDecomposition(BBt);
            w = vector<double>(AdiagAndArrow.g.size()+1);
            copy(AdiagAndArrow.g.begin(), AdiagAndArrow.g.end(), w.begin());
            for (size_t i = 0; i < w.size()-1; ++i) {
                w[i] *= Dinv[i];
            }
            w = B*w;
            for (size_t i = 0; i < w.size()-1; ++i) {
                w[i] -= B(i, numDof-1);
            }
            v = cholesky.solve(w);
            delta = AdiagAndArrow.alpha - dot(Dinv, AdiagAndArrow.g, AdiagAndArrow.g);
            factor = delta / (1 + delta * dot(w, v));
        }

        std::vector<double> operator()(std::vector<double> const & y) const {
            return solve(y);
        }

        std::vector<double> solve(std::vector<double> const & y) const {
            using std::vector;
            using namespace gradylib::std_vector_operators;
            vector<double> x(numDof + numConstraints);
            auto t = AdiagAndArrow.inverse(y);
            copy(t.begin(), t.end(), x.begin());
            vector<double> y2(numConstraints);
            copy(y.begin()+numDof, y.end(), y2.begin());
            vector<double> t2 = cholesky.solve(y2);
            double z = dot(w, t2) * factor;
            for (size_t i = 0; i < numConstraints; ++i) {
                x[i+numDof] = t2[i] - z * v[i];
            }
            return x;
        }
    };
}

#endif //CONTROL_SPARSEPRECONDITIONERS_HPP

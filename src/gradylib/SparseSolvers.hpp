#ifndef SPARSESOLVERS_HPP
#define SPARSESOLVERS_HPP

#include<optional>

#include"SparseMatrix.hpp"

namespace gradylib {
    struct minresOptions {
        std::optional<double> tolerance;
        std::optional<uint64_t> maxIterations;
        std::optional<bool> verbose;
        std::optional<std::vector<double>> x0;
    };

    std::vector<double> minres(FixedSparseMatrix const & A, std::vector<double> const & b, minresOptions opts = {}) {
        using namespace  gradylib::std_vector_operators;
        std::list<std::vector<double>>  x, v, w;
        std::list<double>  beta, gamma, sigma, alpha, resNorm;
        double eta;
        if (!opts.tolerance.has_value()) {
            opts.tolerance = 1E-12;
        }
        if (!opts.maxIterations.has_value()) {
            opts.maxIterations = std::numeric_limits<uint64_t>::max();
        }
        if (!opts.x0.has_value()) {
            opts.x0 = std::vector<double>(A.numRows());
        }
        if (!opts.verbose) {
            opts.verbose = false;
        }
        x.push_back(opts.x0.value());
        v.push_back(std::vector<double>(A.numRows()));
        v.push_back(b - A*x.back());
        beta.push_back(norm(v.back()));
        resNorm.push_back(beta.back());
        eta = beta.back();
        gamma.push_back(1.0);
        gamma.push_back(1.0);
        sigma.push_back(0.0);
        sigma.push_back(0.0);
        w.push_back(std::vector<double>(A.numRows()));
        w.push_back(std::vector<double>(A.numRows()));
        int iter = 0;
        double bNorm = norm(b);
        while (iter < opts.maxIterations.value() && resNorm.back()/bNorm > opts.tolerance.value()) {
            // Lanczos step
            v.back() = v.back() / beta.back();
            alpha.push_back(dot(v.back(), A*v.back()));
            v.push_back(A*v.back() - alpha.back()*v.back() - beta.back()**++v.rbegin());
            beta.push_back(norm(v.back()));
            //Old Givens on the new column of T
            double delta = gamma.back()*alpha.back() - *++gamma.rbegin()*sigma.back()**++beta.rbegin();
            double rho1 = sqrt(delta*delta + beta.back()*beta.back());
            double rho2 = sigma.back()*alpha.back() + *++gamma.rbegin()*gamma.back()**++beta.rbegin();
            double rho3 = *++sigma.rbegin()**++beta.rbegin();
            //New Givens
            gamma.push_back(delta/rho1);
            sigma.push_back(beta.back() / rho1);
            //Update solution
            w.push_back((*++v.rbegin() - rho3**++w.rbegin() - rho2*w.back())/rho1);
            x.push_back(x.back() + gamma.back()*eta*w.back());
            resNorm.push_back(sigma.back() * resNorm.back());
            eta = -sigma.back() * eta;
            ++iter;
            if (opts.verbose.value()) {
                std::cout << "iter " << iter << " rel resnorm: " << resNorm.back() / bNorm << " computed residual norm: " << resNorm.back() << " true residual norm: " << norm(b-A*x.back()) << "\n";
            }
        }
        return x.back();
    }

}

#endif

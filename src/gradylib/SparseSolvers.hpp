#ifndef SPARSESOLVERS_HPP
#define SPARSESOLVERS_HPP

#include<optional>
#include<functional>

#include"SparseMatrix.hpp"

namespace gradylib {
    struct cgOptions {
        std::optional<double> tolerance;
        std::optional<uint64_t> maxIterations;
        std::optional<bool> verbose;
        std::optional<std::vector<double>> x0;
        std::optional<std::function<std::vector<double>(std::vector<double>)>> preconditioner;
    };

    std::vector<double> cg(std::function<std::vector<double>(std::vector<double>)> Amul, std::vector<double> const & b, cgOptions opts = {}) {
        using namespace gradylib::std_vector_operators;
        auto numRows = b.size();
        if (!opts.tolerance.has_value()) {
            opts.tolerance = 1E-12;
        }
        if (!opts.maxIterations.has_value()) {
            opts.maxIterations = std::numeric_limits<uint64_t>::max();
        }
        if (!opts.x0.has_value()) {
            opts.x0 = std::vector<double>(numRows);
        }
        if (!opts.verbose) {
            opts.verbose = false;
        }
        auto Kinv = [&opts](std::vector<double> const & v) {
            if (opts.preconditioner.has_value()) {
                return opts.preconditioner.value()(v);
            } else {
                return v;
            }
        };
        std::list<std::vector<double>> r, w, p, q;
        std::list<double> rho, beta, alpha;
        r.push_back(b - Amul(opts.x0.value()));
        int iter = 1;
        double normB = norm(b);
        while (iter <= opts.maxIterations && norm(r.back())/normB > opts.tolerance) {
            w.push_back(Kinv(r.back()));
            rho.push_back(dot(w.back(), r.back()));
            if (iter == 1) {
                p.push_back(w.back());
            } else {
                beta.push_back(rho.back() / *++rho.rbegin());
                p.push_back(w.back() + beta.back() * p.back());
            }
            q.push_back(Amul(p.back()));
            alpha.push_back(rho.back() / dot(p.back(), q.back()));
            opts.x0.value() = opts.x0.value() + alpha.back() * p.back();
            r.push_back(r.back() - alpha.back() * q.back());
            if (opts.verbose.value()) {
                std::cout << "iter " << iter << " rel resnorm: " << norm(b-Amul(opts.x0.value()))/normB << "\n";
            }
            ++iter;
        }
        return opts.x0.value();
    }

    struct minresOptions {
        std::optional<double> tolerance;
        std::optional<uint64_t> maxIterations;
        std::optional<bool> verbose;
        std::optional<std::vector<double>> x0;
        std::optional<std::function<std::vector<double>(std::vector<double>)>> preconditioner;
    };

    std::vector<double> minresPrec(FixedSparseMatrix const & A, std::vector<double> const & b, minresOptions opts = {}) {
        using namespace  gradylib::std_vector_operators;
        std::list<std::vector<double>>  x, v, w, t;
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
        auto Kinv = [&opts](std::vector<double> const & v) {
            if (opts.preconditioner.has_value()) {
                return opts.preconditioner.value()(v);
            } else {
                return v;
            }
        };
        x.push_back(opts.x0.value());
        v.push_back(std::vector<double>(A.numRows()));
        t.push_back(std::vector<double>(A.numRows()));
        t.push_back(b - A*x.back());
        v.push_back(Kinv(t.back()));
        beta.push_back(sqrt(dot(t.back(), v.back())));
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
            t.back() = t.back() / beta.back();
            alpha.push_back(dot(v.back(), A*v.back()));
            t.push_back(A*v.back() - alpha.back()*t.back() - beta.back()**++t.rbegin());
            v.push_back(Kinv(t.back()));
            beta.push_back(sqrt(dot(t.back(), v.back())));
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
            if (iter > 2) {
                x.pop_front();
                v.pop_front();
                w.pop_front();
                t.pop_front();
            }
        }
        std::cout << "stats: " << iter << " " << norm(b - A*x.back())/norm(b) << "\n";
        return x.back();
    }

    std::vector<double> minres(FixedSparseMatrix const & A, std::vector<double> const & b, minresOptions opts = {}) {
        if (opts.preconditioner.has_value()) {
            return minresPrec(A, b, opts);
        }
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
        std::cout << "stats: " << iter << " " << norm(b - A*x.back())/norm(b) << "\n";
        return x.back();
    }

    std::vector<double> minres(std::function<std::vector<double>(std::vector<double>)> Amul, std::vector<double> const & b, minresOptions opts = {}) {
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
        auto Arows = b.size();
        if (!opts.x0.has_value()) {
            opts.x0 = std::vector<double>(Arows);
        }
        if (!opts.verbose) {
            opts.verbose = false;
        }
        x.push_back(opts.x0.value());
        v.push_back(std::vector<double>(Arows));
        v.push_back(b - Amul(x.back()));
        beta.push_back(norm(v.back()));
        resNorm.push_back(beta.back());
        eta = beta.back();
        gamma.push_back(1.0);
        gamma.push_back(1.0);
        sigma.push_back(0.0);
        sigma.push_back(0.0);
        w.push_back(std::vector<double>(Arows));
        w.push_back(std::vector<double>(Arows));
        int iter = 0;
        double bNorm = norm(b);
        while (iter < opts.maxIterations.value() && resNorm.back()/bNorm > opts.tolerance.value()) {
            // Lanczos step
            v.back() = v.back() / beta.back();
            alpha.push_back(dot(v.back(), Amul(v.back())));
            v.push_back(Amul(v.back()) - alpha.back()*v.back() - beta.back()**++v.rbegin());
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
                std::cout << "iter " << iter << " rel resnorm: " << resNorm.back() / bNorm << " computed residual norm: " << resNorm.back() << " true residual norm: " << norm(b-Amul(x.back())) << "\n";
            }
        }
        std::cout << "stats: " << iter << " " << norm(b - Amul(x.back()))/norm(b) << "\n";
        return x.back();
    }

}

#endif

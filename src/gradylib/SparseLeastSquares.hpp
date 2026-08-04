#ifndef SPARSELEASTSQUARES_HPP
#define SPARSELEASTSQUARES_HPP

#include<numeric>
#include<optional>

#include"SparseMatrix.hpp"

namespace gradylib {
    struct cglsOptions {
        std::optional<double> tolerance;
        std::optional<std::vector<double>> x0;
        std::optional<FixedSparseMatrix> At;
        std::optional<uint64_t> maxIterations;
        std::optional<bool> verbose;
        //preconditioner
    };

    namespace std_vector_operators {
        inline double norm(std::vector<double> const & v) {
            return sqrt(std::accumulate(v.begin(), v.end(), 0.0, [](double sum, double element){ return sum + element*element;}));
        }

        inline std::vector<double> normalize(std::vector<double> v) {
            double n = 1.0 / norm(v);
            for (double & t : v) t *= n;
            return v;
        }

        inline std::vector<double> operator-(std::vector<double> const & x, std::vector<double> const & y) {
            std::vector<double> ret(x.size());
            for (auto [ret, x, y] : std::views::zip(ret, x, y)) {
                ret = x - y;
            }
            return ret;
        }

        inline std::vector<double> operator+(std::vector<double> const & x, std::vector<double> const & y) {
            std::vector<double> ret(x.size());
            for (auto [ret, x, y] : std::views::zip(ret, x, y)) {
                ret = x + y;
            }
            return ret;
        }

        inline std::vector<double> operator*(double a, std::vector<double> const & x) {
            std::vector ret(x);
            for (auto & t : ret) {
                t *= a;
            }
            return ret;
        }

        inline double dot(std::vector<double> const & x, std::vector<double> const & y) {
            double d = 0;
            for (auto [x, y] : std::views::zip(x, y)) {
                d += x*y;
            }
            return d;
        }

        inline std::vector<double> randomVector(uint32_t n) {
            std::vector<double> ret(n);
            double a = 1.0 / RAND_MAX;
            for (double & t : ret) t = rand()*a;
            return ret;
        }
    }


    inline std::vector<double> cgls(FixedSparseMatrix const & A, std::vector<double> const & b, cglsOptions opts = {}) {
        using namespace std_vector_operators;
        std::list<double> rho, beta, alpha;
        std::list<std::vector<double>> s, x, r, p, q;
        if (!opts.At.has_value()) {
            opts.At.emplace(A.transpose());
        }
        FixedSparseMatrix const & At = opts.At.value();
        if (!opts.x0.has_value()) {
            opts.x0.emplace(std::vector<double>(A.numColumns()));
        }
        double tol = opts.tolerance.value_or(1E-2);
        uint64_t maxIter = opts.maxIterations.value_or(std::numeric_limits<uint64_t>::max());
        x.push_back(opts.x0.value());
        r.push_back(b - A*x.back());
        s.push_back(At*r.back());
        int i = 1;
        double normB = norm(b);
        while (norm(r.back())/normB > tol && i < maxIter) {
            rho.push_back(dot(s.back(), s.back()));
            if (i == 1) {
                p.push_back(s.back());
            } else {
                beta.push_back(rho.back() / *++rho.rbegin());
                p.push_back(s.back() + beta.back()*p.back());
            }
            q.push_back(A*p.back());
            alpha.push_back(rho.back() / dot(q.back(), q.back()));
            x.push_back(x.back() + alpha.back() * p.back());
            r.push_back(r.back() - alpha.back() * q.back());
            s.push_back(At*r.back());
            ++i;
            if (opts.verbose.value_or(false) && i % 100 == 0) {
                std::cout << "CGLS norm: " << norm(r.back())/normB << " iter: " << i << "\n";
            }
            if (i > 2) {
                p.pop_front();
                q.pop_front();
                r.pop_front();
                s.pop_front();
                x.pop_front();
                alpha.pop_front();
                beta.pop_front();
            }
        }
        if (opts.verbose.value_or(false)) {
            std::cout << "CGLS norm: " << norm(r.back())/normB << " iter: " << i << "\n";
        }
        return x.back();
    }
}

#endif

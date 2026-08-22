//
// Created by Grady Schofield on 8/18/26.
//

#ifndef CONTROL_SPARSEDIRECT_HPP
#define CONTROL_SPARSEDIRECT_HPP

#include<ranges>

#include"SparseMatrix.hpp"
#include"OpenHashSetTC.hpp"

namespace gradylib {
    namespace sparse_direct_utilities {

        class SparseUpperTriangularMatrix {
            std::vector<std::vector<uint32_t>> rowIndexes;
            std::vector<std::vector<double>> columns;
        public:

            SparseUpperTriangularMatrix() = default;

            SparseUpperTriangularMatrix(std::vector<std::vector<uint32_t>> && rowIndexes, std::vector<std::vector<double>> && columns)
                : rowIndexes(std::move(rowIndexes)), columns(std::move(columns))
            {
            }

            std::vector<double> solve(std::vector<double> y) const {
                std::vector<double> x(y.size());
                for (uint32_t n = 0; n < rowIndexes.size(); ++n) {
                    uint32_t j = rowIndexes.size() - 1 - n;
                    std::vector<double> const & c = columns[j];
                    double t = y[j] / c.back();
                    x[j] = t;
                    std::vector<uint32_t> const & rIdx = rowIndexes[j];
                    for (uint32_t i = 0; i < rIdx.size()-1; ++i) {
                        y[rIdx[i]] -= t * c[i];
                    }
                }
                return x;
            }
        };

        class SparseLowerTriangularMatrix {
            std::vector<std::vector<uint32_t>> rowIndexes;
            std::vector<std::vector<double>> columns;
        public:

            SparseLowerTriangularMatrix() = default;

            SparseLowerTriangularMatrix(uint32_t n)
                : rowIndexes(n), columns(n)
            {
            }

            void addRow(uint32_t i, std::vector<uint32_t> const & columnIndexes, std::vector<double> const & values) {
                for (auto [j, el] : std::views::zip(columnIndexes, values)) {
                    rowIndexes[j].push_back(i);
                    columns[j].push_back(el);
                }
            }

            void fillDiagonal(uint32_t i, double value) {
                rowIndexes[i].push_back(i);
                columns[i].push_back(value);
            }

            //To be used in a sparse Cholesky factorization
            //y is fit to have (xIndexes.back() - xIndexes.front()) number of elements
            std::vector<double> solve(std::vector<uint32_t> const & xIndexes, std::vector<double> y) const {
                std::vector<double> x;
                x.reserve(xIndexes.size());
                for (uint32_t k = 0; k < xIndexes.size(); ++k) {
                    uint32_t j = xIndexes[k];
                    std::vector<uint32_t> const & rIdx = rowIndexes[j];
                    std::vector<double> const & cols = columns[j];
                    double t = y[j-xIndexes.front()] / cols[0];
                    x.push_back(t);
                    for (uint32_t m = 1; m < rIdx.size() && rIdx[m] <= xIndexes.back(); ++m) {
                        y[rIdx[m]-xIndexes.front()] -= cols[m] * t;
                    }
                }
                return x;
            }

            std::vector<double> solve(std::vector<double> y) const {
                std::vector<double> x(y.size());
                for (uint32_t j = 0; j < rowIndexes.size(); ++j) {
                    std::vector<double> const & cols = columns[j];
                    double t = y[j] / cols[0];
                    x[j] = t;
                    std::vector<uint32_t> const & rIdx = rowIndexes[j];
                    for (uint32_t i = 1; i < rIdx.size(); ++i) {
                        y[rIdx[i]] -= t * cols[i];
                    }
                }
                return x;
            }

            SparseUpperTriangularMatrix transpose() const {
                FreeSparseMatrix m;
                for (uint32_t j = 0; j < rowIndexes.size(); ++j) {
                    for (auto [i, val] : std::views::zip(rowIndexes[j], columns[j])) {
                        m(j, i) = val;
                    }
                }
                std::vector<uint32_t> columnCounts(rowIndexes.size());
                for (auto [i, j, elem] : m) {
                    columnCounts[j] += 1;
                }
                std::vector<std::vector<uint32_t>> rowIndexes2(rowIndexes.size());
                std::vector<std::vector<double>> columns2(rowIndexes.size());
                for (uint32_t i = 0; i < rowIndexes2.size(); ++i) {
                    rowIndexes2[i].reserve(columnCounts[i]);
                    columns2[i].reserve(columnCounts[i]);
                }
                for (auto [i, j, elem] : m) {
                    rowIndexes2[j].push_back(i);
                    columns2[j].push_back(elem);
                }
                struct Data {
                    uint32_t row;
                    double el;
                };
                std::vector<Data> sorted;
                for (uint32_t i = 0; i < rowIndexes2.size(); ++i) {
                    for (auto [j, el] : std::views::zip(rowIndexes2[i], columns2[i])) {
                        sorted.emplace_back(j, el);
                    }
                    sort(sorted.begin(), sorted.end(), [](auto x, auto y) {return x.row < y.row;});
                    for (auto [j, el, sortedData] : std::views::zip(rowIndexes2[i], columns2[i], sorted)) {
                        j = sortedData.row;
                        el = sortedData.el;
                    }
                    sorted.clear();
                }
                return SparseUpperTriangularMatrix(std::move(rowIndexes2), std::move(columns2));
            }
        };

        class EliminationTree {
            std::vector<uint32_t> _parent;

        public:
            EliminationTree(FixedSparseMatrix const & A)
                : _parent(A.numRows())
            {
                for (uint32_t i = 0; i < A.numRows(); ++i) {
                    _parent[i] = i;
                }
                std::vector<uint32_t> Ak_idx;
                std::vector<double> Ak;
                for (uint32_t i = 1; i < A.numRows(); ++i) {
                    A.fillLowerTriangularRow(Ak_idx, Ak, i);
                    for (auto ii : Ak_idx) {
                        while (_parent[ii] != ii) {
                            ii = _parent[ii];
                        }
                        _parent[ii] = i;
                    }
                }
            }

            std::vector<uint32_t> reach(std::vector<uint32_t> const & Ak, int limit = std::numeric_limits<uint32_t>::max()) {
                OpenHashSetTC<uint32_t> r;
                for (uint32_t i : Ak) {
                    while (i < limit && !r.insert(i) && _parent[i] != i) {
                        i = _parent[i];
                    }
                }
                std::vector<uint32_t> v;
                v.reserve(r.size());
                // Need to implement LegacyInputIterator requirements everywhere so std::vector v(r.begin(), r.end()); works
                for (uint32_t i : r) v.push_back(i);
                sort(v.begin(), v.end());
                return v;
            }

            uint32_t root(uint32_t i ) const {
                while (_parent[i] != i) {
                    i = _parent[i];
                }
                return i;
            }

            uint32_t parent(uint32_t i) const {
                return _parent[i];
            }
        };

        class CholeskyDecomposition {
            SparseLowerTriangularMatrix L;
            SparseUpperTriangularMatrix Lt;
        public:

            CholeskyDecomposition() = default;

            CholeskyDecomposition(FixedSparseMatrix const & m) {
                EliminationTree etree(m);
                std::vector<double> Ak;
                std::vector<uint32_t> Ak_idx;
                std::vector<double> y;
                y.reserve(m.numRows());
                L = SparseLowerTriangularMatrix(m.numRows());
                for (uint32_t i = 0; i < m.numRows(); ++i) {
                    double d = 0;
                    if (i > 0) {
                        m.fillLowerTriangularRow(Ak_idx, Ak, i);
                        std::vector<uint32_t> xIndexes = etree.reach(Ak_idx, i);
                        if (!xIndexes.empty()) {
                            y.resize(1 + xIndexes.back() - xIndexes.front(), 0.0);
                            for (auto [j, el] : std::views::zip(Ak_idx, Ak)) {
                                y[j-xIndexes.front()] = el;
                            }
                            auto Lk = L.solve(xIndexes, y);
                            L.addRow(i, xIndexes, Lk);
                            for (double x : Lk) {
                                d += x*x;
                            }
                        }
                    }
                    L.fillDiagonal(i, sqrt(m(i,i) - d));
                    Ak.clear();
                    Ak_idx.clear();
                    y.clear();
                }
                Lt = L.transpose();
            }

            std::vector<double> solve(std::vector<double> y) const {
                return Lt.solve(L.solve(y));
            }
        };
    }

}

#endif

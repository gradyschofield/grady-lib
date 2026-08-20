//
// Created by Grady Schofield on 8/18/26.
//

#ifndef CONTROL_SPARSEDIRECT_HPP
#define CONTROL_SPARSEDIRECT_HPP

#include"SparseMatrix.hpp"
#include"OpenHashSetTC.hpp"

namespace gradylib {
    namespace sparse_direct_utilities {

        class SparseUpperTriangularMatrix {
            std::vector<std::vector<uint32_t>> columnIndexes;
            std::vector<std::vector<double>> rows;
        public:

            SparseUpperTriangularMatrix(std::vector<std::vector<uint32_t>> && columnIndexes, std::vector<std::vector<double>> && rows)
                : columnIndexes(std::move(columnIndexes)), rows(std::move(rows))
            {
            }

            std::vector<double> solve(std::vector<double> y) const {
                std::vector<double> x(y.size());
                for (uint32_t n = 0; n < columnIndexes.size(); ++n) {
                    uint32_t j = columnIndexes.size() - 1 - n;
                    std::vector<double> const & r = rows[j];
                    double t = y[j] / r[0];
                    x[j] = t;
                    std::vector<uint32_t> const & cIdx = columnIndexes[j];
                    for (uint32_t i = 1; i < columnIndexes.size(); ++i) {
                        y[cIdx[i]] -= t * r[i];
                    }
                }
                return x;
            }
        };

        class SparseLowerTriangularMatrix {
            std::vector<std::vector<uint32_t>> rowIndexes;
            std::vector<std::vector<double>> columns;
        public:

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
                    double t = y[k] / cols[0];
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
                    for (uint32_t i = 1; i < rowIndexes.size(); ++i) {
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
                std::vector<uint32_t> columnCounts(rowIndexes);
                for (auto [i, j, elem] : m) {
                    columnCounts[i] += 1;
                }
                std::vector<std::vector<uint32_t>> columnIndexes(rowIndexes.size());
                std::vector<std::vector<double>> rows(rowIndexes.size());
                for (uint32_t i = 0; i < columnIndexes.size(); ++i) {
                    columnIndexes[i].reserve(columnCounts[i]);
                    rows[i].reserve(columnCounts[i]);
                }
                for (auto [i, j, elem] : m) {
                    columnIndexes[j].push_back(i);
                    rows[j].push_back(elem);
                }
                struct Data {
                    uint32_t col;
                    double el;
                };
                std::vector<Data> sorted;
                for (uint32_t i = 0; i < columnIndexes.size(); ++i) {
                    for (auto [j, el] : std::views::zip(columnIndexes[i], rows[i])) {
                        sorted.emplace_bcak(j, el);
                    }
                    sort(sorted.begin(), sorted.end(), [](auto x, auto y) {return x.col < y.col;});
                    for (auto [j, el, sortedData] : std::views::zip(columnIndexes[i], rows[i], sorted)) {
                        j = sortedData.col;
                        el = sortedData.el;
                    }
                    sorted.clear();
                }
                return SparseUpperTriangularMatrix(std::move(columnIndexes), std::move(rows));
            }
        };

        class EliminationTree {
            std::vector<uint32_t> parent;
        public:
            EliminationTree(FixedSparseMatrix const & A) {
            }

            std::vector<uint32_t> reach(std::vector<uint32_t> const & Ak) {
                OpenHashSetTC<uint32_t> r;
                for (uint32_t i : Ak) {
                    while (!r.insert(i) && parent[i] != i) {
                        i = parent[i];
                    }
                }
                std::vector<uint32_t> v;
                v.reserve(r.size());
                // Need to implement LegacyInputIterator requirements everywhere so std::vector v(r.begin(), r.end()); works
                for (uint32_t i : r) v.push_back(i);
                sort(v.begin(), v.end());
                return v;
            }
        };

        class CholeskyDecomposition {
            SparseLowerTriangularMatrix L;
            SparseUpperTriangularMatrix Lt;
        public:

            CholeskyDecomposition(FixedSparseMatrix const & m) {
                EliminationTree etree(m);
                std::vector<double> Ak;
                std::vector<uint32_t> Ak_idx;
                std::vector<double> y;
                y.reserve(m.numRows());
                SparseLowerTriangularMatrix L(m.numRows());
                for (uint32_t i = 0; i < m.numRows(); ++i) {
                    double d = 0;
                    if (i > 0) {
                        m.fillLowerTriangularRow(Ak_idx, Ak, i);
                        std::vector<uint32_t> xIndexes = etree.reach(Ak_idx);
                        y.resize(xIndexes.back() - xIndexes.front());
                        for (auto [j, el] : std::views::zip(Ak_idx, Ak)) {
                            y[j] = el;
                        }
                        auto Lk = L.solve(xIndexes, y);
                        L.addRow(i, xIndexes, Lk);
                        for (double x : Lk) {
                            d += x*x;
                        }
                    }
                    L.fillDiagonal(i, sqrt(m(i,i) - d));
                    Ak.clear();
                    Ak_idx.clear();
                    y.clear();
                }
            }

            std::vector<double> solve(std::vector<double> y) const {
                return Lt.solve(L.solve(y));
            }
        };
    }

    sparse_direct_utilities::SparseLowerTriangularMatrix choleskyDecomposition(FixedSparseMatrix const & A) {
        using namespace sparse_direct_utilities;
        EliminationTree etree(A);
    }
}

#endif

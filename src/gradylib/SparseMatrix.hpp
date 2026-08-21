#ifndef SPARSEMATRIX_HPP
#define SPARSEMATRIX_HPP

#include<cassert>
#include<list>
#include<numeric>
#include<sstream>
#include<type_traits>
#include<utility>
#include<vector>
#include<ranges>

#include"OpenHashMapTC.hpp"
#include "SparseMatrix.hpp"

namespace gradylib {
    namespace file_utilities {
        template<typename T>
        static decltype(auto) tole(T i) {
            if constexpr (std::endian::native == std::endian::big) {
                return std::byteswap(i);
            } else {
                return i;
            }
        }

        template<typename T>
        static decltype(auto) fromle(T i) {
            if constexpr (std::endian::native == std::endian::big) {
                return std::byteswap(i);
            } else {
                return i;
            }
        }

        template<typename Elem>
        std::vector<Elem> vectorFromFile(std::filesystem::path path) {
            if (!std::filesystem::exists(path)) {
                return std::vector<Elem>{};
            }
            std::ifstream ifs(path);
            auto readle = [&ifs]<typename T>(T & x) {
                static_assert(std::is_trivially_copyable_v<T>);
                ifs.read(reinterpret_cast<char *>(&x), sizeof(x));
            };
            auto readArray = [&ifs, &readle]<typename T>() {
                uint64_t numElements;
                readle(numElements);
                size_t numBytes = sizeof(T) * numElements;
                std::vector<T> ret(numElements);
                size_t offset = 0;
                constexpr auto maxArg = std::numeric_limits<std::streamsize>::max();
                while (numBytes > maxArg) {
                    ifs.read(reinterpret_cast<char *>(ret.data() + offset), maxArg);
                    offset += maxArg;
                    numBytes -= maxArg;
                }
                ifs.read(reinterpret_cast<char *>(ret.data() + offset), numBytes);
                return ret;
            };

            return readArray.template operator()<Elem>();
        }

        template<typename Elem>
        void toFile(std::vector<Elem> const & v, std::filesystem::path path) {
            std::ofstream ofs(path, std::ios::binary);
            auto writele = [&ofs]<typename T>(const T& x) {
                static_assert(std::is_trivially_copyable_v<T>);
                auto t = tole(x);
                ofs.write(reinterpret_cast<char const *>(&t), sizeof(x));
            };
            auto writeArray = [&ofs, &writele](const auto& p, uint64_t numElements) {
                using Tptr = std::remove_cvref_t<decltype(p)>;
                static_assert(std::is_pointer_v<Tptr>);
                using T = std::remove_pointer_t<Tptr>;
                size_t numBytes = sizeof(T) * numElements;
                writele(numElements);
                constexpr auto maxArg = std::numeric_limits<std::streamsize>::max();
                size_t offset = 0;
                while (numBytes > maxArg) {
                    ofs.write(reinterpret_cast<char const *>(p+offset), maxArg);
                    offset += maxArg;
                    numBytes -= maxArg;
                }
                ofs.write(reinterpret_cast<char const *>(p+offset), numBytes);
            };

            writeArray(v.data(), v.size());
        }

    }

    namespace std_vector_operators {
        inline void zero(std::vector<double> & v) {
            memset(v.data(), 0, sizeof(double) * v.size());
        }

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

        inline std::vector<double> operator*(std::vector<double> const & x, std::vector<double> const & y) {
            assert(x.size() <= y.size());
            std::vector ret(x);
            for (size_t i = 0; i < x.size(); ++i) {
                ret[i] *= y[i];
            }
            return ret;
        }

        inline std::vector<double> operator/(std::vector<double> const & x, double a) {
            std::vector ret(x);
            double ainv = 1.0 / a;
            for (auto & t : ret) {
                t *= ainv;
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

        template<template<typename...> typename Container>
        std::vector<double> orth(std::vector<double> w, Container<std::vector<double>> const & V) {
            double n1 = norm(w);
            for (auto & v : V) {
                double d = dot(w, v);
                for (size_t i = 0; i < w.size(); ++i) {
                    w[i] -= d*v[i];
                }
            }
            if (norm(w) < n1/2) {
                return orth(w, V);
            }
            return normalize(w);
        }

        inline std::vector<double> randomVector(uint32_t n) {
            std::vector<double> ret(n);
            double a = 1.0 / RAND_MAX;
            for (double & t : ret) t = rand()*a;
            return ret;
        }
    }

    class MatrixCoordinate {
        uint32_t i;
        uint32_t j;
    public:
        MatrixCoordinate() = default;
        MatrixCoordinate(MatrixCoordinate const &) = default;

        MatrixCoordinate(uint32_t i, uint32_t j)
                : i(i), j(j)
        {
        }

        uint32_t getI() const {
            return i;
        }

        uint32_t getJ() const {
            return j;
        }

        MatrixCoordinate transpose() const {
            return MatrixCoordinate(j, i);
        }

        bool operator==(MatrixCoordinate const & mc) const {
            return i == mc.i && j == mc.j;
        }

        bool operator<(MatrixCoordinate const & mc) const {
            if (i < mc.i) {
                return true;
            } else if (i == mc.i) {
                return j < mc.j;
            }
            return false;
        }
    };
}

namespace std {
    template<>
    struct hash<gradylib::MatrixCoordinate> {
        size_t operator()(gradylib::MatrixCoordinate const & mc) const noexcept {
            return mc.getI() * 7282340493LL + mc.getJ();
        }
    };
}

namespace gradylib {

    class FreeSparseMatrix;

    class FixedSparseMatrix {
        std::vector<std::vector<double>> rows;
        std::vector<std::vector<uint32_t>> columnIndexes;
        uint32_t _numColumns = 0;

        // This is just a helper to get around FreeSparseMatrix being incomplete at this point
        template<typename T>
        FixedSparseMatrix transposeT() const {
            T sm;
            for (uint32_t i = 0; i < columnIndexes.size(); ++i) {
                for (auto [j, value] : std::views::zip(columnIndexes[i], rows[i])) {
                    sm.operator()(j,i) = value;
                }
            }
            return sm.makeFixed();
        }

    public:
        FixedSparseMatrix() = default;
        FixedSparseMatrix(FixedSparseMatrix const & m) = default;
        FixedSparseMatrix(FixedSparseMatrix && m) = default;

        FixedSparseMatrix(std::vector<std::vector<double>> && rows, std::vector<std::vector<uint32_t>> && columnIndexes, uint32_t numColumns)
            : rows(std::move(rows)), columnIndexes(std::move(columnIndexes)), _numColumns(numColumns)
        {
        }


        FixedSparseMatrix & operator=(FixedSparseMatrix && m) noexcept {
            rows = std::move(m.rows);
            columnIndexes = std::move(m.columnIndexes);
            _numColumns = m._numColumns;
            return *this;
        }

        FixedSparseMatrix(std::vector<std::list<uint32_t>> const & nonzeros)
            : rows(nonzeros.size()), columnIndexes(nonzeros.size()), _numColumns(0)
        {
            for (int i = 0; i < nonzeros.size(); ++i) {
                rows[i].resize(nonzeros[i].size());
                columnIndexes[i].reserve(nonzeros[i].size());
                for (uint32_t j : nonzeros[i]) {
                    columnIndexes[i].push_back(j);
                    _numColumns = std::max(_numColumns, j+1);
                }
            }
        }

        bool empty() const {
            return rows.empty();
        }

        uint32_t numColumns() const {
            return _numColumns;
        }

        uint32_t numRows() const {
            return columnIndexes.size();
        }

        struct DiagonalAndArrow {
            std::vector<double> D;
            std::vector<double> g;
            std::vector<double> Dinv;
            double alpha;
            size_t N;

            DiagonalAndArrow(size_t N)
                : D(N-1), g(N-1), N(N)
            {
            }

            void makeIntoPreconditioner(double eps = 1E-4) {
                using namespace gradylib::std_vector_operators;
                for (double & x : D) {
                    x = fabs(x);
                    if (x == 0) {
                        x = 1.0;
                    }
                }
                Dinv = D;
                for (double & x : Dinv) x = 1.0 / x;
                double t = dot(g, Dinv*g);
                double z = alpha - t;
                if (z < 0) {
                    alpha += -z + eps;
                }
            }

            // The length of y can exceed N which aids in the use of this function in certain preconditioners
            std::vector<double> inverse(std::vector<double> const & y) {
                using namespace gradylib::std_vector_operators;
                double gamma = y[N-1];
                double beta = (gamma - dot(g, Dinv*y)) / (alpha - dot(g, Dinv*g));
                std::vector<double> x(N);
                x.back() = beta;
                for (int i = 0; i < N-1; ++i) {
                    x[i] = Dinv[i] * (y[i] - beta * g[i]);
                }
                return x;
            }
        };

        DiagonalAndArrow getDiagonalAndArrow(int N) const {
            DiagonalAndArrow ret(N);
            for (int i = 0; i < N-1; ++i) {
                std::vector<uint32_t> const & cIdxs = columnIndexes[i];
                auto iter = std::lower_bound(cIdxs.begin(), cIdxs.end(), i);
                if (iter == cIdxs.end() || *iter != i) {
                    ret.D[i] = 0;
                } else {
                    auto idx = std::distance(cIdxs.begin(), iter);
                    ret.D[i] = rows[i][idx];
                }

                iter = std::lower_bound(cIdxs.begin(), cIdxs.end(), N-1);
                if (iter == cIdxs.end() || *iter != N-1) {
                    ret.g[i] = 0;
                } else {
                    auto idx = std::distance(cIdxs.begin(), iter);
                    ret.g[i] = rows[i][idx];
                }
            }
            auto iter = std::lower_bound(columnIndexes[N-1].begin(), columnIndexes[N-1].end(), N-1);
            if (iter == columnIndexes[N-1].end() || *iter != N-1) {
                ret.alpha = 0;
            } else {
                auto idx = std::distance(columnIndexes[N-1].begin(), iter);
                ret.alpha = rows[N-1][idx];
            }
            return ret;
        }

        FixedSparseMatrix submatrix(uint32_t i1, uint32_t j1, uint32_t i2, uint32_t j2) const {
            std::vector<std::vector<double>> _rows(i2-i1);
            std::vector<std::vector<uint32_t>> _columnIndexes(i2-i1);
            for (int i = i1; i < i2; ++i) {
                std::vector<uint32_t> const & cIdxs = columnIndexes[i];
                auto iter1 = std::lower_bound(cIdxs.begin(), cIdxs.end(), j1);
                auto iter2 = std::upper_bound(cIdxs.begin(), cIdxs.end(), j2);
                auto count = distance(iter1, iter2);
                auto offset = distance(cIdxs.begin(), iter1);
                auto iter3 = rows[i].begin() + offset;
                std::vector<double> vals;
                std::vector<uint32_t> cols;
                vals.reserve(count);
                cols.reserve(count);
                while (iter1 != iter2) {
                    cols.push_back(*iter1-j1);
                    vals.push_back(*iter3);
                    ++iter1;
                    ++iter3;
                }
                _rows[i-i1] = std::move(vals);
                _columnIndexes[i-i1] = std::move(cols);
            }
            return FixedSparseMatrix{std::move(_rows), std::move(_columnIndexes), j2-j1};
        }

        void fillLowerTriangularRow(std::vector<uint32_t> & Ak_idx, std::vector<double> & Ak, uint32_t const i) const {
            Ak_idx.clear();
            Ak.clear();
            std::vector<uint32_t> const & cIdxs = columnIndexes[i];
            std::vector<double> const & row = rows[i];
            uint32_t k = 0;
            while (cIdxs[k] < i) {
                Ak_idx.push_back(cIdxs[k]);
                Ak.push_back(row[k]);
                ++k;
            }
        }

        double operator()(uint32_t i, uint32_t j) const {
            std::vector<uint32_t> const & cIdxs = columnIndexes[i];
            auto iter = lower_bound(cIdxs.begin(), cIdxs.end(), j);
            if (iter == cIdxs.end() || *iter != i) {
                return 0;
            }
            auto idx = distance(cIdxs.begin(), iter);
            return rows[i][idx];
        }

        std::vector<double> diagonal() const {
            std::vector<double> diag(std::min(numRows(), numColumns()));
            for (int i = 0; i < diag.size(); ++i) {
                std::vector<uint32_t> const & cIdxs = columnIndexes[i];
                auto iter = lower_bound(cIdxs.begin(), cIdxs.end(), i);
                if (iter == cIdxs.end() || *iter != i) {
                    diag[i] = 0;
                } else {
                    auto idx = distance(cIdxs.begin(), iter);
                    diag[i] = rows[i][idx];
                }
            }
            return diag;
        }

        double rowSquareSum(uint32_t i) const {
            double s = 0;
            for (double x : rows[i]) {
                s += x*x;
            }
            return s;
        }

        double rowSquareSum(uint32_t i, std::vector<double> const & scale) const {
            double s = 0;
            for (int j = 0; j < rows[i].size(); ++j) {
                s += rows[i][j] * rows[i][j] * scale[columnIndexes[i][j]];
            }
            return s;
        }

        uint32_t maxDistanceOffDiagonal() const {
            uint32_t ret = 0;
            for (int i = 0; i < numRows(); ++i) {
                for (uint32_t col : columnIndexes[i]) {
                    ret = std::max(ret, col > i ? col - i : i - col);
                }
            }
            return ret;
        }

        void fill(gradylib::OpenHashMapTC<MatrixCoordinate, double> const & elements) {
            auto fillElement = [this](uint32_t i, uint32_t j, double value) {
                std::vector<uint32_t> const & cIdxs = columnIndexes[i];
                auto iter = lower_bound(cIdxs.begin(), cIdxs.end(), j);
                if (iter == cIdxs.end() || *iter != j) {
                    std::stringstream sstr;
                    sstr << "column not found during fill for " << i << " " << j << "error";
                    throw gradylibMakeException(sstr.str());
                }
                auto idx = distance(cIdxs.begin(), iter);
                rows[i][idx] = value;
            };
            for (auto const & [coord, value] : elements) {
                fillElement(coord.getI(), coord.getJ(), value);
            }
        }

        FixedSparseMatrix transpose() const {
            return transposeT<FreeSparseMatrix>();
        }

        std::vector<double> operator*(std::vector<double> const & v) const {
            std::vector<double> ret(numRows());
            for (uint32_t i = 0; i < columnIndexes.size(); ++i) {
                for (auto [j, value] : std::views::zip(columnIndexes[i], rows[i])) {
                    ret[i] += v[j] * value;
                }
            }
            return ret;
        }

        static FixedSparseMatrix fromFile(std::ifstream & ifs) {
            using namespace file_utilities;
            auto readle = [&ifs]<typename T>(T & x) {
                static_assert(std::is_trivially_copyable_v<T>);
                ifs.read(reinterpret_cast<char *>(&x), sizeof(x));
                x = fromle(x);
            };
            auto readArray = [&ifs, &readle]<typename T>() {
                uint64_t numElements;
                readle(numElements);
                size_t numBytes = sizeof(T) * numElements;
                std::vector<T> ret(numElements);
                size_t offset = 0;
                constexpr auto maxArg = std::numeric_limits<std::streamsize>::max();
                while (numBytes > maxArg) {
                    ifs.read(reinterpret_cast<char *>(ret.data() + offset), maxArg);
                    offset += maxArg;
                    numBytes -= maxArg;
                }
                ifs.read(reinterpret_cast<char *>(ret.data() + offset), numBytes);
                return ret;
            };

            size_t numRows;
            uint32_t numColumns;
            readle(numRows);
            readle(numColumns);
            std::vector<std::vector<double>> rows;
            rows.resize(numRows);
            for (size_t i = 0; i < numRows; ++i) {
                rows[i] = readArray.operator()<double>();
            }
            std::vector<std::vector<uint32_t>> columnIndexes;
            columnIndexes.resize(numRows);
            for (size_t i = 0; i < numRows; ++i) {
                columnIndexes[i] = readArray.operator()<uint32_t>();
            }
            return FixedSparseMatrix{std::move(rows), std::move(columnIndexes), numColumns};
        }

        static FixedSparseMatrix fromFile(std::filesystem::path filename) {
            if (!std::filesystem::exists(filename)) {
                return FixedSparseMatrix{};
            }
            std::ifstream ifs(filename, std::ios::binary);
            return fromFile(ifs);
        }

        void toFile(std::ofstream & ofs) const {
            using namespace file_utilities;
            auto writele = [&ofs]<typename T>(const T& x) {
                static_assert(std::is_trivially_copyable_v<T>);
                auto t = tole(x);
                ofs.write(reinterpret_cast<char const *>(&t), sizeof(x));
            };
            auto writeArray = [&ofs, &writele](const auto& p, uint64_t numElements) {
                using Tptr = std::remove_cvref_t<decltype(p)>;
                static_assert(std::is_pointer_v<Tptr>);
                using T = std::remove_pointer_t<Tptr>;
                size_t numBytes = sizeof(T) * numElements;
                writele(numElements);
                constexpr auto maxArg = std::numeric_limits<std::streamsize>::max();
                size_t offset = 0;
                while (numBytes > maxArg) {
                    ofs.write(reinterpret_cast<char const *>(p+offset), maxArg);
                    offset += maxArg;
                    numBytes -= maxArg;
                }
                ofs.write(reinterpret_cast<char const *>(p+offset), numBytes);
            };

            writele(rows.size());
            writele(_numColumns);
            for (auto && r : rows) {
                writeArray(r.data(), r.size());
            }
            for (auto && ci : columnIndexes) {
                writeArray(ci.data(), ci.size());
            }
        }

        void toFile(std::filesystem::path filename) const {
            std::ofstream ofs(filename, std::ios::binary);
            toFile(ofs);
        }

        void printToFile(std::ofstream & ofs) const {
            for (uint32_t i = 0; i < rows.size(); ++i) {
                for (auto && [j, el] : std::views::zip(columnIndexes[i], rows[i])) {
                    ofs << i << " " << j << " " << el << "\n";
                }
            }
        }

        void printToFile(std::filesystem::path path) const {
            std::ofstream ofs(path, std::ios::binary);
            printToFile(ofs);
        }

        class iterator {
            uint32_t i, column;
            FixedSparseMatrix const * m;
            bool finished;

        public:
            iterator(uint32_t i, FixedSparseMatrix const * m)
                : i(i), column(0), m(m)
            {
                finished = m == nullptr;
            }

            iterator(FixedSparseMatrix const * m)
                : i(std::numeric_limits<uint32_t>::max()), column(std::numeric_limits<uint32_t>::max()), m(m), finished(true)
            {
            }

            void operator++() {
                if (!finished) {
                    ++column;
                    if (column == m->columnIndexes[i].size()) {
                        ++i;
                        column = 0;
                        if (i == m->rows.size()) {
                            finished = true;
                            i = std::numeric_limits<uint32_t>::max();
                            column = std::numeric_limits<uint32_t>::max();
                            return;
                        }
                    }
                }
            }

            std::tuple<uint32_t, uint32_t, double> operator*() const {
                return std::make_tuple(i, m->columnIndexes[i][column], m->rows[i][column]);
            }

            bool operator!=(iterator const & iter) const {
                return !this->operator==(iter);
            }

            bool operator==(iterator const & iter) const {
                if (iter.m != m) {
                    return false;
                }
                if (finished && iter.finished) {
                    return true;
                }
                return i == iter.i && column == iter.column;
            }
        };

        iterator begin() const {
            for (uint32_t i = 0; i < rows.size(); ++i) {
                if (!rows[i].empty()) {
                    return iterator{i, this};
                }
            }
            return iterator{nullptr};
        }

        iterator end() const {
            return iterator{this};
        }
    };


    class FreeSparseMatrix {
        gradylib::OpenHashMapTC<MatrixCoordinate, double> elements;

    public:

        double & operator()(uint32_t i, uint32_t j) {
            return elements[MatrixCoordinate{i, j}];
        }

        double operator()(uint32_t i, uint32_t j) const {
            auto z = elements.get(MatrixCoordinate{i, j});
            if (z.has_value()) {
                return z.value();
            }
            return 0;
        }

        void setBoth(uint32_t i, uint32_t j, double x) {
            this->operator()(i,j) = x;
            this->operator()(j,i) = x;
        }

        void incBoth(uint32_t i, uint32_t j, double x) {
            this->operator()(i,j) += x;
            if (i != j) {
                this->operator()(j,i) += x;
            }
        }

        void inc(uint32_t i, uint32_t j, double x) {
            this->operator()(i,j) += x;
        }

        uint32_t numElements() const {
            return elements.size();
        }

        std::vector<std::list<uint32_t>> getNonzeros() const {
            uint32_t maxValue = 0;
            for (auto && [coord, val] : elements) {
                maxValue = std::max(maxValue, coord.getI());
            }
            std::vector<std::list<uint32_t>> nonzeros(maxValue+1);
            for (auto && [coord, val] : elements) {
                    nonzeros[coord.getI()].push_back(coord.getJ());
            }
            for (auto && l : nonzeros) {
                l.sort();
            }
            return nonzeros;
        }


        void insert(int ii, int jj, FixedSparseMatrix const & A) {
            for (auto [i, j, element] : A) {
                this->operator()(ii+i, jj+j) = element;
            }
        }

        void insertTranspose(int ii, int jj, FixedSparseMatrix const & A) {
            for (auto [i, j, element] : A) {
                this->operator()(ii+j, jj+i) = element;
            }
        }

        FixedSparseMatrix makeFixed() const {
            std::vector<std::list<uint32_t>> nonzeros = getNonzeros();
            FixedSparseMatrix fsm(nonzeros);
            fsm.fill(elements);
            return fsm;
        }

        class const_iterator {
            decltype(elements)::const_iterator iter;

        public:
            const_iterator(decltype(elements)::const_iterator iter)
                : iter(iter)
            {
            }

            void operator++() {
                ++iter;
            }

            std::tuple<uint32_t, uint32_t, double> operator*() const {
                auto el = *iter;
                return std::make_tuple(el.first.getI(), el.first.getJ(), el.second);
            }

            bool operator!=(const_iterator const & other) const {
                return !this->operator==(other);
            }

            bool operator==(const_iterator const & other) const {
                return this->iter == other.iter;
            }
        };

        const_iterator begin() const {
            return const_iterator{elements.begin()};
        }

        const_iterator end() const {
            return const_iterator{elements.end()};
        }
    };
}

#endif

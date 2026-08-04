#ifndef CONTROL_SPARSEMATRIX_HPP
#define CONTROL_SPARSEMATRIX_HPP

#include<cstdint>
#include<list>
#include<sstream>
#include<utility>
#include<vector>
#include<ranges>

#include"OpenHashMapTC.hpp"

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

namespace std {
    template<>
    struct hash<MatrixCoordinate> {
        size_t operator()(MatrixCoordinate const & mc) const noexcept {
            return mc.getI() * 7282340493LL + mc.getJ();
        }
    };
}

class FreeSparseMatrix;

class FixedSparseMatrix {
    std::vector<std::vector<double>> rows;
    std::vector<std::vector<uint32_t>> columnIndexes;
    uint32_t _numColumns;

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
    FixedSparseMatrix() {
    }

    FixedSparseMatrix(FixedSparseMatrix const & m)
        : rows(m.rows), columnIndexes(m.columnIndexes), _numColumns(m._numColumns)
    {
    }

    FixedSparseMatrix(FixedSparseMatrix && m)
        : rows(move(m.rows)), columnIndexes(move(m.columnIndexes)), _numColumns(m._numColumns)
    {
    }

    FixedSparseMatrix & operator=(FixedSparseMatrix && m) {
        rows = move(m.rows);
        columnIndexes = move(m.columnIndexes);
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


    FixedSparseMatrix makeFixed() const {
        std::vector<std::list<uint32_t>> nonzeros = getNonzeros();
        FixedSparseMatrix fsm(nonzeros);
        fsm.fill(elements);
        return fsm;
    }
};

#endif //CONTROL_SPARSEMATRIX_HPP

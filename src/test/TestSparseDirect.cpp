//
// Created by Grady Schofield on 8/20/26.
//

#include<catch2/catch_test_macros.hpp>
#include<catch2/matchers/catch_matchers_floating_point.hpp>

#include<gradylib/SparseDirect.hpp>
#include<gradylib/SparseMatrix.hpp>

using namespace std;
using namespace Catch::Matchers;
using namespace gradylib;
using namespace gradylib::sparse_direct_utilities;

TEST_CASE("EliminationTree 1") {
    FreeSparseMatrix fsm;
    fsm(0, 0) = 1.0;
    fsm(1, 1) = 1.0;
    fsm(2, 2) = 1.0;
    FixedSparseMatrix m = fsm.makeFixed();
    EliminationTree etree(m);
    REQUIRE(etree.root(0) == 0);
    REQUIRE(etree.root(1) == 1);
    REQUIRE(etree.root(2) == 2);
}

TEST_CASE("EliminationTree 2") {
    FreeSparseMatrix fsm;
    fsm(0, 0) = 1.0;
    fsm(1, 1) = 1.0;
    fsm(2, 2) = 1.0;
    FixedSparseMatrix m = fsm.makeFixed();
    EliminationTree etree(m);
    REQUIRE(etree.reach(vector{0U}) == vector{0U});
    REQUIRE(etree.reach(vector{1U}) == vector{1U});
    REQUIRE(etree.reach(vector{2U}) == vector{2U});
}

TEST_CASE("EliminationTree 3") {
    FreeSparseMatrix fsm;
    for (int i = 1; i <= 11; ++i) {
        fsm.setBoth(i, i, 1.0);
    }
    // The example is from Timothy A. Davis's book Direct Methods for Sparse Linear System Section, 4.1 page 39
    fsm.setBoth(1, 6, 1.0);
    fsm.setBoth(1, 7, 1.0);
    fsm.setBoth(2, 3, 1.0);
    fsm.setBoth(2, 8, 1.0);
    fsm.setBoth(3, 10, 1.0);
    fsm.setBoth(3, 11, 1.0);
    fsm.setBoth(4, 6, 1.0);
    fsm.setBoth(4, 10, 1.0);
    fsm.setBoth(5, 8, 1.0);
    fsm.setBoth(5, 11, 1.0);
    fsm.setBoth(6, 9, 1.0);
    fsm.setBoth(6, 10, 1.0);
    fsm.setBoth(7, 11, 1.0);
    fsm.setBoth(8, 10, 1.0);
    fsm.setBoth(8, 11, 1.0);
    fsm.setBoth(10, 11, 1.0);
    FreeSparseMatrix fsm2;
    for (auto [i, j, el] : fsm) {
        fsm2.setBoth(i-1, j-1, el);
    }

    FixedSparseMatrix m = fsm2.makeFixed();
    EliminationTree etree(m);
    REQUIRE(etree.parent(0) == 5);
    REQUIRE(etree.parent(1) == 2);
    REQUIRE(etree.parent(2) == 7);
    REQUIRE(etree.parent(3) == 5);
    REQUIRE(etree.parent(4) == 7);
    REQUIRE(etree.parent(5) == 6);
    REQUIRE(etree.parent(6) == 8);
    REQUIRE(etree.parent(7) == 9);
    REQUIRE(etree.parent(8) == 9);
    REQUIRE(etree.parent(9) == 10);
    REQUIRE(etree.parent(10) == 10);

    REQUIRE(etree.root(0) == 10);

    REQUIRE(etree.reach(vector{0u}) == vector{0u, 5u, 6u, 8u, 9u, 10u});
    REQUIRE(etree.reach(vector{1u}) == vector{1u, 2u, 7u, 9u, 10u});
    REQUIRE(etree.reach(vector{2u}) == vector{2u, 7u, 9u, 10u});
    REQUIRE(etree.reach(vector{3u}) == vector{3u, 5u, 6u, 8u, 9u, 10u});
    REQUIRE(etree.reach(vector{4u}) == vector{4u, 7u, 9u, 10u});
    REQUIRE(etree.reach(vector{5u}) == vector{5u, 6u, 8u, 9u, 10u});
    REQUIRE(etree.reach(vector{6u}) == vector{6u, 8u, 9u, 10u});
    REQUIRE(etree.reach(vector{7u}) == vector{7u, 9u, 10u});
    REQUIRE(etree.reach(vector{8u}) == vector{8u, 9u, 10u});
    REQUIRE(etree.reach(vector{9u}) == vector{9u, 10u});
    REQUIRE(etree.reach(vector{10u}) == vector{10u});
}

TEST_CASE("SparseLowerTriangular Solve 1") {
    SparseLowerTriangularMatrix L(10);
    for (int i = 0; i < 10; ++i) {
        L.fillDiagonal(i, i+1);
    }
    vector y(10, 1.0);
    auto x = L.solve(y);
    REQUIRE_THAT(x[0], WithinRel(1.0, 1E-15));
    REQUIRE_THAT(x[1], WithinRel(1.0/2, 1E-15));
    REQUIRE_THAT(x[2], WithinRel(1.0/3, 1E-15));
    REQUIRE_THAT(x[3], WithinRel(1.0/4, 1E-15));
    REQUIRE_THAT(x[4], WithinRel(1.0/5, 1E-15));
    REQUIRE_THAT(x[5], WithinRel(1.0/6, 1E-15));
    REQUIRE_THAT(x[6], WithinRel(1.0/7, 1E-15));
    REQUIRE_THAT(x[7], WithinRel(1.0/8, 1E-15));
    REQUIRE_THAT(x[8], WithinRel(1.0/9, 1E-15));
    REQUIRE_THAT(x[9], WithinRel(1.0/10, 1E-15));
}

TEST_CASE("SparseLowerTriangular Solve 2") {
    SparseLowerTriangularMatrix L(3);
    L.fillDiagonal(0, 1);
    L.fillDiagonal(1, 3);
    L.fillDiagonal(2, 6);
    L.addRow(1, {0}, {2.0});
    L.addRow(2, {0, 1}, {4.0, 5.0});
    vector y{1.0, 2.0, 3.0};
    auto x = L.solve(y);
    REQUIRE_THAT(x[0], WithinRel(1.0, 1E-15));
    REQUIRE_THAT(x[1], WithinRel(0, 1E-15));
    REQUIRE_THAT(x[2], WithinRel(-1.0/6, 1E-15));
}

TEST_CASE("SparseLowerTriangular Solve 3") {
    SparseLowerTriangularMatrix L(4);
    L.fillDiagonal(0, 1);
    L.fillDiagonal(1, 2);
    L.fillDiagonal(2, 4);
    L.fillDiagonal(3, 6);
    L.addRow(2, {0}, {3.0});
    L.addRow(3, {1}, {5.0});
    vector y{1.0, 2.0, 3.0, 4.0};
    auto x = L.solve(y);
    REQUIRE_THAT(x[0], WithinRel(1.0, 1E-15));
    REQUIRE_THAT(x[1], WithinRel(1, 1E-15));
    REQUIRE_THAT(x[2], WithinRel(0, 1E-15));
    REQUIRE_THAT(x[3], WithinRel(-1.0/6, 1E-15));
}

TEST_CASE("SparseUpperTriangular Solve 1") {
    SparseLowerTriangularMatrix L(4);
    L.fillDiagonal(0, 1);
    L.fillDiagonal(1, 2);
    L.fillDiagonal(2, 4);
    L.fillDiagonal(3, 6);
    L.addRow(2, {0}, {3.0});
    L.addRow(3, {1}, {5.0});
    auto Lt = L.transpose();
    vector y{1.0, 2.0, 3.0, 4.0};
    auto x = Lt.solve(y);
    REQUIRE_THAT(x[0], WithinRel(-5.0/4, 1E-15));
    REQUIRE_THAT(x[1], WithinRel(-2.0/3, 1E-15));
    REQUIRE_THAT(x[2], WithinRel(3.0/4, 1E-15));
    REQUIRE_THAT(x[3], WithinRel(2.0/3, 1E-15));
}

TEST_CASE("SparseLowerTriangular Solve 4") {
    SparseLowerTriangularMatrix L(4);
    L.fillDiagonal(0, 1);
    L.fillDiagonal(1, 2);
    vector y{3.0};
    auto x = L.solve(vector {1u}, y);
    REQUIRE(x.size() == 1);
    REQUIRE_THAT(x[0], WithinRel(3.0/2, 1E-15));
}

TEST_CASE("Cholesky 1") {
    FreeSparseMatrix fsm;
    for (int i = 0; i < 3; ++i) {
        fsm.setBoth(i, i, 10.0);
    }
    fsm.setBoth(0, 2, 1.0);

    FixedSparseMatrix m = fsm.makeFixed();
    CholeskyDecomposition cholesky(m);

    vector y{1.0, 2.0, 3.0};
    auto x = cholesky.solve(y);
    REQUIRE_THAT(x[0], WithinRel(7.0/99, 1E-15));
    REQUIRE_THAT(x[1], WithinRel(1.0/5, 1E-15));
    REQUIRE_THAT(x[2], WithinRel(29.0/99, 1E-15));
}


TEST_CASE("Cholesky 2") {
    FreeSparseMatrix fsm;
    for (int i = 1; i <= 11; ++i) {
        fsm.setBoth(i, i, 10.0);
    }
    // The nonzero pattern is from Timothy A. Davis's book Direct Methods for Sparse Linear System Section, 4.1 page 39
    fsm.setBoth(1, 6, 1.0);
    fsm.setBoth(1, 7, 1.0);
    fsm.setBoth(2, 3, 1.0);
    fsm.setBoth(2, 8, 1.0);
    fsm.setBoth(3, 10, 1.0);
    fsm.setBoth(3, 11, 1.0);
    fsm.setBoth(4, 6, 1.0);
    fsm.setBoth(4, 10, 1.0);
    fsm.setBoth(5, 8, 1.0);
    fsm.setBoth(5, 11, 1.0);
    fsm.setBoth(6, 9, 1.0);
    fsm.setBoth(6, 10, 1.0);
    fsm.setBoth(7, 11, 1.0);
    fsm.setBoth(8, 10, 1.0);
    fsm.setBoth(8, 11, 1.0);
    fsm.setBoth(10, 11, 1.0);
    FreeSparseMatrix fsm2;
    for (auto [i, j, el] : fsm) {
        fsm2.setBoth(i-1, j-1, el);
    }

    FixedSparseMatrix m = fsm2.makeFixed();
    CholeskyDecomposition cholesky(m);

    vector<double> y;
    for (int i = 1; i <= 11; ++i) y.push_back(i);
    auto x = cholesky.solve(y);
    REQUIRE_THAT(x[0], WithinRel(-0.0023541818828705496, 1E-13));
    REQUIRE_THAT(x[1], WithinRel(0.12870025487700817, 1E-13));
    REQUIRE_THAT(x[2], WithinRel(0.12428403405878309, 1E-13));
    REQUIRE_THAT(x[3], WithinRel(0.2817036599482077, 1E-13));
    REQUIRE_THAT(x[4], WithinRel(0.35570616887648027, 1E-13));
    REQUIRE_THAT(x[5], WithinRel(0.40872889004682467, 1E-13));
    REQUIRE_THAT(x[6], WithinRel(0.6148129287818809, 1E-13));
    REQUIRE_THAT(x[7], WithinRel(0.588713417171135, 1E-13));
    REQUIRE_THAT(x[8], WithinRel(0.8591271109953176, 1E-13));
    REQUIRE_THAT(x[9], WithinRel(0.7742345104710987, 1E-13));
    REQUIRE_THAT(x[10], WithinRel(0.8542248940640622, 1E-13));
}


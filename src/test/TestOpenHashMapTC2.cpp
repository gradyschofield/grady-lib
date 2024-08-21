//
// Created by Grady Schofield on 8/20/24.
//

#include<catch2/catch_test_macros.hpp>

#include<gradylib/OpenHashMap.hpp>
#include<gradylib/OpenHashMapTC.hpp>

#include<unordered_map>

using namespace gradylib;
using namespace std;

TEST_CASE() {

    long size = 500E3;

    OpenHashMap<int64_t, int64_t> m;
    OpenHashMapTC<int64_t, int64_t> mtc;
    for (long i = 0; i < size; ++i) {
        int idx = rand() % 100000;
        ++m[idx];
        ++mtc[idx];
    }

    REQUIRE(m.size() == mtc.size());
    for (auto && [idx, count] : m) {
        REQUIRE(mtc.contains(idx));
        REQUIRE(mtc.at(idx) == count);
    }
}

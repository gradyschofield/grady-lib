//
// Created by Grady Schofield on 7/21/24.
//

#include<catch2/catch_test_macros.hpp>

#include<iostream>
#include<unordered_map>

#include"gradylib/OpenHashMapTC.hpp"

using namespace std;
using namespace gradylib;

TEST_CASE("Open hash map on trivially copyable types"){

    OpenHashMapTC<int64_t, int32_t> m;
    unordered_map<int64_t, int32_t> test;
    long size = 1E6;
    for (long i = 0; i < size; ++i) {
        int x = rand();
        int y = rand();
        m[x] = y;
        test[x] = y;
    }

    auto startTime = chrono::high_resolution_clock::now();
    REQUIRE(m.size() == test.size());

    for (auto & p : test) {
        REQUIRE(m.contains(p.first));
        REQUIRE(p.second == m[p.first]);
    }
    auto endTime = chrono::high_resolution_clock::now();
    cout << "testing took " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << " ms\n";

    m.write("openhashmaptc.bin");

    OpenHashMapTC<int64_t, int32_t> m2("openhashmaptc.bin");

    REQUIRE(m2.size() == test.size());

    startTime = chrono::high_resolution_clock::now();
    for (auto & p : test) {
        REQUIRE(m2.contains(p.first));
        REQUIRE(p.second == m2.at(p.first));
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "testing took " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << " ms\n";

}
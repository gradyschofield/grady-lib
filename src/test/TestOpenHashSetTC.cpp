#include<catch2/catch_test_macros.hpp>

#include<bit>
#include<chrono>
#include<iostream>
#include<unordered_set>

#include"gradylib/OpenHashSetTC.hpp"

using namespace std;
using namespace gradylib;

TEST_CASE("Open hash set on trivially copyable types"){
    auto myRand = []() {
        return rand() % 100000000;
    };

    using HashSet = OpenHashSetTC<int>;
    HashSet myset;

    long size  = 1E6;
    auto startTime = chrono::high_resolution_clock::now();
    size_t accum = 0;
    for (long i = 0; i < size; ++i) {
        accum += myRand();
    }
    auto endTime = chrono::high_resolution_clock::now();
    long randTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
    cout << "myRand: " << randTime <<  " " << accum << endl;

    myset.reserve(size);
    startTime = chrono::high_resolution_clock::now();
    for (long i = 0; i < size; ++i) {
        myset.insert(myRand());
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "OpenHashSetOfTriviallyCopyables: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() - randTime << endl;

    unordered_set<int> s;
    s.reserve(size);
    startTime = chrono::high_resolution_clock::now();
    for (long i = 0; i < size; ++i) {
        s.insert(myRand());
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "unordered_set: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() - randTime << endl;

    unordered_set<int> test;
    for (int i : myset) {
        test.insert(i);
    }

    REQUIRE(myset.size() == test.size());
    cout << myset.size() << " " << test.size() << "\n";

    startTime = chrono::high_resolution_clock::now();
    unique_ptr<HashSet> myset2 = make_unique<HashSet>(myset);
    endTime = chrono::high_resolution_clock::now();
    cout << chrono::duration_cast<chrono::microseconds>(endTime - startTime).count() << "\n";

    startTime = chrono::high_resolution_clock::now();
    unique_ptr<unordered_set<int>> test2 = make_unique<unordered_set<int>>(test);
    endTime = chrono::high_resolution_clock::now();
    cout << chrono::duration_cast<chrono::microseconds>(endTime - startTime).count() << "\n";


    startTime = chrono::high_resolution_clock::now();
    myset2.reset(nullptr);
    endTime = chrono::high_resolution_clock::now();
    cout << chrono::duration_cast<chrono::microseconds>(endTime - startTime).count() << "\n";


    startTime = chrono::high_resolution_clock::now();
    test2.reset(nullptr);
    endTime = chrono::high_resolution_clock::now();
    cout << chrono::duration_cast<chrono::microseconds>(endTime - startTime).count() << "\n";

    size_t numMods = 1E6;
    for (size_t i = 0; i < numMods; ++i) {
        int x = myRand();
        myset.insert(x);
        test.insert(x);
        int y = myRand();
        myset.erase(y);
        test.erase(y);
    }
    REQUIRE(myset.size() == test.size());

    startTime = chrono::high_resolution_clock::now();
    for (int i : test) {
        REQUIRE(myset.contains(i));
    }

    REQUIRE(test.size() == myset.size());
    endTime = chrono::high_resolution_clock::now();
    cout << "testing from memory time: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << "\n";

    cout << "writing to disk...\n";
    myset.write("a_grand_old_set.bin");

    startTime = chrono::high_resolution_clock::now();
    HashSet myset3("a_grand_old_set.bin");
    endTime = chrono::high_resolution_clock::now();
    cout << "mmap load: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << "\n";

    startTime = chrono::high_resolution_clock::now();
    for (int i : test) {
        REQUIRE(myset3.contains(i));
    }

    REQUIRE(test.size() == myset3.size());
    endTime = chrono::high_resolution_clock::now();
    cout << "testing from mmap load: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << "\n";

}

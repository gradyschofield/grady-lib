#include<catch2/catch_test_macros.hpp>

#include<bit>
#include<chrono>
#include<iostream>
#include<unordered_set>

#include"gradylib/OpenHashSetTC.hpp"

using namespace std;
using namespace gradylib;
namespace fs = std::filesystem;

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

TEST_CASE("OpenHashSetTC reserve too small size"){
    OpenHashSetTC<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(4);
    s.reserve(1);
    REQUIRE(s.size() == 4);
    REQUIRE(s.contains(1));
    REQUIRE(s.contains(2));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(4));
}

TEST_CASE("OpenHashSetTC constructor throws on bad filename"){
    REQUIRE_THROWS(OpenHashSetTC<int>("/gradylib nonexistent file"));
}

TEST_CASE("OpenHashSetTC constructor throws on mmap failure"){
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    OpenHashSetTC<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(4);
    s.write(tmpFile);
    GRADY_LIB_MOCK_OpenHashSetTC_MMAP<int>();
    REQUIRE_THROWS(OpenHashSetTC<int>(tmpFile));
    GRADY_LIB_DEFAULT_OpenHashSetTC_MMAP<int>();
    fs::remove(tmpFile);
}

TEST_CASE("OpenHashSetTC assignment"){
    OpenHashSetTC<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(4);
    OpenHashSetTC<int> s2;
    s2 = s;
    REQUIRE(s2.size() == 4);
    REQUIRE(s2.contains(1));
    REQUIRE(s2.contains(2));
    REQUIRE(s2.contains(3));
    REQUIRE(s2.contains(4));
}

TEST_CASE("OpenHashSetTC self assignment"){
    OpenHashSetTC<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(4);
    OpenHashSetTC<int> s2;
    s = s;
    REQUIRE(s.size() == 4);
    REQUIRE(s.contains(1));
    REQUIRE(s.contains(2));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(4));
}

TEST_CASE("OpenHashSetTC assignment of readonly map fails"){
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    OpenHashSetTC<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(4);
    s.write(tmpFile);
    OpenHashSetTC<int> s2(tmpFile);
    OpenHashSetTC<int> s3;
    REQUIRE_THROWS(s3 = s2);
    fs::remove(tmpFile);
}

TEST_CASE("OpenHashSetTC move assignment"){
    OpenHashSetTC<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(4);
    OpenHashSetTC<int> s2;
    s2 = move(s);
    REQUIRE(s2.size() == 4);
    REQUIRE(s2.contains(1));
    REQUIRE(s2.contains(2));
    REQUIRE(s2.contains(3));
    REQUIRE(s2.contains(4));
    REQUIRE(s.size() == 0);
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(4);
    REQUIRE(s.size() == 4);
    REQUIRE(s.contains(1));
    REQUIRE(s.contains(2));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(4));
}


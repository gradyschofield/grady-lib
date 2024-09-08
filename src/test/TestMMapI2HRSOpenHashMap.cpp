//
// Created by Grady Schofield on 7/25/24.
//

#include<catch2/catch_test_macros.hpp>

#include<string>
#include<unordered_map>

#include"gradylib/AltIntHash.hpp"
#include"gradylib/MMapI2HRSOpenHashMap.hpp"
#include"gradylib/OpenHashMap.hpp"

using namespace std;
using namespace gradylib;
namespace fs = std::filesystem;

int64_t myRand() {
    static int64_t x = 578363509162734;
    x = x * 175055391405843 + 588734109354867;
    return abs(x);
}

TEST_CASE("Integer to highly redundant string open hash map") {
    auto randString = []() {
        int len = rand() % 12 + 3;
        vector<char> buffer(len);
        for (int i = 0; i < len; ++i) {
            buffer[i] = rand() % (1 + 126-32) + 32;
        }
        return string(buffer.data(), len);
    };

    int numStrings = 1E5;
    int numInts = 1E6;
    MMapI2HRSOpenHashMap<int64_t>::Builder b;
    unordered_map<int64_t, string> test;
    OpenHashMap<string, int> strs;
    vector<string> strArray;
    strs.reserve(numStrings);
    strArray.reserve(numStrings);
    for (int i = 0; i < numStrings; ++i ){
        string s = randString();
        while (strs.contains(s)) {
            s = randString();
        }
        strs.put(s, 1);
        strArray.push_back(std::move(s));
    }

    b.reserve(numInts);
    auto startTime = chrono::high_resolution_clock::now();
    for (long i = 0; i < numInts; ++i) {
        long idx = myRand();
        while (b.contains(idx)) {
            idx = rand();
        }
        string const & str = strArray[rand() % strArray.size()];
        b.put(idx, str);
        test.emplace(idx, str);
    }
    auto endTime = chrono::high_resolution_clock::now();
    cout << "build time: " << chrono::duration_cast<chrono::milliseconds>(endTime-startTime).count() << "\n";


    startTime = chrono::high_resolution_clock::now();
    REQUIRE(b.size() == test.size());

    for (auto & [key, str] : test) {
        REQUIRE(b.contains(key));
        REQUIRE(string(b.at(key)) == str);
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "test time: " << chrono::duration_cast<chrono::milliseconds>(endTime-startTime).count() << "\n";


    startTime = chrono::high_resolution_clock::now();
    b.write("hrsmap.bin");
    endTime = chrono::high_resolution_clock::now();
    cout << "write time: " << chrono::duration_cast<chrono::milliseconds>(endTime-startTime).count() << "\n";

    startTime = chrono::high_resolution_clock::now();
    MMapI2HRSOpenHashMap<int64_t> m("hrsmap.bin");
    endTime = chrono::high_resolution_clock::now();
    cout << "load time: " << chrono::duration_cast<chrono::milliseconds>(endTime-startTime).count() << "\n";

    startTime = chrono::high_resolution_clock::now();
    REQUIRE(m.size() == test.size());
    for (auto & [key, str] : test) {
        REQUIRE(m.contains(key));
        REQUIRE(string(m.at(key)) == str);
    }
    for (auto const & [key, str] : m) {
        REQUIRE(test.contains(key));
        REQUIRE(test.at(key) == string(str));
    }
    auto iter = m.begin();
    while (iter != m.end()) {
        REQUIRE(test.contains(iter.key()));
        REQUIRE(test.at(iter.key()) == string(iter.value()));
        ++iter;
    }
    auto copyIter = iter;
    ++iter;
    REQUIRE(copyIter == iter);
    endTime = chrono::high_resolution_clock::now();
    cout << "test time: " << chrono::duration_cast<chrono::milliseconds>(endTime-startTime).count() << "\n";
}

TEST_CASE("MMapI2HRSOpenHashMap at throws on missing element") {
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    MMapI2HRSOpenHashMap<int>::Builder builder;
    builder.put(1, "abc");
    REQUIRE_THROWS(builder.at(0));
    builder.write(tmpFile);
    MMapI2HRSOpenHashMap<int> m(tmpFile);
    REQUIRE_THROWS(m.at(0));
    fs::remove(tmpFile);
}

TEST_CASE("MMapI2HRSOpenHashMap Builder write throws on bad filename") {
    fs::path tmpPath = "/__gradylib_nonexistent_dir";
    fs::path tmpFile = tmpPath / "map.bin";
    MMapI2HRSOpenHashMap<int>::Builder builder;
    builder.put(1, "abc");
    REQUIRE_THROWS( builder.write(tmpFile));
}

TEST_CASE("MMapI2HRSOpenHashMap constructor throws on nonexistent file") {
    fs::path tmpPath = "/__gradylib_nonexistent_dir";
    fs::path tmpFile = tmpPath / "map.bin";
    REQUIRE_THROWS(MMapI2HRSOpenHashMap<int>(tmpFile));
}
TEST_CASE("MMapI2HRSOpenHashMap throw on mmap failure") {
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    MMapI2HRSOpenHashMap<int>::Builder builder;
    builder.put(0, "abc");
    builder.write(tmpFile);
    gradylib::GRADY_LIB_MOCK_MMapI2HRSOpenHashMap_MMAP<int>();
    REQUIRE_THROWS(gradylib::MMapI2HRSOpenHashMap<int>(tmpFile));
    gradylib::GRADY_LIB_DEFAULT_MMapI2HRSOpenHashMap_MMAP<int>();
}

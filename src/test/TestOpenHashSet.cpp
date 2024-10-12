//
// Created by Grady Schofield on 8/17/24.
//

#include<catch2/catch_test_macros.hpp>

#include<iostream>

#include<gradylib/OpenHashSet.hpp>
#include"TestCommon.hpp"

using namespace std;
using namespace gradylib;
namespace fs = std::filesystem;

TEST_CASE("OpenHashSet insertion"){
    OpenHashSet<int> s;
    s.insert(4);
    s.insert(4);
    s.insert(3);
    s.insert(999);
    REQUIRE(!s.contains(1));
    REQUIRE(s.contains(4));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(999));
}

TEST_CASE("OpenHashSet reserve"){
    OpenHashSet<int> s;
    s.reserve(16);
    s.insert(4);
    s.insert(4);
    s.insert(3);
    s.insert(999);
    REQUIRE(s.size() == 3);
    REQUIRE(!s.contains(1));
    REQUIRE(s.contains(4));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(999));
}

TEST_CASE("OpenHashSet reserve to smaller size"){
    OpenHashSet<int> s;
    s.insert(4);
    s.insert(4);
    s.insert(3);
    s.insert(999);
    s.reserve(2);
    REQUIRE(s.size() == 3);
    REQUIRE(s.contains(4));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(999));
}

template<typename IntType>
struct TrashHash {
    size_t operator()(IntType const &i) const noexcept {
        return 0;
    }
};

TEST_CASE("OpenHashSet reserve with early element removed (for coverage)"){
    OpenHashSet<int, TrashHash> s;
    s.insert(4);
    s.insert(4);
    s.insert(3);
    s.insert(999);
    s.erase(4);
    s.reserve(16);
    REQUIRE(s.size() == 2);
    REQUIRE(!s.contains(1));
    REQUIRE(!s.contains(4));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(999));
}

TEST_CASE("OpenHashSet insert"){
    OpenHashSet<int, TrashHash> s;
    s.insert(4);
    s.erase(4);
    s.insert(3);
    REQUIRE(s.size() == 1);
    REQUIRE(s.contains(3));
}

TEST_CASE("OpenHashSet insert previously set key (for coverage)"){
    OpenHashSet<int, TrashHash> s;
    s.insert(4);
    s.erase(4);
    s.insert(4);
    REQUIRE(s.size() == 1);
    REQUIRE(s.contains(4));
}

TEST_CASE("OpenHashSet contains on empty map (for coverage)"){
    OpenHashSet<int> s;
    REQUIRE(s.size() == 0);
    REQUIRE(!s.contains(4));
}

TEST_CASE("OpenHashSet contains on removed element"){
    OpenHashSet<int> s;
    s.insert(1);
    s.erase(1);
    REQUIRE(!s.contains(1));
}

TEST_CASE("OpenHashSet erase on empty map"){
    OpenHashSet<int> s;
    s.erase(1);
    REQUIRE(s.size() == 0);
}

TEST_CASE("OpenHashSet erase on empty map (for coverage)"){
    OpenHashSet<int, TrashHash> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.erase(1);
    s.erase(2);
    s.erase(3);
    for (int i = 0; i < 100; ++i) {
        s.insert(i);
        s.erase(i);
    }
    REQUIRE(s.size() == 0);
}

TEST_CASE("OpenHashSet iterator"){
    OpenHashSet<int, TrashHash> s;
    s.insert(3);
    s.insert(4);
    s.insert(5);
    s.insert(999);
    s.erase(4);
    auto doStuff = [](auto && m) {
        auto iter = m.begin();
        int i = 0;
        while (iter != m.end()) {
            ++i;
            REQUIRE(m.contains(iter.key()));
            REQUIRE(m.contains(*iter));
            ++iter;
        }
        REQUIRE(i == m.size());
        auto iterCopy = iter;
        ++iter;
        REQUIRE(iterCopy == iter);
    };
    doStuff(s);
    doStuff(static_cast<gradylib::OpenHashSet<int, TrashHash> const &>(s));
}

TEST_CASE("OpenHashSet iterator on empty map"){
    OpenHashSet<int, TrashHash> s;
    auto doStuff = [](auto && m) {
        auto iter = m.begin();
        int i = 0;
        while (iter != m.end()) {
            ++i;
            ++iter;
        }
        REQUIRE(i == m.size());
        auto iterCopy = iter;
        ++iter;
        REQUIRE(iterCopy == iter);
    };
    doStuff(s);
    doStuff(static_cast<gradylib::OpenHashSet<int, TrashHash> const &>(s));
}

TEST_CASE("OpenHashSet iterator on empty map with removed elements"){
    OpenHashSet<int, TrashHash> s;
    s.insert(1);
    s.insert(2);
    s.erase(1);
    auto doStuff = [](auto && m) {
        auto iter = m.begin();
        int i = 0;
        while (iter != m.end()) {
            ++i;
            ++iter;
        }
        REQUIRE(i == m.size());
        auto iterCopy = iter;
        ++iter;
        REQUIRE(iterCopy == iter);
    };
    doStuff(s);
    doStuff(static_cast<gradylib::OpenHashSet<int, TrashHash> const &>(s));
}

TEST_CASE("OpenHashSet writeMappable"){
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    OpenHashSet<int> m;
    m.insert(0);
    m.insert(1);
    m.insert(5);
    //gradylib::writeMappable(tmpFile, m);
    //OpenHashSet<int> m2(tmpFile);
    filesystem::remove(tmpFile);
}

TEST_CASE("OpenHashSet write") {
    gradylib::OpenHashSet<StringIntFloat> s;
    s.insert(StringIntFloat{"ruf3", 3, 0.2345});
    s.insert(StringIntFloat{"**4nm_", -5, 0.2345});
    s.insert(StringIntFloat{"@@dn_", numeric_limits<int>::max()-1, -numeric_limits<float>::max()});
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "set.bin";
    s.write(tmpFile, serializeStringIntFloat);
    auto s2 = gradylib::OpenHashSet<StringIntFloat>::read(tmpFile, deserializeStringIntFloat);
    REQUIRE(s.size() == s2.size());
    for (auto && k : s) {
        REQUIRE(s2.contains(k));
    }
    filesystem::remove(tmpFile);
}

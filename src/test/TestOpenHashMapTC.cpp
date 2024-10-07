//
// Created by Grady Schofield on 7/21/24.
//

#include<catch2/catch_test_macros.hpp>

#include<iostream>
#include<unordered_map>

#include"gradylib/OpenHashMapTC.hpp"

using namespace std;
using namespace gradylib;

namespace fs = std::filesystem;

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

TEST_CASE("OpenHashMapTC rehash to smaller/larger size") {
    gradylib::OpenHashMapTC<int, int> m;
    m[0] = 0;
    m[1] = 1;
    m[2] = 2;
    m[3] = 3;
    m.reserve(2);
    REQUIRE(m.size() == 4);
    REQUIRE(m[0] == 0);
    REQUIRE(m[1] == 1);
    REQUIRE(m[2] == 2);
    REQUIRE(m[3] == 3);
    m.reserve(100);
    REQUIRE(m.size() == 4);
    REQUIRE(m[0] == 0);
    REQUIRE(m[1] == 1);
    REQUIRE(m[2] == 2);
    REQUIRE(m[3] == 3);
}

TEST_CASE("OpenHashMapTC copy constructor") {
    gradylib::OpenHashMapTC<int, double> m;
    m[0] = -3;
    m[1] = 2.313;
    m[2] = 6.92;
    m[3] = -1.34E-7;
    gradylib::OpenHashMapTC<int, double> m2(m);
    REQUIRE(m2.size() == m.size());
    for (auto [k, v] : m) {
        REQUIRE(m2.contains(k));
        REQUIRE(m[k] == m2[k]);
    }
}

TEST_CASE("OpenHashMapTC move constructor") {
    gradylib::OpenHashMapTC<int, double> m;
    m[0] = -3;
    m[1] = 2.313;
    m[2] = 6.92;
    m[3] = -1.34E-7;
    gradylib::OpenHashMapTC<int, double> m3(m);
    gradylib::OpenHashMapTC<int, double> m2(move(m3));
    REQUIRE(m2.size() == m.size());
    for (auto [k, v] : m) {
        REQUIRE(m2.contains(k));
        REQUIRE(m[k] == m2[k]);
    }
}

TEST_CASE("OpenHashMapTC assignment operator") {
    gradylib::OpenHashMapTC<int, double> m;
    m[0] = -3;
    m[1] = 2.313;
    m[2] = 6.92;
    m[3] = -1.34E-7;
    gradylib::OpenHashMapTC<int, double> m2;
    m2 = m;
    REQUIRE(m2.size() == m.size());
    for (auto [k, v] : m) {
        REQUIRE(m2.contains(k));
        REQUIRE(m[k] == m2[k]);
    }
}

TEST_CASE("OpenHashMapTC self assignment") {
    gradylib::OpenHashMapTC<int, double> m;
    m[0] = -3;
    m[1] = 2.313;
    m = m;
    REQUIRE(m.size() == 2);
    REQUIRE(m.at(0) == -3);
    REQUIRE(m.at(1) == 2.313);
}

TEST_CASE("OpenHashMapTC self move assignment") {
    gradylib::OpenHashMapTC<int, double> m;
    m[0] = -3;
    m[1] = 2.313;
    m = move(m);
    REQUIRE(m.size() == 2);
    REQUIRE(m.at(0) == -3);
    REQUIRE(m.at(1) == 2.313);
}

TEST_CASE("OpenHashMapTC move assignment operator") {
    gradylib::OpenHashMapTC<int, double> m;
    m[0] = -3;
    m[1] = 2.313;
    m[2] = 6.92;
    m[3] = -1.34E-7;
    gradylib::OpenHashMapTC<int, double> m3(m);
    gradylib::OpenHashMapTC<int, double> m2;
    m2 = move(m3);
    REQUIRE(m2.size() == m.size());
    for (auto [k, v] : m) {
        REQUIRE(m2.contains(k));
        REQUIRE(m[k] == m2[k]);
    }
}

TEST_CASE("OpenHashMapTC constructor from file") {
    gradylib::OpenHashMapTC<int, double> m;
    m[0] = -3;
    m[1] = 2.313;
    m[2] = 6.92;
    m[3] = -1.34E-7;
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    m.write(tmpFile);
    gradylib::OpenHashMapTC<int, double> m2(tmpFile);
    REQUIRE(m2.size() == m.size());
    for (auto [k, v] : m) {
        REQUIRE(m2.contains(k));
        REQUIRE(v == m2.at(k));
    }
    filesystem::remove(tmpFile);
}

TEST_CASE("OpenHashMapTC constructor from file throws on non-existent file") {
    REQUIRE_THROWS(gradylib::OpenHashMapTC<int, double>("/gradylib_nonexistent_file"));
}

TEST_CASE("OpenHashMapTC constructor from file throws on mmap file") {
    gradylib::OpenHashMapTC<int, double> m;
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    m.write(tmpFile);
    gradylib::GRADY_LIB_MOCK_OpenHashMapTC_MMAP<int, double>();
    REQUIRE_THROWS(gradylib::OpenHashMapTC<int, double>(tmpFile));
    gradylib::GRADY_LIB_DEFAULT_OpenHashMapTC_MMAP<int, double>();
    filesystem::remove(tmpFile);
}

TEST_CASE("OpenHashMapTC operator[] throws when object is readonly") {
    gradylib::OpenHashMapTC<int, double> m;
    m[0] = -3;
    m[1] = 2.313;
    m[2] = 6.92;
    m[3] = -1.34E-7;
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    m.write(tmpFile);
    gradylib::OpenHashMapTC<int, double> m2(tmpFile);
    REQUIRE_THROWS(m2[0]);
    filesystem::remove(tmpFile);
}

template<typename IntType>
struct TrashHash {
    size_t operator()(IntType const &i) const noexcept {
        return 0;
    }
};

TEST_CASE("OpenHashMapTC operator[] with elements removed") {
    gradylib::OpenHashMapTC<int, double, TrashHash> m;
    m[0] = -3;
    m[1] = 2.313;
    m[2] = 6.92;
    m[3] = -1.34E-7;
    m.erase(0);
    m.erase(1);
    m.erase(2);
    REQUIRE(m.size() == 1);
    REQUIRE(m[3] == -1.34E-7);
}

TEST_CASE("OpenHashMapTC operator[] on previously removed element") {
    gradylib::OpenHashMapTC<int, double, TrashHash> m;
    m[0] = -3;
    m[1] = 2.313;
    m[2] = 6.92;
    m[3] = -1.34E-7;
    m.erase(0);
    m[0] = -2;
    REQUIRE(m[0] == -2);
}

TEST_CASE("OpenHashMapTC put") {
    gradylib::OpenHashMapTC<int, double, TrashHash> m;
    m.put(0, -3);
    m.put(1, 2.313);
    m.put(2, 6.92);
    m.put(3, -1.34E-7);
    m.erase(0);
    m.put(0, -2);
    REQUIRE(m.size() == 4);
    REQUIRE(m[0] == -2);
    REQUIRE(m[1] == 2.313);
    REQUIRE(m[2] == 6.92);
    REQUIRE(m[3] == -1.34E-7);
}

TEST_CASE("OpenHashMapTC put existing element") {
    gradylib::OpenHashMapTC<int, double, TrashHash> m;
    m.put(0, -3);
    m.put(1, 2.313);
    m.put(2, 6.92);
    m.put(3, -1.34E-7);
    m.put(0, -2);
    REQUIRE(m.size() == 4);
    REQUIRE(m[0] == -2);
    REQUIRE(m[1] == 2.313);
    REQUIRE(m[2] == 6.92);
    REQUIRE(m[3] == -1.34E-7);
}

TEST_CASE("OpenHashMapTC put throws when object is readonly") {
    gradylib::OpenHashMapTC<int, double> m;
    m[0] = -3;
    m[1] = 2.313;
    m[2] = 6.92;
    m[3] = -1.34E-7;
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    m.write(tmpFile);
    gradylib::OpenHashMapTC<int, double> m2(tmpFile);
    REQUIRE_THROWS(m2.put(0, -2));
    filesystem::remove(tmpFile);
}

TEST_CASE("OpenHashMapTC at throws on empty map") {
    gradylib::OpenHashMapTC<int, double> m;
    REQUIRE_THROWS(m.at(0) == 1.0);
}

TEST_CASE("OpenHashMapTC at throws on removed element") {
    gradylib::OpenHashMapTC<int, double> m;
    m.put(0, 1.2);
    m.erase(0);
    REQUIRE_THROWS(m.at(0) == 1.0);
}

TEST_CASE("OpenHashMapTC at throws on nonexistent element") {
    gradylib::OpenHashMapTC<int, double> m;
    m.put(0, 1.2);
    REQUIRE_THROWS(m.at(1) == 1.0);
}

TEST_CASE("OpenHashMapTC get returns empty optional") {
    gradylib::OpenHashMapTC<int, int> m;
    auto l = m.get(1);
    REQUIRE(!l.has_value());
}

TEST_CASE("OpenHashMapTC get") {
    gradylib::OpenHashMapTC<int, int, TrashHash> m;
    m.put(0, 0);
    m.put(1, 1);
    m.put(2, 2);
    REQUIRE(m.size() == 3);
    REQUIRE(m.get(0).value() == 0);
    REQUIRE(m.get(1).value() == 1);
    REQUIRE(m.get(2).value() == 2);
    m.erase(2);
    REQUIRE(!m.get(2).has_value());
    REQUIRE(!m.get(3).has_value());
}

TEST_CASE("OpenHashMapTC const get") {
    gradylib::OpenHashMapTC<int, int, TrashHash> m;
    m.put(0, 0);
    m.put(1, 1);
    m.put(2, 2);
    gradylib::OpenHashMapTC<int, int, TrashHash> const * p = &m;
    REQUIRE(p->size() == 3);
    REQUIRE(p->get(0).value() == 0);
    REQUIRE(p->get(1).value() == 1);
    REQUIRE(p->get(2).value() == 2);
    REQUIRE(!p->get(3).has_value());
}

TEST_CASE("OpenHashMapTC contains on empty map") {
    gradylib::OpenHashMapTC<int, double> m;
    REQUIRE(!m.contains(1) );
}

TEST_CASE("OpenHashMapTC contains on removed element") {
    gradylib::OpenHashMapTC<int, double> m;
    m.put(1, 1.234);
    m.put(2, 2.234);
    m.erase(1);
    REQUIRE(!m.contains(1) );
}

TEST_CASE("OpenHashMapTC contains on nonexistent element") {
    gradylib::OpenHashMapTC<int, double> m;
    m.put(1, 1.234);
    REQUIRE(!m.contains(2) );
}

TEST_CASE("OpenHashMapTC contains on present element") {
    gradylib::OpenHashMapTC<int, double, TrashHash> m;
    m.put(0, -3);
    m.put(1, 2.313);
    m.put(2, 6.92);
    m.put(3, -1.34E-7);
    m.put(0, -2);
    REQUIRE(m.contains(0));
    REQUIRE(m.contains(1));
    REQUIRE(m.contains(2));
    REQUIRE(m.contains(3));
}

TEST_CASE("OpenHashMapTC erase throws when object is readonly") {
    gradylib::OpenHashMapTC<int, double> m;
    m[0] = -3;
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    m.write(tmpFile);
    gradylib::OpenHashMapTC<int, double> m2(tmpFile);
    REQUIRE_THROWS(m2.erase(0));
    filesystem::remove(tmpFile);
}

TEST_CASE("OpenHashMapTC erase does nothing on nonexistent element") {
    gradylib::OpenHashMapTC<int, double, TrashHash> m;
    m.put(0, -3);
    m.put(1, 2.313);
    m.put(2, 6.92);
    m.put(3, -1.34E-7);
    m.erase(-1);
    REQUIRE(m.size() == 4);
}

TEST_CASE("OpenHashMapTC reserve throws when object is readonly") {
    gradylib::OpenHashMapTC<int, double> m;
    m[0] = -3;
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    m.write(tmpFile);
    gradylib::OpenHashMapTC<int, double> m2(tmpFile);
    REQUIRE_THROWS(m2.reserve(100));
    filesystem::remove(tmpFile);
}

TEST_CASE("OpenHashMapTC iterator") {
    gradylib::OpenHashMapTC<int, double, TrashHash> m;
    m[0] = 1.234;
    m[1] = -4.32E-9;
    m[2] = 4.84E23;
    m[3] = 1.32E3;
    m[4] = 8.99E-4;
    m.erase(0);
    m.erase(2);
    auto doStuff = [](auto && m) {
        auto iter = m.begin();
        int i = 0;
        while (iter != m.end()) {
            ++i;
            REQUIRE(iter.value() == m.at(iter.key()));
            ++iter;
        }
        REQUIRE(i == m.size());
        auto iterCopy = iter;
        ++iter;
        REQUIRE(iterCopy == iter);
    };
    doStuff(m);
    doStuff(static_cast<gradylib::OpenHashMapTC<int, double, TrashHash> const &>(m));
}

TEST_CASE("OpenHashMapTC iterator empty map") {
    gradylib::OpenHashMapTC<int, double, TrashHash> m;
    auto doStuff = [](auto && m) {
        auto iter = m.begin();
        int i = 0;
        while (iter != m.end()) {
            ++i;
            REQUIRE(iter.value() == m.at(iter.key()));
            ++iter;
        }
        REQUIRE(i == m.size());
        auto iterCopy = iter;
        ++iter;
        REQUIRE(iterCopy == iter);
    };
    doStuff(m);
    doStuff(static_cast<gradylib::OpenHashMapTC<int, double, TrashHash> const &>(m));
}

TEST_CASE("OpenHashMapTC range for loop") {
    gradylib::OpenHashMapTC<int, double, TrashHash> m;
    m[0] = 1.234;
    m[1] = -4.32E-9;
    m[2] = 4.84E23;
    m[3] = 1.32E3;
    m[4] = 8.99E-4;
    m.erase(0);
    m.erase(2);
    auto doStuff = [](auto && m) {
        for (auto && [k, v] : m) {
            REQUIRE(v == m.at(k));
        }
    };
    doStuff(m);
    doStuff(static_cast<gradylib::OpenHashMapTC<int, double, TrashHash> const &>(m));
}

TEST_CASE("OpenHashMapTC clear") {
    gradylib::OpenHashMapTC<int, double, TrashHash> m;
    m[0] = 1.234;
    m[1] = -4.32E-9;
    m[2] = 4.84E23;
    m[3] = 1.32E3;
    m[4] = 8.99E-4;
    m.clear();
    REQUIRE(!m.contains(0));
    REQUIRE(!m.contains(1));
    REQUIRE(!m.contains(2));
    REQUIRE(!m.contains(3));
    REQUIRE(!m.contains(4));
    REQUIRE(m.size() == 0);
}

TEST_CASE("OpenHashMapTC clear throws on empty map") {
    gradylib::OpenHashMapTC<int, double, TrashHash> m;
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    m.write(tmpFile);
    gradylib::OpenHashMapTC<int, double> m2(tmpFile);
    REQUIRE_THROWS(m2.clear());
    filesystem::remove(tmpFile);
}

TEST_CASE("OpenHashMapTC write throws on file open failure") {
    gradylib::OpenHashMapTC<int, double, TrashHash> m;
    REQUIRE_THROWS(m.write("/gradylib non existent filename"));
}

TEST_CASE("OpenHashMapTC ifstream constructor") {
    gradylib::OpenHashMapTC<int, double> m;
    m[0] = -3;
    m[1] = 2.313;
    m[2] = 6.92;
    m[3] = -1.34E-7;
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    m.write(tmpFile);
    ifstream ifs(tmpFile);
    gradylib::OpenHashMapTC<int, double> m2(ifs);
    REQUIRE(m.size() == 4);
    REQUIRE(m.at(0) == -3);
    REQUIRE(m.at(1) == 2.313);
    REQUIRE(m.at(2) == 6.92);
    REQUIRE(m.at(3) == -1.34E-7);
    filesystem::remove(tmpFile);
}


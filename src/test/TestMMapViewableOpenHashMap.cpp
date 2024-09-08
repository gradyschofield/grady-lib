//
// Created by Grady Schofield on 8/3/24.
//
#include<catch2/catch_test_macros.hpp>

#include<fstream>
#include<span>
#include<vector>

#include<gradylib/MMapViewableOpenHashMap.hpp>

using namespace gradylib;
using namespace std;
namespace fs = std::filesystem;

struct Ser {
    vector<int> x;

    void serialize(ofstream & ofs) const {
        size_t n = x.size();
        ofs.write(static_cast<char*>(static_cast<void*>(&n)), sizeof(n));
        ofs.write(static_cast<char*>(static_cast<void *>(const_cast<int*>(x.data()))), 4 * n);
    }

    struct View {
        std::span<int const> x;
    };

    static decltype(auto) makeView(std::byte const * ptr) {
        size_t n = *static_cast<size_t const *>(static_cast<void const *>(ptr));
        ptr += sizeof(size_t);
        return View{std::span(static_cast<int const *>(static_cast<void const *>(ptr)), n)};
    }
};

//It's necessary to put serialize in the std namespace so the function can be found by argument dependent lookup
namespace std {
    void serialize(ofstream &ofs, vector<int> const & v) {
        size_t n = v.size();
        ofs.write(static_cast<char*>(static_cast<void*>(&n)), sizeof(n));
        ofs.write(static_cast<char*>(static_cast<void *>(const_cast<int*>(v.data()))), sizeof(int) * n);
    }

    template<typename Value>
    decltype(auto) makeView(std::byte const * ptr);

    template<>
    decltype(auto) makeView<vector<int>>(std::byte const * ptr) {
        size_t n = *static_cast<size_t const *>(static_cast<void const *>(ptr));
        ptr += sizeof(size_t);
        return std::span(static_cast<int const *>(static_cast<void const *>(ptr)), n);
    }
}

TEST_CASE("Memory mapped viewable object open hash map") {
    MMapViewableOpenHashMap<int, vector<int>>::Builder z;

    z.put(4, vector<int>{1, 2, 3});

    z.write("viewable.bin");

    MMapViewableOpenHashMap<int, vector<int>> dz("viewable.bin");

    REQUIRE(dz.size() == 1);
    REQUIRE(dz.contains(4));
    auto view = dz.at(4);
    REQUIRE(view.size() == 3);
    for (int i = 0; i < 3; ++i) {
        REQUIRE(view[i] == i+1);
    }

    MMapViewableOpenHashMap<int, Ser>::Builder z2;
    z2.put(5, Ser{{1,2,3}});

    z2.write("viewable2.bin");

    MMapViewableOpenHashMap<int, Ser> dz2("viewable2.bin");

    REQUIRE(dz2.contains(5));
    auto view2 = dz2.at(5);
    REQUIRE(view2.x.size() == 3);
    for (int i = 0; i < 3; ++i) {
        REQUIRE(view2.x[i] == i+1);
    }
}

TEST_CASE("MMapViewableOpenHashMap throws on non-existent file") {
    REQUIRE_THROWS(gradylib::MMapViewableOpenHashMap<int, vector<int>>("/gradylib_non_existent_filename"));
}

TEST_CASE("MMapViewableOpenHashMap throws on mmap failure") {
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    gradylib::MMapViewableOpenHashMap<int, vector<int>>::Builder builder;
    builder.put(1, vector{1, 2, 3});
    builder.put(2, vector{1, 2, 3});
    builder.put(3, vector{1, 2, 3});
    builder.put(4, vector{1, 2, 3});
    builder.write(tmpFile);
    gradylib::GRADY_LIB_MOCK_MMapViewableOpenHashMap_MMAP<int, vector<int>>();
    REQUIRE_THROWS(gradylib::MMapViewableOpenHashMap<int, vector<int>>(tmpFile));
    gradylib::GRADY_LIB_DEFAULT_MMapViewableOpenHashMap_MMAP<int, vector<int>>();
    filesystem::remove(tmpFile);
}

TEST_CASE("MMapViewableOpenHashMap throws on empty map") {
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    gradylib::MMapViewableOpenHashMap<int, vector<int>>::Builder builder;
    builder.put(1, vector{1, 2, 3});
    builder.write(tmpFile);
    gradylib::MMapViewableOpenHashMap<int, vector<int>> m2(tmpFile);
    REQUIRE_THROWS(m2.at(2));
    filesystem::remove(tmpFile);
}

TEST_CASE("MMapViewableOpenHashMap iterator") {
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    gradylib::MMapViewableOpenHashMap<int, vector<int>>::Builder builder;
    builder.put(1, vector{1, 2, 3});
    builder.put(2, vector{4, 5, 6});
    builder.put(3, vector{7, 8, 9});
    builder.put(4, vector{-1, -2, -3});
    builder.write(tmpFile);
    gradylib::MMapViewableOpenHashMap<int, vector<int>> m2(tmpFile);
    REQUIRE(m2.size() == builder.size());
    auto iter = m2.begin();
    while (iter != m2.end()) {
        REQUIRE(builder.contains(iter.key()));
        auto && v1 = builder[iter.key()];
        auto && v2 = iter.value();
        for (int i = 0; i < v1.size(); ++i) {
            REQUIRE(v1[i] == v2[i]);
        }
        ++iter;
    }
    auto iterCopy = iter;
    ++iter;
    REQUIRE(iter == iterCopy);
    filesystem::remove(tmpFile);
}

TEST_CASE("MMapViewableOpenHashMap iterator operator*") {
    fs::path tmpPath = filesystem::temp_directory_path();
    fs::path tmpFile = tmpPath / "map.bin";
    gradylib::MMapViewableOpenHashMap<int, vector<int>>::Builder builder;
    builder.put(1, vector{1, 2, 3});
    builder.put(2, vector{4, 5, 6});
    builder.put(3, vector{7, 8, 9});
    builder.put(4, vector{-1, -2, -3});
    builder.write(tmpFile);
    gradylib::MMapViewableOpenHashMap<int, vector<int>> m2(tmpFile);
    REQUIRE(m2.size() == builder.size());
    for (auto && [idx, v] : m2) {
        REQUIRE(m2.contains(idx));
        auto && v1 = builder[idx];
        for (int i = 0; i < v1.size(); ++i) {
            REQUIRE(v1[i] == v[i]);
        }
    }
    filesystem::remove(tmpFile);
}

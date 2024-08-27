//
// Created by Grady Schofield on 8/25/24.
//

#include<catch2/catch_test_macros.hpp>

#include<fcntl.h>
#include<errno.h>
#include<sys/mman.h>
#include<unistd.h>

#include<gradylib/BitPairSet.hpp>

using namespace gradylib;
using namespace std;

TEST_CASE("BitPairSet resize") {
    BitPairSet bps(10);
    bps.setFirst(1);
    bps.setFirst(3);
    bps.setFirst(5);
    bps.resize(96);
    bps.setSecond(83);
    for (int i = 0; i < bps.size(); ++i) {
        if (i == 1 || i == 3 || i == 5) {
            REQUIRE(bps.isFirstSet(i));
            REQUIRE(!bps.isSecondSet(i));
        } else if (i == 83) {
            REQUIRE(!bps.isFirstSet(i));
            REQUIRE(bps.isSecondSet(i));
        } else {
            REQUIRE(!bps.isEitherSet(i));
        }
    }
}

TEST_CASE("BitPairSet constructors") {
    BitPairSet bps;
    bps.resize(5);
    bps.setFirst(1);
    bps.setFirst(3);
    bps.unsetFirst(3);
    bps.setSecond(3);
    for (int i = 0; i < 5; ++i) {
        if (i == 1) {
            REQUIRE(bps.isFirstSet(i));
            REQUIRE(!bps.isSecondSet(i));
        } else if (i == 3) {
            REQUIRE(!bps.isFirstSet(i));
            REQUIRE(bps.isSecondSet(i));
        } else {
            REQUIRE(!bps.isEitherSet(i));
        }
    }

    BitPairSet bps2(bps);
    for (int i = 0; i < 5; ++i) {
        if (i == 1) {
            REQUIRE(bps2.isFirstSet(i));
            REQUIRE(!bps2.isSecondSet(i));
        } else if (i == 3) {
            REQUIRE(!bps2.isFirstSet(i));
            REQUIRE(bps2.isSecondSet(i));
        } else {
            REQUIRE(!bps2.isEitherSet(i));
        }
    }
    BitPairSet bps3;
    bps3 = bps2;

    for (int i = 0; i < 5; ++i) {
        if (i == 1) {
            REQUIRE(bps3.isFirstSet(i));
            REQUIRE(!bps3.isSecondSet(i));
        } else if (i == 3) {
            REQUIRE(!bps3.isFirstSet(i));
            REQUIRE(bps3.isSecondSet(i));
        } else {
            REQUIRE(!bps3.isEitherSet(i));
        }
    }

    bps3 = bps3;
    for (int i = 0; i < 5; ++i) {
        if (i == 1) {
            REQUIRE(bps3.isFirstSet(i));
            REQUIRE(!bps3.isSecondSet(i));
        } else if (i == 3) {
            REQUIRE(!bps3.isFirstSet(i));
            REQUIRE(bps3.isSecondSet(i));
        } else {
            REQUIRE(!bps3.isEitherSet(i));
        }
    }
}

TEST_CASE("BitPairSet constructor from memory mapping") {
    BitPairSet bps;
    bps.resize(5);
    bps.setFirst(1);
    bps.setFirst(3);
    bps.unsetFirst(3);
    bps.setSecond(3);

    ofstream ofs("bps_test.bin", ios::binary);
    bps.write(ofs);
    ofs.close();
    int fd = open("bps_test.bin", O_RDONLY);
    REQUIRE(fd >= 0);
    size_t mappingSize = std::filesystem::file_size("bps_test.bin");
    void * memoryMapping = mmap(0, mappingSize, PROT_READ, MAP_SHARED, fd, 0);
    if (memoryMapping == MAP_FAILED) {
        unlink("bps_test.bin");
    }
    REQUIRE(memoryMapping != MAP_FAILED);
    std::byte *ptr = static_cast<std::byte *>(memoryMapping);

    BitPairSet bps2(ptr);
    for (int i = 0; i < 5; ++i) {
        if (i == 1) {
            REQUIRE(bps2.isFirstSet(i));
            REQUIRE(!bps2.isSecondSet(i));
        } else if (i == 3) {
            REQUIRE(!bps2.isFirstSet(i));
            REQUIRE(bps2.isSecondSet(i));
        } else {
            REQUIRE(!bps2.isEitherSet(i));
        }
    }
}

TEST_CASE("BitPairSet move constructor") {
    BitPairSet bps;
    bps.resize(5);
    bps.setFirst(1);
    bps.setFirst(3);
    bps.unsetFirst(3);
    bps.setSecond(3);

    BitPairSet bps2(move(bps));
    for (int i = 0; i < 5; ++i) {
        if (i == 1) {
            REQUIRE(bps2.isFirstSet(i));
            REQUIRE(!bps2.isSecondSet(i));
        } else if (i == 3) {
            REQUIRE(!bps2.isFirstSet(i));
            REQUIRE(bps2.isSecondSet(i));
        } else {
            REQUIRE(!bps2.isEitherSet(i));
        }
    }

    bps = move(bps2);
    for (int i = 0; i < 5; ++i) {
        if (i == 1) {
            REQUIRE(bps.isFirstSet(i));
            REQUIRE(!bps.isSecondSet(i));
        } else if (i == 3) {
            REQUIRE(!bps.isFirstSet(i));
            REQUIRE(bps.isSecondSet(i));
        } else {
            REQUIRE(!bps.isEitherSet(i));
        }
    }
}

TEST_CASE("BitPairSet clear") {
    BitPairSet bps;
    bps.resize(5);
    bps.setFirst(1);
    bps.setFirst(3);
    bps.unsetFirst(3);
    bps.setSecond(3);

    bps.clear();
    for (int i = 0; i < bps.size(); ++i) {
        auto p = bps[i];
        REQUIRE(!p.first);
        REQUIRE(!p.second);
    }
}

TEST_CASE("BitPairSet setters/unsetters") {
    BitPairSet bps;
    bps.resize(5);
    bps.setBoth(1);
    REQUIRE(bps.isFirstSet(1));
    REQUIRE(bps.isSecondSet(1));
    bps.unsetBoth(1);
    REQUIRE(!bps.isEitherSet(1));
    bps.setSecond(1);
    REQUIRE(bps.isSecondSet(1));
    bps.unsetSecond(1);
    REQUIRE(!bps.isEitherSet(1));
}


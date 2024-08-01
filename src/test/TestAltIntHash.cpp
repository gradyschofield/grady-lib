//
// Created by Grady Schofield on 7/30/24.
//

#include<unordered_map>

#include<AltIntHash.hpp>
#include<OpenHashMap.hpp>

using namespace gradylib;
using namespace std;

template<typename Test, typename Standard>
void testMap(Test const & test, Standard const & standard) {
    if (test.size() != standard.size()) {
        cout << "Size problem \n";
    }

    for (auto const & [i1, i2] : standard) {
        if (!test.contains(i1)) {
            cout << "key problem\n";
            exit(1);
        } else if (test.at(i1) != i2) {
            cout << "value problem\n";
            exit(1);
        }
    }

}

int main() {
    OpenHashMap<int8_t, int8_t, AltIntHash> map8;
    unordered_map<int8_t, int8_t> testMap8;

    for (int i = 0; i < 127; ++i) {
        map8[i] = i;
        testMap8[i] = i;
    }

    testMap(map8, testMap8);

    OpenHashMap<uint8_t, uint8_t, AltIntHash> map8u;
    unordered_map<uint8_t, uint8_t> testMap8u;

    for (int i = 0; i < 255; ++i) {
        map8u[i] = i;
        testMap8u[i] = i;
    }

    testMap(map8u, testMap8u);

    OpenHashMap<int16_t, int16_t, AltIntHash> map16;
    unordered_map<int16_t, int16_t> testMap16;

    for (int i = 0; i < 32767; ++i) {
        map16[i] = i;
        testMap16[i] = i;
    }

    testMap(map16, testMap16);

    OpenHashMap<uint16_t, uint16_t, AltIntHash> map16u;
    unordered_map<uint16_t, uint16_t> testMap16u;

    for (int i = 0; i < 65535; ++i) {
        map16u[i] = i;
        testMap16u[i] = i;
    }

    testMap(map16u, testMap16u);

    OpenHashMap<uint64_t, uint64_t, AltIntHash> map64u;
    unordered_map<uint64_t, uint64_t> testMap64u;

    int64_t num = 1E7;
    cout << "starting 64 bit test" << endl;
    auto startTime = chrono::high_resolution_clock::now();
    for (int i = 0; i < num; ++i) {
        map64u[i] = i;
    }
    auto endTime = chrono::high_resolution_clock::now();
    cout << "build time " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << "\n";

    OpenHashMap<uint64_t, uint64_t> map64uReg;
    cout << "starting 64 bit test regular hash" << endl;
    startTime = chrono::high_resolution_clock::now();
    for (int i = 0; i < num; ++i) {
        map64uReg[i] = i;
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "build time " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << "\n";

}
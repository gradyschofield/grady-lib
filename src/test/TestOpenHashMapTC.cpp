//
// Created by Grady Schofield on 7/21/24.
//

#include<iostream>
#include<unordered_map>

#include<OpenHashMapTC.hpp>

using namespace std;
using namespace gradylib;

int main(int argc, char ** argv) {

    OpenHashMapTC<int64_t, int32_t> m;
    unordered_map<int64_t, int32_t> test;
    long size = 1E8;
    for (long i = 0; i < size; ++i) {
        int x = rand();
        int y = rand();
        m[x] = y;
        test[x] = y;
    }

    auto startTime = chrono::high_resolution_clock::now();
    if (m.size() != test.size()) {
        cout << "size problem\n";
        exit(1);
    }

    for (auto & p : test) {
        if (!m.contains(p.first)) {
            cout << "key missing\n";
            exit(1);
        } else if (p.second != m[p.first]) {
            cout << "wrong value\n";
            exit(1);
        }
    }
    auto endTime = chrono::high_resolution_clock::now();
    cout << "testing took " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << " ms\n";

    m.write("openhashmaptc.bin");

    OpenHashMapTC<int64_t, int32_t> m2("openhashmaptc.bin");

    if (m2.size() != test.size()) {
        cout << "size problem\n";
        exit(1);
    }

    startTime = chrono::high_resolution_clock::now();
    for (auto & p : test) {
        if (!m2.contains(p.first)) {
            cout << "key missing\n";
            exit(1);
        } else if (p.second != m2.at(p.first)) {
            cout << "wrong value\n";
            exit(1);
        }
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "testing took " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << " ms\n";

    return 0;
}
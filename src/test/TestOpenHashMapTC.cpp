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
    long size = 1E7;
    for (long i = 0; i < size; ++i) {
        int x = rand();
        int y = rand();
        m[x] = y;
        test[x] = y;
    }

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
    return 0;
}
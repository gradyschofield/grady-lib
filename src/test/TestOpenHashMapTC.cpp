//
// Created by Grady Schofield on 7/21/24.
//

#include<iostream>

#include<OpenHashMapTC.hpp>

using namespace std;
using namespace gradylib;

int main(int argc, char ** argv) {
    OpenHashMapTC<int64_t, int32_t> m;

    m[1234] = 4353;

    for (auto & i : m) {
        cout << i.first << " " << i.second << "\n";
        i.second = 4;
    }
    for (auto & i : m) {
        cout << i.first << " " << i.second << "\n";
    }
    return 0;
}
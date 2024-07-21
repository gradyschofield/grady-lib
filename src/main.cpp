#include<chrono>
#include<iostream>
#include<unordered_set>

#include<OpenHashSetOfTriviallyCopyables.hpp>

using namespace std;

int main() {
    OpenHashSetOfTriviallyCopyables<int> myset;

    long size  = 1E8;
    myset.reserve(size);
    auto startTime = chrono::high_resolution_clock::now();
    for (long i = 0; i < size; ++i) {
        myset.insert(rand());
    }
    auto endTime = chrono::high_resolution_clock::now();
    cout << "OpenHashSetOfTriviallyCopyables: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << endl;

    unordered_set<int> s;
    s.reserve(size);
    startTime = chrono::high_resolution_clock::now();
    for (long i = 0; i < size; ++i) {
        s.insert(rand());
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "unordered_set: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << endl;
    return 0;
}

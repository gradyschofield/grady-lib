#include<bit>
#include<chrono>
#include<iostream>
#include<unordered_set>

#include<OpenHashSetTC.hpp>

using namespace std;

int myRand() {
    return rand() % 100000000;
}

int main() {
    OpenHashSetTC<int> myset;

    long size  = 1E8;
    auto startTime = chrono::high_resolution_clock::now();
    size_t accum = 0;
    for (long i = 0; i < size; ++i) {
        accum += myRand();
    }
    auto endTime = chrono::high_resolution_clock::now();
    long randTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
    cout << "myRand: " << randTime <<  " " << accum << endl;

    myset.reserve(size);
    startTime = chrono::high_resolution_clock::now();
    for (long i = 0; i < size; ++i) {
        myset.insert(myRand());
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "OpenHashSetOfTriviallyCopyables: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() - randTime << endl;

    unordered_set<int> s;
    s.reserve(size);
    startTime = chrono::high_resolution_clock::now();
    for (long i = 0; i < size; ++i) {
        s.insert(myRand());
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "unordered_set: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() - randTime << endl;

    unordered_set<int> test;
    for (int i : myset) {
        test.insert(i);
    }

    cout << myset.size() << " " << test.size() << "\n";

    startTime = chrono::high_resolution_clock::now();
    unique_ptr<OpenHashSetTC<int>> myset2 = make_unique<OpenHashSetTC<int>>(myset);
    endTime = chrono::high_resolution_clock::now();
    cout << chrono::duration_cast<chrono::microseconds>(endTime - startTime).count() << "\n";

    startTime = chrono::high_resolution_clock::now();
    unique_ptr<unordered_set<int>> test2 = make_unique<unordered_set<int>>(test);
    endTime = chrono::high_resolution_clock::now();
    cout << chrono::duration_cast<chrono::microseconds>(endTime - startTime).count() << "\n";


    startTime = chrono::high_resolution_clock::now();
    myset2.reset(nullptr);
    endTime = chrono::high_resolution_clock::now();
    cout << chrono::duration_cast<chrono::microseconds>(endTime - startTime).count() << "\n";


    startTime = chrono::high_resolution_clock::now();
    test2.reset(nullptr);
    endTime = chrono::high_resolution_clock::now();
    cout << chrono::duration_cast<chrono::microseconds>(endTime - startTime).count() << "\n";

    size_t numMods = 1E8;
    for (size_t i = 0; i < numMods; ++i) {
        int x = myRand();
        myset.insert(x);
        test.insert(x);
        int y = myRand();
        myset.erase(y);
        test.erase(y);
    }
    cout << myset.size() << " " << test.size() << "\n";

    startTime = chrono::high_resolution_clock::now();
    for (int i : test) {
        if (!myset.contains(i)) {
            cout << "problem\n";
        }
    }

    if (test.size() != myset.size()) {
        cout << "size problem\n";
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "testing from memory time: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << "\n";

    cout << "writing to disk...\n";
    myset.write("a_grand_old_set.bin");

    startTime = chrono::high_resolution_clock::now();
    OpenHashSetTC<int> myset3("a_grand_old_set.bin");
    endTime = chrono::high_resolution_clock::now();
    cout << "mmap load: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << "\n";

    startTime = chrono::high_resolution_clock::now();
    for (int i : test) {
        if (!myset3.contains(i)) {
            cout << "problem\n";
        }
    }

    if (test.size() != myset3.size()) {
        cout << "size problem\n";
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "testing from mmap load: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << "\n";

    cout << (endian::native == endian::big ? "big" : "little") << "\n";

    return 0;
}

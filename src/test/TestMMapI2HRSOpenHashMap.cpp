//
// Created by Grady Schofield on 7/25/24.
//

#include<string>
#include<unordered_map>

#include<MMapI2HRSOpenHashMap.hpp>
#include<OpenHashMap.hpp>

using namespace std;
using namespace gradylib;

string randString() {
    int len = rand() % 12 + 3;
    vector<char> buffer(len);
    for (int i = 0; i < len; ++i) {
        buffer[i] = rand() % (1 + 126-32) + 32;
    }
    return string(buffer.data(), len);
}

int64_t myRand() {
    static int64_t x = 578363509162734;
    x = x * 175055391405843 + 588734109354867;
    return abs(x);
}

int main() {
    int numStrings = 1E6;
    int numInts = 1E7;
    MMapI2HRSOpenHashMap<int64_t>::Builder b;
    unordered_map<int64_t, string> test;
    OpenHashMap<string, int> strs;
    vector<string> strArray;
    strs.reserve(numStrings);
    strArray.reserve(numStrings);
    for (int i = 0; i < numStrings; ++i ){
        string s = randString();
        while (strs.contains(s)) {
            s = randString();
        }
        strs.put(s, 1);
        strArray.push_back(move(s));
    }

    b.reserve(numInts);
    auto startTime = chrono::high_resolution_clock::now();
    for (long i = 0; i < numInts; ++i) {
        long idx = myRand();
        while (b.contains(idx)) {
            idx = rand();
        }
        string const & str = strArray[rand() % strArray.size()];
        b.put(idx, str);
        test.emplace(idx, str);
    }
    auto endTime = chrono::high_resolution_clock::now();
    cout << "build time: " << chrono::duration_cast<chrono::milliseconds>(endTime-startTime).count() << "\n";


    startTime = chrono::high_resolution_clock::now();
    if (b.size() != test.size()) {
        cout << "size problem\n";
        exit(1);
    }
    for (auto & [key, str] : test) {
        if (!b.contains(key)) {
            cout << "map does not contain key " << key << "\n";
            exit(1);
        } else if (b.at(key) != str) {
            cout << "wrong value " << key << " " << str << " " << b.at(key) << "\n";
            exit(1);
        }
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "test time: " << chrono::duration_cast<chrono::milliseconds>(endTime-startTime).count() << "\n";


    startTime = chrono::high_resolution_clock::now();
    b.write("hrsmap.bin");
    endTime = chrono::high_resolution_clock::now();
    cout << "write time: " << chrono::duration_cast<chrono::milliseconds>(endTime-startTime).count() << "\n";

    startTime = chrono::high_resolution_clock::now();
    MMapI2HRSOpenHashMap<int64_t> m("hrsmap.bin");
    endTime = chrono::high_resolution_clock::now();
    cout << "load time: " << chrono::duration_cast<chrono::milliseconds>(endTime-startTime).count() << "\n";

    startTime = chrono::high_resolution_clock::now();
    if (m.size() != test.size()) {
        cout << "size problem\n";
        exit(1);
    }
    for (auto & [key, str] : test) {
        if (!m.contains(key)) {
            cout << "map does not contain key " << key << "\n";
            exit(1);
        } else if (m.at(key) != str) {
            cout << "wrong value " << key << " " << str << " " << m.at(key) << "\n";
            exit(1);
        }
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "test time: " << chrono::duration_cast<chrono::milliseconds>(endTime-startTime).count() << "\n";
    return 0;
}
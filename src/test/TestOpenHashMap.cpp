//
// Created by Grady Schofield on 7/22/24.
//

#include<chrono>
#include<string>
#include<unordered_map>

#include<OpenHashMap.hpp>

using namespace std;

string randString() {
    int len = rand() % 12 + 3;
    vector<char> buffer(len);
    for (int i = 0; i < len; ++i) {
        buffer[i] = rand() % (1 + 126-32) + 32;
    }
    return string(buffer.data(), len);
}

int main(int argc, char ** argv) {
    long mapSize = 1E9;
    OpenHashMap<string, long> map;
    map.reserve(mapSize);
    vector<string> strs;
    strs.reserve(mapSize);
    auto startTime = chrono::high_resolution_clock::now();
    for (long i = 0; i < mapSize; ++i) {
        strs.push_back(randString());
    }
    auto endTime = chrono::high_resolution_clock::now();
    cout << "string create time " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << " ms\n";

    startTime = chrono::high_resolution_clock::now();
    for (long i = 0; i < mapSize; ++i) {
        map[strs[i]] = map.size();
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "build time " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << " ms\n";

    unordered_map<string, long> test;
    startTime = chrono::high_resolution_clock::now();
    for (long i = 0; i < mapSize; ++i) {
        test[strs[i]] = test.size();
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "unordered_map build time " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << " ms\n";

    for (auto & p : test) {
        if (!map.contains(p.first)) {
            cout << "Missing key\n";
        } else if (map[p.first] != p.second) {
            cout << "Wrong value\n";
        }
    }
    if (test.size() != map.size()) {
        cout << "size problem";
    }

    return 0;
}
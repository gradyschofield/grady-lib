//
// Created by Grady Schofield on 7/22/24.
//

#include<chrono>
#include<string>
#include<unordered_map>
#include<unordered_set>

#include<AltIntHash.hpp>
#include<OpenHashMap.hpp>
#include<MMapS2IOpenHashMap.hpp>

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
    long mapSize = 1E6;
    gradylib::OpenHashMap<string, long> map;
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
    long idx = 0;
    for (long i = 0; i < mapSize; ++i) {
        map[strs[i]] = idx++;
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "build time " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << " ms\n";
    cout << "Map size " << map.size() << "\n";

    unordered_map<string, long> test;
    startTime = chrono::high_resolution_clock::now();
    idx = 0;
    for (long i = 0; i < mapSize; ++i) {
        test[strs[i]] = idx++;
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "unordered_map build time " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << " ms\n";

    startTime = chrono::high_resolution_clock::now();
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
    endTime = chrono::high_resolution_clock::now();
    cout << "OpenHashMap check " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << " ms\n";

    gradylib::writeMappable("stringmap.bin", map);

    gradylib::MMapS2IOpenHashMap<long> map2("stringmap.bin");

    startTime = chrono::high_resolution_clock::now();
    for (auto & p : test) {
        if (!map2.contains(p.first)) {
            cout << "Missing key\n";
        } else if (map2[p.first] != p.second) {
            cout << "Wrong value\n";
        }
    }
    if (test.size() != map2.size()) {
        cout << "size problem";
    }
    endTime = chrono::high_resolution_clock::now();
    cout << "MMapS2IOpenHashMap check " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << " ms\n";

    int num = 0;
    struct AltHash {
        size_t operator()(long const &i) const noexcept {
            return i * 94123453451234 + 4123451435554345;
        }
    };
    gradylib::OpenHashMap<long, string, gradylib::AltHash<long>> sidx;
    //sidx.reserve(map2.size());
    for (auto [str, idx] : map2) {
        //sidx.emplace(idx, string(str));
        sidx[idx] = string(str);
    }

    if (sidx.size() != map2.size()) {
        cout << "Size problem for sidx " << sidx.size() << " " << map2.size() << "\n";
        exit(1);
    }

    for (auto [idx, str] : sidx) {
        if (!map2.contains(str)) {
            cout << "sidx doesn't contain " << idx << "\n";
            exit(1);
        } else if (map2[str] != idx) {
            cout << "sidx doesn't contain write value for " << idx << " " << str << " " << sidx[idx] << "\n";
            exit(1);
        }
    }



    return 0;
}
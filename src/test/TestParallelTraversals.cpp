//
// Created by Grady Schofield on 7/27/24.
//

#include<AltIntHash.hpp>
#include<OpenHashMap.hpp>

using namespace gradylib;
using namespace std;

int main() {
    OpenHashMap<int64_t, int64_t, AltIntHash<int64_t>> m;
    size_t num = 100000;
    for (size_t i = 0; i < num; ++i) {
        m[i] = rand();
    }
    auto startTime = chrono::high_resolution_clock::now();
#if 1
    auto f = m.parallelForEach([](decltype(m) & partial, int64_t const & key, int64_t const & value){
        partial[key] = value;
    },
    [size = m.size()](int threadIdx, int numThreads) {
        return OpenHashMap<int64_t, int64_t, AltIntHash<int64_t>>(size / numThreads + numThreads);
    },
    [size = m.size()](int numThreads) {
        return OpenHashMap<int64_t, int64_t, AltIntHash<int64_t>>(size);
    });
#else
    auto f = m.parallelForEach([](decltype(m) & partial, int64_t const & key, int64_t const & value){
                                   partial[key] = value;
                               });
#endif
    auto r = f.get();
    auto endTime = chrono::high_resolution_clock::now();
    cout << "time: " << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() << " ms\n";
    std::cout << r.size() << " " << m.size() << "\n";
    return 0;
}
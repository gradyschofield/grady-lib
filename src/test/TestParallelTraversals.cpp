//
// Created by Grady Schofield on 7/27/24.
//

#include<catch2/catch_test_macros.hpp>

#include"gradylib/AltIntHash.hpp"
#include"gradylib/OpenHashMap.hpp"

using namespace gradylib;
using namespace std;

TEST_CASE() {
    OpenHashMap<int64_t, int64_t, AltIntHash> m;
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
        return OpenHashMap<int64_t, int64_t, AltIntHash>(size / numThreads + numThreads);
    },
    [size = m.size()](int numThreads) {
        return OpenHashMap<int64_t, int64_t, AltIntHash>(size);
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
    REQUIRE( r.size() == m.size());
}
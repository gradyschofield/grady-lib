//
// Created by Grady Schofield on 7/27/24.
//

#include<ParallelTraversals.hpp>

using namespace gradylib;
using namespace std;

int main() {
    OpenHashMap<int64_t, int64_t> m;
    ThreadPool tp;
    future<decltype(m)> f = parallelForEach<decltype(m)>(tp, m, [](decltype(m) & partial, int64_t const & key, int64_t const & value){
        partial[key] = value;
    });
    return 0;
}
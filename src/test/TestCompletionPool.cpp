//
// Created by Grady Schofield on 8/9/24.
//

#include<catch2/catch_test_macros.hpp>

#include<iostream>

#include<gradylib/AltIntHash.hpp>
#include<gradylib/CompletionPool.hpp>
#include<gradylib/ThreadPool.hpp>
#include<gradylib/OpenHashMap.hpp>

using namespace gradylib;
using namespace std;

TEST_CASE(){
    ThreadPool tp;
    CompletionPool<int> completionPool;
    for (int i = 0; i < tp.size(); ++i) {
        tp.add([&completionPool]() {
            for (int i = 0; i < 10000; ++i) {
                completionPool.add(i);
            }
        });
    }
    tp.wait();
    OpenHashMap<int, long, AltIntHash> result;
    for (int i : completionPool) {
        result[i] += 1;
    }
    long total = 0;
    for (auto [val, count] : result) {
        if (count != tp.size()) {
            cout << "problem " << val << " " << count << "\n";
        }
        total += count;
    }
    REQUIRE(tp.size() * 10000 == total);
}
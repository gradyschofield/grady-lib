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

TEST_CASE("Completion Pool"){
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

TEST_CASE("Completion Pool Heavy"){
    int64_t numCompletions = 10000000;
    CompletionPool<int64_t> completionPool;
    auto writer = [numCompletions, &completionPool]() {
        for (int64_t i = 0; i < numCompletions; ++i) {
            completionPool.add(i);
        }
    };
    atomic<bool> stop{false};
    int64_t numFinished = 0;
    auto reader = [numCompletions, &numFinished, &stop, &completionPool]() {
        bool lastPass = false;
        int64_t numLoops = 0;
        while (!lastPass) {
            if (stop.load(std::memory_order_relaxed)) {
                lastPass = true;
            }
            bool entered = false;
            for (auto i: completionPool) {
                entered = true;
                ++numFinished;
            }
            if (entered) {
                ++numLoops;
            }
        }
        cout << "Average CompletionPool partial result size " << numFinished / (double)numLoops << endl;
    };
    thread writerThread(writer);
    thread readerThread(reader);
    writerThread.join();
    stop.store(true, std::memory_order_relaxed);
    readerThread.join();
    REQUIRE(numFinished == numCompletions);
}

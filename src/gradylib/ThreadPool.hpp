/*
MIT License

Copyright (c) 2024 Grady Schofield

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include<concepts>
#include<condition_variable>
#include<functional>
#include<iostream>
#include<mutex>
#include<queue>
#include<thread>

namespace gradylib {

    class ThreadPool {
        std::queue<std::function<void()>> work;
        std::vector<std::thread> threads;
        std::mutex mutable workMutex;
        std::condition_variable workerConditionVariable;
        std::condition_variable waiterConditionVariable;
        std::atomic<int> freeThreads{0};
        std::atomic<bool> stop{false};

    public:

        int size() const {
            return threads.size();
        }

        ThreadPool(int numThreads = std::thread::hardware_concurrency())
            : freeThreads(numThreads)
        {
            for (int i = 0; i < numThreads; ++i) {
                threads.emplace_back([this, i]() {
                    while (true) {
                        std::unique_lock lock(workMutex);
                        workerConditionVariable.wait_for(lock, std::chrono::milliseconds(500), [this]{
                            return !work.empty() || stop.load(std::memory_order_relaxed);
                        });
                        if (stop.load(std::memory_order_relaxed)) {
                            return;
                        }
                        if (!work.empty()) {
                            //auto f = std::move(work.front());
                            auto f{std::move(work.front())};
                            work.pop();
                            freeThreads.fetch_sub(1, std::memory_order_relaxed);
                            lock.unlock();
                            f();
                            freeThreads.fetch_add(1, std::memory_order_relaxed);
                            waiterConditionVariable.notify_one();
                        }
                    }
                });
            }
        }

        bool isEmpty() const {
            std::unique_lock lock(workMutex);
            return work.empty();
        }

        template<std::invocable Invocable>
        void add(Invocable && f) {
            workMutex.lock();
            work.push(std::forward<Invocable>(f));
            workMutex.unlock();
            workerConditionVariable.notify_one();
        }

        template<std::invocable<size_t,size_t> Invocable>
        void allocateOverThreads(size_t count, Invocable && f) {
            workMutex.lock();
            size_t start = 0;
            size_t stop = 0;
            for (int i = 0; i < threads.size(); ++i) {
                stop = start + count / threads.size() + (i < count % threads.size() ? 1 : 0);
                work.push([start, stop, f]() {
                    f(start, stop);
                });
                start = stop;
            }
            workMutex.unlock();
            workerConditionVariable.notify_one();
        }

        void wait() {
            std::unique_lock lock(workMutex);
            if (work.empty() && freeThreads.load(std::memory_order_relaxed) == threads.size()) {
                return;
            }
            while (true) {
                waiterConditionVariable.wait_for(lock, std::chrono::milliseconds(500), [this]{
                    return work.empty() && freeThreads.load(std::memory_order_relaxed) == threads.size();
                });
                if (work.empty() && freeThreads.load(std::memory_order_relaxed) == threads.size()) {
                    return;
                }
            }
        }

        ~ThreadPool() {
            stop.store(true, std::memory_order_relaxed);
            workerConditionVariable.notify_all();
            for (auto & t : threads) {
                t.join();
            }
        }
    };
}

namespace gradylib_helpers {
    inline std::unique_ptr<gradylib::ThreadPool> GRADY_LIB_DEFAULT_THREADPOOL;
    inline std::mutex GRADY_LIB_DEFAULT_THREADPOOL_MUTEX;
}


//
// Created by Grady Schofield on 7/27/24.
//

#ifndef GRADY_LIB_PARALLELTRAVERSALS_HPP
#define GRADY_LIB_PARALLELTRAVERSALS_HPP

#include<concepts>
#include<future>
#include<vector>

#include<Common.hpp>
#include<OpenHashMap.hpp>
#include<ThreadPool.hpp>

namespace gradylib {

    template<typename Key, typename Value>
    void mergePartials(OpenHashMap<Key, Value> & m1, OpenHashMap<Key, Value> const & m2) {
        for (auto const & [key, value] : m2) {
            m1[key] = value;
        }
    }

    template<Mergeable ReturnValue, typename Key, typename Value, typename HashFunction, typename Callable>
    requires std::is_invocable_r_v<void, Callable, ReturnValue &, Key const &, Value const &> && std::is_copy_constructible_v<Callable>
    std::future<ReturnValue> parallelForEach(ThreadPool & tp, OpenHashMap<Key, Value, HashFunction> const & m, Callable && f, size_t numThreads) {
        if (numThreads == 0) {
            numThreads = tp.size();
        }
        numThreads = std::min(numThreads, m.size());
        struct Result {
            ReturnValue final;
            std::mutex finalMutex;
            std::promise<ReturnValue> promise;
        };
        std::shared_ptr<Result> result = std::make_shared<Result>();
        size_t start = 0;
        for (size_t i = 0; i < numThreads; ++i) {
            size_t stop = m.keys.size() / numThreads + (i < m.keys.size() % numThreads ? 1 : 0);
            tp.add([start, stop, m, f, result]() {
                ReturnValue partial;
                for (size_t j = start; j < stop; ++j) {
                    if (m.setFlags.isFirstSet(j)) {
                        f(partial, m.keys[j], m.values[j]);
                    }
                }
                std::lock_guard lg(result->finalMutex);
                mergePartials(result->final, partial);
                if (result.use_count() == 1) {
                    result->promise.set_value(std::move(result->final));
                }
            });
        }
        return result->promise.get_future();
    }
}

#endif //GRADY_LIB_PARALLELTRAVERSALS_HPP

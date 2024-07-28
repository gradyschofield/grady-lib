//
// Created by Grady Schofield on 7/27/24.
//

#ifndef GRADY_LIB_PARALLELTRAVERSALS_HPP
#define GRADY_LIB_PARALLELTRAVERSALS_HPP

#include<OpenHashMap.hpp>
#include<ThreadPool.hpp>

namespace gradylib {

    template<typename T>
    concept Mergeable = requires(T a, T b) {
        { mergePartials(a, b) } -> std::same_as<void>;
    };

    template<typename Key, typename Value>
    void mergePartials(OpenHashMap<Key, Value> & m1, OpenHashMap<Key, Value> & m2) {
        for (auto & [key, value] : m2) {
            m1[key] = value;
        }
        m2 = {};
    }

    template<Mergeable ReturnValue, typename Key, typename Value, typename Callable>
    std::future<ReturnValue> parallelForEach(ThreadPool & tp, OpenHashMap<Key, Value> const & m, Callable && f) {
    }
}

#endif //GRADY_LIB_PARALLELTRAVERSALS_HPP

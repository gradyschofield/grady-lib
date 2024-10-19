//
// Created by Grady Schofield on 10/18/24.
//

#include"CompletionPool.hpp"

#pragma once

namespace gradylib {

    template<typename T>
    class CompletionPoolPair {

        CompletionPool<T> pool1;
        CompletionPool<T> pool2;
        bool which = false;

    public:

        void swap() {
            which = !which;
        }

        template<typename Invocable>
        requires std::is_invocable_r_v<T, Invocable>
        gradylib_helpers::CompletionPoolResultWrapper<T> wrap(Invocable && invocable) {
            CompletionPool<T> & pool = which ? pool1 : pool2;
            return gradylib_helpers::CompletionPoolResultWrapper<T>(std::forward<Invocable>(invocable), pool);
        }

        template<typename Invocable>
        requires std::is_invocable_r_v<T, Invocable>
        gradylib_helpers::CompletionPoolResultWrapper<T> wrap(bool whichParam, Invocable && invocable) {
            CompletionPool<T> & pool = whichParam ? pool1 : pool2;
            return gradylib_helpers::CompletionPoolResultWrapper<T>(std::forward<Invocable>(invocable), pool);
        }
    };

}
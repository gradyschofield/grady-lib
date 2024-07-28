//
// Created by Grady Schofield on 7/27/24.
//

#ifndef GRADY_LIB_COMMON_HPP
#define GRADY_LIB_COMMON_HPP

#include<concepts>

namespace gradylib {
    template<typename T>
    concept Mergeable = requires(T a, T b) {
        { mergePartials(a, b) } -> std::same_as<void>;
    };

    class ThreadPool;
}

#endif //GRADY_LIB_COMMON_HPP

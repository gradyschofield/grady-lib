//
// Created by Grady Schofield on 7/27/24.
//

#ifndef GRADY_LIB_PARALLELTRAVERSALS_HPP
#define GRADY_LIB_PARALLELTRAVERSALS_HPP

#include<concepts>
#include<future>
#include<vector>

namespace gradylib {

    template<typename T>
    struct PartialDefaultConstructor {
        T operator()(int threadIdx, int numThreads) {
            return T{};
        }
    };

    template<typename T>
    struct FinalDefaultConstructor {
        T operator()(int numThreads) {
            return T{};
        }
    };

}

#endif //GRADY_LIB_PARALLELTRAVERSALS_HPP

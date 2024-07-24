//
// Created by Grady Schofield on 7/24/24.
//

#ifndef GRADY_LIB_ALTINTHASH_HPP
#define GRADY_LIB_ALTINTHASH_HPP

#include<cstddef>
#include<cstdint>

namespace gradylib {
    template<typename IntType>
    struct AltHash {
        size_t operator()(IntType const &i) const noexcept {
            return i * 94123453451234 + 4123451435554345;
        }
    };
}

#endif //GRADY_LIB_ALTINTHASH_HPP

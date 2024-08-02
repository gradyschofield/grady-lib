//
// Created by Grady Schofield on 7/24/24.
//

#ifndef GRADY_LIB_ALTINTHASH_HPP
#define GRADY_LIB_ALTINTHASH_HPP

#include<bit>
#include<cstddef>
#include<cstdint>

namespace gradylib {
    template<typename IntType>
    struct AltIntHash {
        size_t operator()(IntType const &i) const noexcept {
            size_t t = i;
            return t * 94123453451234 + 4123451435554345;
        }
    };
}

#endif //GRADY_LIB_ALTINTHASH_HPP

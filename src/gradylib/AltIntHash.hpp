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

    template<typename T>
    struct AltHash {
        size_t operator()(T const &key) const noexcept {
            // The hash for integral types on some systems is just the identity.  This can be TERRIBLE for very large maps.
            if constexpr (std::is_integral_v<T>) {
                size_t t = key;
                return t * 94123453451234 + 4123451435554345;
            }
            else {
                return std::hash<T>{}(key);
            }
        }
    };
}


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

namespace gradylib_helpers {
    template<typename T>
    concept Mergeable = requires(T a, T b) {
        { mergePartials(a, b) } -> std::same_as<void>;
    };

    template<typename KeyType, typename Key>
    concept equality_comparable = requires (KeyType const & k1, Key const & k2) {
        { k2 == k1 } -> std::same_as<bool>;
    };
}

namespace gradylib_helpers {

    template<int alignment>
    int getPadLength(int64_t pos) {
        int padLength = alignment - pos % alignment;
        return padLength == alignment ? 0 : padLength;
    }

    template<int alignment>
    void writePad(std::ofstream & ofs) {
        static std::vector<char> pad(alignment, 0);
        int64_t pos = ofs.tellp();
        int padLength = getPadLength<alignment>(pos);
        ofs.write(pad.data(), padLength);
    }
}


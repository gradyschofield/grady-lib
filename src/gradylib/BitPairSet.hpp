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


/*
 * For use in the OpenHashSet/Map.  Copying large vector<bool>'s is mighty slow on my current system.
 */

#ifndef GRADY_LIB_BITPAIRSET_HPP
#define GRADY_LIB_BITPAIRSET_HPP

#include<algorithm>
#include<bit>
#include<cstdint>
#include<cstring>
#include<fstream>
#include<iostream>
#include<utility>

#include<gradylib/Exception.hpp>

namespace gradylib {
    class BitPairSet {
        using UnderlyingInt = uint32_t;
        static constexpr int bitShiftForDivision = std::countr_zero(sizeof(UnderlyingInt) * 8 / 2);
        static constexpr size_t mask = (1 << bitShiftForDivision) - 1;
        UnderlyingInt *underlying = nullptr;
        size_t setSize = 0;
        bool readOnly = false;

        inline size_t getUnderlyingLength(size_t len) const {
            size_t base = len >> bitShiftForDivision;
            return base + ((mask & len) != 0 ? 1 : 0);
        }

        inline void unset(size_t idx, UnderlyingInt pairMask) {
            if (readOnly) {
                throw gradylibMakeException("Tried to modify read only BitPairSet");
            }
            size_t base = idx >> bitShiftForDivision;
            size_t offsetShift = (idx & mask) << 1;
            underlying[base] &= ~(pairMask << offsetShift);
        }

        inline void set(size_t idx, UnderlyingInt pairMask) {
            if (readOnly) {
                throw gradylibMakeException("Tried to modify read only BitPairSet");
            }
            size_t base = idx >> bitShiftForDivision;
            size_t offsetShift = (idx & mask) << 1;
            underlying[base] |= pairMask << offsetShift;
        }

        bool isSet(size_t idx, UnderlyingInt pairMask) const {
            size_t base = idx >> bitShiftForDivision;
            size_t offsetShift = (idx & mask) << 1;
            UnderlyingInt t = underlying[base] >> offsetShift;
            return (t & pairMask) != 0;
        }

    public:

        BitPairSet() = default;

        BitPairSet(UnderlyingInt *underlying, size_t setSize)
                : underlying(underlying), setSize(setSize), readOnly(true) {
        }

        BitPairSet(BitPairSet const &s) {
            size_t len = getUnderlyingLength(s.setSize);
            underlying = new UnderlyingInt[len];
            memcpy(underlying, s.underlying, len * sizeof(UnderlyingInt));
            setSize = s.setSize;
        }

        BitPairSet(void const * memoryMapping)
                : underlying(
                static_cast<UnderlyingInt *>(
                        const_cast<void *>(
                                static_cast<void const *>(
                                        8 + static_cast<std::byte const *>(memoryMapping))))),
                  setSize(*static_cast<size_t const *>(memoryMapping)),
                  readOnly(true) {
        }

        BitPairSet(std::ifstream & ifs) {
            ifs.read(static_cast<char*>(static_cast<void*>(&setSize)), sizeof(setSize));
            size_t len = getUnderlyingLength(setSize);
            underlying = new UnderlyingInt[len];
            ifs.read(static_cast<char*>(static_cast<void*>(underlying)), sizeof(UnderlyingInt) * len);
        }

        ~BitPairSet() {
            if (!readOnly) {
                delete[] underlying;
            }
        }


        BitPairSet &operator=(BitPairSet const &s) {
            if (this == &s) {
                return *this;
            }
            size_t len = getUnderlyingLength(s.setSize);
            underlying = new UnderlyingInt[len];
            memcpy(underlying, s.underlying, len * sizeof(UnderlyingInt));
            setSize = s.setSize;
            return *this;
        }

        BitPairSet(BitPairSet &&s)
                : underlying(s.underlying), setSize(s.setSize), readOnly(s.readOnly) {
            s.underlying = nullptr;
            s.setSize = 0;
        }

        BitPairSet &operator=(BitPairSet &&s) noexcept {
            underlying = s.underlying;
            setSize = s.setSize;
            readOnly = s.readOnly;
            s.underlying = nullptr;
            s.setSize = 0;
            return *this;
        }

        BitPairSet(size_t size)
                : underlying(new UnderlyingInt[getUnderlyingLength(size)]), setSize(size) {
            memset(underlying, 0, getUnderlyingLength(size) * sizeof(UnderlyingInt));
        }

        void write(std::ofstream &ofs) const {
            ofs.write((char *) &setSize, 8);
            size_t len = getUnderlyingLength(setSize);
            ofs.write((char *) underlying, sizeof(UnderlyingInt) * len);
        }

        size_t size() const {
            return setSize;
        }

        void clear() {
            memset(underlying, 0, getUnderlyingLength(setSize) * sizeof(UnderlyingInt));
        }

        void resize(size_t size) {
            if (readOnly) {
                throw gradylibMakeException("Tried to resize a read only BitPairSet");
            }
            size_t newSize = getUnderlyingLength(size);
            UnderlyingInt *newUnderlying = new UnderlyingInt[newSize];
            size_t currentSize = getUnderlyingLength(setSize);
            memcpy(newUnderlying, underlying, std::min(newSize, currentSize) * sizeof(UnderlyingInt));
            if (newSize > currentSize) {
                memset(&newUnderlying[currentSize], 0, (newSize - currentSize) * sizeof(UnderlyingInt));
            }
            delete[] underlying;
            underlying = newUnderlying;
            setSize = size;
        }

        std::pair<bool, bool> operator[](int idx) const {
            size_t base = idx >> bitShiftForDivision;
            size_t offsetShift = (idx & mask) << 1;
            UnderlyingInt t = underlying[base] >> offsetShift;
            return {(t & 0b10) != 0, (t & 0b01) != 0};
        }

        void setFirst(size_t idx) {
            set(idx, 0b10);
        }

        void setSecond(size_t idx) {
            set(idx, 0b01);
        }

        void setBoth(size_t idx) {
            set(idx, 0b11);
        }

        void unsetFirst(size_t idx) {
            unset(idx, 0b10);
        }

        void unsetSecond(size_t idx) {
            unset(idx, 0b01);
        }

        void unsetBoth(size_t idx) {
            unset(idx, 0b11);
        }

        bool isFirstSet(size_t idx) const {
            return isSet(idx, 0b10);
        }

        bool isSecondSet(size_t idx) const {
            return isSet(idx, 0b01);
        }

        bool isEitherSet(size_t idx) const {
            return isSet(idx, 0b11);
        }
    };
}

#endif

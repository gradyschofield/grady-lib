//
// Created by Grady Schofield on 7/20/24.
//

/*
 * For use in the OpenHashSet/Map.  Copying large vector<bool>'s is mighty slow.
 */

#ifndef GRADY_LIB_BITPAIRSET_HPP
#define GRADY_LIB_BITPAIRSET_HPP

#include<algorithm>
#include<bit>
#include<cstdint>
#include<utility>

class BitPairSet {
    using UnderlyingInt = uint32_t;
    static constexpr int bitShiftForDivision = std::countr_zero(sizeof(UnderlyingInt) * 8 / 2);
    static constexpr size_t mask = (1 << bitShiftForDivision) - 1;
    UnderlyingInt * underlying = nullptr;
    size_t setSize = 0;

    inline size_t getUnderlyingLength(size_t len) {
        size_t base = len >> bitShiftForDivision;
        return base + ((mask & len) != 0 ? 1 : 0);
    }

    inline void unset(size_t idx, UnderlyingInt pairMask) {
        size_t base = idx >> bitShiftForDivision;
        size_t offsetShift = (idx & mask) << 1;
        underlying[base] &= ~(pairMask << offsetShift);
    }

    inline void set(size_t idx, UnderlyingInt pairMask) {
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

    BitPairSet(BitPairSet const & s) {
        size_t len = getUnderlyingLength(s.setSize);
        underlying = new UnderlyingInt[len];
        memcpy(underlying, s.underlying, len * sizeof(UnderlyingInt));
        setSize = s.setSize;
    }

    BitPairSet & operator=(BitPairSet const & s) {
        BitPairSet t(s);
        std::swap(t, *this);
        return *this;
    }

    BitPairSet(BitPairSet && s)
        : underlying(s.underlying), setSize(s.setSize)
    {
        s.underlying = nullptr;
        s.setSize = 0;
    }

    BitPairSet & operator=(BitPairSet && s) {
        underlying = s.underlying;
        setSize = s.setSize;
        s.underlying = nullptr;
        s.setSize = 0;
        return *this;
    }

    BitPairSet(size_t size)
        : underlying(new UnderlyingInt[getUnderlyingLength(size)]), setSize(size)
    {
        memset(underlying, 0, getUnderlyingLength(size) * sizeof(UnderlyingInt));
    }

    size_t size() const {
        return setSize;
    }

    ~BitPairSet() {
        delete [] underlying;
    }

    void resize(size_t size) {
        size_t newSize = getUnderlyingLength(size);
        UnderlyingInt * newUnderlying = new UnderlyingInt[newSize];
        size_t currentSize = getUnderlyingLength(setSize);
        memcpy(newUnderlying, underlying, std::min(newSize, currentSize));
        if (newSize > currentSize) {
            memset(&newUnderlying[currentSize], 0, (newSize - currentSize) * sizeof(UnderlyingInt));
        }
        delete [] underlying;
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

#endif

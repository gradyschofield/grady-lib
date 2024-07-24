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

#ifndef GRADY_LIB_OPENHASHMAPTC_HPP
#define GRADY_LIB_OPENHASHMAPTC_HPP

#include<type_traits>
#include<vector>

#include<BitPairSet.hpp>

namespace gradylib {
    template<typename Key, typename Value, typename HashFunction = std::hash<Key>>
    requires std::is_trivially_copyable_v<Key> &&
             std::is_trivially_copyable_v<Value> &&
             std::is_default_constructible_v<Key> &&
             std::is_default_constructible_v<Value>
    class OpenHashMapTC {
        std::vector<Key> keys;
        std::vector<Value> values;
        BitPairSet setFlags;
        double loadFactor = 0.8;
        double growthFactor = 1.2;
        size_t mapSize = 0;
        HashFunction hashFunction = HashFunction{};

        void rehash(size_t size = 0) {
            size_t newSize;
            if (size > 0) {
                if (size < mapSize) {
                    return;
                }
                newSize = size / loadFactor;
            } else {
                newSize = std::max<size_t>(keys.size() + 1, std::max<size_t>(1, keys.size()) * growthFactor);
            }
            std::vector<Key> newKeys(newSize);
            std::vector<Value> newValues(newSize);
            BitPairSet newSetFlags(newSize);
            for (size_t i = 0; i < keys.size(); ++i) {
                if (!setFlags.isFirstSet(i)) {
                    continue;
                }
                Key const &k = keys[i];
                size_t hash = hashFunction(k);
                size_t idx = hash % newKeys.size();
                while (newSetFlags.isFirstSet(idx)) {
                    ++idx;
                    idx = idx == newKeys.size() ? 0 : idx;
                }
                newSetFlags.setBoth(idx);
                newKeys[idx] = k;
                newValues[idx] = values[i];
            }
            std::swap(keys, newKeys);
            std::swap(values, newValues);
            std::swap(setFlags, newSetFlags);
        }

    public:

        Value &operator[](Key const &key) {
            size_t hash;
            size_t idx;
            size_t startIdx;
            size_t firstUnsetIdx = -1;
            bool isFirstUnsetIdxSet = false;
            if (!keys.empty()) {
                hash = hashFunction(key);
                idx = hash % keys.size();
                startIdx = idx;
                for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                    if (!isFirstUnsetIdxSet && !isSet) {
                        firstUnsetIdx = idx;
                        isFirstUnsetIdxSet = true;
                    }
                    if (isSet && keys[idx] == key) {
                        return values[idx];
                    }
                    if (wasSet && keys[idx] == key) {
                        break;
                    }
                    ++idx;
                    idx = idx == keys.size() ? 0 : idx;
                    if (startIdx == idx) break;
                }
            }
            if (mapSize >= keys.size() * loadFactor) {
                rehash();
                hash = hashFunction(key);
                idx = hash % keys.size();
                startIdx = idx;
                while (setFlags.isFirstSet(idx)) {
                    ++idx;
                    idx = idx == keys.size() ? 0 : idx;
                    if (startIdx == idx) break;
                }
            } else {
                idx = isFirstUnsetIdxSet ? firstUnsetIdx : idx;
            }
            setFlags.setBoth(idx);
            keys[idx] = key;
            ++mapSize;
            return values[idx];
        }

        void emplace(Key const &key, Value const &value) {
            size_t hash = 0;
            size_t idx = 0;
            bool doesContain = false;
            size_t firstUnsetIdx = -1;
            bool isFirstUnsetIdxSet = false;
            size_t startIdx = idx;
            if (keys.size() > 0) {
                hash = hashFunction(key);
                idx = hash % keys.size();
                for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                    if (!isFirstUnsetIdxSet && !isSet) {
                        firstUnsetIdx = idx;
                        isFirstUnsetIdxSet = true;
                    }
                    if (isSet && keys[idx] == key) {
                        values[idx] = value;
                        doesContain = true;
                        break;
                    }
                    if (wasSet && keys[idx] == key) {
                        doesContain = false;
                        break;
                    }
                    ++idx;
                    idx = idx == keys.size() ? 0 : idx;
                    if (startIdx == idx) break;
                }
            }
            if (doesContain) {
                return;
            }
            if (mapSize >= keys.size() * loadFactor) {
                rehash();
                hash = hashFunction(key);
                idx = hash % keys.size();
                startIdx = idx;
                while (setFlags.isFirstSet(idx)) {
                    ++idx;
                    idx = idx == keys.size() ? 0 : idx;
                    if (startIdx == idx) break;
                }
            } else {
                idx = isFirstUnsetIdxSet ? firstUnsetIdx : idx;
            }
            setFlags.setBoth(idx);
            keys[idx] = key;
            values[idx] = value;
            ++mapSize;
        }

        bool contains(Key const &key) {
            if (keys.empty()) {
                return false;
            }
            size_t hash = hashFunction(key);
            size_t idx = hash % keys.size();
            size_t startIdx = idx;
            for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                if (isSet && keys[idx] == key) {
                    return true;
                }
                if (wasSet && keys[idx] == key) {
                    return false;
                }
                ++idx;
                idx = idx == keys.size() ? 0 : idx;
                if (startIdx == idx) break;
            }
            return false;
        }

        void erase(Key const &key) {
            size_t hash = hashFunction(key);
            size_t idx = hash % keys.size();
            size_t startIdx = idx;
            for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                if (keys[idx] == key) {
                    if (isSet) {
                        --mapSize;
                        setFlags.unsetFirst(idx);
                    }
                    return;
                }
                ++idx;
                idx = idx == keys.size() ? 0 : idx;
                if (startIdx == idx) break;
            }
            return;
        }

        void reserve(size_t size) {
            rehash(size);
        }

        class iterator {
            size_t idx;
            OpenHashMapTC *container;
        public:
            iterator(size_t idx, OpenHashMapTC *container)
                    : idx(idx), container(container) {
            }

            bool operator==(iterator const &other) const {
                return idx == other.idx && container == other.container;
            }

            bool operator!=(iterator const &other) const {
                return idx != other.idx || container != other.container;
            }

            const std::pair<Key const &, Value &> operator*() const {
                return {container->keys[idx], container->values[idx]};
            }

            Key const &key() const {
                return container->keys[idx];
            }

            Value &value() {
                return container->values[idx];
            }

            iterator &operator++() {
                if (idx == container->keys.size()) {
                    return *this;
                }
                ++idx;
                while (idx < container->keys.size() && !container->setFlags.isFirstSet(idx)) {
                    ++idx;
                }
                return *this;
            }
        };

        iterator begin() {
            if (mapSize == 0) {
                return iterator(keys.size(), this);
            }
            size_t idx = 0;
            while (idx < keys.size() && !setFlags.isFirstSet(idx)) {
                ++idx;
            }
            return iterator(idx, this);
        }

        iterator end() {
            return iterator(keys.size(), this);
        }

        size_t size() const {
            return mapSize;
        }
    };
}

#endif

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

#include<fstream>
#include<memory>
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
        Key * keys = nullptr;
        Value * values = nullptr;
        size_t mapSize = 0;
        size_t keySize = 0;
        double loadFactor = 0.8;
        double growthFactor = 1.2;
        HashFunction hashFunction = HashFunction{};
        BitPairSet setFlags;

        void rehash(size_t size = 0) {
            size_t newSize;
            if (size > 0) {
                if (size < mapSize) {
                    return;
                }
                newSize = size / loadFactor;
            } else {
                newSize = std::max<size_t>(keySize + 1, std::max<size_t>(1, keySize) * growthFactor);
            }
            Key * newKeys = new Key[newSize];
            Value * newValues = new Value[newSize];
            BitPairSet newSetFlags(newSize);
            for (size_t i = 0; i < keySize; ++i) {
                if (!setFlags.isFirstSet(i)) {
                    continue;
                }
                Key const &k = keys[i];
                size_t hash = hashFunction(k);
                size_t idx = hash % newSize;
                while (newSetFlags.isFirstSet(idx)) {
                    ++idx;
                    idx = idx == newSize ? 0 : idx;
                }
                newSetFlags.setBoth(idx);
                newKeys[idx] = k;
                newValues[idx] = values[i];
            }
            delete [] keys;
            keys = newKeys;
            keySize = newSize;
            delete [] values;
            values = newValues;
            std::swap(setFlags, newSetFlags);
        }

    public:

        OpenHashMapTC() = default;

        OpenHashMapTC(OpenHashMapTC const & m) {
            BitPairSet tmpSetFlags = m.setFlags;
            std::unique_ptr<Key[]> tmpKeys = new Key[m.keySize];
            values = new Value[m.keySize];
            keys = tmpKeys.release();
            std::swap(setFlags, tmpSetFlags);
            keySize = m.keySize;
            mapSize = m.mapSize;
            loadFactor = m.loadFactor;
            growthFactor = m.growthFactor;
            memcpy(keys, m.keys, sizeof(Key) * keySize);
            memcpy(values, m.values, sizeof(Value) * keySize);
        }

        OpenHashMapTC(OpenHashMapTC && m) noexcept
            : keys(m.keys), values(m.values), keySize(m.keySize), mapSize(m.mapSize),
            loadFactor(m.loadFactor), growthFactor(m.growthFactor), setFlags(std::move(m.setFlags))
        {
            m.keys = nullptr;
            m.values = nullptr;
            m.keySize = 0;
            m.mapSize = 0;
        }

        OpenHashMapTC & operator=(OpenHashMapTC const & m) {
            if (this == &m) {
                return *this;
            }
            BitPairSet tmpSetFlags = m.setFlags;
            std::unique_ptr<Key[]> tmpKeys = new Key[m.keySize];
            values = new Value[m.keySize];
            keys = tmpKeys.release();
            std::swap(setFlags, tmpSetFlags);
            keySize = m.keySize;
            mapSize = m.mapSize;
            loadFactor = m.loadFactor;
            growthFactor = m.growthFactor;
            memcpy(keys, m.keys, sizeof(Key) * keySize);
            memcpy(values, m.values, sizeof(Value) * keySize);
            return *this;
        }

        OpenHashMapTC & operator=(OpenHashMapTC && m) noexcept {
            if (this == &m) {
                return *this;
            }
            keys = m.keys;
            values = m.values;
            keySize = m.keySize;
            mapSize = m.mapSize;
            loadFactor = m.loadFactor;
            growthFactor = m.growthFactor;
            setFlags = std::move(m.setFlags);
            m.keys = nullptr;
            m.values = nullptr;
            m.keySize = 0;
            m.mapSize = 0;
            return *this;
        }

        explicit OpenHashMapTC(std::string filename) {

        }

        Value &operator[](Key const &key) {
            size_t hash;
            size_t idx;
            size_t startIdx;
            size_t firstUnsetIdx = -1;
            bool isFirstUnsetIdxSet = false;
            if (keySize > 0) {
                hash = hashFunction(key);
                idx = hash % keySize;
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
                    idx = idx == keySize ? 0 : idx;
                    if (startIdx == idx) break;
                }
            }
            if (mapSize >= keySize * loadFactor) {
                rehash();
                hash = hashFunction(key);
                idx = hash % keySize;
                startIdx = idx;
                while (setFlags.isFirstSet(idx)) {
                    ++idx;
                    idx = idx == keySize ? 0 : idx;
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
            if (keySize > 0) {
                hash = hashFunction(key);
                idx = hash % keySize;
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
                    idx = idx == keySize ? 0 : idx;
                    if (startIdx == idx) break;
                }
            }
            if (doesContain) {
                return;
            }
            if (mapSize >= keySize * loadFactor) {
                rehash();
                hash = hashFunction(key);
                idx = hash % keySize;
                startIdx = idx;
                while (setFlags.isFirstSet(idx)) {
                    ++idx;
                    idx = idx == keySize ? 0 : idx;
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
            if (keySize == 0) {
                return false;
            }
            size_t hash = hashFunction(key);
            size_t idx = hash % keySize;
            size_t startIdx = idx;
            for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                if (isSet && keys[idx] == key) {
                    return true;
                }
                if (wasSet && keys[idx] == key) {
                    return false;
                }
                ++idx;
                idx = idx == keySize ? 0 : idx;
                if (startIdx == idx) break;
            }
            return false;
        }

        void erase(Key const &key) {
            size_t hash = hashFunction(key);
            size_t idx = hash % keySize;
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
                idx = idx == keySize ? 0 : idx;
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
                if (idx == container->keySize) {
                    return *this;
                }
                ++idx;
                while (idx < container->keySize && !container->setFlags.isFirstSet(idx)) {
                    ++idx;
                }
                return *this;
            }
        };

        iterator begin() {
            if (mapSize == 0) {
                return iterator(keySize, this);
            }
            size_t idx = 0;
            while (idx < keySize && !setFlags.isFirstSet(idx)) {
                ++idx;
            }
            return iterator(idx, this);
        }

        iterator end() {
            return iterator(keySize, this);
        }

        size_t size() const {
            return mapSize;
        }


        void write(std::string filename) {
            std::ofstream ofs(filename, std::ios::binary);
            if (ofs.fail()) {
                std::cout << "Couldn't open " << filename << " for writing in OpenHashMapTC::write\n";
                exit(1);
            }
            ofs.write(static_cast<char*>(static_cast<void*>(&mapSize)), 8);
            ofs.write(static_cast<char*>(static_cast<void*>(&keySize)), 8);
            ofs.write(static_cast<char*>(static_cast<void*>(&loadFactor)), 8);
            ofs.write(static_cast<char*>(static_cast<void*>(&growthFactor)), 8);
            ofs.write(static_cast<char*>(static_cast<void*>(keys)), sizeof(Key) * keySize);
            ofs.write(static_cast<char*>(static_cast<void*>(values)), sizeof(Value) * keySize);
            setFlags.write(ofs);
        }
    };
}

#endif

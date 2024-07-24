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

#ifndef GRADY_LIB_OPENHASHMAP_HPP
#define GRADY_LIB_OPENHASHMAP_HPP

#include<fstream>
#include<string>
#include<type_traits>
#include<vector>

#include<BitPairSet.hpp>

namespace gradylib {

    template<typename Key, typename Value>
    requires std::is_default_constructible_v<Key> && std::is_default_constructible_v<Value>
    class OpenHashMap {

        std::vector<Key> keys;
        std::vector<Value> values;
        BitPairSet setFlags;
        double loadFactor = 0.8;
        double growthFactor = 1.2;
        size_t mapSize = 0;

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
            if (mapSize > 0) {
                for (size_t i = 0; i < keys.size(); ++i) {
                    if (!setFlags.isFirstSet(i)) {
                        continue;
                    }
                    Key const &k = keys[i];
                    size_t hash = std::hash<Key>{}(k);
                    size_t idx = hash % newKeys.size();
                    while (newSetFlags.isFirstSet(idx)) {
                        ++idx;
                        idx = idx == newKeys.size() ? 0 : idx;
                    }
                    newSetFlags.setBoth(idx);
                    newKeys[idx] = k;
                    newValues[idx] = values[i];
                }
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
                hash = std::hash<Key>{}(key);
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
                hash = std::hash<Key>{}(key);
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
            size_t hash = std::hash<Key>{}(key);
            size_t idx = hash % keys.size();
            bool doesContain = false;
            size_t firstUnsetIdx = -1;
            bool isFirstUnsetIdxSet = false;
            size_t startIdx = idx;
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
            if (doesContain) {
                return;
            }
            if (mapSize >= keys.size() * loadFactor) {
                rehash();
                hash = std::hash<Key>{}(key);
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
            size_t hash = std::hash<Key>{}(key);
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
            size_t hash = std::hash<Key>{}(key);
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
            OpenHashMap *container;
        public:
            iterator(size_t idx, OpenHashMap *container)
                    : idx(idx), container(container) {
            }

            bool operator==(iterator const &other) const {
                return idx == other.idx;
            }

            bool operator!=(iterator const &other) const {
                return idx != other.idx;
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

        template<typename IndexType>
        friend void writeMappable(std::string filename, OpenHashMap<std::string, IndexType> const & m);
    };


    template<typename IndexType>
    void writeMappable(std::string filename, OpenHashMap<std::string, IndexType> const & m) {
        std::ofstream ofs(filename);
        if (ofs.fail()) {
            std::cout << "Couldn't open file " << filename << " in writeMappable.\n";
            exit(1);
        }
        size_t mapSize = m.mapSize;
        ofs.write(static_cast<char*>(static_cast<void*>(&mapSize)), 8);
        size_t keySize = m.keys.size();
        ofs.write(static_cast<char*>(static_cast<void*>(&keySize)), 8);
        size_t valueOffset = 0;
        auto const valueOffsetWritePos = ofs.tellp();
        ofs.write(static_cast<char*>(static_cast<void*>(&valueOffset)), 8);
        size_t bitPairSetOffset = 0;
        auto const bitPairSetOffsetWritePos = ofs.tellp();
        ofs.write(static_cast<char*>(static_cast<void*>(&bitPairSetOffset)), 8);
        size_t keyOffset = 0;
        for (size_t i = 0; i < m.keys.size(); ++i) {
            ofs.write(static_cast<char*>(static_cast<void*>(&keyOffset)), 8);
            int32_t strLen = m.keys[i].length();
            int32_t strSize = 4 + strLen + (4 - strLen % 4);
            keyOffset += strSize;
        }
        std::vector<char> pad(4, 0);
        for (size_t i = 0; i < m.keys.size(); ++i) {
            int32_t len = m.keys[i].length();
            ofs.write(static_cast<char*>(static_cast<void*>(&len)), 4);
            ofs.write(m.keys[i].data(), len);
            ofs.write(pad.data(), 4 - len % 4);
        }

        valueOffset = ofs.tellp();
        ofs.write(static_cast<char*>(const_cast<void*>(static_cast<void const *>(m.values.data()))), sizeof(IndexType) * keySize);

        bitPairSetOffset = ofs.tellp();
        m.setFlags.write(ofs);

        ofs.seekp(valueOffsetWritePos, std::ios::beg);
        ofs.write(static_cast<char*>(static_cast<void*>(&valueOffset)), 8);
        ofs.seekp(bitPairSetOffsetWritePos, std::ios::beg);
        ofs.write(static_cast<char*>(static_cast<void*>(&bitPairSetOffset)), 8);
    }
}

#endif //GRADY_LIB_OPENHASHMAP_HPP

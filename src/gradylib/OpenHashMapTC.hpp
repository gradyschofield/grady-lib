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

#include<fcntl.h>
#include<sys/mman.h>
#include<unistd.h>

#include<filesystem>
#include<fstream>
#include<memory>
#include<type_traits>
#include<vector>

#include"BitPairSet.hpp"

namespace gradylib {

    template<typename Key, typename Value, template<typename> typename HashFunction = std::hash>
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
        int fd = -1;
        void const * memoryMapping = nullptr;
        size_t mappingSize = 0;
        HashFunction<Key> hashFunction = HashFunction<Key>{};
        BitPairSet setFlags;
        bool readOnly = false;

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

        void setFromMemoryMapping(void const * startPtr) {
            readOnly = true;
            std::byte const *ptr = static_cast<std::byte const *>(startPtr);
            std::byte const *base = ptr;
            mapSize = *static_cast<size_t const *>(static_cast<void const *>(ptr));
            ptr += 8;
            keySize = *static_cast<size_t const *>(static_cast<void const *>(ptr));
            ptr += 8;
            loadFactor = *static_cast<double const *>(static_cast<void const *>(ptr));
            ptr += 8;
            growthFactor = *static_cast<double const *>(static_cast<void const *>(ptr));
            ptr += 8;
            size_t valueOffset = *static_cast<size_t const *>(static_cast<void const *>(ptr));
            ptr += 8;
            size_t bitPairSetOffset = *static_cast<size_t const *>(static_cast<void const *>(ptr));
            ptr += 8;
            keys = static_cast<Key *>(const_cast<void *>(static_cast<void const *>(ptr)));
            ptr = base + valueOffset;
            values = static_cast<Value *>(const_cast<void *>(static_cast<void const *>(ptr)));
            ptr = base + bitPairSetOffset;
            setFlags = BitPairSet(ptr);
        }

    public:
        typedef Key key_type;
        typedef Value mapped_type;

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
            loadFactor(m.loadFactor), growthFactor(m.growthFactor), fd(m.fd),
            memoryMapping(m.memoryMapping), mappingSize(m.mappingSize), setFlags(std::move(m.setFlags)),
            readOnly(m.readOnly)
        {
            m.keys = nullptr;
            m.values = nullptr;
            m.keySize = 0;
            m.mapSize = 0;
            m.fd = -1;
            m.memoryMapping = nullptr;
            m.mappingSize = 0;
            m.readOnly = false;
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
            fd = m.fd;
            memoryMapping = m.memoryMapping;
            mappingSize = m.mappingSize;
            setFlags = std::move(m.setFlags);
            readOnly = m.readOnly;
            m.keys = nullptr;
            m.values = nullptr;
            m.keySize = 0;
            m.mapSize = 0;
            m.fd = -1;
            m.memoryMapping = nullptr;
            m.mappingSize = 0;
            m.readOnly = false;
            return *this;
        }

        ~OpenHashMapTC() {
            if (memoryMapping) {
                munmap(const_cast<void *>(memoryMapping), mappingSize);
                close(fd);
            } else if (!readOnly) {
                delete [] keys;
                delete [] values;
            }
        }

        explicit OpenHashMapTC(std::string filename) {
            fd = open(filename.c_str(), O_RDONLY);
            if (fd < 0) {
                std::cout << "Error opening file " << filename << "\n";
                exit(1);
            }
            mappingSize = std::filesystem::file_size(filename);
            memoryMapping = mmap(0, mappingSize, PROT_READ, MAP_SHARED, fd, 0);
            if (memoryMapping == MAP_FAILED) {
                std::cout << "memory map failed: " << strerror(errno) << "\n";
                exit(1);
            }
            setFromMemoryMapping(memoryMapping);
        }

        explicit OpenHashMapTC(char const * startPtr)
            : OpenHashMapTC(std::string(startPtr))
        {
        }

        explicit OpenHashMapTC(void const * startPtr) {
            setFromMemoryMapping(startPtr);
        }

        Value &operator[](Key const &key) {
            if (readOnly) {
                std::cout << "Cannot modify mmap\n";
                exit(1);
            }
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

        void put(Key const &key, Value const &value) {
            if (readOnly) {
                std::cout << "Cannot modify mmap\n";
                exit(1);
            }
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

        Value const & at(Key const &key) const {
            if (keySize == 0) {
                std::cout << "key not found in map\n";
                exit(1);
            }
            size_t hash = hashFunction(key);
            size_t idx = hash % keySize;
            size_t startIdx = idx;
            for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                if (isSet && keys[idx] == key) {
                    return values[idx];
                }
                if (wasSet && keys[idx] == key) {
                    std::cout << "key not found in map\n";
                    exit(1);
                }
                ++idx;
                idx = idx == keySize ? 0 : idx;
                if (startIdx == idx) break;
            }
            std::cout << "key not found in map\n";
            exit(1);
        }

        bool contains(Key const &key) const {
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
            if (readOnly) {
                std::cout << "Cannot modify mmap\n";
                exit(1);
            }
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
            if (readOnly) {
                std::cout << "Cannot modify mmap\n";
                exit(1);
            }
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

        class const_iterator {
            size_t idx;
            OpenHashMapTC const *container;
        public:
            const_iterator(size_t idx, OpenHashMapTC const *container)
                    : idx(idx), container(container) {
            }

            bool operator==(const_iterator const &other) const {
                return idx == other.idx && container == other.container;
            }

            bool operator!=(const_iterator const &other) const {
                return idx != other.idx || container != other.container;
            }

            const std::pair<Key const &, Value const &> operator*() const {
                return {container->keys[idx], container->values[idx]};
            }

            Key const &key() const {
                return container->keys[idx];
            }

            Value const & value() const {
                return container->values[idx];
            }

            const_iterator &operator++() {
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

        const_iterator begin() const {
            if (mapSize == 0) {
                return const_iterator(keySize, this);
            }
            size_t idx = 0;
            while (idx < keySize && !setFlags.isFirstSet(idx)) {
                ++idx;
            }
            return const_iterator(idx, this);
        }

        const_iterator end() const {
            return const_iterator(keySize, this);
        }

        size_t size() const {
            return mapSize;
        }

        void clear() {
            if (readOnly) {
                std::cout << "Can't clear a readonly set\n";
                exit(1);
            }
            setFlags.clear();
            mapSize = 0;
        }

        void write(std::string filename, int alignment = alignof(void*)) const {
            std::ofstream ofs(filename, std::ios::binary);
            if (ofs.fail()) {
                std::cout << "Couldn't open " << filename << " for writing in OpenHashMapTC::write\n";
                exit(1);
            }
            write(ofs, alignment);
        }

        void write(std::ofstream & ofs, int alignment = alignof(void*)) const {
            size_t startFileOffset = ofs.tellp();
            size_t t;
            t = mapSize;
            ofs.write(static_cast<char*>(static_cast<void *>(&t)), 8);
            t = keySize;
            ofs.write(static_cast<char*>(static_cast<void *>(&t)), 8);
            double d = loadFactor;
            ofs.write(static_cast<char*>(static_cast<void *>(&d)), 8);
            d = growthFactor;
            ofs.write(static_cast<char*>(static_cast<void *>(&d)), 8);
            size_t valuesOffset = 0;
            size_t bitPairSetOffset = 0;
            auto valueOffsetPos = ofs.tellp();
            ofs.write(static_cast<char*>(static_cast<void*>(&valuesOffset)), 8);
            auto bitPairSetOffsetPos = ofs.tellp();
            ofs.write(static_cast<char*>(static_cast<void*>(&bitPairSetOffset)), 8);
            ofs.write(static_cast<char*>(static_cast<void*>(keys)), sizeof(Key) * keySize);
            int valuePad = alignment - ofs.tellp() % alignment;
            for (int i = 0; i < valuePad; ++i) {
                char t = 0;
                ofs.write(&t, 1);
            }
            valuesOffset = static_cast<size_t>(ofs.tellp()) - startFileOffset;
            ofs.write(static_cast<char*>(static_cast<void*>(values)), sizeof(Value) * keySize);
            int bitPairSetOffsetPad = alignment - ofs.tellp() % alignment;
            for (int i = 0; i < bitPairSetOffsetPad; ++i) {
                char t = 0;
                ofs.write(&t, 1);
            }
            bitPairSetOffset = static_cast<size_t>(ofs.tellp()) - startFileOffset;
            setFlags.write(ofs);

            ofs.seekp(valueOffsetPos);
            ofs.write(static_cast<char*>(static_cast<void*>(&valuesOffset)), 8);

            ofs.seekp(bitPairSetOffsetPos);
            ofs.write(static_cast<char*>(static_cast<void*>(&bitPairSetOffset)), 8);
        }
    };
}

#endif

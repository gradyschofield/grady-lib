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

#ifndef GRADY_LIB_MMAPI2SOPENHASHMAP_HPP
#define GRADY_LIB_MMAPI2SOPENHASHMAP_HPP

#include<fcntl.h>
#include<sys/mman.h>
#include<unistd.h>

#include<filesystem>
#include<iostream>
#include<string>
#include<string_view>

#include"BitPairSet.hpp"
#include"OpenHashMap.hpp"

namespace gradylib {

    /*
     * This is a readonly data structure for quickly loading an OpenHashMap<IndexType, std::string> from disk
     */
    template<typename IndexType, template<typename> typename HashFunction = std::hash>
    class MMapI2SOpenHashMap {
        size_t const *valueOffsets = nullptr;
        IndexType const * keys = nullptr;
        void const * values = nullptr;
        BitPairSet setFlags;
        size_t mapSize = 0;
        size_t keySize = 0;
        int fd = -1;
        void * memoryMapping = nullptr;
        size_t mappingSize = 0;
        HashFunction<IndexType> hashFunction = HashFunction<IndexType>{};

        std::string_view getValue(std::byte const * ptr) const {
            int32_t len = *static_cast<int32_t const *>(static_cast<void const *>(ptr));
            char const * p = static_cast<char const *>(static_cast<void const *>(ptr + 4));
            return std::string_view(p, len);
        }

    public:
        typedef IndexType key_type;
        typedef std::string mapped_type;

        MMapI2SOpenHashMap() = default;

        MMapI2SOpenHashMap(MMapI2SOpenHashMap const &) = delete;

        MMapI2SOpenHashMap &operator=(MMapI2SOpenHashMap const &) = delete;

        MMapI2SOpenHashMap(MMapI2SOpenHashMap &&m) noexcept
                : setFlags(std::move(m.setFlags)) {
            valueOffsets = m.valueOffsets;
            keys = m.keys;
            values = m.values;
            mapSize = m.mapSize;
            keySize = m.keySize;
            fd = m.fd;
            memoryMapping = m.memoryMapping;
            mappingSize = m.mappingSize;
            m.keys = nullptr;
            m.values = nullptr;
            m.mapSize = 0;
            m.keySize = 0;
            m.fd = -1;
            m.memoryMapping = nullptr;
            m.mappingSize = 0;
        }

        MMapI2SOpenHashMap &operator=(MMapI2SOpenHashMap &&m) noexcept {
            valueOffsets = m.valueOffsets;
            keys = m.keys;
            values = m.values;
            setFlags = std::move(m.setFlags);
            mapSize = m.mapSize;
            keySize = m.keySize;
            fd = m.fd;
            memoryMapping = m.memoryMapping;
            mappingSize = m.mappingSize;
            m.keys = nullptr;
            m.values = nullptr;
            m.mapSize = 0;
            m.keySize = 0;
            m.fd = -1;
            m.memoryMapping = nullptr;
            m.mappingSize = 0;
            return *this;
        }

        explicit MMapI2SOpenHashMap(std::string filename) {
            fd = open(filename.c_str(), O_RDONLY);
            if (fd < 0) {
                std::ostringstream sstr;
                sstr << "Couldn't open " << filename << " in MMapI2SOpenHashMap";
                throw gradylibMakeException(sstr.str());
            }
            mappingSize = std::filesystem::file_size(filename);
            memoryMapping = mmap(0, mappingSize, PROT_READ, MAP_SHARED, fd, 0);
            if (memoryMapping == MAP_FAILED) {
                close(fd);
                memoryMapping = nullptr;
                std::ostringstream sstr;
                sstr << "mmap failed " << strerror(errno);
                throw gradylibMakeException(sstr.str());
            }
            std::byte *ptr = static_cast<std::byte *>(memoryMapping);
            std::byte *base = ptr;
            mapSize = *static_cast<size_t *>(static_cast<void *>(ptr));
            ptr += 8;
            keySize = *static_cast<size_t *>(static_cast<void *>(ptr));
            ptr += 8;
            size_t bitPairSetOffset = *static_cast<size_t *>(static_cast<void *>(ptr));
            ptr += 8;
            valueOffsets = static_cast<size_t*>(static_cast<void *>(ptr));
            ptr += 8 * keySize;
            keys = static_cast<IndexType *>(static_cast<void *>(ptr));
            ptr += sizeof(IndexType) * keySize;
            values = static_cast<void *>(ptr);
            setFlags = BitPairSet(static_cast<void *>(base + bitPairSetOffset));
        }

        ~MMapI2SOpenHashMap() {
            if (memoryMapping) {
                munmap(memoryMapping, mappingSize);
                close(fd);
            }
        }

        std::string_view operator[](IndexType key) const {
            if (keySize == 0) {
                std::ostringstream sstr;
                sstr << key << " not found in map";
                throw gradylibMakeException(sstr.str());
            }
            size_t hash;
            size_t idx;
            size_t startIdx;

            hash = hashFunction(key);
            idx = hash % keySize;
            startIdx = idx;
            for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                IndexType k = keys[idx];
                if (isSet && k == key) {
                    std::byte const *valuePtr = static_cast<std::byte const *>(values) + valueOffsets[idx];
                    return getValue(valuePtr);
                }
                if (wasSet && k == key) {
                    std::ostringstream sstr;
                    sstr << key << " not found in map";
                    throw gradylibMakeException(sstr.str());
                }
                ++idx;
                idx = idx == keySize ? 0 : idx;
                if (startIdx == idx) {
                    std::ostringstream sstr;
                    sstr << key << " not found in map";
                    throw gradylibMakeException(sstr.str());
                }
            }
            std::ostringstream sstr;
            sstr << key << " not found in map";
            throw gradylibMakeException(sstr.str());
        }

        bool contains(IndexType key) const {
            if (keySize == 0) {
                return false;
            }
            size_t hash = hashFunction(key);
            size_t idx = hash % keySize;
            size_t startIdx = idx;
            for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                IndexType k = keys[idx];
                if (isSet && k == key) {
                    return true;
                }
                if (wasSet && k == key) {
                    return false;
                }
                ++idx;
                idx = idx == keySize ? 0 : idx;
                if (startIdx == idx) break;
            }
            return false;
        }

        size_t size() const {
            return mapSize;
        }

        class const_iterator {
            size_t idx;
            MMapI2SOpenHashMap const * container;
        public:
            const_iterator(size_t idx, MMapI2SOpenHashMap const * container)
                    : idx(idx), container(container) {
            }

            bool operator==(const_iterator const &other) const {
                return idx == other.idx && container == other.container;
            }

            bool operator!=(const_iterator const &other) const {
                return idx != other.idx || container != other.container;
            }

            std::pair<IndexType const &, std::string_view const> operator*() const {
                std::byte const *valuePtr = static_cast<std::byte const *>(container->values) + container->valueOffsets[idx];
                return {container->keys[idx], container->getValue(valuePtr)};
            }

            IndexType const & key() const {
                return container->keys[idx];
            }

            std::string_view const value() const {
                std::byte const *valuePtr = static_cast<std::byte const *>(container->values) + container->valueOffsets[idx];
                return container->getValue(valuePtr);
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

        OpenHashMap<IndexType, std::string> clone() const {
            OpenHashMap<IndexType, std::string> ret;
            ret.reserve(size());
            for (auto && [idx, sview] : *this) {
                ret.put(idx, std::string(sview));
            }
            return ret;
        }
    };
}

#endif

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

#ifndef GRADY_LIB_MMAPS2IOPENHASHMAP_HPP
#define GRADY_LIB_MMAPS2IOPENHASHMAP_HPP

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
     * This is a readonly data structure for quickly loading an OpenHashMap<std::string, IndexType> from disk
     */
    template<typename IndexType>
    class MMapS2IOpenHashMap {
        int64_t const *keyOffsets = nullptr;
        void const * keys = nullptr;
        IndexType const * values = nullptr;
        BitPairSet setFlags;
        size_t mapSize = 0;
        size_t keySize = 0;
        int fd = -1;
        void * memoryMapping = nullptr;
        size_t mappingSize = 0;
        static inline void* (*mmapFunc)(void *, size_t, int, int, int, off_t) = mmap;

        std::string_view getKey(std::byte const * ptr) const {
            int32_t len = *static_cast<int32_t const *>(static_cast<void const *>(ptr));
            char const * p = static_cast<char const *>(static_cast<void const *>(ptr + 4));
            return std::string_view(p, len);
        }

        std::byte const * incKeyPtr(std::byte const * keyPtr) const {
            int32_t len = *static_cast<int32_t const *>(static_cast<void const *>(keyPtr));
            return keyPtr + 4 + len + (4 - len%4);
        }

    public:
        typedef std::string key_type;
        typedef IndexType mapped_type;

        MMapS2IOpenHashMap() = default;

        MMapS2IOpenHashMap(MMapS2IOpenHashMap const &) = delete;

        MMapS2IOpenHashMap &operator=(MMapS2IOpenHashMap const &) = delete;

        MMapS2IOpenHashMap(MMapS2IOpenHashMap &&m) noexcept
                : setFlags(std::move(m.setFlags)) {
            keyOffsets = m.keyOffsets;
            keys = m.keys;
            values = m.values;
            mapSize = m.mapSize;
            keySize = m.keySize;
            fd = m.fd;
            memoryMapping = m.memoryMapping;
            mappingSize = m.mappingSize;
            m.keyOffsets = nullptr;
            m.keys = nullptr;
            m.values = nullptr;
            m.mapSize = 0;
            m.keySize = 0;
            m.fd = -1;
            m.memoryMapping = nullptr;
            m.mappingSize = 0;
        }

        MMapS2IOpenHashMap &operator=(MMapS2IOpenHashMap &&m) noexcept {
            keyOffsets = m.keyOffsets;
            keys = m.keys;
            values = m.values;
            setFlags = std::move(m.setFlags);
            mapSize = m.mapSize;
            keySize = m.keySize;
            fd = m.fd;
            memoryMapping = m.memoryMapping;
            mappingSize = m.mappingSize;
            m.keyOffsets = nullptr;
            m.keys = nullptr;
            m.values = nullptr;
            m.mapSize = 0;
            m.keySize = 0;
            m.fd = -1;
            m.memoryMapping = nullptr;
            m.mappingSize = 0;
            return *this;
        }

        explicit MMapS2IOpenHashMap(std::filesystem::path filename) {
            fd = open(filename.c_str(), O_RDONLY);
            if (fd < 0) {
                std::ostringstream sstr;
                sstr << "Couldn't open " << filename << " in MMapS2IOpenHashMap";
                throw gradylibMakeException(sstr.str());
            }
            mappingSize = std::filesystem::file_size(filename);
            memoryMapping = mmapFunc(0, mappingSize, PROT_READ, MAP_SHARED, fd, 0);
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
            size_t valueOffset = *static_cast<size_t *>(static_cast<void *>(ptr));
            ptr += 8;
            size_t bitPairSetOffset = *static_cast<size_t *>(static_cast<void *>(ptr));
            ptr += 8;
            keyOffsets = static_cast<int64_t *>(static_cast<void *>(ptr));
            ptr += 8 * keySize;
            keys = static_cast<void *>(static_cast<void *>(ptr));
            values = static_cast<IndexType *>(static_cast<void *>(base + valueOffset));
            setFlags = BitPairSet(static_cast<void *>(base + bitPairSetOffset));
        }

        ~MMapS2IOpenHashMap() {
            if (memoryMapping) {
                munmap(memoryMapping, mappingSize);
                close(fd);
            }
        }

        IndexType operator[](std::string_view key) const {
            if (keySize == 0) {
                std::ostringstream sstr;
                sstr << key << " not found in map";
                throw gradylibMakeException(sstr.str());
            }
            size_t hash;
            size_t idx;
            size_t startIdx;

            hash = std::hash<std::string_view>{}(key);
            idx = hash % keySize;
            startIdx = idx;
            std::byte const *keyPtr = static_cast<std::byte const *>(keys) + keyOffsets[idx];
            for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                std::string_view k = getKey(keyPtr);
                if (isSet && k == key) {
                    return values[idx];
                }
                if (wasSet && k == key) {
                    std::ostringstream sstr;
                    sstr << key << " not found in map";
                    throw gradylibMakeException(sstr.str());
                }
                keyPtr = incKeyPtr(keyPtr);
                ++idx;
                if (idx == keySize) {
                    idx = 0;
                    keyPtr = static_cast<std::byte const *>(keys);
                }
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

        bool contains(std::string_view key) const {
            if (keySize == 0) {
                return false;
            }
            size_t hash = std::hash<std::string_view>{}(key);
            size_t idx = hash % keySize;
            size_t startIdx = idx;
            std::byte const *keyPtr = static_cast<std::byte const *>(keys) + keyOffsets[idx];
            for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                std::string_view k = getKey(keyPtr);
                if (isSet && k == key) {
                    return true;
                }
                if (wasSet && k == key) {
                    return false;
                }
                keyPtr = incKeyPtr(keyPtr);
                ++idx;
                if (idx == keySize) {
                    idx = 0;
                    keyPtr = static_cast<std::byte const *>(keys);
                }
                if (startIdx == idx) break;
            }
            return false;
        }

        size_t size() const {
            return mapSize;
        }

        class const_iterator {
            size_t idx;
            MMapS2IOpenHashMap const * container;
        public:
            const_iterator(size_t idx, MMapS2IOpenHashMap const * container)
                    : idx(idx), container(container) {
            }

            bool operator==(const_iterator const &other) const {
                return idx == other.idx && container == other.container;
            }

            bool operator!=(const_iterator const &other) const {
                return idx != other.idx || container != other.container;
            }

            std::pair<std::string_view const, IndexType const &> operator*() const {
                std::byte const *keyPtr = static_cast<std::byte const *>(container->keys) + container->keyOffsets[idx];
                return {container->getKey(keyPtr), container->values[idx]};
            }

            std::string_view const key() const {
                std::byte const *keyPtr = static_cast<std::byte const *>(container->keys) + container->keyOffsets[idx];
                return container->getKey(keyPtr);
            }

            IndexType const & value() {
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

        OpenHashMap<std::string, IndexType> clone() const {
            OpenHashMap<std::string, IndexType> ret;
            ret.reserve(size());
            for (auto && [sview, idx] : *this) {
                ret.put(sview, idx);
            }
            return ret;
        }

        template<typename>
        friend void GRADY_LIB_MOCK_MMapS2IOpenHashMap_MMAP();
    };

    template<typename IndexType>
    void GRADY_LIB_MOCK_MMapS2IOpenHashMap_MMAP() {
        MMapS2IOpenHashMap<IndexType>::mmapFunc = [](void *, size_t, int, int, int, off_t) -> void *{
            return MAP_FAILED;
        };
    }
}

#endif

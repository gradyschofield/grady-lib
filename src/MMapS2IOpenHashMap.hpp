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

#include<fcntl.h>
#include<sys/mman.h>
#include<unistd.h>

#include<filesystem>
#include<iostream>
#include<string>
#include<string_view>

#include<BitPairSet.hpp>

namespace gradylib {
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

    public:
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

        explicit MMapS2IOpenHashMap(std::string filename) {
            fd = open(filename.c_str(), O_RDONLY);
            if (fd < 0) {
                std::cout << "Couldn't open " << filename << " in MMapS2IOpenHashMap\n";
                exit(1);
            }
            mappingSize = std::filesystem::file_size(filename);
            memoryMapping = mmap(0, mappingSize, PROT_READ, MAP_SHARED, fd, 0);
            if (memoryMapping == MAP_FAILED) {
                std::cout << "mmap failed " << strerror(errno) << "\n";
                exit(1);
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

        std::string_view getKey(std::byte const * ptr) const {
            int32_t len = *static_cast<int32_t const *>(static_cast<void const *>(ptr));
            char const * p = static_cast<char const *>(static_cast<void const *>(ptr + 4));
            return std::string_view(p, len);
        }

        std::byte const * incKeyPtr(std::byte const * keyPtr) const {
            int32_t len = *static_cast<int32_t const *>(static_cast<void const *>(keyPtr));
            return keyPtr + 4 + len + (4 - len%4);
        }

        IndexType operator[](std::string_view key) const {
            if (keySize == 0) {
                std::cout << key << " not found in map\n";
                exit(1);
            }
            size_t hash;
            size_t idx;
            size_t startIdx;
            size_t firstUnsetIdx = -1;
            bool isFirstUnsetIdxSet = false;

            hash = std::hash<std::string_view>{}(key);
            idx = hash % keySize;
            startIdx = idx;
            std::byte const *keyPtr = static_cast<std::byte const *>(keys) + keyOffsets[idx];
            for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                if (!isFirstUnsetIdxSet && !isSet) {
                    firstUnsetIdx = idx;
                    isFirstUnsetIdxSet = true;
                }
                std::string_view k = getKey(keyPtr);
                if (isSet && k == key) {
                    return values[idx];
                }
                if (wasSet && k == key) {
                    std::cout << key << " not found in map\n";
                    exit(1);
                }
                keyPtr = incKeyPtr(keyPtr);
                ++idx;
                if (idx == keySize) {
                    idx = 0;
                    keyPtr = static_cast<std::byte const *>(keys);
                }
                if (startIdx == idx) {
                    std::cout << key << " not found in map\n";
                    exit(1);
                }
            }
            std::cout << key << " not found in map\n";
            exit(1);
        }

        bool contains(std::string_view key) {
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

        OpenHashMap<std::string, IndexType> clone() const {

        }
    };
}
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

#ifndef GRADY_LIB_MMAPOPENHASHMAPVIEWABLE_HPP
#define GRADY_LIB_MMAPOPENHASHMAPVIEWABLE_HPP

#include<fcntl.h>
#include<sys/mman.h>
#include<unistd.h>

#include<functional>
#include<fstream>
#include<string>

#include"OpenHashMap.hpp"
#include"OpenHashMapTC.hpp"

namespace gradylib {

    template<typename ValueType>
    concept serializable_global = requires (std::ofstream & os, ValueType const & v) {
        serialize(os, v);
    };

    template<typename ValueType>
    concept serializable_method = requires (std::ofstream & os, ValueType const & v) {
        { v.serialize(os) } -> std::same_as<void>;
    };

    template<typename ValueType>
    concept viewable_global = requires (std::byte const * ptr) {
        makeView<ValueType>(ptr);
    };

    template<typename ValueType>
    concept viewable_method = requires (std::byte const * ptr) {
        ValueType::makeView(ptr);
    };

    template<typename Key, typename Value, template<typename> typename HashFunction = std::hash>
    requires (serializable_global<Value> || serializable_method<Value>) &&
             (viewable_global<Value> || viewable_method<Value>) &&
             std::is_trivially_copyable_v<Key> &&
             std::is_default_constructible_v<Key>
    class MMapViewableOpenHashMap {
        OpenHashMapTC<Key, int64_t, HashFunction> valueOffsets;
        std::byte const * valuePtr = nullptr;
        int fd = -1;
        size_t mappingSize = 0;
        void const * memoryMapping = nullptr;
        static inline void* (*mmapFunc)(void *, size_t, int, int, int, off_t) = mmap;

    public:

        MMapViewableOpenHashMap(std::filesystem::path filename) {
            fd = open(filename.c_str(), O_RDONLY);
            if (fd < 0) {
                std::ostringstream sstr;
                sstr << "Error opening file " << filename;
                throw gradylibMakeException(sstr.str());
            }

            mappingSize = std::filesystem::file_size(filename);
            memoryMapping = mmapFunc(0, mappingSize, PROT_READ, MAP_SHARED, fd, 0);
            if (memoryMapping == MAP_FAILED) {
                close(fd);
                memoryMapping = nullptr;
                std::ostringstream sstr;
                sstr << "memory map failed: " << strerror(errno);
                throw gradylibMakeException(sstr.str());
            }

            std::byte const * base = static_cast<std::byte const *>(memoryMapping);
            size_t mapOffset = *static_cast<size_t const *>(static_cast<void const *>(base));
            valuePtr = base + 8;
            valueOffsets = OpenHashMapTC<Key, int64_t, HashFunction>(static_cast<void const *>(base + mapOffset));
        }

        bool contains(Key const & key) const {
            return valueOffsets.contains(key);
        }

        size_t size() const {
            return valueOffsets.size();
        }

        decltype(auto) at(Key const & key) const {
            if (!valueOffsets.contains(key)) {
                std::ostringstream sstr;
                sstr << "Map doesn't contain key";
                throw gradylibMakeException(sstr.str());
            }
            int64_t off = valueOffsets.at(key);
            std::byte const * ptr = valuePtr + valueOffsets.at(key);
            if constexpr (viewable_global<Value>) {
                return makeView<Value>(ptr);
            } else {
                return Value::makeView(ptr);
            }
        }

        ~MMapViewableOpenHashMap() {
            if (memoryMapping) {
                munmap(const_cast<void *>(memoryMapping), mappingSize);
                close(fd);
            }
        }

        class const_iterator {
            OpenHashMapTC<Key, int64_t, HashFunction>::const_iterator iter;
            MMapViewableOpenHashMap const *container;

        public:
            const_iterator(OpenHashMapTC<Key, int64_t, HashFunction>::const_iterator iter, MMapViewableOpenHashMap const * container)
                    : iter(iter), container(container) {
            }

            bool operator==(const_iterator const &other) const {
                return iter == other.iter && container == other.container;
            }

            bool operator!=(const_iterator const &other) const {
                return iter != other.iter || container != other.container;
            }

            std::pair<Key const &, decltype(container->at(std::declval<Key const &>()))> operator*() {
                return {iter.key(), container->at(iter.key())};
            }

            Key const &key() const {
                return iter.key();
            }

            decltype(auto) value() {
                return container->at(iter.key());
            }

            const_iterator &operator++() {
                if (iter == container->valueOffsets.end()) {
                    return *this;
                }
                ++iter;
                return *this;
            }
        };

        const_iterator begin() const {
            return const_iterator(valueOffsets.begin(), this);
        }

        const_iterator end() const {
            return const_iterator(valueOffsets.end(), this);
        }

        class Builder {
            OpenHashMap<Key, Value, HashFunction> m;

        public:

            template<typename KeyType, typename ValueType>
            requires (serializable_global<ValueType> || serializable_method<ValueType>) &&
                    (viewable_global<ValueType> || viewable_method<ValueType>) &&
                    (std::is_same_v<std::remove_cvref_t<KeyType>, Key> || std::is_convertible_v<std::remove_cvref_t<KeyType>, Key>) &&
                    std::is_same_v<std::remove_cvref_t<ValueType>, Value>
            void put(KeyType && key, ValueType && value) {
                m.put(std::forward<KeyType>(key), std::forward<ValueType>(value));
            }

            template<typename KeyType>
            requires std::is_same_v<std::remove_reference_t<KeyType>, Key> || std::is_convertible_v<std::remove_reference_t<KeyType>, Key>
            Value & operator[](KeyType && key) {
                return m[std::forward<KeyType>(key)];
            }

            void reserve(size_t size) {
                m.reserve(size);
            }

            template<typename KeyType>
            requires std::is_convertible_v<std::remove_reference_t<KeyType>, Key>
            bool contains(KeyType const & key) const {
                return m.contains(key);
            }

            size_t size() const {
                return m.size();
            }

            void write(std::filesystem::path filename, int alignment = alignof(void*)) {

                auto writePad = [pad=std::vector<char>(alignment, 0)](std::ofstream & ofs, int alignment) {
                    int64_t pos = ofs.tellp();
                    int padLength = alignment - pos % alignment;
                    if (padLength == alignment) {
                        return;
                    }
                    ofs.write(pad.data(), padLength);
                };

                std::ofstream ofs(filename, std::ios::binary);
                if (ofs.fail()) {
                    std::ostringstream sstr;
                    sstr << "Unable to open file for writing " << filename;
                    throw gradylibMakeException(sstr.str());
                }

                int64_t mapOffset = 0;
                ofs.write(static_cast<char *>(static_cast<void*>(&mapOffset)), 8);

                OpenHashMapTC<Key, int64_t, HashFunction> valueOffsets;
                valueOffsets.reserve(m.size());
                auto valueStartOffset = ofs.tellp();
                for (auto const & [key, value] : m) {
                    valueOffsets[key] = ofs.tellp() - valueStartOffset;
                    if constexpr (serializable_global<Value>) {
                        serialize(ofs, value);
                    } else {
                        value.serialize(ofs);
                    }
                    writePad(ofs, alignment);
                }

                mapOffset = ofs.tellp();
                valueOffsets.write(ofs, alignment);

                ofs.seekp(0, std::ios::beg);
                ofs.write(static_cast<char *>(static_cast<void*>(&mapOffset)), 8);
            }
        };

        template<typename, typename, template<typename> typename>
        friend void GRADY_LIB_MOCK_MMapViewableOpenHashMap_MMAP();

        template<typename, typename, template<typename> typename>
        friend void GRADY_LIB_DEFAULT_MMapViewableOpenHashMap_MMAP();
    };

    template<typename IndexType, typename IntermediateIndexType = uint32_t, template<typename> typename HashFunction = std::hash>
    void GRADY_LIB_MOCK_MMapViewableOpenHashMap_MMAP() {
        MMapViewableOpenHashMap<IndexType, IntermediateIndexType, HashFunction>::mmapFunc = [](void *, size_t, int, int, int, off_t) -> void *{
            return MAP_FAILED;
        };
    }

    template<typename IndexType, typename IntermediateIndexType = uint32_t, template<typename> typename HashFunction = std::hash>
    void GRADY_LIB_DEFAULT_MMapViewableOpenHashMap_MMAP() {
        MMapViewableOpenHashMap<IndexType, IntermediateIndexType, HashFunction>::mmapFunc = mmap;
    }
}

#endif //GRADY_LIB_MMAPOPENHASHMAPVIEWABLE_HPP

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

#include<functional>
#include<fstream>
#include<string>

#include<OpenHashMap.hpp>
#include<OpenHashMapTC.hpp>

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
        { makeView<ValueType>(ptr) } -> std::same_as<std::span<ValueType const>>;
    };

    template<typename ValueType>
    concept viewable_method = requires (std::byte const * ptr) {
        { ValueType::makeView(ptr) } -> std::same_as<std::span<ValueType const>>;
    };

    template<typename Key, typename Value, template<typename> typename HashFunction = std::hash>
    class MMapViewableOpenHashMap {
        OpenHashMapTC<Key, int64_t> valueOffsets;
        void const * valuePtr;
        HashFunction<Key> hashFunction = HashFunction<Key>{};

    public:

        MMapViewableOpenHashMap(std::string filename) {
            
        }

        class Builder {
            OpenHashMapTC<Key, Value, HashFunction> m;

        public:
            template<typename KeyType, typename ValueType>
            requires (serializable_global<ValueType> || serializable_method<ValueType>) &&
                (viewable_global<ValueType> || viewable_method<ValueType>) &&
                (std::is_same_v<std::remove_reference_t<KeyType>, Key> || std::is_convertible_v<std::remove_reference_t<KeyType>, Key>) &&
                std::is_same_v<std::remove_const_t<std::remove_reference_t<ValueType>>, Value>
            void put(KeyType && key, ValueType && value) {
                m.put(std::forward<KeyType>(key), std::forward<ValueType>(value));
            }

            void write(std::string filename, int alignment = alignof(void*)) {
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
                    std::cout << "Unable to open file for writing " << filename << "\n";
                    exit(1);
                }
                int64_t mapOffset = 0;
                ofs.write(static_cast<char *>(static_cast<void*>(&mapOffset)), 8);
                OpenHashMapTC<Key, int64_t> valueOffsets;
                for (auto const & [key, value] : m) {
                    valueOffsets[key] = ofs.tellp();
                    if constexpr (serializable_global<Value>) {
                        serialize(ofs, value);
                    } else {
                        value.serialize(ofs);
                    }
                    writePad(ofs);
                }
                mapOffset = ofs.tellp();
                valueOffsets.write(ofs, alignment);

                ofs.seekp(0, std::ios::beg);
                ofs.write(static_cast<char *>(static_cast<void*>(&mapOffset)), 8);
            }
        };
    };
}

#endif //GRADY_LIB_MMAPOPENHASHMAPVIEWABLE_HPP

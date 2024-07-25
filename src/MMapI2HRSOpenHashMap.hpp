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

#ifndef GRADY_LIB_MMAPI2HRSOPENHASHMAP_HPP
#define GRADY_LIB_MMAPI2HRSOPENHASHMAP_HPP

#include<string>
#include<type_traits>

#include<OpenHashMap.hpp>
#include<OpenHashMapTC.hpp>

namespace gradylib {

    template<typename IndexType, typename IntermediateIndexType = uint32_t>
    requires std::is_integral_v<IndexType>
    class MMapI2HRSOpenHashMap {
        OpenHashMapTC<IndexType, IntermediateIndexType> intMap;
        int fd = -1;
        void * memoryMapping = nullptr;
        size_t mappingSize = 0;

        explicit MMapI2HRSOpenHashMap(std::string filename) {

        }

        class Builder {
            OpenHashMapTC<IndexType, IntermediateIndexType> intMap;
            OpenHashMap<std::string, IntermediateIndexType> stringMap;
            std::vector<std::string> strings;

        public:
            bool contains(IndexType idx) const {
                return intMap.contains(idx);
            }

            void emplace(IndexType idx, std::string &&str) {
                IntermediateIndexType strIdx = 0;
                if (!stringMap.contains(str)) {
                    strIdx = stringMap.size();
                    strings.push_back(str);
                    stringMap.emplace(std::forward<std::string>(str), strIdx);
                } else {
                    strIdx = stringMap.at(idx);
                }
                intMap[idx] = strIdx;
            }

            std::string_view const &at(IndexType idx) const {
                if (!contains(idx)) {
                    std::cout << "Map doesn't contain " << idx << "\n";
                    exit(1);
                }
                return strings.at(intMap.at(idx));
            }

            void write(std::string filename) const {

            }
        };
    };
}

#endif //GRADY_LIB_MMAPI2HRSOPENHASHMAP_HPP
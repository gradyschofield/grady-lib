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

#include<fcntl.h>
#include<sys/mman.h>
#include<unistd.h>

#include<fstream>
#include<string>
#include<string_view>
#include<type_traits>

#include<OpenHashMap.hpp>
#include<OpenHashMapTC.hpp>

namespace gradylib {

    template<typename IndexType, typename IntermediateIndexType = uint32_t>
    requires std::is_integral_v<IndexType>
    class MMapI2HRSOpenHashMap {
        OpenHashMapTC<IndexType, IntermediateIndexType> intMap;
        void const * stringMapping;
        int fd = -1;
        void * memoryMapping = nullptr;
        size_t mappingSize = 0;

        explicit MMapI2HRSOpenHashMap(std::string filename) {
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
            std::byte *ptr = static_cast<std::byte *>(memoryMapping);
            std::byte *base = ptr;
            size_t stringsOffset = *static_cast<size_t*>(static_cast<void*>(ptr));
            ptr += 8;
            intMap = OpenHashMapTC<IndexType, IntermediateIndexType>(ptr);
            stringMapping = base + stringsOffset;
        }

        ~MMapI2HRSOpenHashMap() {
            if (memoryMapping) {
                munmap(memoryMapping, mappingSize);
                close(fd);
            }
        }

        bool contains(IndexType idx) const {
            return intMap.contains(idx);
        }

        std::string_view const & at(IndexType idx) const {
            if (!contains(idx)) {
                std::cout << "Map doesn't contain " << idx << "\n";
                exit(1);
            }
            std::byte const * base = static_cast<std::byte const *>(static_cast<void const *>(stringMapping));
            std::byte const * ptr = base + intMap.at(idx);
            int32_t len = *static_cast<int32_t const *>(static_cast<void const *>(ptr));
            ptr += 4;
            return std::string_view(static_cast<char const *>(static_cast<void const *>(ptr)), len);
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

            void write(std::string filename, int alignment = alignof(void*)) const {
                std::ofstream ofs(filename, std::ios::binary);
                if (ofs.fail()) {
                    std::cout << "Problem opening file " << filename << "\n";
                    exit(1);
                }
                size_t stringsOffset = 0;
                ofs.write(static_cast<char*>(static_cast<void*>(&stringsOffset)), 8);
                intMap.write(ofs, alignment);
                int padSize = alignment - ofs.tellp() % alignment;
                for (int i = 0; i < padSize; ++i) {
                    char t = 0;
                    ofs.write(&t, 1);
                }
                stringsOffset = ofs.tellp();
                for (std::string const & s : strings) {
                    int32_t len = s.length();
                    ofs.write(static_cast<char*>(static_cast<void*>(&len)), 4);
                    ofs.write(s.data(), len);
                    int padSize = 4 - ofs.tellp() % 4;
                    for (int i = 0; i < padSize; ++i) {
                        char t = 0;
                        ofs.write(&t, 1);
                    }
                }
                ofs.seekp(0, std::ios::beg);
                ofs.write(static_cast<char*>(static_cast<void*>(&stringsOffset)), 8);
            }
        };
    };
}

#endif //GRADY_LIB_MMAPI2HRSOPENHASHMAP_HPP
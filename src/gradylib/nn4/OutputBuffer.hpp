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

#pragma once

#include<cstddef>
#include<cstdlib>
#include<cstring>

#include<ostream>

#include"gradylib/OpenHashMap.hpp"
#include"DataType.hpp"
#include"Tensor.hpp"

namespace gradylib {
    namespace nn {
        struct OutputBuffer {
            size_t numElements = 0;
            size_t numBytes = 0;
            void * ptr = nullptr;
            int batchSize = 0;
            int batchUseCount = 0;
            bool isEvaluated_ = false;
            bool isInitialized_ = false;
        public:

            OutputBuffer() = default;
            OutputBuffer(OutputBuffer const &) = delete;
            OutputBuffer & operator=(OutputBuffer const &) = delete;
            OutputBuffer(OutputBuffer && ob)
                    : numElements(ob.numElements), numBytes(ob.numBytes), ptr(ob.ptr), batchSize(ob.batchSize)
            {
                ob.numElements = 0;
                ob.numBytes = 0;
                ob.ptr = nullptr;
                ob.batchSize = 0;
            }

            OutputBuffer & operator=(OutputBuffer && ob) {
                free(ptr);
                numElements = ob.numElements;
                numBytes = ob.numBytes;
                ptr = ob.ptr;
                batchSize = ob.batchSize;
                ob.numElements = 0;
                ob.numBytes = 0;
                ob.ptr = nullptr;
                ob.batchSize = 0;
                return *this;
            }

            static size_t getNumBytes(DataType dataType, size_t numElements, int batchSize) {
                return batchSize * numElements * sizeInBytes(dataType);
            }

            bool atLeastAsLarge(DataType dataType, size_t numElements, int batchSize) {
                return numBytes >= getNumBytes(dataType, numElements, batchSize);
            }

            int getBatchUseCount() const {
                return batchUseCount;
            }

            OutputBuffer(DataType dataType, size_t numElements, int batchSize)
                    : numElements(numElements * batchSize), numBytes(getNumBytes(dataType, numElements, batchSize)), batchSize(batchSize)
            {
                posix_memalign(&ptr, 64, numBytes);
                memset(ptr, 0, numBytes);
            }

            template<typename T>
            T * getPtr(int batchCount = 1) {
                batchUseCount = batchCount;
                return static_cast<T*>(ptr);
            }

            bool isEvaluated() const {
                return isEvaluated_;
            }

            bool isInitialized() const {
                return isInitialized_;
            }

            void setInitialized() {
                isInitialized_ = true;
            }

            void resetEvaluated() {
                isEvaluated_ = false;
            }

            void setEvaluated() {
                isEvaluated_ = true;
            }

            friend std::ostream & operator<<(std::ostream & os, OutputBuffer const & ob);
        };

        std::ostream & operator<<(std::ostream & os, OutputBuffer const & ob) {
            os << "ptr: " << ob.ptr << " ne: " << ob.numElements << " nb: " << ob.numBytes << " bs: " << ob.batchSize;
            return os;
        }

        OpenHashMap<int, OutputBuffer> outputBuffers;
        OpenHashMap<int, OutputBuffer> partialBuffers;

        void * getPartialPtr(Tensor const & tensor) {
            auto ob = partialBuffers.get(tensor.getExpression()->getId());
            return ob.has_value() ? ob.value().getPtr<void>() : nullptr;
        }

        void * getOutputPtr(Tensor const & tensor) {
            auto ob = outputBuffers.get(tensor.getExpression()->getId());
            return ob.has_value() ? ob.value().getPtr<void>() : nullptr;
        }
    }
}
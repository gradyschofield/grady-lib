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

#include"Expression.hpp"
#include"OutputBuffer.hpp"
#include"Util.hpp"
#include"Tensor.hpp"

namespace gradylib {
    namespace nn {

        class BufferAllocator {
        public:

            static void findExprAllocations(Expression const & expression, OpenHashMap<int, OutputBuffer> & bufferMap, int batchSizeIn) {
                int batchSize = holds_alternative<Value>(expression.getExpressionType()) ? 1 : batchSizeIn;
                size_t numElements = product(expression.getDimensions());
                auto ob = bufferMap.get(expression.getId());
                if (!ob.has_value() || !ob.value().atLeastAsLarge(expression.getDataType(), numElements, batchSize)) {
                    bufferMap.put(expression.getId(), OutputBuffer(expression.getDataType(), numElements, batchSize));
                }
            }

            static void findAllocations(Expression const & expression, int batchSize) {
                findExprAllocations(expression, outputBuffers, batchSize);
                for (Expression const & e : expression.getOperands()) {
                    findAllocations(e, batchSize);
                }
            }

            static void findAllocations(Tensor const & tensor, int batchSize) {
                findAllocations(tensor.getExpression(), batchSize);
            }
        };
    }
}

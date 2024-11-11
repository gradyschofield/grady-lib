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
        class Initializer {
        public:
            static void initializeRecurse(Expr const & expression) {
                if (Value const * v = get_if<Value>(&expression->getExpressionType())) {
                    OutputBuffer & ob = outputBuffers.at(expression->getId());
                    float * weights = ob.getPtr<float>();
                    for (int i = 0; i < product(expression->getDimensions()); ++i) {
                        weights[i] = 1.0;
                    }
                    ob.setInitialized();
                }
                for (Expr const & e : expression->getOperands()) {
                    initializeRecurse(e);
                }
            }

            static void initialize(Tensor const & tensor) {
                initializeRecurse(tensor.getExpression());
            }
        };

    }
}

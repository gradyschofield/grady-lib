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

#include<algorithm>
#include<vector>
#include<type_traits>

#include<Accelerate/Accelerate.h>

#include"Expression.hpp"
#include"BufferAllocator.hpp"
#include"Tensor.hpp"

namespace gradylib {
    namespace nn {
        class Evaluator {
            int batchSize = 0;
            std::vector<Expression> terminalNodes;

            void deriveDatatypesAndAllocateBuffers(Expression & e, int batchSize) {
                DatatypeDeriver::deriveDatatypes(e);
                BufferAllocator::findAllocations(e, 64);
            }

        public:
            Evaluator(Tensor const & t)
                    : terminalNodes{t.getExpression()}
            {
                deriveDatatypesAndAllocateBuffers(terminalNodes.front(), 64);
            }

            template<typename... Ts>
            requires (std::same_as<Tensor, std::remove_cvref_t<Ts>> && ...)
            Evaluator(Ts&&... ts)
                    : terminalNodes{(ts.getExpression(), ...)}
            {
                for (Expression & e : terminalNodes) {
                    deriveDatatypesAndAllocateBuffers(e, 64);
                }
            }

            void evaluate() {
            }

            void computeAggregateSlices(Expression const & e, OutputBuffer & outBuffer, std::vector<int> const & slices, bool preFilled = false) {
                std::vector<int> const & dimensions = e.getDimensions();
                OutputBuffer & weightBuffer = outputBuffers.at(e.getOperands().front().getId());
                float * emb = weightBuffer.getPtr<float>();
                float * out = outBuffer.getPtr<float>(1);
                if (!preFilled) {
                    memset(out, 0, sizeof(float) * dimensions[0]);
                }
                for (int j : slices) {
                    for (int i = 0; i < dimensions[0]; ++i) {
                        out[i] += emb[j * dimensions[0] + i];
                    }
                }
            }

            void copyPartialToOutput(Expression const & e, OutputBuffer & partialBuffer, OutputBuffer & outBuffer, int batchSize) {
                size_t numElements = batchSize * product(e.getDimensions());
                float * partial = partialBuffer.getPtr<float>(batchSize);
                float * out = outBuffer.getPtr<float>(batchSize);
                memcpy(out, partial, numElements * sizeof(float));
            }

            void partialInRecurse(Expression const & e, SliceList const & sliceList, std::vector<int> const & slices) {
                if (AggregateSlices const * as = get_if<AggregateSlices>(&e.getExpressionType())) {
                    if (sliceList.getId() == as->getSliceList().getId()) {
                        BufferAllocator::findExprAllocations(e, partialBuffers, 1);
                        OutputBuffer & pb = partialBuffers.at(e.getId());
                        computeAggregateSlices(e, pb, slices);
                        std::cout << "Do the slice into " << pb << "\n";
                    }
                }
                for (Expression const & subExpr : e.getOperands()) {
                    partialInRecurse(subExpr, sliceList, slices);
                }
            }

            void partialIn(SliceList const & sliceList, std::vector<int> const & slices) {
                for (Expression const & e : terminalNodes) {
                    partialInRecurse(e, sliceList, slices);
                }
            }

            void inRecurse(Expression const & e, SliceList const & sliceList, std::vector<int> const & slices) {
                if (AggregateSlices const * as = get_if<AggregateSlices>(&e.getExpressionType())) {
                    if (sliceList.getId() == as->getSliceList().getId()) {
                        BufferAllocator::findExprAllocations(e, outputBuffers, 1);
                        OutputBuffer & ob = outputBuffers.at(e.getId());
                        auto pb = partialBuffers.get(e.getId());
                        bool preFilled = pb.has_value();
                        if (preFilled) {
                            copyPartialToOutput(e, pb.value(), ob, 1);
                        }
                        computeAggregateSlices(e, ob, slices, preFilled);
                        ob.setEvaluated();
                        std::cout << "Do the slice into " << ob << "\n";
                    }
                }
                for (Expression const & subExpr : e.getOperands()) {
                    inRecurse(subExpr, sliceList, slices);
                }
            }

            void in(SliceList const & sliceList, std::vector<int> const & slices) {
                for (Expression const & e : terminalNodes) {
                    inRecurse(e, sliceList, slices);
                }
            }

            void setBatchSize(int batchSize) {
                this->batchSize = batchSize;
                for (Expression const & expression : terminalNodes) {
                    BufferAllocator::findAllocations(expression, 64);
                }
            }

            struct ComputeExpression{
                Expression const & e;

                void operator()(Value const & v) {
                    throw gradylibMakeException("Attempt to evaluate Value ExpressionType.  It must not have been initialized.");
                }
                void operator()(Contraction const & v) {
                    int numOperands = e.getOperands().size();
                    // The following says: if (matrix multiplication with a vector or matrix)
                    if (numOperands == 2 && // we are contracting two tensors
                        e.getOperands().front().getDimensions().size() == 2 &&  // the left tensor is a matrix
                        e.getOperands().back().getDimensions().size() == 1 && // the right tensor is a vector
                        v.getAxis().size() == 1 && // There is only one contraction index
                        v.getAxis().front() == 1 // The contraction index is the second dimension
                            ) {
                        Expression const & op1 = e.getOperands().front();
                        Expression const & op2 = e.getOperands().back();
                        OutputBuffer & ob1 = outputBuffers.at(op1.getId());
                        OutputBuffer & ob2 = outputBuffers.at(op2.getId());
                        OutputBuffer & outOb = outputBuffers.at(e.getId());
                        float * p1 = ob1.getPtr<float>();
                        float * p2 = ob2.getPtr<float>();
                        float * out = outOb.getPtr<float>();
                        int M = op1.getDimensions()[0];
                        int K = op1.getDimensions()[1];
                        int N = ob2.getBatchUseCount();
                        cblas_sgemm(CblasColMajor, CblasNoTrans, CblasNoTrans,
                                    M, N, K, 1.0, p1, M, p2, K, 0.0, out, M);
                    } else {
                        throw gradylibMakeException("Implement more complex contractions for this to work");
                    }
                }
                void operator()(ElementwiseOperator const & v) {
                    float * out = outputBuffers.at(e.getId()).getPtr<float>();
                    Expression const & op1 = e.getOperands().front();
                    OutputBuffer & opBuffer1 = outputBuffers.at(op1.getId());
                    int batch1 = opBuffer1.getBatchUseCount();
                    float * p1 = opBuffer1.getPtr<float>();
                    size_t numElements = product(e.getDimensions());
                    size_t off1 = 0;
                    size_t offOut = 0;
                    auto op = ElementwiseFunctionVisitor<float>{offOut, off1, numElements, out, p1};
                    for (int b = 0; b < batch1; ++b) {
                        std::visit(op, v.getFunction());
                    }
                }
                void operator()(BinaryOperator const & v) {
                    float * out = outputBuffers.at(e.getId()).getPtr<float>();
                    Expression const & op1 = e.getOperands().front();
                    Expression const & op2 = e.getOperands().back();
                    OutputBuffer & opBuffer1 = outputBuffers.at(op1.getId());
                    OutputBuffer & opBuffer2 = outputBuffers.at(op2.getId());
                    int batch1 = opBuffer1.getBatchUseCount();
                    int batch2 = opBuffer2.getBatchUseCount();
                    if (batch1 != batch2 && batch1 > 1 && batch2 > 1) {
                        throw gradylibMakeException("Batch size mismatch");
                    }
                    float * p1 = opBuffer1.getPtr<float>();
                    float * p2 = opBuffer2.getPtr<float>();
                    size_t numElements = product(e.getDimensions());
                    size_t off1 = 0;
                    size_t off2 = 0;
                    size_t offOut = 0;
                    auto op = BinaryFunctionVisitor<float>{offOut, off1, off2, numElements, out, p1, p2};
                    for (int b = 0; b < std::max(batch1, batch2); ++b) {
                        std::visit(op, v.getFunction());
                        if (opBuffer1.getBatchUseCount() == 1) off1 = 0;
                        if (opBuffer2.getBatchUseCount() == 1) off2 = 0;
                    }
                }
                void operator()(AggregateSlices const & v) {
                    throw gradylibMakeException("Attempt to evaluate AggregateSlices ExpressionType.  Aren't these always input?");
                }
                void operator()(Undefined const & v) {
                    throw gradylibMakeException("Attempt to evaluate Undefined ExpressionType");
                }
            };

            void resultRecurse(Expression const & e) {
                auto ob = outputBuffers.get(e.getId());
                if (holds_alternative<Value>(e.getExpressionType())) {
                    if (!ob.has_value() || !ob.value().isInitialized()) {
                        throw gradylibMakeException("A value is uninitialized during evaluation.");
                    }
                    return;
                }
                if (ob.has_value() && ob.value().isEvaluated()) {
                    return;
                }
                if (ob.has_value() && e.getOperands().empty() && (!ob.value().isEvaluated() && !ob.value().isInitialized())) {
                    throw gradylibMakeException("input has not been entered");
                }
                for (Expression const & op : e.getOperands()) {
                    resultRecurse(op);
                }
                std::cout << "do the thing ";
                std::visit(Print{}, e.getExpressionType());
                std::cout << "\n";
                std::visit(ComputeExpression{e}, e.getExpressionType());
            }

            std::vector<void *> result() {
                std::vector<void *> ret;
                for (Expression const & expression : terminalNodes) {
                    resultRecurse(expression);
                    OutputBuffer & ob = outputBuffers.at(expression.getId());
                    ret.push_back(ob.getPtr<void>());
                }
                return ret;
            }

            void * result(Tensor const & tensor) {
                return nullptr;
            }

            void * result(std::string name) {
                return nullptr;
            }
        };
    }
}
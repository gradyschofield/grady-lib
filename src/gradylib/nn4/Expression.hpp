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
#include<iostream>
#include<memory>
#include<optional>
#include<string>
#include<unordered_map>
#include<variant>
#include<vector>

#include"gradylib/nn4/DataType.hpp"

namespace gradylib {
    namespace nn {
        inline int EXPR_ID = 0;

        class Expression;

        inline std::unordered_map<int, std::shared_ptr<Expression>> tensors;
        class Relu {};
        class Sigmoid {};
        class Add {
            template<typename T>
            T operator()(T x, T y) {return x + y;};
        };

        using ElementwiseFunction = std::variant<Relu, Sigmoid>;
        using BinaryFunction = std::variant<Add>;

        template<typename T>
        struct ElementwiseFunctionVisitor {
            size_t & offOut;
            size_t & off1;
            size_t numElements;
            T * out;
            T * op1;

            void operator()(Relu const &) {
                for (size_t i = 0; i < numElements; ++i) {
                    out[offOut++] = std::max(static_cast<T>(0), op1[off1++]);
                }
            }

            void operator()(Sigmoid const &) {
                for (size_t i = 0; i < numElements; ++i) {
                    out[offOut++] = 1 / (1 + exp(-op1[off1++]));
                }
            }
        };

        template<typename T>
        struct BinaryFunctionVisitor {
            size_t & offOut;
            size_t & off1;
            size_t & off2;
            size_t numElements;
            T * out;
            T * op1;
            T * op2;

            void operator()(Add const &) {
                for (size_t i = 0; i < numElements; ++i) {
                    out[offOut++] = op1[off1++] + op2[off2++];
                }
            }
        };

        class Value{
            ParameterPurpose purpose = FreeParameter;
            DataType dtype;
            std::vector<int> dimensions;
        public:
            Value(std::vector<int> d) : dimensions(d) {}
            std::vector<int> const & getDimensions() const {return dimensions;};
        };

        class Contraction{
            std::vector<size_t> axis;
        public:
            Contraction(std::vector<size_t> axis) : axis(axis) {}
            std::vector<size_t> const & getAxis() const {return axis;}
        };

        class ElementwiseOperator{
            ElementwiseFunction function;
        public:
            ElementwiseOperator(ElementwiseFunction function) : function(function) {}
            ElementwiseFunction getFunction() const {
                return function;
            }
        };

        class BinaryOperator{
            BinaryFunction function;
        public:
            BinaryOperator(BinaryFunction function) : function(function){}
            BinaryFunction getFunction() const {
                return function;
            }
        };

        class SliceList {
            int id;
            std::string name;
        public:
            SliceList()
                    : id(EXPR_ID++)
            {
            }

            SliceList(std::string name) : id(EXPR_ID++), name(name) {}
            int getId() const {
                return id;
            }
            std::string const & getName() const {
                return name;
            }
        };

        class AggregateSlices{
            SliceList sliceList;
        public:
            AggregateSlices(SliceList sliceList) : sliceList(sliceList) {}
            SliceList const & getSliceList() const {
                return sliceList;
            }
        };

        class Undefined{};

        class Concatenate {
            std::vector<int> operandSizes;
            std::vector<int> commonDimensions;
        public:
            Concatenate(std::vector<int> operandSizes, std::vector<int> commonDimensions)
                : operandSizes(operandSizes), commonDimensions(commonDimensions)
            {
            }
            std::vector<int> const & getOperandSizes() {
                return operandSizes;
            }
            std::vector<int> const & getCommonDimensions() {
                return commonDimensions;
            }
        };

        struct Print {
            void operator()(Value const & v) { std::cout << "Value "; for (int i : v.getDimensions()) std::cout << i << " ";}
            void operator()(Contraction const & v) { std::cout << "Contraction "; }
            void operator()(ElementwiseOperator const & v) {
                std::visit(Print{}, v.getFunction());
            }
            void operator()(BinaryOperator const & v) {
                std::visit(Print{}, v.getFunction());
            }
            void operator()(AggregateSlices const & v) { std::cout << "AggregateSlices "; }
            void operator()(Undefined const & v) { std::cout << "Undefined "; }
            void operator()(Relu const & v) { std::cout << "ReLU "; }
            void operator()(Sigmoid const & v) { std::cout << "Sigmoid "; }
            void operator()(Add const & v) { std::cout << "Add "; }
            void operator()(Concatenate const & v) { std::cout << "Concatenate "; }
        };

        using ExpressionType = std::variant<
                Value,
                Contraction,
                ElementwiseOperator,
                BinaryOperator,
                AggregateSlices,
                SliceList,
                Undefined,
                Concatenate
        >;

        typedef std::shared_ptr<Expression> Expr;

        class Expression {
            ExpressionType expressionType = Undefined{};
            int id;
            DataType dataType;
            std::vector<int> dimensions;
            std::optional<std::string> name;
            std::vector<Expr> operands;
        public:

            ExpressionType const & getExpressionType() const {
                return expressionType;
            }

            std::vector<Expr> const & getOperands() const {
                return operands;
            }

            std::vector<Expr> & getOperands() {
                return operands;
            }

            template<typename... Args>
            Expression(ExpressionType expressionType, int id, DataType dataType, std::vector<int> dimensions, Args... args)
                    : expressionType(expressionType), id(id), dataType(dataType), dimensions(dimensions), operands{args...}
            {
            }

            std::vector<int> const & getDimensions() const {
                return dimensions;
            }

            std::vector<int> & getDimensions() {
                return dimensions;
            }

            DataType getDataType() const {
                return dataType;
            }

            int getId() const {
                return id;
            }

            void setDataType(DataType dataType) {
                this->dataType = dataType;
            }

            void setName(std::string name) {
                this->name.emplace(name);
            }
        };
    }
}
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

#include<any>
#include<cassert>
#include<iostream>
#include<memory>
#include<optional>
#include<string>
#include<unordered_map>
#include<variant>
#include<vector>

#include"gradylib/OpenHashMap.hpp"

#include"gradylib/nn4/DataType.hpp"
#include"gradylib/nn4/Expression.hpp"

namespace gradylib {
    namespace nn {

        class Tensor {
            Expr expression;

            void addToTable() {
                tensors.emplace(expression->getId(), expression);
            };

        public:

            Expr const & getExpression() const {
                return expression;
            }

            Expr & getExpression() {
                return expression;
            }

            int firstDimension() const {
                return expression->getDimensions().front();
            }

            int lastDimension() const {
                return expression->getDimensions().back();
            }

            template<typename... Sizes>
            requires (std::is_integral_v<Sizes> && ...)
            std::vector<int> makeVector(Sizes... sizes) {
                std::vector<int> ret;
                (ret.push_back(sizes), ...);
                return ret;
            }

            template<typename... Sizes>
            requires (std::is_integral_v<Sizes> && ...)
            Tensor(DataType dataType, Sizes... sizes)
                    : expression(make_shared<Expression>(Value{makeVector(sizes...)}, EXPR_ID++, dataType, makeVector(sizes...)))
            {
                tensors[expression->getId()] = expression;
            }

            template<typename... Sizes>
            requires (std::is_integral_v<Sizes> && ...)
            Tensor(Sizes... sizes)
                    : expression(make_shared<Expression>(Value{makeVector(sizes...)}, EXPR_ID++, Float, makeVector(sizes...)))
            {
                tensors[expression->getId()] = expression;
            }

            Tensor(std::shared_ptr<Expression> && ptr)
                    : expression(move(ptr))
            {
                tensors[expression->getId()] = expression;
            }

            int ndim() const {
                return expression->getDimensions().size();
            }

            std::vector<int> const & getDimensions() const {
                return expression->getDimensions();
            }

            std::vector<int> & getDimensions() {
                return expression->getDimensions();
            }

            void setName(std::string name) {
                expression->setName(name);
            }

            Tensor operator+(Tensor const &t) {
                assert(getDimensions() == t.getDimensions());
                return Tensor{make_shared<Expression>(BinaryOperator{Add{}},
                                                      EXPR_ID++,
                                                      Derived,
                                                      expression->getDimensions(),
                                                      this->getExpression(),
                                                      t.getExpression())};
            }

            template<typename... Ts>
            requires (std::is_same_v<Ts, Tensor> && ...)
            Tensor contract(std::vector<size_t> axis, Ts... ts) {
                std::vector<std::vector<int>> tmpDim;
                (tmpDim.push_back(ts.getDimensions()), ...);
                std::vector<int> const & thisDim = expression->getDimensions();
                for (int i = 0; i < tmpDim.size(); ++i) {
                    assert(thisDim[axis[i]] == tmpDim[i][0]);
                }
                std::vector<int> retDim(thisDim);
                for (int a : axis) {
                    for (int i = a+1; i < retDim.size(); ++i) {
                        retDim[a] = retDim[i];
                    }
                    retDim.pop_back();
                }
                for (std::vector<int> const & d : tmpDim) {
                    for (int i = 1; i < d.size(); ++i) {
                        retDim.push_back(d[i]);
                    }
                }
                return Tensor{make_shared<Expression>(Contraction{axis}, EXPR_ID++, Derived, retDim, this->getExpression(), (ts.getExpression(), ...))};
            }

            template<typename T>
            requires std::same_as<SliceList, std::remove_cvref_t<T>>
            Tensor aggregateSlices(T && sliceList) {
                auto resultDim = getDimensions();
                resultDim.pop_back();
                return Tensor{make_shared<Expression>(AggregateSlices{sliceList}, EXPR_ID++, Derived, resultDim, this->getExpression())};
            }

            Tensor operator*(Tensor const &t) {
                return contract({expression->getDimensions().size()-1}, t);
            }

            friend Tensor dot(Tensor const &t1, Tensor const &t2) {
                assert(t1.getDimensions().size() <= 2);
                assert(t1.getDimensions() == t2.getDimensions());
                std::vector<int> dimension{t1.getDimensions().size() == 2 ? t1.getDimensions()[1] : 1};
                size_t axis = t1.getDimensions().size() - 1;
                return Tensor{make_shared<Expression>(Contraction{std::vector{axis}}, EXPR_ID++, Derived, dimension, t1.getExpression(), t2.getExpression())};
            }

            friend Tensor relu(Tensor const & t) {
                return Tensor{make_shared<Expression>(ElementwiseOperator{Relu{}}, EXPR_ID++, Derived, t.getDimensions(), t.getExpression())};
            }

            friend Tensor sigmoid(Tensor const & t) {
                return Tensor{make_shared<Expression>(ElementwiseOperator{Sigmoid{}}, EXPR_ID++, Derived, t.getDimensions(), t.getExpression())};
            }
        };
    }
}


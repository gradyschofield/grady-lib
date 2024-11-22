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

#include<iostream>
#include<list>
#include<string>
#include<variant>
#include<vector>

namespace gradylib {
    namespace nn {
        namespace activation{
            class LabelSpec{};

            class Scalar;

            typedef std::shared_ptr<Scalar> ScalarPtr;

            class Terminal{
                std::string name;
                std::vector<int> index;
            public:
                bool operator==(Terminal const & t) const { return true; };
            };
            class Displacement{
                std::string name;
                std::vector<int> index;
            public:
                bool operator==(Displacement const & t) const { return true; };
            };
            class Negate{};
            class Value{
                double x;
            public:
                Value(double x) : x(x) {}
                double getValue() const {
                    return x;
                }
                bool operator==(Value const & t) const { return true; };
            };
            class Add{};
            class Divide{};
            class Multiply{};
            class Subtract{};
            class Log{};
            class Exp{};

            typedef std::variant<Displacement, Terminal, Negate, Add, Divide, Multiply, Subtract, Log, Exp, Value> IntermediateOperation;

            class Label {
            public:
                LabelSpec operator=(double x) {
                    return LabelSpec{};
                }
            };

            class Scalar {
                IntermediateOperation expr;
                std::vector<ScalarPtr> operands;
            public:
                Scalar(IntermediateOperation e) : expr(e) {}

                template<typename... Ss>
                requires (std::is_same_v<ScalarPtr, Ss> && ...)
                Scalar(IntermediateOperation e, Ss... ss)
                    : expr(e)
                {
                    (operands.push_back(ss), ...);
                }

                IntermediateOperation const & getOperation() const {
                    return expr;
                }

                std::vector<ScalarPtr> const & getOperands() const {
                    return operands;
                }

                std::vector<ScalarPtr> & getOperands() {
                    return operands;
                }

                bool basicEquals(ScalarPtr const & p) {
                    if (p->getOperation().index() == getOperation().index()) {
                        if (std::holds_alternative<Terminal>(getOperation())) {
                            return std::get<Terminal>(getOperation()) == std::get<Terminal>(p->getOperation());
                        } else if (std::holds_alternative<Displacement>(getOperation())) {
                            return std::get<Displacement>(getOperation()) == std::get<Displacement>(p->getOperation());
                        } else if (std::holds_alternative<Value>(getOperation())) {
                            return std::get<Value>(getOperation()) == std::get<Value>(p->getOperation());
                        }
                    }
                    return false;
                }
            };

            template<typename Arg>
            struct IntermediateOperationVisitor {
                Arg arg;
                std::vector<ScalarPtr> const & operands;

                template<typename Func>
                Arg applyBinary(Func && func) {
                    return func(std::visit(IntermediateOperationVisitor{arg, operands[0]->getOperands()}, operands[0]->getOperation()),
                           std::visit(IntermediateOperationVisitor{arg, operands[1]->getOperands()}, operands[1]->getOperation()));
                }
                template<typename Func>
                Arg applyUnary(Func && func) {
                    return func(std::visit(IntermediateOperationVisitor{arg, operands[0]->getOperands()}, operands[0]->getOperation()));
                }
                Arg operator()(Displacement const & t) {
                    throw gradylibMakeException("Can't evaluate derivative with IntermediateOperationVisitor");
                }
                Arg operator()(Terminal const & t) { return arg; }
                Arg operator()(Negate const & t) {
                    return applyUnary([](Arg x) {return -x;});
                }
                Arg operator()(Add const & t) {
                    return applyBinary([](Arg x, Arg y) { return x + y;});
                }
                Arg operator()(Subtract const & t) {
                    return applyBinary([](Arg x, Arg y) { return x - y;});
                }
                Arg operator()(Multiply const & t) {
                    return applyBinary([](Arg x, Arg y) { return x * y;});
                }
                Arg operator()(Divide const & t) {
                    return applyBinary([](Arg x, Arg y) { return x / y;});
                }
                Arg operator()(Log const & t) {
                    return applyUnary([](Arg x) { return log(x); });
                }
                Arg operator()(Exp const & t) {
                    return applyUnary([](Arg x) { return exp(x); });
                }
                Arg operator()(Value const & t) { return t.getValue(); }
            };

            struct IntermediateDerivativeVisitor {
                std::vector<ScalarPtr> const & operands;

                ScalarPtr operator()(Displacement const & t) {
                    return std::make_shared<Scalar>(Value{0});
                }
                ScalarPtr operator()(Terminal const & t) {
                    return std::make_shared<Scalar>(Displacement{});
                }
                ScalarPtr operator()(Negate const & t) {
                    ScalarPtr op = operands[0];
                    return std::make_shared<Scalar>(Negate{}, std::visit(IntermediateDerivativeVisitor{op->getOperands()}, op->getOperation()));
                }
                ScalarPtr operator()(Add const & t) {
                    ScalarPtr op1 = operands[0];
                    ScalarPtr op2 = operands[1];
                    return std::make_shared<Scalar>(Add{},
                                                    std::visit(IntermediateDerivativeVisitor{op1->getOperands()}, op1->getOperation()),
                                                    std::visit(IntermediateDerivativeVisitor{op2->getOperands()}, op2->getOperation()));
                }
                ScalarPtr operator()(Subtract const & t) {
                    ScalarPtr op1 = operands[0];
                    ScalarPtr op2 = operands[1];
                    return std::make_shared<Scalar>(Subtract{},
                                                    std::visit(IntermediateDerivativeVisitor{op1->getOperands()}, op1->getOperation()),
                                                    std::visit(IntermediateDerivativeVisitor{op2->getOperands()}, op2->getOperation()));
                }
                ScalarPtr operator()(Multiply const & t) {
                    ScalarPtr op1 = operands[0];
                    ScalarPtr op2 = operands[1];
                    auto p1 = std::make_shared<Scalar>(Multiply{}, std::visit(IntermediateDerivativeVisitor{op1->getOperands()}, op1->getOperation()), op2);
                    auto p2 = std::make_shared<Scalar>(Multiply{}, op1, std::visit(IntermediateDerivativeVisitor{op2->getOperands()}, op2->getOperation()));
                    return std::make_shared<Scalar>(Add{}, p1, p2);
                }
                ScalarPtr operator()(Divide const & t) {
                    ScalarPtr op1 = operands[0];
                    ScalarPtr op2 = operands[1];
                    auto p1 = std::make_shared<Scalar>(Multiply{}, std::visit(IntermediateDerivativeVisitor{op1->getOperands()}, op1->getOperation()), op2);
                    auto p2 = std::make_shared<Scalar>(Multiply{}, op1, std::visit(IntermediateDerivativeVisitor{op2->getOperands()}, op2->getOperation()));
                    auto diff = std::make_shared<Scalar>(Subtract{}, p1, p2);
                    auto denominator = std::make_shared<Scalar>(Multiply{}, op2, op2);
                    return std::make_shared<Scalar>(Divide{}, diff, denominator);

                }
                ScalarPtr operator()(Log const & t) {
                    ScalarPtr op = operands[0];
                    ScalarPtr deriv = std::visit(IntermediateDerivativeVisitor{op->getOperands()}, op->getOperation());
                    return std::make_shared<Scalar>(Divide{}, deriv, op);
                }
                ScalarPtr operator()(Exp const & t) {
                    ScalarPtr op = operands[0];
                    ScalarPtr deriv = std::visit(IntermediateDerivativeVisitor{op->getOperands()}, op->getOperation());
                    ScalarPtr expOp = std::make_shared<Scalar>(Exp{}, op);
                    return std::make_shared<Scalar>(Multiply{}, deriv, expOp);
                }
                ScalarPtr  operator()(Value const & t) {
                    return std::make_shared<Scalar>(Value{0});
                }
            };

            class Intermediate {
                ScalarPtr scalarPtr;

                void collectMultipliersRecurse(std::list<ScalarPtr> & multipliers, ScalarPtr const & p) {
                    if (std::holds_alternative<Multiply>(p->getOperation())) {
                        collectMultipliersRecurse(multipliers, p->getOperands().front());
                        collectMultipliersRecurse(multipliers, p->getOperands().back());
                    } else if (std::holds_alternative<Value>(p->getOperation())) {
                        multipliers.push_back(p);
                    } else if (std::holds_alternative<Terminal>(p->getOperation())) {
                        multipliers.push_back(p);
                    } else if (std::holds_alternative<Displacement>(p->getOperation())) {
                        multipliers.push_back(p);
                    }
                }

                std::list<ScalarPtr> collectMultipliers(ScalarPtr const & p) {
                    std::list<ScalarPtr> ret;
                    collectMultipliersRecurse(ret, p);
                    return ret;
                }

                bool removeProductOperandRecurse(ScalarPtr & parent, ScalarPtr & op, ScalarPtr & otherOp, ScalarPtr & operand) {
                    if (std::holds_alternative<Multiply>(op->getOperation())) {
                        auto lArg = op->getOperands().front();
                        auto rArg = op->getOperands().back();
                        if (removeProductOperandRecurse(op, lArg, rArg, operand)) return true;
                        if (removeProductOperandRecurse(op, rArg, lArg, operand)) return true;
                    } else if (op->basicEquals(operand)) {
                        parent = otherOp;
                        return true;
                    }
                    return false;
                }

                void removeProductOperand(ScalarPtr & op, ScalarPtr & operand) {
                    if (op->basicEquals(operand)) {
                        op = std::make_shared<Scalar>(Value{1});
                        return;
                    }
                    if (!std::holds_alternative<Multiply>(op->getOperation())) {
                        throw gradylibMakeException("removeProductOperand should be called on Multiply operation");
                    }
                    auto lArg = op->getOperands().front();
                    auto rArg = op->getOperands().back();
                    bool removed = removeProductOperandRecurse(op, lArg, rArg, operand);
                    if (!removed) {
                        removed = removeProductOperandRecurse(op, rArg, lArg, operand);
                    }
                    if (!removed) {
                        throw gradylibMakeException("removeProductOperand could not find a factor to remove");
                    }
                }

                void factor(ScalarPtr & parent, ScalarPtr & current) {
                    for (ScalarPtr & operand : current->getOperands()) {
                        factor(current, operand);
                    }
                    if (std::holds_alternative<Add>(current->getOperation())) {
                        ScalarPtr & op1 = current->getOperands()[0];
                        ScalarPtr & op2 = current->getOperands()[1];
                        auto rollingProduct = current;
                        // originalAddOperation will point to an element of an operand array
                        ScalarPtr * originalAddOperation = nullptr;
                        bool firstProduct = true;
                        if (std::holds_alternative<Multiply>(op1->getOperation()) &&
                                std::holds_alternative<Multiply>(op2->getOperation())) {
                            auto leftArgs = collectMultipliers(op1);
                            auto rightArgs = collectMultipliers(op2);
                            while(!leftArgs.empty()) {
                                auto lArg = leftArgs.front();
                                auto iter = std::find_if(rightArgs.begin(), rightArgs.end(),
                                                         [&lArg](auto & x) {
                                                             return x->basicEquals(lArg);
                                                         });
                                if (iter != rightArgs.end()) {
                                    auto rArg = std::move(*iter);
                                    rightArgs.erase(iter);
                                    rollingProduct = std::make_shared<Scalar>(Multiply{}, lArg, rollingProduct);
                                    if (firstProduct) {
                                        firstProduct = false;
                                        originalAddOperation = &rollingProduct->getOperands().back();
                                    }
                                    removeProductOperand(op1, lArg);
                                    removeProductOperand(op2, rArg);
                                }
                                leftArgs.pop_front();
                            }
                        }
                        if (originalAddOperation) {
                            ScalarPtr & p = *originalAddOperation;
                            if (std::holds_alternative<Value>(p->getOperands().front()->getOperation()) && std::holds_alternative<Value>(p->getOperands().back()->getOperation())) {
                                double val1 = std::get<Value>(p->getOperands().front()->getOperation()).getValue();
                                double val2 = std::get<Value>(p->getOperands().back()->getOperation()).getValue();
                                *originalAddOperation = std::make_shared<Scalar>(Value{val1+val2});
                            }
                        }
                        current = rollingProduct;
                    }
                }

            public:
                Intermediate() : scalarPtr(std::make_shared<Scalar>(Terminal{})) { }
                Intermediate(ScalarPtr s) : scalarPtr(s) {}
                Intermediate with(LabelSpec labelSpec) {
                }

                ScalarPtr const & getScalarPtr() const {
                    return scalarPtr;
                }

                ScalarPtr & getScalarPtr() {
                    return scalarPtr;
                }

                Intermediate operator-(){
                    return Intermediate{};
                }

                template<typename T>
                T evaluate(T t) const {
                    return std::visit(IntermediateOperationVisitor{t, scalarPtr->getOperands()}, scalarPtr->getOperation());
                }

                Intermediate derivative() {
                    return Intermediate{std::visit(IntermediateDerivativeVisitor{scalarPtr->getOperands()}, scalarPtr->getOperation())};
                }

                void simplify() {
                    auto head = std::make_shared<Scalar>(Displacement{});
                    factor(head, scalarPtr);
                }
            };

            inline ScalarPtr toValue(double x) {
                return std::make_shared<Scalar>(Value{x});
            }

            class Probability {
            public:
                template<typename... Is>
                requires (std::is_integral_v<Is> && ...)
                Intermediate operator[](Is... is) {
                    return Intermediate{};
                }

                template<typename... Is>
                requires (std::is_integral_v<Is> && ...)
                Intermediate operator[](std::string name, Is... is) {
                    return Intermediate{};
                }
            };

            inline Label operator-(double x, Label t) {
                return Label{};
            }

            inline Intermediate log(Probability p) {
                return Intermediate{};
            }

            inline Intermediate log(Intermediate i) {
                return Intermediate{std::make_shared<Scalar>(Log{}, i.getScalarPtr())};
            }

            inline Intermediate exp(Intermediate i) {
                return Intermediate{std::make_shared<Scalar>(Exp{}, i.getScalarPtr())};
            }

            inline Intermediate max(double x, Intermediate i) {
                return Intermediate{};
            }

            inline Intermediate operator-(double x, Probability i) {
                return Intermediate{};
            }

            inline Intermediate operator+(double x, Intermediate i) {
                return Intermediate{std::make_shared<Scalar>(Add{}, toValue(x), i.getScalarPtr())};
            }

            inline Intermediate operator-(double x, Intermediate i) {
                return Intermediate{std::make_shared<Scalar>(Subtract{}, toValue(x), i.getScalarPtr())};
            }

            inline Intermediate operator/(double x, Intermediate i) {
                return Intermediate{std::make_shared<Scalar>(Divide{}, toValue(x), i.getScalarPtr())};
            }

            inline Intermediate operator*(double x, Intermediate i) {
                return Intermediate{std::make_shared<Scalar>(Multiply{}, toValue(x), i.getScalarPtr())};
            }

            inline Intermediate operator+(Intermediate i, double x) {
                return Intermediate{std::make_shared<Scalar>(Add{}, i.getScalarPtr(), toValue(x))};
            }

            inline Intermediate operator-(Intermediate i, double x) {
                return Intermediate{std::make_shared<Scalar>(Subtract{}, i.getScalarPtr(), toValue(x))};
            }

            inline Intermediate operator/(Intermediate i, double x) {
                return Intermediate{std::make_shared<Scalar>(Divide{}, i.getScalarPtr(), toValue(x))};
            }

            inline Intermediate operator*(Intermediate i, double x) {
                return Intermediate{std::make_shared<Scalar>(Multiply{}, i.getScalarPtr(), toValue(x))};
            }

            inline Intermediate operator+(Intermediate i1, Intermediate i2) {
                return Intermediate{std::make_shared<Scalar>(Add{}, i1.getScalarPtr(), i2.getScalarPtr())};
            }

            inline Intermediate operator-(Intermediate i1, Intermediate i2) {
                return Intermediate{std::make_shared<Scalar>(Subtract{}, i1.getScalarPtr(), i2.getScalarPtr())};
            }

            inline Intermediate operator*(Intermediate i1, Intermediate i2) {
                return Intermediate{std::make_shared<Scalar>(Multiply{}, i1.getScalarPtr(), i2.getScalarPtr())};
            }

            inline Intermediate operator/(Intermediate i1, Intermediate i2) {
                return Intermediate{std::make_shared<Scalar>(Divide{}, i1.getScalarPtr(), i2.getScalarPtr())};
            }

            inline Intermediate operator*(Label l, Intermediate i) {
                return Intermediate{};
            }

            inline Label y;
            inline Probability p;
            inline Intermediate x;
            inline Intermediate h{std::make_shared<Scalar>(Displacement{})};

            inline Intermediate sigmoid = 1 / (1 + exp(-x));

            inline Intermediate relu = max(0, x);
        }
    }
}


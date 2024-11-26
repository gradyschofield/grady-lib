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
#include<unordered_set>
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
                Terminal() = default;
                Terminal(std::string name) : name(name) {}

                template<typename... Indexes>
                requires (std::is_integral_v<Indexes> && ...)
                Terminal(std::string name, Indexes... indexes)
                    : name(name), index{indexes...}
                {
                }

                bool operator==(Terminal const & t) const { return name == t.name && index == t.index; };

                std::string const & getName() const {
                    return name;
                }
            };

            class Displacement{
                std::string name;
                std::vector<int> index;
            public:
                Displacement() = default;
                Displacement(std::string name) : name(name) {}

                template<typename... Indexes>
                requires (std::is_integral_v<Indexes> && ...)
                Displacement(std::string name, Indexes... indexes)
                    : name(name), index{indexes...}
                {
                }

                bool operator==(Displacement const & t) const { return name == t.name && index == t.index; };

                std::string const & getName() const {
                    return name;
                }
            };
            class Negate{};
            class Value{
                double x;
            public:
                Value(double x) : x(x) {}
                double getValue() const {
                    return x;
                }
                bool operator==(Value const & t) const { return x == t.x; };
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

                template<typename... Ts>
                static bool variantHasOneOf(ScalarPtr const & p) {
                    return (std::holds_alternative<Ts>(p->getOperation()) || ...);
                }

                template<typename... Ts>
                static int positionOfOneThatIs(ScalarPtr & p1, ScalarPtr & p2) {
                    if ((std::holds_alternative<Ts>(p1->getOperation()) || ...)) {
                        return 1;
                    }
                    return 2;
                }

                template<typename... Ts>
                static ScalarPtr & oneThatIs(ScalarPtr & p1, ScalarPtr & p2) {
                    if ((std::holds_alternative<Ts>(p1->getOperation()) || ...)) {
                        return p1;
                    }
                    return p2;
                }

                template<typename... Ts>
                static ScalarPtr & otherOf(ScalarPtr & p1, ScalarPtr & p2) {
                    if ((std::holds_alternative<Ts>(p1->getOperation()) || ...)) {
                        return p2;
                    }
                    return p1;
                }

                bool expandRecurse2(ScalarPtr & current) {
                    auto fixUp = [](ScalarPtr & current) {
                        if (variantHasOneOf<Multiply>(current) &&
                            (variantHasOneOf<Add, Subtract>(current->getOperands().front()) ||
                             variantHasOneOf<Add, Subtract>(current->getOperands().back()))) {
                            int positionOfSumChild = positionOfOneThatIs<Add, Subtract>(current->getOperands().front(), current->getOperands().back());
                            auto sumChild = oneThatIs<Add, Subtract>(current->getOperands().front(), current->getOperands().back());
                            auto otherChild = otherOf<Add, Subtract>(current->getOperands().front(), current->getOperands().back());
                            ScalarPtr newMul1, newMul2;
                            if (positionOfSumChild == 1) {
                                newMul1 = std::make_shared<Scalar>(Multiply{}, sumChild->getOperands().front(), otherChild);
                                newMul2 = std::make_shared<Scalar>(Multiply{}, sumChild->getOperands().back(), otherChild);
                            } else {
                                newMul1 = std::make_shared<Scalar>(Multiply{}, otherChild, sumChild->getOperands().front());
                                newMul2 = std::make_shared<Scalar>(Multiply{}, otherChild, sumChild->getOperands().back());
                            }
                            current = std::make_shared<Scalar>(sumChild->getOperation(), newMul1, newMul2);
                            return true;
                        }
                        return false;
                    };
                    bool didFixup = fixUp(current);
                    for (ScalarPtr & op : current->getOperands()) {
                        didFixup = didFixup || expandRecurse2(op);
                    }
                    return didFixup;
                }

                bool expandRecurse(ScalarPtr & parent, ScalarPtr & current, ScalarPtr & parentsOtherOperand) {
                    if (current->getOperands().size() == 2) {
                        bool expanded = expandRecurse(current, current->getOperands().front(), current->getOperands().back());
                        if (!expanded) {
                            expandRecurse(current, current->getOperands().back(), current->getOperands().front());
                        }
                    } else {
                        ScalarPtr null(nullptr);
                        for (ScalarPtr & op : current->getOperands()) {
                            expandRecurse(current, op, null);
                        }
                    }
                    if (parent &&
                        std::holds_alternative<Multiply>(parent->getOperation()) &&
                        (std::holds_alternative<Add>(current->getOperation()) ||
                         std::holds_alternative<Subtract>(current->getOperation())) ) {

                        auto & leftOp = current->getOperands().front();
                        auto & rightOp = current->getOperands().back();
                        auto newMul1 = std::make_shared<Scalar>(Multiply{}, parentsOtherOperand, leftOp);
                        auto newMul2 = std::make_shared<Scalar>(Multiply{}, parentsOtherOperand, rightOp);
                        leftOp = newMul1;
                        rightOp = newMul2;
                        parent = current;
                        bool expanded = expandRecurse(newMul1, newMul1->getOperands().front(), newMul1->getOperands().back());
                        if (!expanded) {
                            expandRecurse(newMul1, newMul1->getOperands().back(), newMul1->getOperands().front());
                        }
                        expanded = expandRecurse(newMul2, newMul2->getOperands().front(), newMul2->getOperands().back());
                        if (!expanded) {
                            expandRecurse(newMul2, newMul2->getOperands().back(), newMul2->getOperands().front());
                        }
                        return true;
                    }
                    return false;
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

                void expand() {
                    ScalarPtr null(nullptr);
                    while (expandRecurse2(scalarPtr));
                }

                static void listTopLevelPolynomialTerms(ScalarPtr current, std::vector<std::vector<ScalarPtr>> & terms) {
                    if (!variantHasOneOf<Terminal, Value, Displacement, Multiply, Add, Subtract>(current)) {
                       throw gradylibMakeException("listTopLevelTerms");
                    }
                    if (!current->getOperands().empty()) {
                        for (ScalarPtr & op : current->getOperands()) {
                            if (variantHasOneOf<Add, Subtract>(current) && !variantHasOneOf<Add, Subtract>(op)) {
                                terms.push_back({});
                            }
                            listTopLevelPolynomialTerms(op, terms);
                        }
                    } else {
                        if (terms.empty()) {
                            terms.push_back({});
                        }
                        terms.back().push_back(current);
                    }
                }

                bool equalPolynomialTerms(Intermediate & z) {
                    std::vector<std::vector<ScalarPtr>> terms1, terms2;
                    listTopLevelPolynomialTerms(scalarPtr, terms1);
                    listTopLevelPolynomialTerms(z.scalarPtr, terms2);
                    if (terms1.size() != terms2.size()) return false;
                    std::unordered_set<int> foundTerms;
                    for (int i = 0; i < terms1.size(); ++i) {
                        std::vector<ScalarPtr> const & t1 = terms1[i];
                        bool found = false;
                        for (int j = 0; j < terms2.size() && !found; ++j) {
                            if (foundTerms.contains(j)) continue;
                            std::vector<ScalarPtr> const & t2 = terms2[j];
                            if (t1.size() != t2.size()) continue;
                            std::unordered_set<int> foundFactors;
                            for (int k = 0; k < t1.size(); ++k) {
                                ScalarPtr const & f1 = t1[k];
                                bool found2 = false;
                                for (int m = 0; m < t1.size() && !found2; ++m) {
                                    if (foundFactors.contains(m)) continue;
                                    ScalarPtr const & f2 = t2[m];
                                    if (f1->basicEquals(f2)) {
                                        found2 = true;
                                        foundFactors.insert(m);
                                    }
                                }
                                if (!found2) break;
                            }
                            if (foundFactors.size() == t1.size()) {
                                found = true;
                            }
                        }
                        if (!found) return false;
                    }
                    return true;
                }

                template<typename... Indexes>
                requires (std::is_integral_v<Indexes> && ...)
                Intermediate operator[](Indexes... indexes) {
                    if (!std::holds_alternative<Terminal>(scalarPtr->getOperation()) &&
                            !std::holds_alternative<Displacement>(scalarPtr->getOperation())) {
                        throw gradylibMakeException("Can only set indices for Terminals and Displacements");
                    }
                    if (std::holds_alternative<Terminal>(scalarPtr->getOperation()) ){
                        auto & t = std::get<Terminal>(scalarPtr->getOperation());
                        return  std::make_shared<Scalar>(Terminal{t.getName(), indexes...});
                    }
                    if (std::holds_alternative<Displacement>(scalarPtr->getOperation()) ){
                        auto & t = std::get<Displacement>(scalarPtr->getOperation());
                        return  std::make_shared<Scalar>(Displacement{t.getName(), indexes...});
                    }
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


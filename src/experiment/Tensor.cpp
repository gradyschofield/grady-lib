#include<any>
#include<cassert>
#include<iostream>
#include<optional>
#include<unordered_map>
#include<variant>
#include<vector>

#include<gradylib/OpenHashMap.hpp>

using namespace std;

namespace gradylib {
    namespace nn {
        namespace expression {

            enum ParameterPurpose {
                InputParameter,
                FreeParameter
            };

            enum DataType {
                Bfloat,
                Float,
                Double,
                I8,
                I16,
                I32,
                I64,
                U8,
                U16,
                U32,
                U64,
                Derived
            };

            template<DataType dataType>
            struct DataTypeTraits;

            template<>
            struct DataTypeTraits<Float> {
                using type = float;
                static int const sizeInBytes = sizeof(type);
            };

            template<>
            struct DataTypeTraits<Derived> {
                using type = void_t<>;
                static int const sizeInBytes = 0;
            };

            int sizeInBytes(DataType dataType) {
                switch(dataType) {
                    case Float: return DataTypeTraits<Float>::sizeInBytes;
                    default: return 0;
                }
            }

            int EXPR_ID = 0;

            class Relu{};
            class Sigmoid{};
            class Add {};

            using ElementwiseFunction = std::variant<Relu, Sigmoid>;
            using BinaryFunction = std::variant<Add>;

            class Value{
                ParameterPurpose purpose = FreeParameter;
                DataType dtype;
                vector<int> dimensions;
            public:
                Value(vector<int> d) : dimensions(d) {}
                vector<int> const & getDimensions() const {return dimensions;};
            };

            class Contraction{};

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
                string name;
            public:
                SliceList()
                    : id(EXPR_ID++)
                {
                }

                SliceList(string name) : id(EXPR_ID++), name(name) {}
                int getId() const {
                    return id;
                }
                string const & getName() const {
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

            struct Print {
                void operator()(Value const & v) { cout << "Value "; for (int i : v.getDimensions()) cout << i << " ";}
                void operator()(Contraction const & v) { cout << "Contraction "; }
                void operator()(ElementwiseOperator const & v) {
                    std::visit(Print{}, v.getFunction());
                }
                void operator()(BinaryOperator const & v) {
                    std::visit(Print{}, v.getFunction());
                }
                void operator()(AggregateSlices const & v) { cout << "AggregateSlices "; }
                void operator()(Undefined const & v) { cout << "Undefined "; }
                void operator()(Relu const & v) { cout << "ReLU "; }
                void operator()(Sigmoid const & v) { cout << "Sigmoid "; }
                void operator()(Add const & v) { cout << "Add "; }
            };

            using ExpressionType = variant<
                Value,
                Contraction,
                ElementwiseOperator,
                BinaryOperator,
                AggregateSlices,
                SliceList,
                Undefined
            >;

            class Expression {
                ExpressionType expressionType = Undefined{};
                int id;
                DataType dataType;
                vector<int> dimensions;
                optional<string> name;
                vector<Expression> operands;
            public:

                ExpressionType const & getExpressionType() const {
                    return expressionType;
                }

                vector<Expression> const & getOperands() const {
                    return operands;
                }

                vector<Expression> & getOperands() {
                    return operands;
                }

                template<typename... Args>
                Expression(ExpressionType expressionType, int id, DataType dataType, vector<int> dimensions, Args... args)
                    : expressionType(expressionType), id(id), dataType(dataType), dimensions(dimensions), operands{args...}
                {
                }

                vector<int> const & getDimensions() const {
                    return dimensions;
                }

                vector<int> & getDimensions() {
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

                void setName(string name) {
                    this->name.emplace(name);
                }
            };

            struct OutputBuffer {
                size_t numElements = 0;
                size_t numBytes = 0;
                void * ptr = nullptr;
                int batchSize = 0;

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

                OutputBuffer(DataType dataType, size_t numElements, int batchSize)
                    : numElements(numElements * batchSize), numBytes(getNumBytes(dataType, numElements, batchSize)), batchSize(batchSize)
                {
                    posix_memalign(&ptr, 64, numBytes);
                    memset(ptr, 0, numBytes);
                }
            };

            ostream & operator<<(ostream & os, OutputBuffer const & ob) {
                os << "ptr: " << ob.ptr << " ne: " << ob.numElements << " nb: " << ob.numBytes << " bs: " << ob.batchSize;
                return os;
            }

            unordered_map<int, shared_ptr<Expression>> tensors;
            OpenHashMap<int, OutputBuffer> outputBuffers;
            OpenHashMap<int, OutputBuffer> partialBuffers;

            class Tensor {

                shared_ptr<Expression> expression;

                void addToTable() {
                    tensors.emplace(expression->getId(), expression);
                };

            public:

                Expression const & getExpression() const {
                    return *expression;
                }

                Expression & getExpression() {
                    return *expression;
                }

                int firstDimension() const {
                    return expression->getDimensions().front();
                }

                int lastDimension() const {
                    return expression->getDimensions().back();
                }

                template<typename... Sizes>
                requires (std::is_integral_v<Sizes> && ...)
                vector<int> makeVector(Sizes... sizes) {
                    vector<int> ret;
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

                Tensor(shared_ptr<Expression> && ptr)
                    : expression(move(ptr))
                {
                    tensors[expression->getId()] = expression;
                }

                int ndim() const {
                    return expression->getDimensions().size();
                }

                vector<int> const & getDimensions() const {
                    return expression->getDimensions();
                }

                vector<int> & getDimensions() {
                    return expression->getDimensions();
                }

                void setName(string name) {
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
                Tensor contract(vector<size_t> axis, Ts... ts) {
                    vector<vector<int>> tmpDim;
                    (tmpDim.push_back(ts.getDimensions()), ...);
                    vector<int> const & thisDim = expression->getDimensions();
                    for (int i = 0; i < tmpDim.size(); ++i) {
                        assert(thisDim[axis[i]] == tmpDim[i][0]);
                    }
                    vector<int> retDim(thisDim);
                    for (int a : axis) {
                        for (int i = a+1; i < retDim.size(); ++i) {
                            retDim[a] = retDim[i];
                        }
                        retDim.pop_back();
                    }
                    for (vector<int> const & d : tmpDim) {
                        for (int i = 1; i < d.size(); ++i) {
                            retDim.push_back(d[i]);
                        }
                    }
                    return Tensor{make_shared<Expression>(Contraction{}, EXPR_ID++, Derived, retDim, this->getExpression(), (ts.getExpression(), ...))};
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
                    vector<int> dimension{t1.getDimensions().size() == 2 ? t1.getDimensions()[1] : 1};
                    return Tensor{make_shared<Expression>(Contraction{}, EXPR_ID++, Derived, dimension, t1.getExpression(), t2.getExpression())};
                }

                friend Tensor relu(Tensor const & t) {
                    return Tensor{make_shared<Expression>(ElementwiseOperator{Relu{}}, EXPR_ID++, Derived, t.getDimensions(), t.getExpression())};
                }

                friend Tensor sigmoid(Tensor const & t) {
                    return Tensor{make_shared<Expression>(ElementwiseOperator{Sigmoid{}}, EXPR_ID++, Derived, t.getDimensions(), t.getExpression())};
                }
            };

            template<typename T>
            void matmul(Tensor &result, Tensor const &weights, Tensor &in) {
            }

            class PrettyPrinter {
            public:

                static void print(Expression const & expression) {
                    cout << "( ";
                    std::visit(Print{}, expression.getExpressionType());
                    for (auto && op : expression.getOperands()) {
                        print(op);
                    }
                    cout << " )";
                }

                static void print(Tensor const & tensor) {
                    print(tensor.getExpression());
                    cout << "\n";
                }
            };

            class DatatypeDeriver {
            public:
                static void deriveDatatypes(Expression & expression) {
                    expression.setDataType(Float);
                    for (Expression & e : expression.getOperands()) {
                        deriveDatatypes(e);
                    }
                }

                static void deriveDatatypes(Tensor & tensor) {
                    deriveDatatypes(tensor.getExpression());
                }
            };

            size_t product(vector<int> const & v) {
                size_t ret = 1;
                for (int i : v) {
                    ret *= i;
                }
                return ret;
            }

            class TemporaryAllocator {
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

            class Initializer {
            public:
                static void initializeRecurse(Expression const & expression) {
                    if (Value const * v = get_if<Value>(&expression.getExpressionType())) {
                        OutputBuffer & ob = outputBuffers.at(expression.getId());
                        float * weights = static_cast<float*>(ob.ptr);
                        for (int i = 0; i < product(expression.getDimensions()); ++i) {
                            weights[i] = 1.0;
                        }
                    }
                    for (Expression const & e : expression.getOperands()) {
                        initializeRecurse(e);
                    }
                }

                static void initialize(Tensor const & tensor) {
                    initializeRecurse(tensor.getExpression());
                }
            };

            class Evaluator {
                int batchSize = 0;
                vector<Expression> terminalNodes;

            public:
                Evaluator(Tensor const & t)
                    : terminalNodes{t.getExpression()}
                {
                }

                template<typename... Ts>
                requires (std::same_as<Tensor, std::remove_cvref_t<Ts>> && ...)
                Evaluator(Ts&&... ts)
                        : terminalNodes{(ts.getExpression(), ...)}
                {
                }

                void evaluate() {
                }

                void computeAggregateSlices(Expression const & e, OutputBuffer & outBuffer, vector<int> const & slices, bool preFilled = false) {
                    vector<int> const & dimensions = e.getDimensions();
                    OutputBuffer & weightBuffer = outputBuffers.at(e.getOperands().front().getId());
                    float * emb = static_cast<float*>(weightBuffer.ptr);
                    float * out = static_cast<float*>(outBuffer.ptr);
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
                    float * partial = static_cast<float*>(partialBuffer.ptr);
                    float * out = static_cast<float*>(outBuffer.ptr);
                    memcpy(out, partial, numElements);
                }

                void partialInRecurse(Expression const & e, SliceList const & sliceList, vector<int> const & slices) {
                    if (AggregateSlices const * as = get_if<AggregateSlices>(&e.getExpressionType())) {
                        if (sliceList.getId() == as->getSliceList().getId()) {
                            TemporaryAllocator::findExprAllocations(e, partialBuffers, 1);
                            OutputBuffer & pb = partialBuffers.at(e.getId());
                            computeAggregateSlices(e, pb, slices);
                            cout << "Do the slice into " << pb << "\n";
                        }
                    }
                    for (Expression const & subExpr : e.getOperands()) {
                        partialInRecurse(subExpr, sliceList, slices);
                    }
                }

                void partialIn(SliceList const & sliceList, vector<int> const & slices) {
                    for (Expression const & e : terminalNodes) {
                        partialInRecurse(e, sliceList, slices);
                    }
                }

                void inRecurse(Expression const & e, SliceList const & sliceList, vector<int> const & slices) {
                    if (AggregateSlices const * as = get_if<AggregateSlices>(&e.getExpressionType())) {
                        if (sliceList.getId() == as->getSliceList().getId()) {
                            TemporaryAllocator::findExprAllocations(e, outputBuffers, 1);
                            OutputBuffer & ob = outputBuffers.at(e.getId());
                            auto pb = partialBuffers.get(e.getId());
                            bool preFilled = pb.has_value();
                            if (preFilled) {
                                copyPartialToOutput(e, pb.value(), ob, 1);
                            }
                            computeAggregateSlices(e, ob, slices, preFilled);
                            cout << "Do the slice into " << ob << "\n";
                        }
                    }
                    for (Expression const & subExpr : e.getOperands()) {
                        inRecurse(subExpr, sliceList, slices);
                    }
                }

                void in(SliceList const & sliceList, vector<int> const & slices) {
                    for (Expression const & e : terminalNodes) {
                        inRecurse(e, sliceList, slices);
                    }
                }

                void setBatchSize(int batchSize) {
                    this->batchSize = batchSize;
                    for (Expression const & expression : terminalNodes) {
                        TemporaryAllocator::findAllocations(expression, 64);
                    }
                }

                void resultRecurse(Expression const & e) {
                    for (Expression const & op : e.getOperands()) {
                        resultRecurse(op);
                    }

                }

                vector<void *> result() {
                    vector<void *> ret;
                    for (Expression const & expression : terminalNodes) {
                        resultRecurse(expression);
                        OutputBuffer & ob = outputBuffers.at(expression.getId());
                        ret.push_back(ob.ptr);
                    }
                    return ret;
                }

                void * result(Tensor const & tensor) {
                    return nullptr;
                }

                void * result(string name) {
                    return nullptr;
                }
            };

            void * getPartialPtr(Tensor const & tensor) {
                auto ob = partialBuffers.get(tensor.getExpression().getId());
                return ob.has_value() ? ob.value().ptr : nullptr;
            }

            void * getOutputPtr(Tensor const & tensor) {
                auto ob = outputBuffers.get(tensor.getExpression().getId());
                return ob.has_value() ? ob.value().ptr : nullptr;
            }
        }
    }
}

int main(int argc, char ** argv) {
    using namespace gradylib::nn::expression;

    auto addPerceptron = []<typename T> requires std::same_as<Tensor, std::remove_cvref_t<T>>(int size, T && input) {
        Tensor weights(size, input.firstDimension());
        Tensor bias(size);
        return weights * input + bias;
    };
    Tensor weights(50, 20), bias(50);
    Tensor input(20), input2(20);
    //Tensor<float> out = weights * input + bias;
    Tensor out = addPerceptron(50, input);

    PrettyPrinter::print(out);

    Tensor t3(50, 20, 20);

    auto out2a = t3.contract({1, 2}, input, input2);
    auto out3 = dot(out, out2a);

    auto out4 = relu(out3);

    auto out5 = out4;

    SliceList sliceList("in");
    Tensor embedding(90, 120000);
    Tensor emb = embedding.aggregateSlices(sliceList);
    auto l1_1 = relu(addPerceptron(64, emb));
    auto l2_1 = relu(addPerceptron(32, l1_1));
    auto out1 = sigmoid(addPerceptron(1, l2_1));
    out1.setName("out");

    PrettyPrinter::print(out1);
    DatatypeDeriver::deriveDatatypes(out1);
    TemporaryAllocator::findAllocations(out1, 64);
    Initializer::initialize(out1);
    Evaluator evaluator(out1);
    evaluator.setBatchSize(64);
    evaluator.partialIn(sliceList, vector<int>{1, 2, 3});
    evaluator.in(sliceList, vector<int>{4, 5, 6, 7});
    float * pptr = static_cast<float*>(getPartialPtr(emb));
    float * optr = static_cast<float*>(getOutputPtr(emb));
    //evaluator.in("in", vector<int>{});
    evaluator.result();
    //evaluator.result(out1);
    //evaluator.result("out");


    for (auto && [id, ob ] : outputBuffers) {
        cout << id << " " << ob << " ";
        std::visit(Print{}, tensors[id]->getExpressionType());
        cout << "\n";
    }

    auto l1_2 = relu(addPerceptron(64, embedding.aggregateSlices(sliceList)));
    auto l2_2 = relu(addPerceptron(32, l1_2));
    auto out2 = sigmoid(addPerceptron(1, l2_2));

    return 0;
}

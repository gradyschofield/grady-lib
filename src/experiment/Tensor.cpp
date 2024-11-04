#include<any>
#include<cassert>
#include<iostream>
#include<unordered_map>
#include<variant>
#include<vector>

using namespace std;

namespace expression {

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

    class SliceList {};

    class Relu{};
    class Sigmoid{};
    class Add {};

    using ElementwiseFunction = std::variant<Relu, Sigmoid>;
    using BinaryFunction = std::variant<Add>;

    class Value{
        DataType dtype;
        vector<int> dimensions;
    public:


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
    class AggregateSlices{
        SliceList sliceList;
    public:
        AggregateSlices(SliceList sliceList) : sliceList(sliceList) {}
    };
    class Undefined{};

    struct Print {
        void operator()(Value const & v) { cout << "Value "; }
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
        Undefined
    >;

    class Expression {
        ExpressionType expressionType = Undefined{};
        vector<Expression> operands;
    public:

        ExpressionType const & getExpressionType() const {
            return expressionType;
        }

        vector<Expression> const & getOperands() const {
            return operands;
        }

        template<typename... Args>
        Expression(ExpressionType expressionType, Args... args)
            : expressionType(expressionType), operands{args...}
        {
        }
    };

    class TensorImpl;

    unordered_map<int, shared_ptr<TensorImpl>> tensors;

    class TensorImpl {
        Expression expression;
        int id;
        DataType dataType;
        vector<int> dimensions;

    public:
        TensorImpl(Expression expression, int id, DataType dataType)
                : expression(expression), id(id), dataType(dataType)
        {
        }

        TensorImpl(Expression expression, int id, DataType dataType, vector<int> dimensions)
            : expression(expression), id(id), dataType(dataType), dimensions(dimensions)
        {
        }

        Expression getExpression() const {
            return expression;
        }

        int firstDimension() const {
            return dimensions.front();
        }

        int lastDimension() const {
            return dimensions.back();
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
    };

    class Tensor {

        shared_ptr<TensorImpl> impl;
        inline static int TENSOR_ID = 0;

        void addToTable() {
            tensors.emplace(impl->getId(), impl);
        };

    public:

        Expression getExpression() const {
            return impl->getExpression();
        }

        int firstDimension() const {
            return impl->firstDimension();
        }

        int lastDimension() const {
            return impl->lastDimension();
        }

        template<typename... Sizes>
        requires (std::is_integral_v<Sizes> && ...)
        Tensor(DataType dataType, Sizes... sizes)
            : impl(make_shared<TensorImpl>(Value{}, TENSOR_ID++, dataType))
        {
            (impl->getDimensions().push_back(sizes), ...);
        }

        template<typename... Sizes>
        requires (std::is_integral_v<Sizes> && ...)
        Tensor(Sizes... sizes)
                : impl(make_shared<TensorImpl>(Expression{Value{}}, TENSOR_ID++, Float))
        {
            (impl->getDimensions().push_back(sizes), ...);
        }

        template<typename... Sizes>
        requires (std::is_integral_v<Sizes> && ...)
        Tensor(Expression expression, Sizes... sizes)
            : impl(make_shared<TensorImpl>(expression, TENSOR_ID++, Derived))
        {
            (impl->getDimensions().push_back(sizes), ...);
        }

        template<typename... Sizes>
        requires (std::is_integral_v<Sizes> && ...)
        Tensor(Expression expression, vector<int> dimensions)
            : impl(make_shared<TensorImpl>(expression, TENSOR_ID++, Derived, dimensions))
        {
        }

        int ndim() const {
            return impl->getDimensions().size();
        }

        vector<int> const & getDimensions() const {
            return impl->getDimensions();
        }

        vector<int> & getDimensions() {
            return impl->getDimensions();
        }

        Tensor operator+(Tensor const &t) {
            assert(getDimensions() == t.getDimensions());
            return Tensor{
                    {BinaryOperator{Add{}}, this->getExpression(), t.getExpression()},
                    impl->getDimensions()
            };
        }

        template<typename... Ts>
        requires (std::is_same_v<Ts, Tensor> && ...)
        Tensor contract(vector<size_t> axis, Ts... ts) {
            vector<vector<int>> tmpDim;
            (tmpDim.push_back(ts.getDimensions()), ...);
            vector<int> const & thisDim = impl->getDimensions();
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
            return Tensor{{Contraction{}, this->getExpression(), (ts.getExpression(), ...)}, retDim};
        }

        Tensor aggregateSlices(SliceList sliceList) {
            auto resultDim = getDimensions();
            resultDim.pop_back();
            return Tensor{{AggregateSlices{sliceList}, this->getExpression()}, resultDim};
        }

        Tensor operator*(Tensor const &t) {
            return contract({impl->getDimensions().size()-1}, t);
        }

        friend Tensor dot(Tensor const &t1, Tensor const &t2) {
            assert(t1.getDimensions().size() <= 2);
            assert(t1.getDimensions() == t2.getDimensions());
            return Tensor{{Contraction{}, t1.getExpression(), t2.getExpression()}, (t1.getDimensions().size() == 2 ? t1.getDimensions()[1] : 1)};
        }

        friend Tensor relu(Tensor const & t) {
            return Tensor{{ElementwiseOperator{Relu{}}, t.getExpression()}, t.getDimensions()};
        }

        friend Tensor sigmoid(Tensor const & t) {
            return Tensor{{ElementwiseOperator{Sigmoid{}}, t.getExpression()}, t.getDimensions()};
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
}

int main(int argc, char ** argv) {
    using namespace expression;

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

    SliceList sliceList;
    Tensor embedding(90, 120000);
    auto l1_1 = relu(addPerceptron(64, embedding.aggregateSlices(sliceList)));
    auto l2_1 = relu(addPerceptron(32, l1_1));
    auto out1 = sigmoid(addPerceptron(1, l2_1));

    PrettyPrinter::print(out1);

    auto l1_2 = relu(addPerceptron(64, embedding.aggregateSlices(sliceList)));
    auto l2_2 = relu(addPerceptron(32, l1_2));
    auto out2 = sigmoid(addPerceptron(1, l2_2));

    return 0;
}

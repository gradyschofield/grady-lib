#include"DataTypeDeriver.hpp"
#include"PrettyPrinter.hpp"
#include"BufferAllocator.hpp"
#include"Initializer.hpp"
#include"Evaluator.hpp"
#include"Tensor.hpp"

using namespace std;

int main(int argc, char ** argv) {
    using namespace gradylib::nn;

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
    auto out1 = addPerceptron(1, l2_1);
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
    float * optr1= static_cast<float*>(getOutputPtr(l1_1));
    float * optr2 = static_cast<float*>(getOutputPtr(l2_1));
    float * outptr2 = static_cast<float*>(getOutputPtr(out1));
    //evaluator.in("in", vector<int>{});
    vector<void *> ret = evaluator.result();
    cout << *pptr << " " << *optr << " " << *optr1 << " " << *optr2 << " " << *outptr2 << "\n";
    cout << "Result: " << *static_cast<float*>(ret[0]) << "\n";

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

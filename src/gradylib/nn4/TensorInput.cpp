//
// Created by Grady Schofield on 11/15/24.
//
#include"gradylib/nn4/activation/Scalarati.hpp"
#include"Evaluator.hpp"
#include"Initializer.hpp"
#include"PrettyPrinter.hpp"
#include"Tensor.hpp"
#include"Trainer.hpp"

using namespace std;

int main(int argc, char ** argv) {
    using namespace gradylib::nn;

    auto addPerceptron = []<typename T> requires std::same_as<Tensor, std::remove_cvref_t<T>>(int size, T && input) {
        Tensor weights(size, input.firstDimension());
        Tensor bias(size);
        return weights * input + bias;
    };

    SliceList sliceList("in");
    Tensor embedding(64, 64);
    Tensor emb = embedding.aggregateSlices(sliceList);
    auto l1_1 = relu(addPerceptron(64, emb));
    auto l2_1 = relu(addPerceptron(32, l1_1));
    auto out1 = sigmoid(addPerceptron(1, l2_1));
    out1.setName("out");

    PrettyPrinter::print(out1);
    Evaluator evaluator(out1);
    Initializer::initialize(evaluator, out1);
    evaluator.setBatchSize(64);
    evaluator.partialIn(sliceList, vector<int>{1, 2, 3});
    evaluator.in(sliceList, vector<int>{4, 5, 6, 7});
    float * pptr = static_cast<float*>(evaluator.getPartialPtr(emb));
    float * optr = static_cast<float*>(evaluator.getOutputPtr(emb));
    float * optr1 = static_cast<float*>(evaluator.getOutputPtr(l1_1));
    float * optr2 = static_cast<float*>(evaluator.getOutputPtr(l2_1));
    float * outptr2 = static_cast<float*>(evaluator.getOutputPtr(out1));
    //evaluator.in("in", vector<int>{});
    vector<void *> ret = evaluator.result();
    cout << *pptr << " " << *optr << " " << *optr1 << " " << *optr2 << " " << *outptr2 << "\n";
    cout << "Result: " << *static_cast<float*>(ret[0]) << "\n";

    Trainer trainer;
    {
        using namespace gradylib::nn::activation;
        trainer = Trainer(out1, (1-y)*log(1-p) + y*log(p));
    }
    trainer.train({1, 0, 1, 1}, {
            {1,1,3,4},
            {1,1,3,4}
    });


    return 0;
}

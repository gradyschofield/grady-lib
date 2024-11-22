
#include<catch2/catch_test_macros.hpp>

#include<vector>

#include<gradylib/nn4/Tensor.hpp>
#include<gradylib/nn4/Initializer.hpp>
#include<gradylib/nn4/Evaluator.hpp>

using namespace std;

TEST_CASE("Basic MLP"){
    using namespace gradylib::nn;

    auto addPerceptron = []<typename T> requires std::same_as<Tensor, std::remove_cvref_t<T>>(int size, T && input) {
        Tensor weights(size, input.firstDimension());
        Tensor bias(size);
        return weights * input + bias;
    };

    SliceList sliceList("in");
    Tensor embedding(90, 120000);
    Tensor emb = embedding.aggregateSlices(sliceList);
    auto l1_1 = relu(addPerceptron(64, emb));
    auto l2_1 = relu(addPerceptron(32, l1_1));
    auto out1 = addPerceptron(1, l2_1);

    Evaluator evaluator(out1);
    Initializer::initialize(evaluator, out1);
    evaluator.setBatchSize(64);
    evaluator.partialIn(sliceList, vector<int>{1, 2, 3});
    evaluator.in(sliceList, vector<int>{4, 5, 6, 7});
    vector<void *> ret = evaluator.result();
    float * pptr = static_cast<float*>(evaluator.getPartialPtr(emb));
    for (int i = 0; i < 90; ++i) {
        REQUIRE(pptr[i] == 3.0f);
    }
    float * optr = static_cast<float*>(evaluator.getOutputPtr(emb));
    for (int i = 0; i < 90; ++i) {
        REQUIRE(optr[i] == 7);
    }
    float * optr1= static_cast<float*>(evaluator.getOutputPtr(l1_1));
    for (int i = 0; i < 64; ++i) {
        REQUIRE(optr1[i] == 631);
    }
    float * optr2 = static_cast<float*>(evaluator.getOutputPtr(l2_1));
    for (int i = 0; i < 32; ++i) {
        REQUIRE(optr2[i] == 40385);
    }
    float * outptr = static_cast<float*>(evaluator.getOutputPtr(out1));
    for (int i = 0; i < 1; ++i) {
        REQUIRE(outptr[i] == 1292321.0f);
    }
}

TEST_CASE("Multi out MLP"){
    using namespace gradylib::nn;

    auto addPerceptron = []<typename T> requires std::same_as<Tensor, std::remove_cvref_t<T>>(int size, T && input) {
        Tensor weights(size, input.firstDimension());
        Tensor bias(size);
        return weights * input + bias;
    };

    SliceList sliceList("in");
    Tensor embedding(90, 120000);
    Tensor emb = embedding.aggregateSlices(sliceList);
    auto l1_1 = relu(addPerceptron(64, emb));
    auto l2_1 = relu(addPerceptron(32, l1_1));
    auto out1 = addPerceptron(1, l2_1);

    auto l1_2 = relu(addPerceptron(64, emb));
    auto l2_2 = relu(addPerceptron(32, l1_2));
    auto out2 = addPerceptron(1, l2_2);

    Evaluator evaluator(out1, out2);
    Initializer::initialize(evaluator, out1);
    Initializer::initialize(evaluator, out2);
    evaluator.setBatchSize(64);
    evaluator.partialIn(sliceList, vector<int>{1, 2, 3});
    evaluator.in(sliceList, vector<int>{4, 5, 6, 7});
    vector<void *> ret = evaluator.result();
    float * pptr = static_cast<float*>(evaluator.getPartialPtr(emb));
    for (int i = 0; i < 90; ++i) {
        REQUIRE(pptr[i] == 3.0f);
    }
    float * optr = static_cast<float*>(evaluator.getOutputPtr(emb));
    for (int i = 0; i < 90; ++i) {
        REQUIRE(optr[i] == 7);
    }
    float * optr1= static_cast<float*>(evaluator.getOutputPtr(l1_1));
    for (int i = 0; i < 64; ++i) {
        REQUIRE(optr1[i] == 631);
    }
    float * optr2 = static_cast<float*>(evaluator.getOutputPtr(l2_1));
    for (int i = 0; i < 32; ++i) {
        REQUIRE(optr2[i] == 40385);
    }
    float * outptr = static_cast<float*>(evaluator.getOutputPtr(out1));
    for (int i = 0; i < 1; ++i) {
        REQUIRE(outptr[i] == 1292321.0f);
    }
}

TEST_CASE("Merge MLP"){
    using namespace gradylib::nn;

    auto addPerceptron = []<typename... Ts>
            requires (std::same_as<Tensor, std::remove_cvref_t<Ts>> && ...)
            (int size, Ts&&... inputs) {
        size_t inputDim = (inputs.firstDimension() + ...);
        Tensor weights(size, inputDim);
        Tensor bias(size);
        return weights * concatenate(inputs...) + bias;
    };

    SliceList sliceList("in");
    Tensor embedding(90, 120000);
    Tensor emb = embedding.aggregateSlices(sliceList);
    auto l1_1 = relu(addPerceptron(64, emb));
    auto l2_1 = relu(addPerceptron(32, l1_1));
    auto out1 = addPerceptron(1, l2_1, l1_1);


    Evaluator evaluator(out1);
    Initializer::initialize(evaluator, out1);
    evaluator.setBatchSize(64);
    evaluator.partialIn(sliceList, vector<int>{1, 2, 3});
    evaluator.in(sliceList, vector<int>{4, 5, 6, 7});
    vector<void *> ret = evaluator.result();
    float * pptr = static_cast<float*>(evaluator.getPartialPtr(emb));
    for (int i = 0; i < 90; ++i) {
        REQUIRE(pptr[i] == 3.0f);
    }
    float * optr = static_cast<float*>(evaluator.getOutputPtr(emb));
    for (int i = 0; i < 90; ++i) {
        REQUIRE(optr[i] == 7);
    }
    float * optr1= static_cast<float*>(evaluator.getOutputPtr(l1_1));
    for (int i = 0; i < 64; ++i) {
        REQUIRE(optr1[i] == 631);
    }
    float * optr2 = static_cast<float*>(evaluator.getOutputPtr(l2_1));
    for (int i = 0; i < 32; ++i) {
        REQUIRE(optr2[i] == 40385);
    }
    float * outptr = static_cast<float*>(evaluator.getOutputPtr(out1));
    for (int i = 0; i < 1; ++i) {
        REQUIRE(outptr[i] == 1332705.0f);
    }
}

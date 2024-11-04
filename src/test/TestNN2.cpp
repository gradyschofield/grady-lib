//
// Created by Grady Schofield on 10/29/24.
//

#include<gradylib/nn2/Embedding.hpp>
#include<gradylib/nn2/Evaluator.hpp>
#include<gradylib/nn2/FeatureIndexLookup.hpp>
#include<gradylib/nn2/MergeLayer.hpp>
#include<gradylib/nn2/Linear.hpp>

using namespace gradylib::nn;
using namespace std;

int main(int argc, char ** argv) {
    Embedding inputEmbedding1(1000, 96);
    Embedding inputEmbedding2(1000, 96);
    MergeLayer x{inputEmbedding1, inputEmbedding2};

    Linear layer1(96 * 2, 64);
    Linear layer2(64, 32);

    MergeLayer layer3(layer1, layer2);

    Linear layer4(layer3, {1, Sigmoid});

    auto x1 = inputEmbedding1(??);
    auto x2 = inputEmbedding2(??);
    auto y = MergeLayer{x1, x2};


    Evaluator evaluator = layer4.evaluator()
            .allowPartial(inputEmbedding1)  // This is actually not needed.  We can wait for the first call to partialIn
            .isStatic(inputEmbedding2); // Helps figure the temporaries necessary
    vector<int> embeddingIdx0;
    vector<int> embeddingIdx1;
    vector<int> embeddingIdx2;
    evaluator.partialIn(inputEmbedding1, embeddingIdx0);
    evaluator.in(inputEmbedding1, embeddingIdx1)
            .in(inputEmbedding2, embeddingIdx2)
            .evaluate();

    evaluator.inputEmbedding(embeddingIdx1)
            .inputEmedding2(embeddingIdx2)
            .backprop(targets);

}
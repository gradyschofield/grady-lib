//
// Created by Grady Schofield on 8/10/24.
//

#include<vector>

#include<gradylib/nn/Embedding.hpp>
#include<gradylib/nn/FeatureIndexLookup.hpp>
#include<gradylib/nn/MergeLayer.hpp>
#include<gradylib/nn/Linear.hpp>
#include<gradylib/nn/TrainingOptions.hpp>
#include<gradylib/nn/TrainingReport.hpp>

using namespace gradylib::nn;
using namespace std;

int main(int argc, char ** argv) {
    //CsvTrainingFiles csvFiles(list<string>);
    //FeatureIndexLookup featureIndexLookup("modelSpec.json");
    FeatureIndexLookup featureIndexLookup(1000);
    Embedding inputEmbedding1(featureIndexLookup, 96);
    Embedding inputEmbedding2(featureIndexLookup, 96);
    MergeLayer x{inputEmbedding1, inputEmbedding2};
    Linear layer1(x, 64);
    Linear layer2(layer1, 32);
    MergeLayer layer3(layer1, layer2);
    Linear layer4(layer3, {1, Sigmoid});

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

    /*
    Target target("modelSpec.json");


     */
    TrainingOptions trainingOptions("modelSpec.json");

    //layer4.train(target, trainingOptions);

    return 0;
}
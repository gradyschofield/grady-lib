//
// Created by Grady Schofield on 8/10/24.
//

#include<gradylib/nn/Embedding.hpp>
#include<gradylib/nn/FeatureIndexLookup.hpp>
#include<gradylib/nn/MergeLayer.hpp>
#include<gradylib/nn/MLP.hpp>

using namespace gradylib::nn;

int main(int argc, char ** argv) {
    //CsvTrainingFiles csvFiles(list<string>);
    //FeatureIndexLookup featureIndexLookup("modelSpec.json");
    FeatureIndexLookup featureIndexLookup(1000);
    Embedding inputEmbedding(featureIndexLookup, 96);
    MLP layer1(inputEmbedding, 64);
    MLP layer2(layer1,32);
    MergeLayer layer3(layer1, layer2);
    MLP layer4(layer3, {1, Sigmoid});

    /*
    Target target("modelSpec.json");

    TrainingOptions trainingOptions("modelSpec.json");
    trainingOptions.setLearningRate(0.01);

    layer3.train(target, trainingOptions);
     */

    return 0;
}
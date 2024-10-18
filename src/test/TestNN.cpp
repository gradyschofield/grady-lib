//
// Created by Grady Schofield on 8/10/24.
//

#include<gradylib/nn/Embedding.hpp>
#include<gradylib/nn/FeatureIndexLookup.hpp>
#include<gradylib/nn/MergeLayer.hpp>
#include<gradylib/nn/Linear.hpp>
#include<gradylib/nn/TrainingOptions.hpp>
#include<gradylib/nn/TrainingReport.hpp>

using namespace gradylib::nn;

int main(int argc, char ** argv) {
    //CsvTrainingFiles csvFiles(list<string>);
    //FeatureIndexLookup featureIndexLookup("modelSpec.json");
    FeatureIndexLookup featureIndexLookup(1000);
    Embedding inputEmbedding(featureIndexLookup, 96);
    Linear layer1(inputEmbedding, 64);
    Linear layer2(layer1, 32);
    MergeLayer layer3(layer1, layer2);
    Linear layer4(layer3, {1, Sigmoid});

    /*
    Target target("modelSpec.json");


     */
    TrainingOptions trainingOptions("modelSpec.json");

    layer4.train(target, trainingOptions);

    return 0;
}
//
// Created by Grady Schofield on 8/11/24.
//

#include<variant>

#include<gradylib/OpenHashMap.hpp>

#include<gradylib/nn/Embedding.hpp>
#include<gradylib/nn/FeatureIndexLookup.hpp>
#include<gradylib/nn/MergeLayer.hpp>
#include<gradylib/nn/MLP.hpp>
#include<gradylib/nn/TrainingOptions.hpp>
#include<gradylib/nn/TrainingReport.hpp>

using namespace gradylib;
using namespace gradylib::nn;

class CsvTrainingFiles;
class FeatureIndexer;
class Sample;

typedef std::variant<double,
        std::vector<double>,
        int64_t,
        std::vector<int64_t>,
        std::string,
        std::vector<std::string>
        > Feature;

class Sample {
    OpenHashMap<std::string, Feature> features;
};

int main(int argc, char ** argv) {
    CsvTrainingFiles csvFiles(list<string>);
    FeatureIndexer featureIndexer("modelSpec.json");

    auto iter = csvFiles.begin();
    Sample sample = *iter;

    featureIndexer.buildIndex(csvFiles, 10E6);

    return 0;
}

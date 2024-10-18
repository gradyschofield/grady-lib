//
// Created by Grady Schofield on 10/13/24.
//

#include<vector>

#include"Embedding.hpp"
#include"FeatureIndexLookup.hpp"
#include"MergeLayer.hpp"
#include"Linear.hpp"

using namespace std;
using namespace gradylib::nn;

struct Sample {
    vector<int> in;
};
class InputMapper {
public:
    tuple<vector<int>, vector<int>> operator()(Sample const & sample);
} ;

class Model {
public:
    template<typename T>
    Model(T inputMapper){

    }
};

int main(int argc, char ** argv) {
    int embeddingSize = 4000;
    FeatureIndexLookup fil1(embeddingSize);
    Embedding emb1(fil1, 20);
    FeatureIndexLookup fil2(embeddingSize);
    Embedding emb2(fil1, 20);
    MergeLayer merge{emb1, emb2};
    Linear fc1(merge, 64);
    Linear fc2(fc1, 32);
    Linear fc3(fc2, {1, ReLU});

    InputMapper inputMapper{fil1, fil2};
    Model model{inputMapper, fc3};
    model()

    return 0;
}
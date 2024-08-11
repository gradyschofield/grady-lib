//
// Created by Grady Schofield on 8/11/24.
//

#ifndef GRADY_LIB_TRAININGOPTIONS_HPP
#define GRADY_LIB_TRAININGOPTIONS_HPP

#include<string>

#include<gradylib/nn/ComputeBackend.hpp>

namespace gradylib {
    namespace nn {

        class TrainingOptions {
            float learningRate = 0.01;
            ComputeBackendType computeBackendType = Reference;
        public:
            TrainingOptions(std::string filename) {
            }

            TrainingOptions & setLearningRate(float learningRate) {
                this->learningRate = learningRate;
            }

            TrainingOptions & setComputeBackend(ComputeBackendType computeBackendType) {
                this->computeBackendType = computeBackendType;
            }
        };

    }
}

#endif //GRADY_LIB_TRAININGOPTIONS_HPP

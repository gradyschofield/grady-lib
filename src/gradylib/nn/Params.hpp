//
// Created by Grady Schofield on 8/11/24.
//

#ifndef GRADY_LIB_PARAMS_HPP
#define GRADY_LIB_PARAMS_HPP

#include<string>

namespace gradylib {
    namespace nn {
        enum Activation {
            ReLU,
            Sigmoid,
            NoOp
        };

        enum Bias {
            HasBias,
            NoBias
        };

        class Params {
            int numOutputs;
            Activation activation = ReLU;
            Bias bias = HasBias;

        public:
            Params(int numOutputs, Activation activation, Bias bias)
                    : numOutputs(numOutputs), activation(activation), bias(bias)
            {
            }

            Params(int numOutputs, Activation activation)
                    : numOutputs(numOutputs), activation(activation)
            {
            }

            Params(int numOutputs)
                : numOutputs(numOutputs)
            {
            }

            int getNumOutputs() const {
                return numOutputs;
            }

            Activation getActivation() const {
                return activation;
            }
        };
    }
}
#endif //GRADY_LIB_PARAMS_HPP

//
// Created by Grady Schofield on 8/10/24.
//

#ifndef GRADY_LIB_LINEAR_HPP
#define GRADY_LIB_LINEAR_HPP

#include"Layer.hpp"
#include"Params.hpp"

namespace gradylib {
    namespace nn {

        class LinearParams {
            int numOutputs;
            Activation activation = ReLU;
            Bias bias = HasBias;

        public:
            LinearParams(int numOutputs, Activation activation, Bias bias)
                    : numOutputs(numOutputs), activation(activation), bias(bias)
            {
            }

            LinearParams(int numOutputs, Activation activation)
                    : numOutputs(numOutputs), activation(activation)
            {
            }

            LinearParams(int numOutputs)
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

        template<typename WeightType = float, typename BiasType = WeightType>
        class Linear : public Layer {

        public:

            typedef WeightType weight_type;
            typedef BiasType bias_type;

            /*
            Linear(int numInputs, int numOutputs)
                : impl(std::make_shared<LinearImpl>(numInputs, numOutputs))
            {
            }
             */

            Linear(Node const & input, LinearParams params)
                : impl(std::make_shared<LinearImpl>(input, params.getNumOutputs()))
            {
            }

            int getNumOutputs() const override {
                return impl->getNumOutputs();
            }
        };

    }
}

#endif //GRADY_LIB_LINEAR_HPP

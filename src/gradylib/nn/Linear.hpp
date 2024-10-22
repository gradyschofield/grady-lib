//
// Created by Grady Schofield on 8/10/24.
//

#ifndef GRADY_LIB_LINEAR_HPP
#define GRADY_LIB_LINEAR_HPP

#include<gradylib/nn/Node.hpp>
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
        class Linear : public Node {

            class LinearImpl : public NodeImpl {
                int numInputs;
                int numOutputs;
                std::shared_ptr<NodeImpl const> input;

            public:
                LinearImpl(int numInputs, int numOutputs)
                    : numInputs(numInputs), numOutputs(numOutputs)
                {
                }

                LinearImpl(Node const & input, int numOutputs)
                    : numInputs(input.getNumOutputs()), numOutputs(numOutputs), input(getImpl(input))
                {
                }

                int getNumOutputs() const override {
                    return numOutputs;
                }
            };

            std::shared_ptr<LinearImpl> impl;

            std::shared_ptr<NodeImpl> getImpl() const override {
                return impl;
            }

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

            TrainingReport train(TrainingOptions const & trainingOptions) override {
            }
        };

    }
}

#endif //GRADY_LIB_LINEAR_HPP

//
// Created by Grady Schofield on 8/10/24.
//

#ifndef GRADY_LIB_MLP_HPP
#define GRADY_LIB_MLP_HPP

#include<gradylib/nn/Node.hpp>

namespace gradylib {
    namespace nn {

        template<typename WeightType = float, typename BiasType = WeightType>
        class MLP : public Node {

            class MLPImpl : public NodeImpl {
                int numInputs;
                int numOutputs;
                std::shared_ptr<NodeImpl const> input;

            public:
                MLPImpl(int numInputs, int numOutputs)
                    : numInputs(numInputs), numOutputs(numOutputs)
                {
                }

                MLPImpl(Node const & input, int numOutputs)
                    : numInputs(input.getNumOutputs()), numOutputs(numOutputs), input(getImpl(input))
                {
                }

                int getNumOutputs() const override {
                    return numOutputs;
                }
            };

            std::shared_ptr<MLPImpl> impl;

            std::shared_ptr<NodeImpl> getImpl() const override {
                return impl;
            }

        public:

            typedef WeightType weight_type;
            typedef BiasType bias_type;

            /*
            MLP(int numInputs, int numOutputs)
                : impl(std::make_shared<MLPImpl>(numInputs, numOutputs))
            {
            }
             */

            MLP(Node const & input, Params params)
                : impl(std::make_shared<MLPImpl>(input, params.getNumOutputs()))
            {
            }

            int getNumOutputs() const override {
                return impl->getNumOutputs();
            }
        };

    }
}

#endif //GRADY_LIB_MLP_HPP

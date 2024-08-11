//
// Created by Grady Schofield on 8/11/24.
//

#ifndef GRADY_LIB_MERGELAYER_HPP
#define GRADY_LIB_MERGELAYER_HPP

#include<memory>
#include<vector>

#include<gradylib/nn/Node.hpp>
#include<gradylib/nn/Params.hpp>

namespace gradylib {
    namespace nn {

        template<typename WeightType = float, typename BiasType = WeightType>
        class MergeLayer : public Node {
            class MergeLayerImpl : public NodeImpl {
                int numInputs;
                int numOutputs;
                std::vector<std::shared_ptr<NodeImpl const>> input;

            public:

                template<typename ...Args>
                MergeLayerImpl(Args... input)
                        : input{getImpl(input)...}
                {
                    for (auto && n : MergeLayerImpl::input) {
                        numInputs += n->getNumOutputs();
                    }
                    numOutputs = numInputs;
                }

                int getNumOutputs() const override {
                    return numOutputs;
                }

            };

            std::shared_ptr<MergeLayerImpl> impl;

            std::shared_ptr<NodeImpl> getImpl() const override {
                return impl;
            }

        public:

            template<typename ...Args>
            MergeLayer(Args... t)
                : impl(std::make_shared<MergeLayerImpl>(t...))
            {
            }

            int getNumOutputs() const override {
                return impl->getNumOutputs();
            }
        };
    }
}

#endif //GRADY_LIB_MERGELAYER_HPP

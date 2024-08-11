//
// Created by Grady Schofield on 8/10/24.
//

#ifndef GRADY_LIB_EMBEDDING_HPP
#define GRADY_LIB_EMBEDDING_HPP

#include<gradylib/nn/Node.hpp>

namespace gradylib {
    namespace nn {

        template<typename WeightType = float, typename BiasType = WeightType>
        class Embedding : public Node {
            class EmbeddingImpl : public NodeImpl {
                int numInputs;
                int numOutputs;
                std::shared_ptr<NodeImpl const> input;

            public:

                template<IndexGenerator T>
                EmbeddingImpl(T const & input, int numOutputs)
                    : numInputs(input.getNumOutputs()), numOutputs(numOutputs), input(getImpl(input))
                {
                }

                int getNumOutputs() const override {
                    return numOutputs;
                }

            };

            std::shared_ptr<EmbeddingImpl> impl;

            std::shared_ptr<NodeImpl> getImpl() const override {
                return impl;
            }

        public:

            template<IndexGenerator T>
            Embedding(T const & t, int numOutputs)
                : impl(std::make_shared<EmbeddingImpl>(t, numOutputs))
            {
            }

            int getNumOutputs() const override {
                return impl->getNumOutputs();
            }
        };
    }
}
#endif //GRADY_LIB_EMBEDDING_HPP

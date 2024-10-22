//
// Created by Grady Schofield on 8/10/24.
//

#ifndef GRADY_LIB_EMBEDDING_HPP
#define GRADY_LIB_EMBEDDING_HPP

#include<iostream>
#include<sstream>
#include<variant>

#include<gradylib/nn/Node.hpp>
#include"Params.hpp"
#include<gradylib/Exception.hpp>

namespace gradylib {
    namespace nn {

        class EmbeddingParams {
            Bias bias = NoBias;

        public:
            EmbeddingParams() {
            }

            Bias getBias() const {
                return bias;
            }
        };

        template<typename WeightType = float, typename BiasType = WeightType>
        class Embedding : public Node {
            class EmbeddingImpl : public NodeImpl {
                int64_t numInputs;
                int64_t numOutputs;
                EmbeddingParams embeddingParams;
                LayoutParams layoutParams;
                std::shared_ptr<NodeImpl const> input;
                int64_t leadingDimension;
                WeightType * weights = nullptr;
                BiasType * bias = nullptr;

                void allocateData() {
                    leadingDimension = layoutParams.getLeadingDimension(numOutputs);
                    int ret = posix_memalign(reinterpret_cast<void**>(&weights), 64, leadingDimension * numInputs * sizeof(WeightType));
                    if (!ret) {
                        std::ostringstream ostr;
                        ostr << "Couldn't allocate embedding weights with posix_memalign, leadingDimension: " << leadingDimension << " numInputs: " << numInputs << ".";
                        ostr << "Error message: " << strerror(errno) << ".";
                        gradylibMakeException(ostr.str());
                    }
                    if (embeddingParams.getBias() == HasBias) {
                        int ret = posix_memalign(reinterpret_cast<void**>(&bias), 64, leadingDimension * numInputs * sizeof(BiasType));
                        if (!ret) {
                            std::ostringstream ostr;
                            ostr << "Couldn't allocate embedding bias with posix_memalign, leadingDimension: " << leadingDimension << " numInputs: " << numInputs << ".";
                            ostr << "Error message: " << strerror(errno) << ".";
                            gradylibMakeException(ostr.str());
                        }
                    }
                }

                void initData() {
                    WeightType * t = weights;
                    for (int64_t i = 0; i < numInputs; ++i) {
                        for (int64_t j = 0; j < numOutputs; ++j) {
                            t[j] = rand() / (WeightType)RAND_MAX - 1;
                        }
                        t += leadingDimension;
                    }
                    if (bias) {
                        for (int64_t j = 0; j < numOutputs; ++j) {
                            bias[j] = rand() / (BiasType)RAND_MAX - 1;
                        }
                    }
                }

            public:

                template<IndexGenerator T>
                EmbeddingImpl(T const & input, int numOutputs, EmbeddingParams embeddingParams)
                    : numInputs(input.getNumOutputs()), numOutputs(numOutputs), embeddingParams(embeddingParams), input(getImpl(input))
                {
                    allocateData();
                    initData();
                }

                EmbeddingImpl(int numInputs, int numOutputs, EmbeddingParams embeddingParams)
                        : numInputs(numInputs), numOutputs(numOutputs), embeddingParams(embeddingParams)
                {
                    allocateData();
                    initData();
                }

                int getNumOutputs() const override {
                    return numOutputs;
                }

                WeightType * getWeights() {
                    return weights;
                }

                BiasType * getBias() {
                    return bias;
                }

                int64_t getLeadingDimension() {
                    return leadingDimension;
                }

                ~EmbeddingImpl() {
                    free(weights);
                    free(bias);
                }
            };

            std::shared_ptr<EmbeddingImpl> impl;

            std::shared_ptr<NodeImpl> getImpl() const override {
                return impl;
            }

        public:

            template<IndexGenerator T>
            Embedding(T const & t, int numOutputs, EmbeddingParams embeddingParams = EmbeddingParams{})
                : impl(std::make_shared<EmbeddingImpl>(t, numOutputs, embeddingParams))
            {
            }

            Embedding(int numInputs, int numOutputs, EmbeddingParams embeddingParams = EmbeddingParams{})
                    : impl(std::make_shared<EmbeddingImpl>(numInputs, numOutputs, embeddingParams))
            {
            }

            int getNumOutputs() const override {
                return impl->getNumOutputs();
            }

            WeightType * getWeights() {
                return impl->getWeights();
            }

            BiasType * getBias() {
                return impl->getBias();
            }

            int64_t getLeadingDimension() {
                return impl->getLeadingDimension();
            }


            TrainingReport train(TrainingOptions const & trainingOptions) override {
                throw std::runtime_error("Embedding::train not implemented");
            }
        };
    }
}
#endif //GRADY_LIB_EMBEDDING_HPP

//
// Created by Grady Schofield on 8/10/24.
//

#ifndef GRADY_LIB_FEATUREINDEXLOOKUP_HPP
#define GRADY_LIB_FEATUREINDEXLOOKUP_HPP

#include<gradylib/nn/Node.hpp>

namespace gradylib {
    namespace nn {
        class FeatureIndexLookup : public Node {
            class FeatureIndexLookupImpl : public NodeImpl {
                int numTerms;
            public:
                FeatureIndexLookupImpl(int numTerms)
                    : numTerms(numTerms)
                {
                }

                int getNumOutputs() const override {
                    return numTerms;
                }
            };

            std::shared_ptr<FeatureIndexLookupImpl> impl;

            std::shared_ptr<NodeImpl> getImpl() const override {
                return impl;
            }

        public:

            FeatureIndexLookup(int numTerms)
                : impl(std::make_shared<FeatureIndexLookupImpl>(numTerms))
            {
            }

            std::vector<int> operator()() const {
                return {};
            }

            int getNumOutputs() const override {
                return impl->getNumOutputs();
            }
        };
    }
}

#endif //GRADY_LIB_FEATUREINDEXLOOKUP_HPP

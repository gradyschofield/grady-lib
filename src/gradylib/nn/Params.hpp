//
// Created by Grady Schofield on 8/11/24.
//

#ifndef GRADY_LIB_PARAMS_HPP
#define GRADY_LIB_PARAMS_HPP

#include<string>
#include<variant>

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

        enum LeadingDimensionConfig {
            CacheLine,
            VectorWidth,
            NoPad
        };

        class LayoutParams {
            std::variant<int64_t, LeadingDimensionConfig> leadingDimension = NoPad;

        public:

            int64_t getLeadingDimension(int numOutputs) const {
                // TODO implement vector and cacheline ld computation
                return numOutputs;
            }
        };

    }
}
#endif //GRADY_LIB_PARAMS_HPP

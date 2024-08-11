//
// Created by Grady Schofield on 8/11/24.
//

#ifndef GRADY_LIB_COMPUTEBACKEND_HPP
#define GRADY_LIB_COMPUTEBACKEND_HPP

namespace gradylib {
    namespace nn {
        enum ComputeBackendType {
            AccelerateFramework,
            BLIS,
            Reference,
            MKL,
            CUDA
        };

        class Buffer {
        };

        class Allocator {
        };

        class Kernels {
        };

        class ComputeBackend {
        };
    }
}

#endif //GRADY_LIB_COMPUTEBACKEND_HPP

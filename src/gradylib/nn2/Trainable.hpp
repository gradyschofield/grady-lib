//
// Created by Grady Schofield on 10/27/24.
//

#pragma once

namespace gradylib {
    namespace nn {
        class Trainable {
        public:
            virtual void backward() = 0;
            virtual ~Trainable(){}
        };
    }
}

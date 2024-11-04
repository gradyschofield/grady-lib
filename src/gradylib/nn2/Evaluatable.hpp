//
// Created by Grady Schofield on 10/27/24.
//

#pragma once

namespace gradylib {
    namespace nn {
        class Evaluatable {
        public:
            virtual void forward() = 0;
            virtual ~Evaluatable(){}
        };
    }
}

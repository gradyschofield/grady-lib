//
// Created by Grady Schofield on 10/26/24.
//

#pragma once

namespace gradylib {
    namespace nn {
        class Node;

        class Evaluator {
        public:
            Evaluator & isStatic(Node & node) {
            }

            template<typename T>
            Evaluator & partialIn(Node & node, T && provider) {
            }

            template<typename T>
            Evaluator & in(Node & node, T && provider) {
            }

            decltype(auto) evaluate() {
            }
        };
    }
}
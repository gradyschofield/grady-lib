//
// Created by Grady Schofield on 10/27/24.
//

#pragma once

#include<vector>
#include<functional>

namespace gradylib {
    namespace nn {
        class Composable {
            std::vector<Composable const *> inputs;
            std::vector<Composable const *> outputs;

            void addOutput(Composable const & c) {
                outputs.push_back(&c);
            }

        public:
            template<typename... Cs>
            requires std::is_base_of_v<Composable, std::remove_cvref_t<Cs>...>
            Composable(Cs && ... composables) {
                (composables.addOutput(*this), ...);
            }

            virtual ~Composable(){}
        };
    }
}

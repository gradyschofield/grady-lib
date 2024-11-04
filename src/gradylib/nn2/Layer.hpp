//
// Created by Grady Schofield on 10/27/24.
//

#pragma once

#include<type_traits>

#include"Evaluatable.hpp"
#include"Trainable.hpp"
#include"Composable.hpp"

namespace gradylib {
    namespace nn {
        class Layer : public Evaluatable, Trainable, Composable {
            template<typename... Layers>
            requires std::is_base_of_v<Composable, Layers...>
            Layer(Layers && ... layers) {

            }
            virtual ~Layer(){}
        };
    }
}

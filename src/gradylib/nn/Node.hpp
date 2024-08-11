//
// Created by Grady Schofield on 8/10/24.
//

#ifndef GRADY_LIB_NODE_HPP
#define GRADY_LIB_NODE_HPP

#include<concepts>
#include<memory>
#include<vector>

#include<gradylib/nn/TrainingReport.hpp>
#include<gradylib/nn/TrainingOptions.hpp>

namespace gradylib {
    namespace nn {
#if 0
        template<typename T>
        concept Node = requires(T n) {
            { n.getNumOutputs() } -> std::convertible_to<int>;
        };
#else
        class NodeImpl;

        class Node {
            virtual std::shared_ptr<NodeImpl> getImpl() const = 0;
        public:
            virtual int getNumOutputs() const = 0;
            virtual TrainingReport train(TrainingOptions const & trainingOptions) = 0;
            virtual ~Node() = default;

            friend NodeImpl;
        };

        class NodeImpl {
        protected:
            std::shared_ptr<NodeImpl> getImpl(Node const & n) {
                return n.getImpl();
            }
        public:
            virtual int getNumOutputs() const = 0;
            virtual ~NodeImpl() = default;
        };


        template<typename T>
        concept IndexGenerator = requires(T n) {
            { n() } -> std::same_as<std::vector<int>>;
            std::is_base_of_v<Node, T>;
        };

#endif

        //typedef std::variant<MLP> Node;
    }
}

#endif //GRADY_LIB_NODE_HPP

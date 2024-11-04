//
// Created by Grady Schofield on 10/29/24.
//
#include<memory>
#include<iostream>

using namespace std;

template<typename T>
class Impl {
    shared_ptr<T> impl;
public:
    Impl() : impl(make_shared<T>()){}
    template<typename... Args>
    Impl(Args&&... args) : impl(make_shared<T>(args...)) {}
    shared_ptr<T> getImpl() { return impl;}
};

class NetworkLayer {
public:
    virtual void forward() = 0;
    virtual void backward() = 0;
    virtual int getNumOutputs() = 0;
};

class NetworkNode {
    vector<shared_ptr<NetworkNode>> inputNodes;
    vector<shared_ptr<NetworkNode>> outputNodes;
    shared_ptr<NetworkLayer> layerImpl;

public:

    void addImpl(shared_ptr<NetworkLayer> const & impl) {
        layerImpl = impl;
    }

    void addInput(shared_ptr<NetworkNode> const & node) {
        inputNodes.push_back(node);
    }

    void addOutput(shared_ptr<NetworkNode> const & node) {
        outputNodes.push_back(node);
    }
};

template<typename T>
struct LayerTraits;

template<typename T, typename U>
requires (std::is_same_v<NetworkLayer, T> || std::is_base_of_v<Impl<T>, U>)
T & as(U & t) {
    if constexpr (std::is_same_v<T, NetworkLayer>) {
        return *t.Impl<typename LayerTraits<std::remove_cv_t<U>>::LayerImpl>::getImpl();
    } else {
        return *t.Impl<T>::getImpl();
    }
}

#define DEFINE_LAYER_CLASS(LAYER_NAME, LAYER_IMPL_NAME, LAYER_PARAMS) \
class LAYER_NAME; \
template<> \
struct LayerTraits<LAYER_NAME> { \
    using LayerImpl = LAYER_IMPL_NAME; \
}; \
    class LAYER_NAME : public Impl<NetworkNode>, public Impl<LAYER_IMPL_NAME> { \
    public: \
\
    LAYER_NAME() = default; \
    template<typename... Args> \
    requires (std::is_base_of_v<Impl<NetworkNode>, std::remove_cvref_t<Args>> && ...) \
    explicit Linear(LAYER_PARAMS params, Args&&... args) \
        : Impl<LAYER_IMPL_NAME>(params, (as<NetworkLayer>(args).getNumOutputs() + ...)) \
    { \
        shared_ptr<NetworkNode> thisNode = Impl<NetworkNode>::getImpl(); \
        thisNode->addImpl(Impl<LAYER_IMPL_NAME>::getImpl()); \
        (thisNode->addInput(args.Impl<NetworkNode>::getImpl()), ...); \
        (as<NetworkNode>(args).addOutput(thisNode), ...); \
    } \
\
    template<typename T> \
    requires std::is_base_of_v<Impl<NetworkNode>, std::remove_cvref_t<T>> \
    explicit Linear( std::initializer_list<T> Cs, LAYER_PARAMS params) \
        : Impl<LAYER_IMPL_NAME>(params, [Cs](){ int x = 0; for ( auto t : Cs) { x += as<NetworkLayer>(t).getNumOutputs();} return x;}()) \
    { \
        shared_ptr<NetworkNode> thisNode = Impl<NetworkNode>::getImpl(); \
        for (T t : Cs) { \
            shared_ptr<NetworkNode> inNode = t.Impl<NetworkNode>::getImpl(); \
            thisNode->addInput(inNode); \
            inNode->addOutput(thisNode); \
        } \
    } \
 \
    template<typename T> \
    requires std::is_base_of_v<Impl<NetworkNode>, std::remove_cvref_t<T>> \
    explicit Linear(T c, LAYER_PARAMS params) \
        : Impl<LAYER_IMPL_NAME>(params, as<NetworkLayer>(c).getNumOutputs()) \
    { \
        shared_ptr<NetworkNode> thisNode = Impl<NetworkNode>::getImpl(); \
        shared_ptr<NetworkNode> inNode = c.Impl<NetworkNode>::getImpl(); \
        thisNode->addInput(inNode); \
        inNode->addOutput(thisNode); \
    } \
};



class LinearParams {
    int a;
public:
    LinearParams(int a) : a(a) {}
};

class LinearLayer : public NetworkLayer {
public:
    LinearLayer() {}
    LinearLayer(LinearParams, int) {}

    void forward() override {
        cout << "Linear::forward\n";
    }

    void backward() override{
    }

    int getNumOutputs() override {
        return 0;
    }
};


DEFINE_LAYER_CLASS(Linear, LinearLayer, LinearParams);

int main(int argc, char ** argv) {
    Linear c, c2, c3;
    NetworkLayer & s2 = as<NetworkLayer>(c);

    Linear d(3, c, c2, c3);
    Linear d2({c, c2, c3}, 3);
    Linear d3(c, 4);


    return 0;
}
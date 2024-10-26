//
// Created by Grady Schofield on 10/26/24.
//

#include<iostream>

using namespace std;

class Unextendable {
    int a;
};

class Backpropable {
public:
    virtual void doBackprop() = 0;
};

template<typename T>
void doBackprop(T * t) {
    t->doBackprop();
}

class UnextendableBackprop : public Backpropable {
    Unextendable * unextendable;
public:
    UnextendableBackprop(Unextendable * unextendable)
        : unextendable(unextendable)
    {
    }

    void doBackprop() override {
        cout << "Doing backprop on unextendable\n";
    }
};

template<typename T>
Backpropable * allocateBackpropable(T * t) {
    if constexpr (std::is_same_v<Unextendable, T>) {
        return new UnextendableBackprop(t);
    } else {
        static_assert(false, "allocateBackpropable needs to be extended for type T");
    }
}

template<typename T>
class BackpropableAdapter {
    decltype(allocateBackpropable(declval<T*>())) t;
public:
    BackpropableAdapter(T & t)
        : t(allocateBackpropable(&t))
    {
    }

    BackpropableAdapter(T * t)
        : t(allocateBackpropable(t))
    {
    }

    void doBackprop() {
        ::doBackprop(t);
    }
};

class UnextendableNotImplemented {
};

int main(int argc, char ** argv) {
    Unextendable unextendable;
    BackpropableAdapter bpAdapter(unextendable);
    bpAdapter.doBackprop();

    return 0;
}
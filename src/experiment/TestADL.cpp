//
// Created by Grady Schofield on 9/27/24.
//
#include<utility>

namespace lib {
    /*
    template<typename ValueType>
    concept viewable_global = requires(std::byte const *ptr) {
        makeView(ptr, std::declval<ValueType *>());
    };
     */
    template<typename ValueType>
    concept viewable_global = requires(std::byte const *ptr) {
        makeView<ValueType>(ptr);
    };

    template<typename V> requires viewable_global<V>
    class C {
    public:
    };
}

struct F {
};

struct FV {
};

//template<typename>
//decltype(auto) makeView(std::byte const * ptr);

template<typename F>
auto makeView(std::byte const * ptr) -> decltype(FV{}) {
    return FV{};
}


int main(int argc, char ** argv) {
    lib::C<F> x;
    return 0;
}
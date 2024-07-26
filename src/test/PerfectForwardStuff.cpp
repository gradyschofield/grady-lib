//
// Created by Grady Schofield on 7/25/24.
//

#include<type_traits>
#include<iostream>
#include<string>

using namespace std;

struct A {
    int x;
    A() {
        cout << "Default constructor\n";
        x = 0;
    }
    A(int x)
        : x(x)
    {
        cout << "Regular constructor\n";
    }
    A(A const & a) {
        x = a.x;
        cout << "copy constructor\n";
    }
    A(A && a) {
        x = a.x;
        cout << "move constructor\n";
    }
};

template<typename Z>
requires std::is_same_v<decay_t<Z>, A>
void func(Z && s) {
    A x = forward<Z>(s);
    cout << x.x << "\n";
}

int main() {
    A a{3};
    func(a);
    return 0;
}
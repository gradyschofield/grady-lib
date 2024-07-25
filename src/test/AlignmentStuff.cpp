//
// Created by Grady Schofield on 7/24/24.
//

#include<cstddef>
#include<iostream>

using namespace std;

int main(int argc, char ** argv) {
    struct A {
        int a;
        short b;
        byte c;
    };
    cout << "size: " << sizeof(A) << " alignment: " << alignof(A) << "\n";
    return 0;
}
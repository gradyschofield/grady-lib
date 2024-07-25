//
// Created by Grady Schofield on 7/24/24.
//

#include<cstddef>
#include<iostream>

using namespace std;

int main(int argc, char ** argv) {
    struct A {
        int64_t d;
        int a;
        short b;
        byte c;
    };
    cout << "size: " << sizeof(A) << " alignment: " << alignof(A) << "\n";
    int64_t * t;
    cout << "int64_t* " << alignof(t) << "\n";
    int32_t * t2;
    cout << "int32_t* " << alignof(t2) << "\n";
    void * t4;
    cout << "void* " << alignof(t2) << "\n";
    A * t3;
    cout << "A* " << alignof(t3) << "\n";
    return 0;
}
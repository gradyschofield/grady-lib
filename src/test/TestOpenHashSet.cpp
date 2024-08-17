//
// Created by Grady Schofield on 8/17/24.
//

#include<iostream>

#include<gradylib/OpenHashSet.hpp>

using namespace std;
using namespace gradylib;

int main() {
    OpenHashSet<int> s;
    s.insert(4);
    s.insert(4);
    s.insert(3);
    s.insert(999);
    cout << s.contains(1) << "\n";
    cout << s.contains(4) << "\n";
    cout << s.contains(3) << "\n";
    cout << s.contains(999) << "\n";
    return 0;
}
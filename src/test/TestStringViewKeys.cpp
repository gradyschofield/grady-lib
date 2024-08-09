//
// Created by Grady Schofield on 8/8/24.
//

#include<functional>
#include<iostream>
#include<string>
#include<string_view>

#include<gradylib/OpenHashMap.hpp>

using namespace std;
using namespace gradylib;


int main(int argc, char ** argv) {
    string z("abc");
    string_view s(z);
    cout << s << "\n";
    cout << hash<string>{}(z) << " " << hash<string_view>{}(s) << "\n";
    cout << (z == s) << "\n";
    OpenHashMap<string, int> m;
    m.put("hello", 0xBEEF);
    string hello("hello");
    string_view h(hello);
    if (m.contains(h)) {
        cout << m.at(h) << " " << 0xBEEF << "\n";
    }
    m[h] = 101;
    m.put(h, 102);
    return 0;
}
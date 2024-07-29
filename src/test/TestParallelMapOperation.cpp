//
// Created by Grady Schofield on 7/26/24.
//

#include<string>

#include<OpenHashMap.hpp>

using namespace std;
using namespace gradylib;

int main() {
    OpenHashMap<string, int> m;
    m.put("hello", 3);
    m.put("world", 4);
    double t = m.parallelForEach<double>([](double & x, string const & key, int & value) {
        x += value;
    });
    cout << t << "\n";
    return 0;
}
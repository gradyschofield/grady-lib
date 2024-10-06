//
// Created by Grady Schofield on 10/6/24.
//

#include<string>
#include<iostream>

using namespace std;

int main(int argc, char ** argv) {
    string s("0   allTests                            0x0000000100175718 _ZN8gradylib9ExceptionC2ENSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEENS1_15source_locationE + 1240");
    int nBlanks = 0;
    int len = s.length();
    int i = 0;
    while (true) {
        while (i < len && !isspace(s[i])) ++i;
        while (i < len && isspace(s[i])) ++i;
        ++nBlanks;
        if (nBlanks == 3) break;
    }
    int j = i;
    while (j < len && !isspace(s[j])) ++j;
    cout << "'" << s.substr(i, j-i) << "'\n";
    return 0;
}
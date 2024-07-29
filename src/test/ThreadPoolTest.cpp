//
// Created by Grady Schofield on 7/27/24.
//

#include<fstream>
#include<sstream>
#include<unordered_set>

#include<ThreadPool.hpp>

using namespace std;
using namespace gradylib;

int main() {
    ThreadPool tp(thread::hardware_concurrency());
    cout << "pool size: " << tp.size() << "\n";
    for (int i = 0; i < 100; ++i) {
        long numWork = 100;
        mutex outMutex;
        ofstream ofs("testfile.txt");
        for (long i = 0; i < numWork; ++i) {
            tp.add([i, &ofs, &outMutex]() {
                lock_guard lg(outMutex);
                ofs << i << "\n";
            });
        }
        tp.wait();
        ofs.close();
        ifstream ifs("testfile.txt");
        unordered_set<long> s;
        while (true) {
            string line;
            getline(ifs, line);
            if (ifs.fail()) {
                break;
            }
            istringstream sstr(line);
            long i;
            sstr >> i;
            s.insert(i);
        }
        if (s.size() != numWork) {
            cout << "problem: " << s.size() << " " << numWork << "\n";
        }
    }
}
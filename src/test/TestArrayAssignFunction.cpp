//
// Created by Grady Schofield on 8/2/24.
//

#include<iostream>

#include<gradylib/OpenHashMap.hpp>

using namespace gradylib;
using namespace std;

namespace std {
    template<>
    struct hash<char[3]> {
        size_t operator()(char const (&a)[3]) const noexcept {
            size_t x = 31;
            for (int i = 0; i < 3; ++i) {
                x = x * a[i] + 31;
            }
            return x;
        }
    };
}

template<typename Key, typename KeyType, size_t N>
inline void assignArray(Key (&keyStorage)[N], KeyType (&& keyArg)[N]) {
    if constexpr (std::is_array_v<std::remove_extent_t<Key>>) {
        for (size_t i = 0; i < N; ++i) {
            assignArray(keyStorage[i], std::move(keyArg[i]));
        }
    } else {
        if constexpr (std::is_trivially_copyable_v<std::remove_extent_t<Key>>) {
            cout << "memcpy move assign impl\n";
            memcpy(keyStorage, keyArg, sizeof(Key) * N);
        } else {
            cout << "loop move assign impl\n";
            for (size_t i = 0; i < N; ++i) {
                keyStorage[i] = std::move(keyArg[i]);
            }
        }
    }
}

template<typename Key, typename KeyType, size_t N>
inline void assignArray(Key (&keyStorage)[N], KeyType const (& keyArg)[N]) {
    using TT = std::remove_extent_t<Key>;
    TT x{};
    if constexpr (std::is_array_v<std::remove_extent_t<Key>>) {
        for (size_t i = 0; i < N; ++i) {
            assignArray(keyStorage[i], std::remove_extent_t<KeyType>(keyArg[i]));
        }
    } else {
        if constexpr (std::is_trivially_copyable_v<std::remove_extent_t<Key>>) {
            cout << "memcpy impl\n";
            memcpy(keyStorage, keyArg, sizeof(Key) * N);
        } else {
            cout << "loop copy-assign impl\n";
            for (size_t i = 0; i < N; ++i) {
                keyStorage[i] = keyArg[i];
            }
        }
    }
}

template<typename Key, typename KeyType>
inline void assign(Key & keyStorage, KeyType && keyArg) {
    if constexpr (std::is_array_v<Key>) {
        if constexpr (std::is_rvalue_reference_v<decltype(keyArg)>) {
            assignArray(keyStorage, std::move(keyArg));
        } else {
            assignArray(keyStorage, keyArg);
        }
    } else {
        keyStorage = std::forward<KeyType>(keyArg);
    }
}

struct Integer{
    int i;
    Integer() = default;
    Integer(int i) : i(i){}
    virtual ~Integer(){}
};

ostream & operator<<(ostream & os, Integer const & i) {
    os << i.i;
    return os;
}

int main() {
#if 0
    OpenHashMap<char[3], int> m;
    char x[3] = {'e', 'r', 'd'};
    m.put(x, 3);
    for (auto const & [k, v] : m) {
        cout << x << " " << v << endl;
    }
#endif
    int x = 5;
    int y = 0;
    assign(y, x);
    cout << "y: " << y << endl;
    vector<int> vx{1, 2, 3};
    vector<int> vy;
    cout << "lvalue assign:\n";
    assign(vy, vx);
    cout << vx[0] << " " << vx[1] << " " << vx[2] << endl;
    cout << vy[0] << " " << vy[1] << " " << vy[2] << endl;
    cout << "rvalue assign:\n";
    assign(vy, std::move(vx));
    if (vx.empty()) {
        cout << "vx empty\n";
    } else {
        cout << vx[0] << " " << vx[1] << " " << vx[2] << endl;
    }
    cout << vy[0] << " " << vy[1] << " " << vy[2] << endl;

    int xa[3] = {1, 2, 3};
    int ya[3];

    cout << "array assign\n";
    assign(ya, xa);

    cout << xa[0] << " " << xa[1] << " " << xa[2] << endl;
    cout << ya[0] << " " << ya[1] << " " << ya[2] << endl;

    cout << "Non TC copy assign\n";
    Integer xntc[3] = {{1}, {2}, {3}};
    Integer yntc[3];
    assign(yntc, xntc);
    cout << xntc[0] << " " << xntc[1] << " " << xntc[2] << endl;
    cout << yntc[0] << " " << yntc[1] << " " << yntc[2] << endl;


    cout << "Non TC move assign\n";
    yntc[0] = 0;
    yntc[1] = 0;
    yntc[2] = 0;
    assign(yntc, move(xntc));
    cout << xntc[0] << " " << xntc[1] << " " << xntc[2] << endl;
    cout << yntc[0] << " " << yntc[1] << " " << yntc[2] << endl;

    cout << "matrix copy\n";
    int xm[][3] = {{1,2,3},
                    {4, 5, 6},
                    {7,8,9}};
    int ym[3][3];
    assign(ym, xm);
    cout << sizeof(xm) << "\n";
    cout << (memcmp(xm, ym, sizeof(xm)) == 0 ? "matrices equal" : "matrices not equal") << "\n";

    return 0;
}
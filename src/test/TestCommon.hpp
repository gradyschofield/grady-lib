//
// Created by Grady Schofield on 10/12/24.
//

#pragma once

#include<fstream>
#include<string>

using namespace std;

struct StringIntFloat {
    string name;
    int idx;
    float alpha;

    bool operator==(StringIntFloat const & sif) const {
        return name == sif.name && idx == sif.idx && alpha == sif.alpha;
    }
};

namespace std {
    template<>
    struct hash<StringIntFloat> {
        size_t operator()(StringIntFloat const & sif) const noexcept {
            return hash<string>{}(sif.name) + hash<int>{}(sif.idx) + hash<float>{}(sif.alpha);
        }
    };
}

template<typename T>
char * charCast(T * p) {
    return static_cast<char *>(static_cast<void*>(p));
}

template<typename T>
char * charCast(T const * p) {
    return static_cast<char *>(const_cast<void *>(static_cast<void const *>(p)));
}

inline void serializeStringIntFloat(ofstream & ofs, StringIntFloat const & sif) {
    int len = sif.name.length();
    ofs.write(charCast(&len), 4);
    ofs.write(sif.name.data(), len);
    ofs.write(charCast(&sif.idx), 4);
    ofs.write(charCast(&sif.alpha), 4);
};

inline StringIntFloat deserializeStringIntFloat(ifstream & ifs) {
    int len;
    ifs.read(charCast(&len), 4);
    vector<char> buffer(len);
    ifs.read(buffer.data(), len);
    StringIntFloat sif{string(buffer.data(), len)};
    ifs.read(charCast(&sif.idx), 4);
    ifs.read(charCast(&sif.alpha), 4);
    return sif;
}

inline void serializeString(ofstream & ofs, string const & s) {
    int len = s.length();
    ofs.write(charCast(&len), 4);
    ofs.write(s.data(), len);
};

inline string deserializeString(ifstream & ifs) {
    int len;
    ifs.read(charCast(&len), 4);
    vector<char> buffer(len);
    ifs.read(buffer.data(), len);
    return string(buffer.data(), len);
}


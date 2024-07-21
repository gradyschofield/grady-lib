//
// Created by Grady Schofield on 7/20/24.
//

#ifndef GRADY_LIB_OPENMAPOFTRIVIALLYCOPYABLES_HPP
#define GRADY_LIB_OPENMAPOFTRIVIALLYCOPYABLES_HPP

#include<type_traits>
#include<vector>

template<typename Key>
requires std::is_trivially_copyable_v<Key>
class OpenHashSetOfTriviallyCopyables {

    std::vector<bool> isSet;
    std::vector<Key> keys;
    double loadFactor = 0.8;
    double growthFactor = 1.2;
    size_t setSize = 0;

    void rehash(size_t size = 0) {
        size_t newSize;
        if (size > 0) {
            if (size <= setSize) {
                return;
            }
            newSize = size / loadFactor;
        } else {
            newSize = std::max<size_t>(keys.size() + 1, std::max<size_t>(1, keys.size()) * growthFactor);
        }
        std::vector<Key> newKeys(newSize);
        std::vector<bool> newIsSet(newSize);
        for (size_t i = 0; i < keys.size(); ++i) {
            if (!isSet[i]) {
                continue;
            }
            Key const & k = keys[i];
            size_t hash = std::hash<Key>{}(k);
            size_t idx = hash % newKeys.size();
            while (newIsSet[idx]) {
                ++idx;
                idx = idx == newKeys.size() ? 0 : idx;
            }
            newIsSet[idx] = true;
            newKeys[idx] = k;
        }
        std::swap(isSet, newIsSet);
        std::swap(keys, newKeys);
    }

public:
    void insert(Key const & key) {
        if (setSize >= keys.size() * loadFactor) {
            rehash();
        }
        size_t hash = std::hash<Key>{}(key);
        size_t idx = hash % keys.size();
        while (isSet[idx]) {
            if (keys[idx] == key) {
                return;
            }
            ++idx;
            idx = idx == keys.size() ? 0 : idx;
        }
        isSet[idx] = true;
        keys[idx] = key;
        ++setSize;
    }

    bool contains(Key const & key) {
        size_t hash = std::hash<Key>{}(key);
        size_t idx = hash % keys.size();
        while (isSet[idx]) {
            if (keys[idx] == key) {
                return true;
            }
            ++idx;
            idx = idx == keys.size() ? 0 : idx;
        }
    }

    void erase(Key const & key) {

    }

    void reserve(size_t size) {
        rehash(size);
    }
};

#endif //GRADY_LIB_OPENMAPOFTRIVIALLYCOPYABLES_HPP

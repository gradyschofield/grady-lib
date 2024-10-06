/*
MIT License

Copyright (c) 2024 Grady Schofield

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include<fstream>
#include<future>
#include<string>
#include<type_traits>
#include<vector>

#include"Common.hpp"
#include"BitPairSet.hpp"
#include"ThreadPool.hpp"
#include"ParallelTraversals.hpp"

namespace gradylib {

    template<typename Key, template<typename> typename HashFunction = std::hash>
    requires std::is_default_constructible_v<Key> && std::equality_comparable<Key>
    class OpenHashSet;

    template<typename Key, template<typename> typename HashFunction>
    void mergePartials(OpenHashSet<Key, HashFunction> & m1, OpenHashSet<Key, HashFunction> const & m2) {
        for (auto const & [key] : m2) {
            m1.inset(key);
        }
    }

    template<typename Key, template<typename> typename HashFunction>
    requires std::is_default_constructible_v<Key> && std::equality_comparable<Key>
    class OpenHashSet {

        std::vector<Key> keys;
        BitPairSet setFlags;
        double loadFactor = 0.8;
        double growthFactor = 1.2;
        size_t setSize = 0;
        HashFunction<Key> hashFunction = HashFunction<Key>{};

        void rehash(size_t size = 0) {
            size_t newSize;
            if (size > 0) {
                if (size < setSize) {
                    return;
                }
                newSize = size / loadFactor;
            } else {
                newSize = std::max<size_t>(keys.size() + 1, std::max<size_t>(1, keys.size()) * growthFactor);
            }
            std::vector<Key> newKeys(newSize);
            BitPairSet newSetFlags(newSize);
            if (setSize > 0) {
                for (size_t i = 0; i < keys.size(); ++i) {
                    if (!setFlags.isFirstSet(i)) {
                        continue;
                    }
                    Key & k = keys[i];
                    size_t hash = hashFunction(k);
                    size_t idx = hash % newKeys.size();
                    while (newSetFlags.isFirstSet(idx)) {
                        ++idx;
                        idx = idx == newKeys.size() ? 0 : idx;
                    }
                    newSetFlags.setBoth(idx);
                    newKeys[idx] = std::move(k);
                }
            }
            std::swap(keys, newKeys);
            std::swap(setFlags, newSetFlags);
        }

    public:
        typedef Key key_type;

        OpenHashSet() = default;
        OpenHashSet(size_t size) {
            rehash(size);
        }

        template<typename KeyType>
        requires (std::is_same_v<std::remove_cvref_t<KeyType>, Key> ||
                  std::is_convertible_v<Key, std::remove_cvref_t<KeyType>> ||
                  (std::is_constructible_v<Key, KeyType> && std::is_assignable_v<Key, KeyType>))
        void insert(KeyType && key) {
            size_t hash = 0;
            size_t idx = 0;
            bool doesContain = false;
            size_t firstUnsetIdx = -1;
            bool isFirstUnsetIdxSet = false;
            size_t startIdx = idx;
            if (keys.size() > 0) {
                if constexpr (std::same_as<Key, std::string> && std::same_as<std::remove_cvref_t<KeyType>, std::string_view>) {
                    hash = HashFunction<std::string_view>{}(key);
                } else {
                    hash = hashFunction(key);
                }
                idx = hash % keys.size();
                startIdx = idx;
                for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                    if (!isFirstUnsetIdxSet && !isSet) {
                        firstUnsetIdx = idx;
                        isFirstUnsetIdxSet = true;
                    }
                    if (isSet && keys[idx] == key) {
                        doesContain = true;
                        break;
                    }
                    if (wasSet && keys[idx] == key) {
                        doesContain = false;
                        break;
                    }
                    ++idx;
                    idx = idx == keys.size() ? 0 : idx;
                    if (startIdx == idx) break;
                }
            }
            if (doesContain) {
                return;
            }
            if (setSize >= keys.size() * loadFactor) {
                rehash();
                if constexpr (std::same_as<Key, std::string> && std::same_as<std::remove_cvref_t<KeyType>, std::string_view>) {
                    hash = HashFunction<std::string_view>{}(key);
                } else {
                    hash = hashFunction(key);
                }
                idx = hash % keys.size();
                startIdx = idx;
                while (setFlags.isFirstSet(idx)) {
                    ++idx;
                    idx = idx == keys.size() ? 0 : idx;
                    if (startIdx == idx) break;
                }
            } else {
                idx = isFirstUnsetIdxSet ? firstUnsetIdx : idx;
            }
            setFlags.setBoth(idx);
            keys[idx] = std::forward<KeyType>(key);
            ++setSize;
        }

        template<typename KeyType>
        requires (std::is_convertible_v<Key, std::remove_cvref_t<KeyType>> ||
                  std::is_constructible_v<Key, KeyType>) &&
                  gradylib_helpers::equality_comparable<KeyType, Key>
        bool contains(KeyType const &key) const {
            if (keys.size() == 0) {
                return false;
            }
            size_t hash;
            if constexpr (std::same_as<Key, std::string> && std::same_as<std::remove_cvref_t<KeyType>, std::string_view>) {
                hash = HashFunction<std::string_view>{}(key);
            } else {
                hash = hashFunction(key);
            }
            size_t idx = hash % keys.size();
            size_t startIdx = idx;
            for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                if (isSet && keys[idx] == key) {
                    return true;
                }
                if (wasSet && keys[idx] == key) {
                    return false;
                }
                ++idx;
                idx = idx == keys.size() ? 0 : idx;
                if (startIdx == idx) break;
            }
            return false;
        }

        template<typename KeyType>
        requires (std::is_constructible_v<Key, KeyType> ||
                  std::is_convertible_v<Key, std::remove_cvref_t<KeyType>>) &&
                  gradylib_helpers::equality_comparable<KeyType, Key>
        void erase(KeyType const &key) {
            if (keys.empty()) {
                return;
            }
            size_t hash;
            if constexpr (std::same_as<Key, std::string> && std::same_as<std::remove_cvref_t<KeyType>, std::string_view>) {
                hash = HashFunction<std::string_view>{}(key);
            } else {
                hash = hashFunction(key);
            }
            size_t idx = hash % keys.size();
            size_t startIdx = idx;
            for (auto [isSet, wasSet] = setFlags[idx]; isSet || wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                if (keys[idx] == key) {
                    if (isSet) {
                        --setSize;
                        setFlags.unsetFirst(idx);
                    }
                    return;
                }
                ++idx;
                idx = idx == keys.size() ? 0 : idx;
                if (startIdx == idx) break;
            }
        }

        void reserve(size_t size) {
            rehash(size);
        }

        void clear() {
            setFlags.clear();
            setSize = 0;
        }

        class iterator {
            size_t idx;
            OpenHashSet *container;
        public:
            iterator(size_t idx, OpenHashSet *container)
                    : idx(idx), container(container) {
            }

            bool operator==(iterator const &other) const {
                return idx == other.idx && container == other.container;
            }

            bool operator!=(iterator const &other) const {
                return idx != other.idx || container != other.container;
            }

            Key const & operator*() {
                return container->keys[idx];
            }

            Key const &key() const {
                return container->keys[idx];
            }

            iterator &operator++() {
                if (idx == container->keys.size()) {
                    return *this;
                }
                ++idx;
                while (idx < container->keys.size() && !container->setFlags.isFirstSet(idx)) {
                    ++idx;
                }
                return *this;
            }
        };

        iterator begin() {
            if (setSize == 0) {
                return iterator(keys.size(), this);
            }
            size_t idx = 0;
            while (idx < keys.size() && !setFlags.isFirstSet(idx)) {
                ++idx;
            }
            return iterator(idx, this);
        }

        iterator end() {
            return iterator(keys.size(), this);
        }

        class const_iterator {
            size_t idx;
            OpenHashSet const *container;
        public:
            const_iterator(size_t idx, OpenHashSet const *container)
                    : idx(idx), container(container) {
            }

            bool operator==(const_iterator const &other) const {
                return idx == other.idx && container == other.container;
            }

            bool operator!=(const_iterator const &other) const {
                return idx != other.idx || container != other.container;
            }

            Key const & operator*() const {
                return container->keys[idx];
            }

            Key const &key() const {
                return container->keys[idx];
            }

            const_iterator &operator++() {
                if (idx == container->keys.size()) {
                    return *this;
                }
                ++idx;
                while (idx < container->keys.size() && !container->setFlags.isFirstSet(idx)) {
                    ++idx;
                }
                return *this;
            }
        };

        const_iterator begin() const {
            if (setSize == 0) {
                return const_iterator(keys.size(), this);
            }
            size_t idx = 0;
            while (idx < keys.size() && !setFlags.isFirstSet(idx)) {
                ++idx;
            }
            return const_iterator(idx, this);
        }

        const_iterator end() const {
            return const_iterator(keys.size(), this);
        }

        size_t size() const {
            return setSize;
        }


        template<gradylib_helpers::Mergeable ReturnValue = OpenHashSet<Key, HashFunction>,
                typename Callable,
                typename PartialInitializer = gradylib_helpers::PartialDefaultConstructor<ReturnValue>,
                typename FinalInitializer = gradylib_helpers::FinalDefaultConstructor<ReturnValue>>
        requires std::is_invocable_r_v<void, Callable, ReturnValue &, Key const &> &&
                 std::is_copy_constructible_v<Callable> &&
                 std::is_invocable_r_v<ReturnValue, PartialInitializer, int, int> &&
                 std::is_invocable_r_v<ReturnValue, FinalInitializer, int>
        std::future<ReturnValue> parallelForEach(Callable && f,
                                                 PartialInitializer && partialInitializer = PartialInitializer{},
                                                 FinalInitializer && finalInitializer = FinalInitializer{}) const {
            if (!gradylib_helpers::GRADY_LIB_DEFAULT_THREADPOOL) {
                std::lock_guard lg(gradylib_helpers::GRADY_LIB_DEFAULT_THREADPOOL_MUTEX);
                if (!gradylib_helpers::GRADY_LIB_DEFAULT_THREADPOOL) {
                    gradylib_helpers::GRADY_LIB_DEFAULT_THREADPOOL = std::make_unique<ThreadPool>();
                }
            }
            return parallelForEach(*gradylib_helpers::GRADY_LIB_DEFAULT_THREADPOOL,
                                   std::forward<Callable>(f),
                                   std::forward<PartialInitializer>(partialInitializer),
                                   std::forward<FinalInitializer>(finalInitializer));
        }

        template<gradylib_helpers::Mergeable ReturnValue = OpenHashSet<Key, HashFunction>,
                typename Callable,
                typename PartialInitializer = gradylib_helpers::PartialDefaultConstructor<ReturnValue>,
                typename FinalInitializer = gradylib_helpers::FinalDefaultConstructor<ReturnValue>>
        requires std::is_invocable_r_v<void, Callable, ReturnValue &, Key const &> &&
                 std::is_copy_constructible_v<Callable> &&
                 std::is_invocable_r_v<ReturnValue, PartialInitializer, int, int> &&
                 std::is_invocable_r_v<ReturnValue, FinalInitializer, int>
        std::future<ReturnValue> parallelForEach(ThreadPool & tp,
                                                 Callable && f,
                                                 PartialInitializer && partialInitializer = PartialInitializer{},
                                                 FinalInitializer && finalInitializer = PartialInitializer{},
                                                 size_t numThreads = 0) const {
            if (numThreads == 0) {
                numThreads = tp.size();
            }
            numThreads = std::min(numThreads, size());
            struct Result {
                ReturnValue final;
                std::mutex finalMutex;
                std::promise<ReturnValue> promise;
                size_t remainingThreads;

                Result(ReturnValue && final, size_t remainingThreads)
                        : final(std::move(final)), remainingThreads(remainingThreads)
                {
                }
            };
            std::shared_ptr<Result> result = std::make_shared<Result>(finalInitializer(numThreads), numThreads);
            size_t start = 0;
            for (size_t threadIdx = 0; threadIdx < numThreads; ++threadIdx) {
                size_t stop = start + keys.size() / numThreads + (threadIdx < keys.size() % numThreads ? 1 : 0);
                tp.add([threadIdx, numThreads, start, stop, f, result, this, &partialInitializer]() {
                    ReturnValue partial = partialInitializer(threadIdx, numThreads);
                    for (size_t j = start; j < stop; ++j) {
                        if (setFlags.isFirstSet(j)) {
                            f(partial, keys[j]);
                        }
                    }
                    std::lock_guard lg(result->finalMutex);
                    mergePartials(result->final, partial);
                    if (result->remainingThreads == 1) {
                        result->promise.set_value(std::move(result->final));
                    }
                    --result->remainingThreads;
                });
                start = stop;
            }
            return result->promise.get_future();
        }

        template<template<typename> typename HashFunc>
        friend void writeMappable(std::string filename, OpenHashSet<std::string, HashFunc> const & m);

        template<std::integral IndexType, template<typename> typename HashFunc>
        friend void writeMappable(std::string filename, OpenHashSet<IndexType, HashFunc> const & m);
    };


    template<template<typename> typename HashFunc>
    void writeMappable(std::filesystem::path filename, OpenHashSet<std::string, HashFunc> const & m) {
        std::ofstream ofs(filename);
        if (ofs.fail()) {
            std::ostringstream sstr;
            sstr << "Couldn't open file " << filename << " in writeMappable.";
            throw gradylibMakeException(sstr.str());
        }
        size_t setSize = m.setSize;
        ofs.write(static_cast<char*>(static_cast<void*>(&setSize)), 8);
        size_t keySize = m.keys.size();
        ofs.write(static_cast<char*>(static_cast<void*>(&keySize)), 8);
        size_t bitPairSetOffset = 0;
        auto const bitPairSetOffsetWritePos = ofs.tellp();
        ofs.write(static_cast<char*>(static_cast<void*>(&bitPairSetOffset)), 8);
        size_t keyOffset = 0;
        for (size_t i = 0; i < m.keys.size(); ++i) {
            ofs.write(static_cast<char*>(static_cast<void*>(&keyOffset)), 8);
            int32_t strLen = m.keys[i].length();
            int32_t strSize = 4 + strLen + (4 - strLen % 4);
            keyOffset += strSize;
        }
        std::vector<char> pad(4, 0);
        for (size_t i = 0; i < m.keys.size(); ++i) {
            int32_t len = m.keys[i].length();
            ofs.write(static_cast<char*>(static_cast<void*>(&len)), 4);
            ofs.write(m.keys[i].data(), len);
            ofs.write(pad.data(), 4 - len % 4);
        }

        bitPairSetOffset = ofs.tellp();
        m.setFlags.write(ofs);

        ofs.seekp(bitPairSetOffsetWritePos, std::ios::beg);
        ofs.write(static_cast<char*>(static_cast<void*>(&bitPairSetOffset)), 8);
    }

    template<std::integral IndexType, template<typename> typename HashFunc>
    void writeMappable(std::string filename, OpenHashSet<IndexType, HashFunc> const & m) {
        std::ofstream ofs(filename);
        if (ofs.fail()) {
            std::ostringstream sstr;
            sstr << "Couldn't open file " << filename << " in writeMappable.";
            throw gradylibMakeException(sstr.str());
        }
        size_t setSize = m.setSize;
        ofs.write(static_cast<char*>(static_cast<void*>(&setSize)), 8);
        size_t keySize = m.keys.size();
        ofs.write(static_cast<char*>(static_cast<void*>(&keySize)), 8);
        size_t bitPairSetOffset = 0;
        auto const bitPairSetOffsetWritePos = ofs.tellp();
        ofs.write(static_cast<char*>(static_cast<void*>(&bitPairSetOffset)), 8);
        ofs.write(static_cast<char*>(const_cast<void*>(static_cast<void const *>(m.keys.data()))), sizeof(IndexType) * keySize);
        std::vector<char> pad(4, 0);

        bitPairSetOffset = ofs.tellp();
        m.setFlags.write(ofs);

        ofs.seekp(bitPairSetOffsetWritePos, std::ios::beg);
        ofs.write(static_cast<char*>(static_cast<void*>(&bitPairSetOffset)), 8);
    }

}


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

#include<filesystem>
#include<fstream>
#include<future>
#include<string>
#include<type_traits>
#include<vector>

#include"AltIntHash.hpp"
#include"Common.hpp"
#include"BitPairSet.hpp"
#include"ThreadPool.hpp"
#include"ParallelTraversals.hpp"

/*
 * To some extent OpenHashMap can be used as a drop in replacement for unordered_map.  It has:
 *  - at
 *  - begin
 *  - clear
 *  - contains
 *  - end
 *  - operator[]
 *  - reserve
 *  - size
 *
 * API not available in unordered_map:
 *  - put
 *  - get
 *  - parallelForEach
 *  - writeMappable (for integer -> string or string -> integer maps)
 */

namespace gradylib {
    template<typename Key, typename Value, template<typename> typename HashFunction = gradylib::AltHash>
    requires std::is_default_constructible_v<Key> && std::is_default_constructible_v<Value>
    class OpenHashMap;

    template<typename Key, typename Value, template<typename> typename HashFunction>
    void mergePartials(OpenHashMap<Key, Value, HashFunction> & m1, OpenHashMap<Key, Value, HashFunction> const & m2) {
        for (auto const & [key, value] : m2) {
            m1[key] = value;
        }
    }

    template<typename Key, typename Value, template<typename> typename HashFunction>
    requires std::is_default_constructible_v<Key> && std::is_default_constructible_v<Value>
    class OpenHashMap {

        std::vector<Key> keys;
        std::vector<Value> values;
        BitPairSet setFlags;
        double loadFactor = 0.8;
        double growthFactor = 1.2;
        size_t mapSize = 0;
        HashFunction<Key> hashFunction = HashFunction<Key>{};

        void rehash(size_t size = 0) {
            size_t newSize;
            if (size > 0) {
                // The user called rehash with a parameter that wouldn't fit the current contents.  Do nothing.
                if (size < mapSize) {
                    return;
                }
                newSize = size / loadFactor;
            } else {
                // Increase the map size by a factor of growthFactor, ensuring the new size is at least one greater than the old size
                newSize = std::max<size_t>(keys.size() + 1, keys.size() * growthFactor);
            }
            // Use of vector here is one reason to require default constructibility in the Key and the Value. Vector's usage also
            // prevents us from using C style arrays as keys to their non-assignability. Maybe implement our own array container
            // to get around this.
            std::vector<Key> newKeys(newSize);
            std::vector<Value> newValues(newSize);
            BitPairSet newSetFlags(newSize);
            if (mapSize > 0) {
                for (size_t i = 0; i < keys.size(); ++i) {
                    if (!setFlags.isFirstSet(i)) {
                        // In the current map, this entry is empty
                        continue;
                    }
                    Key & k = keys[i];
                    size_t hash = hashFunction(k);
                    size_t idx = hash % newKeys.size();
                    // Scan the new map for a free space
                    while (newSetFlags.isFirstSet(idx)) {
                        ++idx;
                        idx = idx == newKeys.size() ? 0 : idx;
                    }
                    newSetFlags.setBoth(idx);
                    newKeys[idx] = std::move(k);
                    newValues[idx] = std::move(values[i]);
                }
            }
            std::swap(keys, newKeys);
            std::swap(values, newValues);
            std::swap(setFlags, newSetFlags);
        }

    public:
        typedef Key key_type;
        typedef Value mapped_type;

        OpenHashMap() = default;
        OpenHashMap(size_t size) {
            rehash(size);
        }

        template<typename KeyType>
        requires std::is_same_v<std::remove_cvref_t<KeyType>, Key> ||
                 std::is_convertible_v<Key, std::remove_cvref_t<KeyType>> ||
                 (std::is_constructible_v<Key, KeyType> && std::is_assignable_v<Key, KeyType>)
        Value &operator[](KeyType && key) {
            size_t hash = 0;
            size_t idx = 0;
            // We will have a few opportunities to find the insertion point, with the first available slot taking precedence.
            // Hence, the appearance of insertionIdx = insertionIdx.value_or(idx) in a few places.
            std::optional<size_t> insertionIdx;
            if (!keys.empty()) {
                // Handle the case where string_views are passed to this method for an OpenHashMap<string, *>
                if constexpr (std::same_as<Key, std::string> && std::same_as<std::remove_cvref_t<KeyType>, std::string_view>) {
                    hash = HashFunction<std::string_view>{}(key);
                } else {
                    hash = hashFunction(key);
                }
                idx = hash % keys.size();
                size_t startIdx = idx;
                auto [isSet, wasSet] = setFlags[idx];
                // Scan the key array until the key is found in a 'set' slot or a never-before-set slot is found.
                for (; wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                    if (!insertionIdx.has_value() && !isSet) {
                        // We can't stop iterating here.  We may still find the key later.  If not, this is where we will put the key.
                        insertionIdx = idx;
                    }
                    if (keys[idx] == key) {
                        if (isSet) {
                            return values[idx];
                        }
                        // The key was here, but it has been removed.  We can stop because it can't be found later in the map.
                        insertionIdx = insertionIdx.value_or(idx);
                        break;
                    }
                    ++idx;
                    // Wrap around
                    idx = idx == keys.size() ? 0 : idx;
                    // Stop if we've covered every element.  The key is missing.
                    if (startIdx == idx) break;
                }
                if (!wasSet) {
                    insertionIdx = insertionIdx.value_or(idx);
                }
            }
            // If we have wrapped all the way around, insertionIdx may not be set.
            // However, in this case the map is full so we will go into the next conditional and do a rehash.
            // A new valid location will be produced by this.
            if (mapSize >= static_cast<size_t>(keys.size() * loadFactor)) {
                rehash();
                // Handle the case where string_views are passed to this method for an OpenHashMap<string, *>
                if constexpr (std::same_as<Key, std::string> && std::same_as<std::remove_cvref_t<KeyType>, std::string_view>) {
                    hash = HashFunction<std::string_view>{}(key);
                } else {
                    hash = hashFunction(key);
                }
                idx = hash % keys.size();
                size_t startIdx = idx;
                // Do another scan to find the insertion point in the rehashed map.
                while (setFlags.isFirstSet(idx)) {
                    ++idx;
                    idx = idx == keys.size() ? 0 : idx;
                    if (startIdx == idx) break;
                }
                insertionIdx = idx;
            }
            idx = insertionIdx.value();
            if (setFlags.isSecondSet(idx)) {
                // This slot was previously set and has some spurious value in it.  Let's set it back to default.
                values[idx] = Value{};
            }
            setFlags.setBoth(idx);
            keys[idx] = std::forward<KeyType>(key);
            ++mapSize;
            return values[idx];
        }

        template<typename KeyType, typename ValueType>
        requires (std::is_same_v<std::remove_cvref_t<KeyType>, Key> ||
                 std::is_convertible_v<Key, std::remove_cvref_t<KeyType>> ||
                 (std::is_constructible_v<Key, KeyType> && std::is_assignable_v<Key, KeyType>)) &&
                 std::is_same_v<std::remove_cvref_t<ValueType>, Value>
        void put(KeyType && key, ValueType && value) {
            size_t hash = 0;
            size_t idx = 0;
            // We will have a few opportunities to find the insertion point, with the first available slot taking precedence.
            // Hence, the appearance of insertionIdx = insertionIdx.value_or(idx) in a few places.
            std::optional<size_t> insertionIdx;
            if (keys.size() > 0) {
                // Handle the case where string_views are passed to this method for an OpenHashMap<string, *>
                if constexpr (std::same_as<Key, std::string> && std::same_as<std::remove_cvref_t<KeyType>, std::string_view>) {
                    hash = HashFunction<std::string_view>{}(key);
                } else {
                    hash = hashFunction(key);
                }
                idx = hash % keys.size();
                size_t startIdx = idx;
                auto [isSet, wasSet] = setFlags[idx];
                // Scan the key array until either the key is found in a 'set' slot or a never-before-set slot is found.
                for (; wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                    if (!insertionIdx.has_value() && !isSet) {
                        // We can't stop iterating here.  We may still find the key later.  If not, this is where we will put the key.
                        insertionIdx = idx;
                    }
                    if (keys[idx] == key) {
                        if (isSet) {
                            values[idx] = std::forward<ValueType>(value);
                            return;
                        }
                        // The key was here, but it has been removed.  We can stop because it can't be found later in the map.
                        insertionIdx = insertionIdx.value_or(idx);
                        break;
                    }
                    ++idx;
                    // Wrap around
                    idx = idx == keys.size() ? 0 : idx;
                    // Stop if we've covered every element.  The key is missing.
                    if (startIdx == idx) break;
                }
                if (!wasSet) {
                    insertionIdx = insertionIdx.value_or(idx);
                }
            }
            if (mapSize >= static_cast<size_t>(keys.size() * loadFactor)) {
                rehash();
                // Handle the case where string_views are passed to this method for an OpenHashMap<string, *>
                if constexpr (std::same_as<Key, std::string> && std::same_as<std::remove_cvref_t<KeyType>, std::string_view>) {
                    hash = HashFunction<std::string_view>{}(key);
                } else {
                    hash = hashFunction(key);
                }
                idx = hash % keys.size();
                size_t startIdx = idx;
                // Do another scan to find the insertion point in the rehashed map.
                while (setFlags.isFirstSet(idx)) {
                    ++idx;
                    idx = idx == keys.size() ? 0 : idx;
                    if (startIdx == idx) break;
                }
                insertionIdx = idx;
            }
            idx = insertionIdx.value();
            setFlags.setBoth(idx);
            keys[idx] = std::forward<KeyType>(key);
            values[idx] = std::forward<ValueType>(value);
            ++mapSize;
        }

        template<typename KeyType>
        requires (std::is_convertible_v<Key, std::remove_cvref_t<KeyType>> ||
                 std::is_constructible_v<Key, KeyType>) &&
                 gradylib_helpers::equality_comparable<KeyType, Key>
        bool contains(KeyType const &key) const {
            if (keys.size() == 0) {
                return false;
            }
            size_t hash = 0;
            // Handle the case where string_views are passed to this method for an OpenHashMap<string, *>
            if constexpr (std::same_as<Key, std::string> && std::same_as<std::remove_cvref_t<KeyType>, std::string_view>) {
                hash = HashFunction<std::string_view>{}(key);
            } else {
                hash = hashFunction(key);
            }
            size_t idx = hash % keys.size();
            size_t startIdx = idx;
            for (auto [isSet, wasSet] = setFlags[idx]; wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                if (keys[idx] == key) {
                    if (isSet) {
                        return true;
                    }
                    return false;
                }
                ++idx;
                // Wrap around
                idx = idx == keys.size() ? 0 : idx;
                // Stop if we've covered every element.
                if (startIdx == idx) break;
            }
            return false;
        }

        template<typename KeyType>
        requires (std::is_constructible_v<Key, KeyType> ||
                 std::is_convertible_v<Key, std::remove_cvref_t<KeyType>>) &&
                 gradylib_helpers::equality_comparable<KeyType, Key>
        Value const & at(KeyType const &key) const {
            if (keys.size() == 0) {
                std::ostringstream sstr;
                sstr << "OpenHashMap doesn't contain key";
                throw gradylibMakeException(sstr.str());
            }
            size_t hash = 0;
            // Handle the case where string_views are passed to this method for an OpenHashMap<string, *>
            if constexpr (std::same_as<Key, std::string> && std::same_as<std::remove_cvref_t<KeyType>, std::string_view>) {
                hash = HashFunction<std::string_view>{}(key);
            } else {
                hash = hashFunction(key);
            }
            size_t idx = hash % keys.size();
            size_t startIdx = idx;
            for (auto [isSet, wasSet] = setFlags[idx]; wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                if (keys[idx] == key) {
                    if (isSet) {
                        return values[idx];
                    }
                    std::ostringstream sstr;
                    sstr << "OpenHashMap doesn't contain key";
                    throw gradylibMakeException(sstr.str());
                }
                ++idx;
                // Wrap around
                idx = idx == keys.size() ? 0 : idx;
                // Stop if we've covered every element
                if (startIdx == idx) break;
            }
            std::ostringstream sstr;
            sstr << "OpenHashMap doesn't contain key";
            throw gradylibMakeException(sstr.str());
        }

        // This method returns something like an optional<Value>.
        template<typename KeyType>
        requires (std::is_constructible_v<Key, KeyType> ||
                  std::is_convertible_v<Key, std::remove_cvref_t<KeyType>>) &&
                  gradylib_helpers::equality_comparable<KeyType, Key>
        gradylib_helpers::MapLookup<Value> get(KeyType const &key) {
            if (keys.size() == 0) {
                return gradylib_helpers::MapLookup<Value>();
            }
            size_t hash = 0;
            // Handle the case where string_views are passed to this method for an OpenHashMap<string, *>
            if constexpr (std::same_as<Key, std::string> && std::same_as<std::remove_cvref_t<KeyType>, std::string_view>) {
                hash = HashFunction<std::string_view>{}(key);
            } else {
                hash = hashFunction(key);
            }
            size_t idx = hash % keys.size();
            size_t startIdx = idx;
            for (auto [isSet, wasSet] = setFlags[idx]; wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                if (keys[idx] == key) {
                    if (isSet) {
                        return gradylib_helpers::MapLookup<Value>(&values[idx]);
                    }
                    return gradylib_helpers::MapLookup<Value>();
                }
                ++idx;
                // Wrap around
                idx = idx == keys.size() ? 0 : idx;
                // Stop if we've covered every element
                if (startIdx == idx) break;
            }
            return gradylib_helpers::MapLookup<Value>();
        }

        template<typename KeyType>
        requires (std::is_constructible_v<Key, KeyType> ||
                  std::is_convertible_v<Key, std::remove_cvref_t<KeyType>>) &&
                  gradylib_helpers::equality_comparable<KeyType, Key>
        gradylib_helpers::MapLookup<Value const> get(KeyType const &key) const {
            OpenHashMap<Key, Value, HashFunction> * p = const_cast<OpenHashMap<Key, Value, HashFunction> *>(this);
            return p->get(key).makeConst();
        }

        template<typename KeyType>
        requires (std::is_constructible_v<Key, KeyType> ||
                  std::is_convertible_v<Key, std::remove_cvref_t<KeyType>>) &&
                  gradylib_helpers::equality_comparable<KeyType, Key>
        void erase(KeyType const &key) {
            if (keys.empty()) {
                return;
            }
            size_t hash = 0;
            // Handle the case where string_views are passed to this method for an OpenHashMap<string, *>
            if constexpr (std::same_as<Key, std::string> && std::same_as<std::remove_cvref_t<KeyType>, std::string_view>) {
                hash = HashFunction<std::string_view>{}(key);
            } else {
                hash = hashFunction(key);
            }
            size_t idx = hash % keys.size();
            size_t startIdx = idx;
            for (auto [isSet, wasSet] = setFlags[idx]; wasSet; std::tie(isSet, wasSet) = setFlags[idx]) {
                if (keys[idx] == key) {
                    if (isSet) {
                        --mapSize;
                        // Mark the slot as 'unset'
                        setFlags.unsetFirst(idx);
                    }
                    // We can't find a set slot with this key past this point.
                    return;
                }
                ++idx;
                // Wrap around.
                idx = idx == keys.size() ? 0 : idx;
                // Stop if we've covered every element
                if (startIdx == idx) break;
            }
        }

        void reserve(size_t size) {
            rehash(size);
        }

        void clear() {
            setFlags.clear();
            mapSize = 0;
        }

        class iterator {
            size_t idx;
            OpenHashMap *container;
        public:
            iterator(size_t idx, OpenHashMap *container)
                    : idx(idx), container(container) {
            }

            bool operator==(iterator const &other) const {
                return idx == other.idx && container == other.container;
            }

            bool operator!=(iterator const &other) const {
                return idx != other.idx || container != other.container;
            }

            std::pair<Key const &, Value &> operator*() {
                return {container->keys[idx], container->values[idx]};
            }

            Key const &key() const {
                return container->keys[idx];
            }

            Value &value() {
                return container->values[idx];
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
            if (mapSize == 0) {
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
            OpenHashMap const *container;
        public:
            const_iterator(size_t idx, OpenHashMap const *container)
                    : idx(idx), container(container) {
            }

            bool operator==(const_iterator const &other) const {
                return idx == other.idx && container == other.container;
            }

            bool operator!=(const_iterator const &other) const {
                return idx != other.idx || container != other.container;
            }

            std::pair<Key const &, Value const &> operator*() const {
                return {container->keys[idx], container->values[idx]};
            }

            Key const &key() const {
                return container->keys[idx];
            }

            Value const &value() const {
                return container->values[idx];
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
            if (mapSize == 0) {
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
            return mapSize;
        }

        //template<typename = std::enable_if_t</* key has a serialize method or key has a serialize global and value has a serialize method or global */>
        void write(std::filesystem::path path, std::function<void(std::ofstream &, Key const &)> serializeKey, std::function<void(std::ofstream &, Value const &)> serializeValue) {
            std::ofstream ofs(path, std::ios::binary);
            write(ofs, serializeKey, serializeValue);
        }

        void write(std::ofstream & ofs, std::function<void(std::ofstream &, Key const &)> serializeKey, std::function<void(std::ofstream &, Value const &)> serializeValue) {
            namespace gh = gradylib_helpers;
            ofs.write(gh::charCast(&mapSize), sizeof(size_t));
            size_t keySize = keys.size();
            ofs.write(gh::charCast(&keySize), sizeof(size_t));
            ofs.write(gh::charCast(&loadFactor), 8);
            ofs.write(gh::charCast(&growthFactor), 8);
            for (size_t i = 0; i < keys.size(); ++i) {
                if (setFlags.isFirstSet(i)) {
                    // Write the key/value array index of this element
                    ofs.write(gh::charCast(&i), sizeof(size_t));

                    // We'll come back to this point to write a couple of offsets
                    uint64_t offsetOffset = ofs.tellp();
                    uint64_t dummy = 0;
                    ofs.write(gh::charCast(&dummy), 8);
                    ofs.write(gh::charCast(&dummy), 8);

                    serializeKey(ofs, keys[i]);

                    // We can't know if serializeKey left the file position at the end so put it there
                    ofs.seekp(0, std::ios::end);
                    uint64_t valueOffset = ofs.tellp();
                    serializeValue(ofs, values[i]);

                    // We can't know if serializeValue left the file position at the end so put it there
                    ofs.seekp(0, std::ios::end);
                    uint64_t nextOffset = ofs.tellp();

                    // Write our two offsets to the location marked above
                    ofs.seekp(offsetOffset);
                    ofs.write(gh::charCast(&valueOffset), 8);
                    ofs.write(gh::charCast(&nextOffset), 8);
                    ofs.seekp(0, std::ios::end);
                }
            }

            setFlags.write(ofs);
        }

        static OpenHashMap<Key, Value, HashFunction> read(std::ifstream & ifs, std::function<Key(std::ifstream &)> deserializeKey, std::function<Value(std::ifstream &)> deserializeValue) {
            namespace gh = gradylib_helpers;
            OpenHashMap<Key, Value, HashFunction> ret;
            ifs.read(gh::charCast(&ret.mapSize), sizeof(size_t));
            size_t keySize;
            ifs.read(gh::charCast(&keySize), sizeof(size_t));
            ifs.read(gh::charCast(&ret.loadFactor), 8);
            ifs.read(gh::charCast(&ret.growthFactor), 8);
            ret.keys = std::vector<Key>(keySize);
            ret.values = std::vector<Value>(keySize);
            for (size_t i = 0; i < ret.mapSize; ++i) {
                size_t idx;
                ifs.read(gh::charCast(&idx), sizeof(size_t));
                uint64_t valueOffset, nextOffset;
                ifs.read(gh::charCast(&valueOffset), 8);
                ifs.read(gh::charCast(&nextOffset), 8);
                ret.keys[idx] = deserializeKey(ifs);
                // We can't be sure deserializeKey left the file position at the end the key's bytes hence the following
                ifs.seekg(valueOffset);
                ret.values[idx] = deserializeValue(ifs);
                // We can't be sure deserializeValue left the file position at the end the value's bytes hence the following
                ifs.seekg(nextOffset);
            }
            ret.setFlags = BitPairSet(ifs);
            return ret;
        }

        static OpenHashMap<Key, Value, HashFunction> read(std::filesystem::path path, std::function<Key(std::ifstream &)> deserializeKey, std::function<Value(std::ifstream &)> deserializeValue) {
            std::ifstream ifs(path, std::ios::binary);
            return OpenHashMap<Key, Value, HashFunction>::read(ifs, deserializeKey, deserializeValue);
        }


        // This overload of parallelForEach uses the default thread pool.
        template<gradylib_helpers::Mergeable ReturnValue = OpenHashMap<Key, Value, HashFunction>,
                typename Callable,
                typename PartialInitializer = gradylib_helpers::PartialDefaultConstructor<ReturnValue>,
                typename FinalInitializer = gradylib_helpers::FinalDefaultConstructor<ReturnValue>>
        requires std::is_invocable_r_v<void, Callable, ReturnValue &, Key const &, Value const &> &&
                 std::is_copy_constructible_v<Callable> &&
                 std::is_invocable_r_v<ReturnValue, PartialInitializer, int, int> &&
                 std::is_invocable_r_v<ReturnValue, FinalInitializer, int>
        std::future<ReturnValue> parallelForEach(Callable && f,
                                                 PartialInitializer && partialInitializer = PartialInitializer{},
                                                 FinalInitializer && finalInitializer = FinalInitializer{},
                                                 size_t numThreads = 0) const {
            // If the default thread pool hasn't been created yet then create it now.
            if (!gradylib_helpers::GRADY_LIB_DEFAULT_THREADPOOL) {
                std::lock_guard lg(gradylib_helpers::GRADY_LIB_DEFAULT_THREADPOOL_MUTEX);
                if (!gradylib_helpers::GRADY_LIB_DEFAULT_THREADPOOL) {
                    gradylib_helpers::GRADY_LIB_DEFAULT_THREADPOOL = std::make_unique<ThreadPool>();
                }
            }
            return parallelForEach(*gradylib_helpers::GRADY_LIB_DEFAULT_THREADPOOL,
                                   std::forward<Callable>(f),
                                   std::forward<PartialInitializer>(partialInitializer),
                                   std::forward<FinalInitializer>(finalInitializer),
                                   numThreads);
        }

        // This overload of parallelForEachTakes a thread pool argument
        template<gradylib_helpers::Mergeable ReturnValue = OpenHashMap<Key, Value, HashFunction>,
                typename Callable,
                typename PartialInitializer = gradylib_helpers::PartialDefaultConstructor<ReturnValue>,
                typename FinalInitializer = gradylib_helpers::FinalDefaultConstructor<ReturnValue>>
        requires std::is_invocable_r_v<void, Callable, ReturnValue &, Key const &, Value const &> &&
                 std::is_copy_constructible_v<Callable> &&
                 std::is_invocable_r_v<ReturnValue, PartialInitializer, int, int> &&
                 std::is_invocable_r_v<ReturnValue, FinalInitializer, int>
        std::future<ReturnValue> parallelForEach(ThreadPool & tp,
                                                 Callable && f,
                                                 PartialInitializer && partialInitializer = PartialInitializer{},
                                                 FinalInitializer && finalInitializer = FinalInitializer{},
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
                // The parallelization scheme is to simply partition the arrays backing the map into equal sizes
                size_t stop = start + keys.size() / numThreads + (threadIdx < keys.size() % numThreads ? 1 : 0);
                tp.add([start, stop, f, result, this, partial=partialInitializer(threadIdx, numThreads)]() mutable {
                    for (size_t j = start; j < stop; ++j) {
                        if (setFlags.isFirstSet(j)) {
                            f(partial, keys[j], values[j]);
                        }
                    }
                    std::lock_guard lg(result->finalMutex);
                    mergePartials(result->final, partial);
                    // The lock_guard is protecting remainingThreads
                    if (result->remainingThreads == 1) {
                        result->promise.set_value(std::move(result->final));
                    }
                    --result->remainingThreads;
                });
                start = stop;
            }
            return result->promise.get_future();
        }

        template<typename IndexType>
        friend void writeMappable(std::string filename, OpenHashMap<std::string, IndexType> const & m);

        template<typename IndexType, template<typename> typename HashFunc>
        friend void writeMappable(std::string filename, OpenHashMap<IndexType, std::string, HashFunc> const & m);

        template<typename IndexType, template<typename> typename HashFunc>
        friend void GRADY_LIB_MOCK_OpenHashMap_SET_SECOND_BITS(OpenHashMap<std::string, IndexType, HashFunc> &);
    };


    template<typename IndexType>
    void writeMappable(std::string filename, OpenHashMap<std::string, IndexType> const & m) {
        std::ofstream ofs(filename);
        if (ofs.fail()) {
            std::ostringstream sstr;
            sstr << "Couldn't open file " << filename << " in writeMappable.";
            throw gradylibMakeException(sstr.str());
        }
        size_t mapSize = m.mapSize;
        ofs.write(static_cast<char*>(static_cast<void*>(&mapSize)), 8);
        size_t keySize = m.keys.size();
        ofs.write(static_cast<char*>(static_cast<void*>(&keySize)), 8);
        // We will come back to this position in the file and write the true Value array start position once we know it
        size_t valueOffset = 0;
        auto const valueOffsetWritePos = ofs.tellp();
        ofs.write(static_cast<char*>(static_cast<void*>(&valueOffset)), 8);
        // We will come back to this position in the file and write the true BitPairSet start position once we know it
        size_t bitPairSetOffset = 0;
        auto const bitPairSetOffsetWritePos = ofs.tellp();
        ofs.write(static_cast<char*>(static_cast<void*>(&bitPairSetOffset)), 8);

        size_t keyOffset = 0;
        // This loop will compute the length of each key structure in bytes and use that to compute the offset in bytes
        // to each key given some arbitrary base pointer.  The offset is written to the file.
        for (size_t i = 0; i < m.keys.size(); ++i) {
            ofs.write(static_cast<char*>(static_cast<void*>(&keyOffset)), 8);
            int32_t strLen = m.keys[i].length();
            int32_t strSize = 4 + strLen + gradylib_helpers::getPadLength<4>(strLen);
            keyOffset += strSize;
        }
        // This loop will write the actual key structures
        for (size_t i = 0; i < m.keys.size(); ++i) {
            int32_t len = m.keys[i].length();
            ofs.write(static_cast<char*>(static_cast<void*>(&len)), 4);
            ofs.write(m.keys[i].data(), len);
            gradylib_helpers::writePad<4>(ofs);
        }

        // Write the values
        gradylib_helpers::writePad<8>(ofs);
        valueOffset = ofs.tellp();
        ofs.write(static_cast<char*>(const_cast<void*>(static_cast<void const *>(m.values.data()))), sizeof(IndexType) * keySize);

        // Write the BitPairSet
        gradylib_helpers::writePad<8>(ofs);
        bitPairSetOffset = ofs.tellp();
        m.setFlags.write(ofs);

        // Go back to the Value array and BitPairSet offset locations and write the offsets
        ofs.seekp(valueOffsetWritePos, std::ios::beg);
        ofs.write(static_cast<char*>(static_cast<void*>(&valueOffset)), 8);
        ofs.seekp(bitPairSetOffsetWritePos, std::ios::beg);
        ofs.write(static_cast<char*>(static_cast<void*>(&bitPairSetOffset)), 8);
    }

    template<typename IndexType, template<typename> typename HashFunction>
    void writeMappable(std::string filename, OpenHashMap<IndexType, std::string, HashFunction> const & m) {
        std::ofstream ofs(filename);
        if (ofs.fail()) {
            std::ostringstream sstr;
            sstr << "Couldn't open file " << filename << " in writeMappable.";
            throw gradylibMakeException(sstr.str());
        }
        size_t mapSize = m.mapSize;
        ofs.write(static_cast<char*>(static_cast<void*>(&mapSize)), 8);
        size_t keySize = m.keys.size();
        ofs.write(static_cast<char*>(static_cast<void*>(&keySize)), 8);
        // We will come back to this position in the file and write the true BitPairSet start position once we know it
        size_t bitPairSetOffset = 0;
        auto const bitPairSetOffsetWritePos = ofs.tellp();
        ofs.write(static_cast<char*>(static_cast<void*>(&bitPairSetOffset)), 8);
        size_t valueOffset = 0;
        // This loop will compute the length of each value structure in bytes and use that to compute the offset in bytes
        // to each value given some arbitrary base pointer.  The offset is written to the file.
        for (size_t i = 0; i < keySize; ++i) {
            ofs.write(static_cast<char*>(static_cast<void*>(&valueOffset)), 8);
            int32_t strLen = m.values[i].length();
            int32_t strSize = 4 + strLen + gradylib_helpers::getPadLength<4>(strLen);
            valueOffset += strSize;
        }
        // Write the keys to the file
        ofs.write(static_cast<char*>(const_cast<void*>(static_cast<void const *>(m.keys.data()))), sizeof(IndexType) * keySize);
        // Write the values to the file
        for (size_t i = 0; i < keySize; ++i) {
            int32_t len = m.values[i].length();
            ofs.write(static_cast<char*>(static_cast<void*>(&len)), 4);
            ofs.write(m.values[i].data(), len);
            gradylib_helpers::writePad<4>(ofs);
        }

        // Write the BitPairSet to the file
        gradylib_helpers::writePad<8>(ofs);
        bitPairSetOffset = ofs.tellp();
        m.setFlags.write(ofs);

        // Go back to the BitPairSet start position offset and write it
        ofs.seekp(bitPairSetOffsetWritePos, std::ios::beg);
        ofs.write(static_cast<char*>(static_cast<void*>(&bitPairSetOffset)), 8);
    }

    // The following is for testing.
    template<typename IndexType, template<typename> typename HashFunc>
    void GRADY_LIB_MOCK_OpenHashMap_SET_SECOND_BITS(OpenHashMap<std::string, IndexType, HashFunc> &m) {
        for (size_t i = 0; i < m.setFlags.size(); ++i) {
            m.setFlags.setSecond(i);
        }
    }
}
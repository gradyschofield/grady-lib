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

#include<atomic>
#include<utility>

namespace gradylib {
    template<typename T>
    class CompletionPool {
        struct Node {
            T t;
            Node *next = nullptr;

            Node(T &&t) : t(std::move(t)) {}

            Node(T const &t) : t(t) {}
        };

        std::atomic<Node *> head;

    public:
        CompletionPool()
                : head(nullptr) {
        }

        ~CompletionPool() {
            Node *n = head;
            while (n) {
                Node *next = n->next;
                delete n;
                n = next;
            }
        }

        template<typename U>
        requires std::same_as<std::remove_cvref_t<U>, T>
        void add(U &&t) {
            Node *n = new Node(std::forward<U>(t));
            Node *expected = n->next = head.load(std::memory_order_relaxed);
            while (!head.compare_exchange_strong(expected, n, std::memory_order_release, std::memory_order_relaxed)) {
                n->next = expected;
            }
        }

        /*
         * CompletionPool::begin will steal the work list from the pool,
         * and operator++ on the iterator will delete the list node.
         */
        class iterator {
            Node *n;
        public:
            explicit iterator(Node *n)
                    : n(n) {
            }

            bool operator==(iterator const &other) const {
                return n == other.n;
            }

            bool operator!=(iterator const &other) const {
                return n != other.n;
            }

            T &operator*() {
                return n->t;
            }

            iterator &operator++() {
                if (!n) {
                    return *this;
                }
                auto old = n;
                n = n->next;
                delete old;
                return *this;
            }
        };

        iterator begin() {
            Node * n = head.load(std::memory_order_relaxed);
            while (!head.compare_exchange_strong(n, nullptr, std::memory_order_acquire, std::memory_order_relaxed)) {
                continue;
            }
            return iterator(n);
        }

        iterator end() {
            return iterator(nullptr);
        }
    };
}

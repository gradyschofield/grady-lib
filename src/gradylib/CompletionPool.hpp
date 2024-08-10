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

template<typename T>
class CompletionPool {
    struct Node {
        T t;
        Node * next = nullptr;
    };

    std::atomic<Node*> head;

public:
    CompletionPool()
        : head(nullptr)
    {
    }

    void add(T && t) {
        Node * n = new Node(std::move(t));
        n->next = head.load(std::memory_order_relaxed);
        Node * expected = n->next;
        while(!head.compare_exchange_strong(expected, n, std::memory_order_release, std::memory_order_relaxed)) {
            n->next = expected;
        }
    }

    class iterator {
        Node * n;
    public:
        explicit iterator(Node * n)
            : n(n)
        {
        }

        bool operator==(iterator const &other) const {
            return n == other->n;
        }

        bool operator!=(iterator const &other) const {
            return n != other->n;
        }

        T & operator*() {
            return n->t;
        }

        iterator & operator++() {
            if (!n) {
                return *this;
            }
            n = n->next;
            return *this;
        }
    };

    iterator begin() {
        return iterator(head.load(std::memory_order_acquire));
    }

    iterator end() {
        return iterator(nullptr);
    }
};
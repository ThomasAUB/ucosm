/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * MIT License                                                                     *
 *                                                                                 *
 * Copyright (c) 2024 Thomas AUBERT                                                *
 *                                                                                 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy    *
 * of this software and associated documentation files (the "Software"), to deal   *
 * in the Software without restriction, including without limitation the rights    *
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is           *
 * furnished to do so, subject to the following conditions:                        *
 *                                                                                 *
 * The above copyright notice and this permission notice shall be included in all  *
 * copies or substantial portions of the Software.                                 *
 *                                                                                 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
 * SOFTWARE.                                                                       *
 *                                                                                 *
 * github : https://github.com/ThomasAUB/uLink                                     *
 *                                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

#include <cstddef>
#include <csignal>
#include <type_traits>

namespace ulink {

    // forward declaration
    template<typename T>
    struct Node;

    // non-owning doubly linkled list
    template<typename node_t>
    class List {

        static_assert(
            std::is_convertible_v<node_t*, Node<node_t>*>,
            "Node type error"
            );

        struct Iterator {
            Iterator(Node<node_t>* n) : n(n) {}
            node_t& operator*() { return *static_cast<node_t*>(n); }
            Iterator& operator++() { n = n->next; return *this; }
            bool operator !=(const Iterator& it) const { return (n != it.n); }
            bool operator ==(const Iterator& it) const { return (n == it.n); }
            Node<node_t>* n;
        };

        struct ConstIterator {
            ConstIterator(const Node<node_t>* n) : n(n) {}
            ConstIterator(const Iterator it) : n(it.n) {}
            const node_t& operator*() const { return *static_cast<const node_t*>(n); }
            ConstIterator& operator++() { n = n->next; return *this; }
            bool operator !=(const ConstIterator& it) const { return (n != it.n); }
            bool operator ==(const ConstIterator& it) const { return (n == it.n); }
            const Node<node_t>* n;
        };

    public:

        using iterator = Iterator;
        using const_iterator = ConstIterator;
        using value_type = node_t;
        using size_type = std::size_t;
        using reference = value_type&;
        using const_reference = const value_type&;

        List();

        List(const List<node_t>& other) = delete;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;

        reference front();
        reference back();

        const_reference front() const;
        const_reference back() const;

        size_type size() const;
        bool empty() const;
        void clear();

        void push_front(reference node);
        void push_back(reference node);

        void pop_front();
        void pop_back();

        void insert(const_iterator pos, reference node);
        void erase(iterator pos);

        ~List() { clear(); }

    private:

        void insertAfter(Node<value_type>& pos, reference node);
        void insertBefore(Node<value_type>& pos, reference node);

        Node<value_type> mStartNode;
        Node<value_type> mEndNode;

    };

    template<typename node_t>
    List<node_t>::List() {
        mStartNode.next = static_cast<value_type*>(&mEndNode);
        mEndNode.prev = static_cast<value_type*>(&mStartNode);
    }

    template<typename node_t>
    typename List<node_t>::iterator List<node_t>::begin() {
        return iterator(mStartNode.next);
    }

    template<typename node_t>
    typename List<node_t>::iterator List<node_t>::end() {
        return iterator(&mEndNode);
    }

    template<typename node_t>
    typename List<node_t>::const_iterator List<node_t>::begin() const {
        return const_iterator(mStartNode.next);
    }

    template<typename node_t>
    typename List<node_t>::const_iterator List<node_t>::end() const {
        return const_iterator(&mEndNode);
    }

    template<typename node_t>
    node_t& List<node_t>::front() {
        if (empty()) {
            std::raise(SIGSEGV);
        }
        return *mStartNode.next;
    }

    template<typename node_t>
    node_t& List<node_t>::back() {
        if (empty()) {
            std::raise(SIGSEGV);
        }
        return *mEndNode.prev;
    }

    template<typename node_t>
    const node_t& List<node_t>::front() const {
        if (empty()) {
            std::raise(SIGSEGV);
        }
        return *mStartNode.next;
    }

    template<typename node_t>
    const node_t& List<node_t>::back() const {
        if (empty()) {
            std::raise(SIGSEGV);
        }
        return *mEndNode.prev;
    }

    template<typename node_t>
    typename List<node_t>::size_type List<node_t>::size() const {
        size_type outSize = 0;
        auto* n = mStartNode.next;
        while (n != &mEndNode) {
            outSize++;
            n = n->next;
        }
        return outSize;
    }

    template<typename node_t>
    bool List<node_t>::empty() const {
        return (mStartNode.next == &mEndNode);
    }

    template<typename node_t>
    void List<node_t>::clear() {
        auto* n = mStartNode.next;
        while (n != &mEndNode) {
            auto* t = n;
            n = n->next;
            t->remove();
        }
    }

    template<typename node_t>
    void List<node_t>::push_front(reference node) {
        insertAfter(mStartNode, node);
    }

    template<typename node_t>
    void List<node_t>::push_back(reference node) {
        insertBefore(mEndNode, node);
    }

    template<typename node_t>
    void List<node_t>::pop_front() {
        if (empty()) {
            return;
        }
        mStartNode.next->remove();
    }

    template<typename node_t>
    void List<node_t>::pop_back() {
        if (empty()) {
            return;
        }
        mEndNode.prev->remove();
    }

    template<typename node_t>
    void List<node_t>::insert(const_iterator pos, reference node) {

        if (pos == begin()) {
            push_front(node);
        }
        else if (pos == end()) {
            push_back(node);
        }
        else {
            auto* it = mStartNode.next;
            auto* e = &mEndNode;
            auto* p = &(*pos);

            while (it != e && it != p) {
                it = it->next;
            }

            if (it == p) {
                insertBefore(*it, node);
            }
        }
    }

    template<typename node_t>
    void List<node_t>::erase(iterator pos) {
        if (pos == end()) { // not ideal...
            pop_back();
        }
        else {
            (*pos).remove();
        }
    }

    template<typename node_t>
    void List<node_t>::insertAfter(Node<node_t>& pos, reference node) {
        node.remove();
        node.prev = static_cast<node_t*>(&pos);
        // pos can not be the ending node
        // so next is always supposed to be non-null
        node.next = pos.next;
        node.next->prev = &node;
        pos.next = &node;
    }

    template<typename node_t>
    void List<node_t>::insertBefore(Node<node_t>& pos, reference node) {
        node.remove();
        node.next = static_cast<node_t*>(&pos);
        // pos can not be the starting node
        // so prev is always supposed to be non-null
        node.prev = pos.prev;
        node.prev->next = &node;
        pos.prev = &node;
    }




    template<typename T>
    struct Node {

        void remove();

        bool isLinked() const;

        ~Node() { remove(); }

    protected:

        template<typename node_t>
        friend class List;

        T* prev = nullptr;
        T* next = nullptr;
    };

    template<typename T>
    void Node<T>::remove() {

        if (prev) {
            prev->next = next;
        }

        if (next) {
            next->prev = prev;
        }

        prev = next = nullptr;
    }

    template<typename T>
    bool Node<T>::isLinked() const {
        return (prev != nullptr);
    }




}

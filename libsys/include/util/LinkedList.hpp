#pragma once

#include <util/List.hpp>

/**
 * A LinkedList is an ordered list where each element is contained in a node,
 * and each node has a pointer to the next node which comes after it, forming a
 * chain.
 *
 * LinkedLists are memory efficient in that the size of the list is equal to the
 * LinkedList header and the number of nodes in the list. Array-based stores are
 * costly to grow and shrink, due to their desire to maintain contiguousness,
 * and thus each grow operation grows it by a large amount in anticipation for
 * future insertions. They often don't shrink at all on removal, again to avoid
 * future grow operations.
 *
 * It suffers in random access, as, unlike an array, the elements are not
 * contiguous in memory, and therefore the whole list must be traversed from the
 * beginning in order to find the nth element. It is also, to a much lesser
 * extent, slower to iterate over than an array based store due to the need to
 * dereference and jump all over the heap, whereas arrays are contiguous and may
 * neatly fit in cache lines, etc.
 *
 * @tparam T The type of object the LinkedList holds.
 */
template <typename T> class LinkedList : public virtual List<T>
{
  public:
    class Iterator;

    /**
     * Adds an element to the front of the List.
     * @param obj The object to add.
     * @return `true` if an object was added. `false` otherwise.
     */
    bool push(const T &obj) override
    {
        if (!_first) {
            _first = ArcPtr<Node>::make(obj);
            _last = _first;
        } else {
            _first = _first->insertBefore(obj);
        }

        ++_size;

        return true;
    }

    /**
     * Removes an element from the front of the List.
     * @return The object. If the List is empty, the return value is undefined.
     */
    T pop() override
    {
        auto oldFirst = _first;

        if (_first == _last) {
            _first = nullptr;
            _last = nullptr;
        } else {
            _first = _first->remove()->next;
        }

        if (_size) {
            --_size;
        }

        return oldFirst->value;
    }

    /**
     * Returns the element at the front of the List without removing it.
     * @return The object. If the List is empty, the return value is undefined.
     */
    const T &peek() const override { return _first->value; }

    /**
     * Adds an element to the back of the List.
     * @param obj The object to add.
     * @return `true` if an object was added. `false` otherwise.
     */
    bool enqueue(const T &obj) override
    {
        if (!_last) {
            return push(obj); // list is empty
        }

        _last = _last->insertAfter(obj);
        ++_size;
        return true;
    }

    /**
     * Removes an element from the back of the List.
     * @return The object. If the List is empty, the return value is undefined.
     */
    T unqueue() override
    {
        auto oldLast = _last;

        if (_first == _last) {
            _first = nullptr;
            _last = nullptr;
        } else {
            _last = _last->remove()->prev;
        }

        if (_size) {
            --_size;
        }

        return oldLast->value;
    }

    /**
     * Returns the element at the back of the List without removing it.
     * @return The object. If the List is empty, the return value is undefined.
     */
    const T &peekBack() const override { return _last->value; }

    /**
     * Returns whether or not the List is empty.
     * @return `true` if empty, `false` otherwise.
     */
    bool isEmpty() const { return _first == nullptr; }

    /**
     * The number of elements in the List.
     * @return A size_t equal to the number of elements.
     */
    virtual size_t size() const
    {
        return _size;
    }

    /**
     * Removes the first element equal to `object`.
     * @param object The object to remove.
     * @return `true` if something was removed, false otherwise.
     */
    bool remove(const T &object) override
    {
        if (!_first) {
            return false; // no objects
        }

        auto it = _first;
        while (it != nullptr) {
            if (it->value == object) {
                auto target = it->remove();
                if (target == _first) _first = target->next;
                if (target == _last) _last = target->prev;
                --_size;
                return true;
            }

            it = it->next;
        }

        return false;
    }

    /**
     * Returns the element at the given index. No bounds checking is performed.
     *
     * @note This is a linked list, so this operation is O(n)!
     * @param idx The index of the object.
     * @return The object at that index. If the index is out of bounds, the
     *         return value is undefined.
     */
    T& operator[](size_t idx) const override
    {
        Node *it = _first.get();
        for (; idx > 0; --idx) {
            it = it->next.get();
        }

        return it->value;
    }

    /** An iterator pointing to the first element in the list. */
    Iterator begin() const
    {
        return Iterator{_first.get(), this};
    }

    /**
     * An Iterator "past the end" of the list. When encountered, it means there
     * is nothing else to read and iteration should stop.
     *
     * @return An iterator signifying the end of this LinkedList.
     */
    Iterator end() const { return Iterator{nullptr, this}; }

  private:
    struct Node : public Object {
        Node(const T &v) : value(v) {}

        /**
         * Inserts an object into the List positioned before this node.
         * @param obj The object to add.
         * @return The newly created node.
         */
        ArcPtr<Node> insertBefore(const T &obj)
        {
            auto n = ArcPtr<Node>::make(obj);
            n->next = this;
            n->prev = prev;
            if (prev) prev->next = n;
            prev = n;
            return n;
        }

        /**
         * Inserts an object into the List positioned after this node.
         * @param obj The object to add.
         * @return The newly created node.
         */
        ArcPtr<Node> insertAfter(const T &obj)
        {
            auto n = ArcPtr<Node>::make(obj);
            n->prev = this;
            n->next = next;
            if (next) next->prev = n;
            next = n;
            return n;
        }

        /**
         * Removes itself from the list. :(
         * @return ArcPtr to itself.
         */
        ArcPtr<Node> remove()
        {
            // store this in an ArcPtr in case our neighbors are our sole owners
            ArcPtr<Node> self = this;
            if (prev) prev->next = next;
            if (next) next->prev = prev;
            return self;
        }

        T value;
        ArcPtr<Node> prev = nullptr;
        ArcPtr<Node> next = nullptr;
    };

    ArcPtr<Node> _first = nullptr;
    ArcPtr<Node> _last = nullptr;
    size_t _size = 0;

  public:
    class Iterator : public Object
    {
        friend class LinkedList;
      public:
        /**
         * (pre-increment) Advances the iterator one element forward.
         * @return A reference to the next Iterator in the Collection.
         */
        Iterator& operator++()
        {
            _obj = _obj->next.get();
            return *this;
        }

        /**
         * (post-increment) Advances the Iterator one element forward.
         * @return An Iterator equal to this before the increment.
         */
        Iterator operator++(int)
        {
            Iterator it = *this;
            this->operator++();
            return it;
        }

        /**
         * Retrieves a reference to the pointed-to element.
         * @return A reference to the element the Iterator is pointing at. If it
         *         isn't pointing to anything (e.g. end of the Collection), the
         *         result is undefined.
         */
        const T& operator*() const { return _obj->value; }

        /**
         * Equality operator.
         * @param rhs The iterator to compare to.
         * @return `true` if they are equal, `false` otherwise.
         */
        bool operator==(const Iterator &rhs) const
        {
            return _obj == rhs._obj && _parent == rhs._parent;
        }

        /**
         * Inequality operator. Equivalent to !(this == rhs).
         * @param rhs The Iterator to compare to.
         * @return `true` if they're not equal, `false` otherwise.
         */
        bool operator!=(const Iterator &rhs) const { return !operator==(rhs); }

      private:
        Iterator(Node const *node, LinkedList const *parent)
                : _obj(node), _parent(parent)
        {}

        Node const *_obj;
        LinkedList const *_parent;
    };
};
/*
 * cache_vector.h
 *
 * Copyright © 2018 Oliver Adams
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef CPPDATALIB_CACHE_VECTOR_H
#define CPPDATALIB_CACHE_VECTOR_H

#include <cstdlib>
#include <iterator>
#include <memory>
//#include "global.h"

#ifndef CPPDATALIB_CPP11
#include <vector>
#endif

namespace cppdatalib
{
    namespace core
    {
        /* cache_vector_n: A vector class that stores N elements on the stack, and allocates if it needs to grow.
         * This allows the vector to defer allocation until it grows past N elements. There should be a speed increase
         * in using cache_vector_n over std::vector for shallow (perhaps one level deep) stack-implemented-on-a-vector usage.
         */

#ifndef CPPDATALIB_CPP11
        template<typename T, size_t N>
        class cache_vector_n : public std::vector<T>
        {
        public:
            cache_vector_n() : std::vector<T>() {}
            explicit cache_vector_n(size_t n, const T &v) : std::vector<T>(n, v) {}
        };
#else
        template<typename T, size_t N, typename Allocator = std::allocator<T>>
        class cache_vector_n
        {
            friend class iterator;
            friend class const_iterator;

            static_assert(N != 0, "cache_vector_n::N must be greater than zero");

        public:
            typedef size_t size_type;
            typedef T value_type;
            typedef value_type &reference;
            typedef const value_type &const_reference;
            typedef Allocator allocator_type;
            typedef typename std::allocator_traits<allocator_type>::pointer pointer;
            typedef typename std::allocator_traits<allocator_type>::const_pointer const_pointer;

            class iterator : std::iterator<std::random_access_iterator_tag,
                                           typename Allocator::value_type,
                                           typename Allocator::difference_type,
                                           typename Allocator::pointer,
                                           typename Allocator::reference>
            {
                friend class cache_vector_n;

                typedef std::iterator<std::random_access_iterator_tag,
                                        typename Allocator::value_type,
                                        typename Allocator::difference_type,
                                        typename Allocator::pointer,
                                        typename Allocator::reference> base;

            public:
                typedef typename base::value_type value_type;
                typedef typename base::difference_type difference_type;
                typedef typename base::reference reference;
                typedef typename base::pointer pointer;

            private:
                iterator(cache_vector_n *parent, typename cache_vector_n<T, N, Allocator>::size_type pos = 0) : mParent(parent), mPtr(parent->valueAddress(pos)), mPos(pos) {}

            public:
                iterator() : mParent(nullptr), mPtr(nullptr), mPos(0) {}

                iterator &operator++()
                {
                    if (++mPos == N)
                        mPtr = mParent->mSlow;
                    else
                        ++mPtr;
                    return *this;
                }

                iterator operator++(int) {iterator temp(*this); ++*this; return temp;}

                iterator &operator--()
                {
                    if (mPos-- == N)
                        mPtr = mParent->mFast + (N-1);
                    else
                        --mPtr;
                    return *this;
                }

                iterator operator--(int) {iterator temp(*this); --*this; return temp;}

                iterator &operator+=(difference_type n)
                {
                    mPos += n;
                    mPtr = mParent->valueAddress(mPos);
                    return *this;
                }
                iterator operator+(difference_type n) const
                {
                    iterator temp(*this);
                    return temp += n;
                }
                friend iterator operator+(difference_type n, const iterator &it)
                {
                    return it + n;
                }

                iterator &operator-=(difference_type n) {return operator+=(-n);}
                iterator operator-(difference_type n) const {return operator+(-n);}

                difference_type operator-(const iterator &other) const
                {
                    return difference_type(mPos) - difference_type(other.mPos);
                }

                reference operator[](difference_type n) {return *(operator+(n));}
                reference operator*() {return *mPtr;}
                pointer operator->() {return mPtr;}

                bool operator<(const iterator &other) const {return mPtr < other.mPtr;}
                bool operator>(const iterator &other) const {return mPtr > other.mPtr;}
                bool operator<=(const iterator &other) const {return mPtr <= other.mPtr;}
                bool operator>=(const iterator &other) const {return mPtr >= other.mPtr;}
                bool operator==(const iterator &other) const {return mPtr == other.mPtr;}
                bool operator!=(const iterator &other) const {return !(*this == other);}

            private:
                cache_vector_n<T, N, Allocator> *mParent;
                pointer mPtr;
                typename cache_vector_n<T, N, Allocator>::size_type mPos;
            };

            class const_iterator : std::iterator<std::random_access_iterator_tag,
                                           const typename Allocator::value_type,
                                           typename Allocator::difference_type,
                                           typename Allocator::pointer,
                                           const typename Allocator::reference>
            {
                friend class cache_vector_n;

                typedef std::iterator<std::random_access_iterator_tag,
                                        typename Allocator::value_type,
                                        typename Allocator::difference_type,
                                        typename Allocator::const_pointer,
                                        typename Allocator::const_reference> base;

            public:
                typedef typename base::value_type value_type;
                typedef typename base::difference_type difference_type;
                typedef typename base::reference reference;
                typedef typename base::pointer pointer;

            private:
                const_iterator(const cache_vector_n *parent, typename cache_vector_n<T, N, Allocator>::size_type pos = 0) : mParent(parent), mPtr(parent->valueAddress(pos)), mPos(pos) {}

            public:
                const_iterator() : mParent(nullptr), mPtr(nullptr), mPos(0) {}

                const_iterator &operator++()
                {
                    if (++mPos == N)
                        mPtr = mParent->mSlow;
                    else
                        ++mPtr;
                    return *this;
                }

                const_iterator operator++(int) {const_iterator temp(*this); ++*this; return temp;}

                const_iterator &operator--()
                {
                    if (mPos-- == N)
                        mPtr = mParent->mFast + (N-1);
                    else
                        --mPtr;
                    return *this;
                }

                const_iterator operator--(int) {const_iterator temp(*this); --*this; return temp;}

                const_iterator &operator+=(difference_type n)
                {
                    mPos += n;
                    mPtr = mParent->valueAddress(mPos);
                    return *this;
                }
                const_iterator operator+(difference_type n) const
                {
                    const_iterator temp(*this);
                    return temp += n;
                }
                friend const_iterator operator+(difference_type n, const const_iterator &it)
                {
                    return it + n;
                }

                const_iterator &operator-=(difference_type n) {return operator+=(-n);}
                const_iterator operator-(difference_type n) const {return operator+(-n);}

                difference_type operator-(const const_iterator &other) const
                {
                    return difference_type(mPos) - difference_type(other.mPos);
                }

                reference operator[](difference_type n) const {return *(operator+(n));}
                reference operator*() const {return *mPtr;}
                pointer operator->() const {return mPtr;}

                bool operator<(const const_iterator &other) const {return mPtr < other.mPtr;}
                bool operator>(const const_iterator &other) const {return mPtr > other.mPtr;}
                bool operator<=(const const_iterator &other) const {return mPtr <= other.mPtr;}
                bool operator>=(const const_iterator &other) const {return mPtr >= other.mPtr;}
                bool operator==(const const_iterator &other) const {return mPtr == other.mPtr;}
                bool operator!=(const const_iterator &other) const {return !(*this == other);}

            private:
                const cache_vector_n<T, N, Allocator> *mParent;
                pointer mPtr;
                typename cache_vector_n<T, N, Allocator>::size_type mPos;
            };

            typedef std::reverse_iterator<iterator> reverse_iterator;
            typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

            explicit cache_vector_n(const allocator_type &alloc = allocator_type())
                : mSlow(nullptr)
                , mSize(0)
                , mSlowCapacity(0)
                , alloc(alloc)
            {}

            explicit cache_vector_n(size_type n, const allocator_type &alloc = allocator_type())
                : mSlow(nullptr)
                , mSize(0)
                , mSlowCapacity(0)
                , alloc(alloc)
            {
                growCapacity(n);
                construct(N, n);
                mSize = n;
            }

            explicit cache_vector_n(size_type n, const value_type &v, const allocator_type &alloc = allocator_type())
                : mSlow(nullptr)
                , mSize(0)
                , mSlowCapacity(0)
                , alloc(alloc)
            {
                growCapacity(n);
                construct(0, n, v);
                mSize = n;
            }

            cache_vector_n(const cache_vector_n &rhs)
                : mSlow(nullptr)
                , mSize(0)
                , mSlowCapacity(0)
                , alloc(std::allocator_traits<Allocator>::select_on_container_copy_construction(rhs.alloc))
            {
                growCapacity(rhs.size());
                copyConstruct(0, rhs.size(), rhs.begin());
                mSize = rhs.size();
            }

            cache_vector_n(const cache_vector_n &rhs, const allocator_type &alloc)
                : mSlow(nullptr)
                , mSize(0)
                , mSlowCapacity(0)
                , alloc(alloc)
            {
                growCapacity(rhs.size());
                copyConstruct(0, rhs.size(), rhs.begin());
                mSize = rhs.size();
            }

            cache_vector_n(cache_vector_n &&rhs)
                : mSlow(nullptr)
                , mSize(0)
                , mSlowCapacity(0)
            {
                swap(rhs);
            }

            cache_vector_n(cache_vector_n &&rhs, const allocator_type &alloc)
                : mSlow(nullptr)
                , mSize(0)
                , mSlowCapacity(0)
                , alloc(alloc)
            {
                swapWithoutAllocator(rhs);
            }

            cache_vector_n(std::initializer_list<value_type> list, const allocator_type &alloc = allocator_type())
                : mSlow(nullptr)
                , mSize(0)
                , mSlowCapacity(0)
                , alloc(alloc)
            {
                growCapacity(list.size());
                moveConstruct(0, list.size(), list.begin());
                mSize = list.size();
            }

            ~cache_vector_n()
            {
                destroy(0, size());
                if (mSlow)
                    std::allocator_traits<allocator_type>::deallocate(alloc, mSlow, mSlowCapacity);
            }

            iterator begin() {return iterator(this, 0);}
            iterator end() {return iterator(this, size());}
            reverse_iterator rbegin() {return reverse_iterator(begin());}
            reverse_iterator rend() {return reverse_iterator(end());}

            const_iterator begin() const {return const_iterator(this, 0);}
            const_iterator end() const {return const_iterator(this, size());}
            const_iterator cbegin() const {return const_iterator(this, 0);}
            const_iterator cend() const {return const_iterator(this, size());}
            const_reverse_iterator rbegin() const {return const_reverse_iterator(begin());}
            const_reverse_iterator rend() const {return const_reverse_iterator(end());}
            const_reverse_iterator crbegin() const {return const_reverse_iterator(cbegin());}
            const_reverse_iterator crend() const {return const_reverse_iterator(cend());}

            void swap(cache_vector_n &rhs)
            {
                swapByAllocTraits(rhs);
            }

            void push_back(const value_type &v)
            {
                const size_type pos = size();
                growCapacity(pos + 1);
                construct(pos, pos + 1, v);
                ++mSize;
            }
            void push_back(value_type &&v)
            {
                const size_type pos = size();
                growCapacity(pos + 1);
                construct(pos, pos + 1, std::move(v));
                ++mSize;
            }

            template<typename... Args>
            void emplace_back(Args&&... args)
            {
                const size_type pos = size();
                growCapacity(pos + 1);
                construct(pos, pos + 1, std::forward<Args>(args)...);
                ++mSize;
            }

            void pop_back() noexcept
            {
                --mSize;
                destroy(mSize, mSize + 1);
            }

            bool empty() const {return mSize == 0;}
            size_type size() const {return mSize;}
            size_type capacity() const {return mSlowCapacity + N;}
            size_type max_size() const {return std::numeric_limits<size_type>::max();}

            reference operator[](size_type idx) {return value(idx);}
            const_reference operator[](size_type idx) const {return value(idx);}

            reference at(size_type idx)
            {
                if (idx >= size())
                    throw std::out_of_range("cache_vector_n: invalid index provided to at() member function");
                return value(idx);
            }
            const_reference at(size_type idx) const
            {
                if (idx >= size())
                    throw std::out_of_range("cache_vector_n: invalid index provided to at() member function");
                return value(idx);
            }

            void reserve(size_type sz) {growCapacity(sz);}
            void resize(size_type sz)
            {
                if (sz < size())
                    destroy(sz, size());
                else if (sz > size())
                {
                    growCapacity(sz);
                    construct(size(), sz);
                }
                mSize = sz;
            }
            void resize(size_type sz, const value_type &v)
            {
                if (sz < size())
                    destroy(sz, size());
                else if (sz > size())
                {
                    growCapacity(sz);
                    construct(size(), sz, v);
                }
                mSize = sz;
            }

            reference front() {return value(0);}
            const_reference front() const {return value(0);}

            reference back() {return value(size() - 1);}
            const_reference back() const {return value(size() - 1);}

            allocator_type get_allocator() const noexcept {return alloc;}

            void clear() noexcept
            {
                destroy(0, size());
                mSize = 0;
            }

            cache_vector_n &operator=(const cache_vector_n &other)
            {
                if (other.size() > size())
                {
                    growCapacity(other.size());
                    copyConstruct(size(), other.size(), other.begin() + size());
                }
                else if (other.size() < size())
                    destroy(other.size(), size());

                mSize = other.size();
                std::copy(other.begin(), other.begin() + size(), begin());

                if (typename std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment())
                    alloc = other.alloc;

                return *this;
            }
            cache_vector_n &operator=(cache_vector_n &&other)
            {
                swapWithoutAllocator(other);

                if (typename std::allocator_traits<allocator_type>::propagate_on_container_move_assignment())
                    alloc = std::move(alloc);

                return *this;
            }
            cache_vector_n &operator=(std::initializer_list<value_type> list) {assign(list); return *this;}

            void assign(size_type n, const value_type &v)
            {
                destroy(0, size());
                growCapacity(n);
                construct(0, n, v);
                mSize = n;
            }
            void assign(std::initializer_list<value_type> list)
            {
                destroy(0, size());
                growCapacity(list.size());
                moveConstruct(0, list.size(), list.begin());
                mSize = list.size();
            }

            /* TODO:
             *
             *  erase()
             *  insert()
             *  emplace()
             *  relational operators
             *
             */

            void shrink_to_fit() {/* TODO: implement shrinking */}

        private:
            void swapByAllocTraits(cache_vector_n &rhs)
            {
                using std::swap;
                if (mSize >= N && rhs.mSize >= N)
                {
                    for (size_type i = 0; i < N; ++i)
                        swap(mFast[i], rhs.mFast[i]);
                }
                else if (mSize < rhs.mSize)
                {
                    for (size_type i = 0; i < mSize; ++i)
                        swap(mFast[i], rhs.mFast[i]);
                    moveConstruct(mSize, rhs.mSize, rhs.mFast + mSize);
                    rhs.destroy(mSize, rhs.mSize);
                }
                else if (rhs.mSize < mSize)
                {
                    for (size_type i = 0; i < mSize; ++i)
                        swap(mFast[i], rhs.mFast[i]);
                    rhs.moveConstruct(rhs.mSize, mSize, mFast + rhs.mSize);
                    destroy(rhs.mSize, mSize);
                }
                swap(mSlow, rhs.mSlow);
                swap(mSize, rhs.mSize);
                swap(mSlowCapacity, rhs.mSlowCapacity);
                if (typename std::allocator_traits<allocator_type>::propagate_on_container_swap())
                    swap(alloc, rhs.alloc);
            }

            void swapWithoutAllocator(cache_vector_n &rhs)
            {
                using std::swap;
                if (mSize >= N && rhs.mSize >= N)
                {
                    for (size_type i = 0; i < N; ++i)
                        swap(mFast[i], rhs.mFast[i]);
                }
                else if (mSize < rhs.mSize)
                {
                    for (size_type i = 0; i < mSize; ++i)
                        swap(mFast[i], rhs.mFast[i]);
                    moveConstruct(mSize, rhs.mSize, rhs.mFast + mSize);
                    rhs.destroy(mSize, rhs.mSize);
                }
                else if (rhs.mSize < mSize)
                {
                    for (size_type i = 0; i < mSize; ++i)
                        swap(mFast[i], rhs.mFast[i]);
                    rhs.moveConstruct(rhs.mSize, mSize, mFast + rhs.mSize);
                    destroy(rhs.mSize, mSize);
                }
                swap(mSlow, rhs.mSlow);
                swap(mSize, rhs.mSize);
                swap(mSlowCapacity, rhs.mSlowCapacity);
            }

            // Find currently-used size of mSlow
            size_type slow_size() const {return mSize > N? mSize - N: 0;}

            // Grow the current capacity to hold `required` objects
            // NOTE: DOES NOT CHANGE VECTOR SIZE!
            void growCapacity(size_type required)
            {
                if (capacity() < required)
                {
                    const size_type old_slow_size = slow_size();
                    const size_type new_slow_size = std::max(required - N, old_slow_size + (old_slow_size >> 1)); // Grow by 1.5 times, at the minimum

                    // Allocate new block
                    T *new_mSlow = std::allocator_traits<allocator_type>::allocate(alloc, new_slow_size);

                    if (mSlow != nullptr)
                    {
                        // Move-construct new block
                        pMoveConstruct(new_mSlow, mSlow, mSlow + old_slow_size);

                        // Destroy current block
                        destroy(N, mSize);

                        // Deallocate current block
                        std::allocator_traits<allocator_type>::deallocate(alloc, mSlow, mSlowCapacity);
                    }

                    mSlow = new_mSlow;
                    mSlowCapacity = required - N;
                }
            }

            // Default- or Copy-initialize values in specified range
            template<typename... Args>
            void construct(size_type begin, size_type end, Args&&... args)
            {
                size_type i;

                try {
                    for (i = begin; i < std::min(N, end); ++i)
                        mFast[i] = T(std::forward<Args>(args)...);
                    for (i = std::max(N, begin); i < end; ++i)
                        std::allocator_traits<allocator_type>::construct(alloc, mSlow + (i - N), std::forward<Args>(args)...);
                } catch (...) {
                    destroy(begin, i);
                    throw;
                }
            }
            void construct(size_type begin, size_type end)
            {
                size_type i;

                try {
                    for (i = begin; i < std::min(N, end); ++i)
                        mFast[i] = {};
                    for (i = std::max(N, begin); i < end; ++i)
                        std::allocator_traits<allocator_type>::construct(alloc, mSlow + (i - N), T{});
                } catch (...) {
                    destroy(begin, i);
                    throw;
                }
            }

            // Move-construct values from specified range
            template<typename It>
            void moveConstruct(const size_type pos, const size_type end, It begin)
            {
                size_type i;

                try {
                    for (i = pos; i < std::min(N, pos); ++i, ++begin)
                        mFast[i] = std::move(*begin);
                    for (i = std::max(N, pos); i < end; ++i, ++begin)
                        std::allocator_traits<allocator_type>::construct(alloc, mSlow + (i - N), std::move(*begin));
                } catch (...) {
                    destroy(pos, i);
                    throw;
                }
            }

            // Copy-construct values from specified range
            template<typename It>
            void copyConstruct(const size_type pos, const size_type end, It begin)
            {
                size_type i;

                try {
                    for (i = pos; i < std::min(N, end); ++i, ++begin)
                        mFast[i] = *begin;
                    for (i = std::max(N, pos); i < end; ++i, ++begin)
                        std::allocator_traits<allocator_type>::construct(alloc, mSlow + (i - N), *begin);
                } catch (...) {
                    destroy(pos, i);
                    throw;
                }
            }

            // Destroy values in specified range
            void destroy(size_type begin, size_type end) noexcept
            {
                for (size_type i = std::max(N, begin); i < end; ++i)
                    std::allocator_traits<allocator_type>::destroy(alloc, mSlow + (i - N));
            }

            // Default- or Copy-initialize values in specified array
            template<typename... Args>
            void pConstruct(T *begin, T *end, Args&&... args)
            {
                for (; begin < end; ++begin)
                    std::allocator_traits<allocator_type>::construct(alloc, begin, std::forward<Args>(args)...);
            }
            void pConstruct(T *begin, T *end)
            {
                for (; begin < end; ++begin)
                    std::allocator_traits<allocator_type>::construct(alloc, begin, T{});
            }

            // Move-construct values in specified array
            template<typename It>
            void pMoveConstruct(T *result, It begin, It end)
            {
                for (; begin < end; ++begin)
                    std::allocator_traits<allocator_type>::construct(alloc, result++, std::move(*begin));
            }

            // Copy-construct values in specified array
            template<typename It>
            void pCopyConstruct(T *result, It begin, It end)
            {
                for (; begin < end; ++begin)
                    std::allocator_traits<allocator_type>::construct(alloc, result++, *begin);
            }

            // Destroy values in specified array
            void pDestroy(T *begin, T *end)
            {
                for (; begin < end; ++begin)
                    std::allocator_traits<allocator_type>::destroy(alloc, begin);
            }

            // Return a reference to the specified value
            T &value(size_type pos)
            {
                return pos < N? mFast[pos]: mSlow[pos - N];
            }
            const T &value(size_type pos) const
            {
                return pos < N? mFast[pos]: mSlow[pos - N];
            }

            T *valueAddress(size_type pos)
            {
                return pos < N? mFast + pos: mSlow + (pos - N);
            }
            const T *valueAddress(size_type pos) const
            {
                return pos < N? mFast + pos: mSlow + (pos - N);
            }

            T mFast[N];
            T *mSlow;
            size_type mSize, mSlowCapacity;
            allocator_type alloc;
        };
#endif // CPPDATALIB_CPP11
    }
}

#endif // CPPDATALIB_CACHE_VECTOR_H

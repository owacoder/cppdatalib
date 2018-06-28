/*
 * global.h
 *
 * Copyright © 2017 Oliver Adams
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

#ifndef CPPDATALIB_GLOBAL_H
#define CPPDATALIB_GLOBAL_H

#include <cstdlib>
#include <vector>
#include <array>
#include <limits>

#if defined(__linux) || defined(__linux__)
#define CPPDATALIB_LINUX
#elif defined(__WIN32) || defined(__WIN64)
#define CPPDATALIB_WINDOWS
#endif

#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
#define CPPDATALIB_X86_64
#define CPPDATALIB_X86
#define CPPDATALIB_LITTLE_ENDIAN
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__IA32__) || defined(_M_I86) || defined(_M_IX86) || defined(__X86__) || defined(_X86_) || defined(__I86__) || defined(__THW_INTEL__) || defined(__INTEL__) || defined(__386)
#define CPPDATALIB_X86
#define CPPDATALIB_LITTLE_ENDIAN
#endif

#if __cplusplus >= 201703L
#define CPPDATALIB_CPP17
#endif

#if __cplusplus >= 201402L
#define CPPDATALIB_CPP14
#endif

#if __cplusplus >= 201103L
#define CPPDATALIB_CPP11
#endif

namespace cppdatalib
{
    namespace core
    {
        enum
        {
            max_utf8_code_sequence_size = 4,
#ifdef CPPDATALIB_BUFFER_SIZE
            buffer_size = CPPDATALIB_BUFFER_SIZE,
#else
            buffer_size = 2048,
#endif
#ifdef CPPDATALIB_CACHE_SIZE
            cache_size = CPPDATALIB_CACHE_SIZE
#else
            cache_size = 3
#endif
        };

        enum network_library
        {
            unknown_network_library = -1
#ifdef CPPDATALIB_ENABLE_CURL_NETWORK
            , curl_network_library
#endif
#ifdef CPPDATALIB_ENABLE_QT_NETWORK
            , qt_network_library
#endif
#ifdef CPPDATALIB_ENABLE_POCO_NETWORK
            , poco_network_library
#endif
            , network_library_count
#ifdef CPPDATALIB_DEFAULT_NETWORK_LIBRARY
            , default_network_library = CPPDATALIB_DEFAULT_NETWORK_LIBRARY
#else
            , default_network_library = 0
#endif
        };

#ifdef CPPDATALIB_BOOL_T
        typedef CPPDATALIB_BOOL_T bool_t;
#else
        typedef bool bool_t;
#endif

#ifdef CPPDATALIB_INT_T
        typedef CPPDATALIB_INT_T int_t;
#else
        typedef int64_t int_t;
#endif

#ifdef CPPDATALIB_UINT_T
        typedef CPPDATALIB_UINT_T uint_t;
#else
        typedef uint64_t uint_t;
#endif

#ifdef CPPDATALIB_REAL_T
        typedef CPPDATALIB_REAL_T real_t;
#else
        typedef double real_t;
#define CPPDATALIB_REAL_DIG std::numeric_limits<double>::max_digits10
#endif

#ifdef CPPDATALIB_CSTRING_T
        typedef CPPDATALIB_CSTRING_T cstring_t;
#else
        typedef const char *cstring_t;
#endif

#ifdef CPPDATALIB_STRING_T
        typedef CPPDATALIB_STRING_T string_t;
#else
        typedef std::string string_t;
#endif

        template<typename T>
        struct optional
        {
            optional() : val_{}, used_(false) {}
            optional(T v) : val_(v), used_(true) {}

            optional &operator=(T v) {val_ = v; used_ = true; return *this;}

            template<typename U>
            bool operator==(const optional<U> &other)
            {
                if (used_ != other.used_)
                    return false;
                else if (!used_ && !other.used_)
                    return true;

                return val_ == other.val_;
            }
            template<typename U>
            bool operator!=(const optional<U> &other) {return !(*this == other);}

            T value() const {return val_;}
            const T *operator->() const {return &val_;}
            T operator*() const {return val_;}

            explicit operator bool() const {return has_value();}
            bool has_value() const {return used_;}
            void reset() {val_ = {}; used_ = false;}

        private:
            T val_;
            bool used_;
        };

        typedef optional<uint64_t> optional_size;

#ifndef CPPDATALIB_DISABLE_TEMP_STRING
#ifdef CPPDATALIB_STRING_VIEW_T
        typedef CPPDATALIB_STRING_VIEW_T string_view_t;
#elif defined(CPPDATALIB_CPP17)
        typedef std::string_view string_view_t;
#else
        namespace impl
        {
            template<typename T>
            class string_view_t
            {
            public:
                typedef T value_type;
                typedef T *pointer;
                typedef const T *const_pointer;
                typedef T &reference;
                typedef const T &const_reference;
                typedef const_pointer const_iterator;
                typedef const_iterator iterator;
                typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
                typedef const_reverse_iterator reverse_iterator;
                typedef std::size_t size_type;
                typedef std::ptrdiff_t difference_type;

                static constexpr size_type npos = -1;

                string_view_t() noexcept : data_(nullptr), size_(0) {}
                string_view_t(const string_view_t &other) noexcept = default;
                string_view_t(const_pointer nul_terminated) : data_(nul_terminated), size_(strlen(nul_terminated)) {}
                string_view_t(const_pointer data, size_type size) : data_(data), size_(size) {}
                string_view_t(const string_t &str) : data_(str.data()), size_(str.size()) {}

                explicit operator std::basic_string<value_type>() const {return std::basic_string<value_type>(data(), size());}

                constexpr const_iterator begin() const noexcept {return data_;}
                constexpr const_iterator end() const noexcept {return data_ + size();}
                constexpr const_iterator cbegin() const noexcept {return data_;}
                constexpr const_iterator cend() const noexcept {return data_ + size();}
                constexpr const_reverse_iterator rbegin() const noexcept {return const_reverse_iterator(end());}
                constexpr const_reverse_iterator rend() const noexcept {return const_reverse_iterator(begin());}
                constexpr const_reverse_iterator crbegin() const noexcept {return const_reverse_iterator(end());}
                constexpr const_reverse_iterator crend() const noexcept {return const_reverse_iterator(begin());}

                constexpr const_pointer data() const noexcept {return data_;}
                constexpr const_reference operator[](size_type idx) const {return data_[idx];}
                constexpr const_reference at(size_type idx) const {return idx >= size()? (throw std::out_of_range("impl::string_view_t - element access is out of bounds"), front()): data_[idx];}

                void remove_prefix(size_type n) {data_ += n;}
                void remove_suffix(size_type n) {size_ -= n;}
                void swap(string_view_t &other)
                {
                    using namespace std;
                    swap(data_, other.data_);
                    swap(size_, other.size_);
                }

                constexpr const_reference front() const {return *data_;}
                constexpr const_reference back() const {return data_[size() - 1];}

                constexpr size_type size() const {return size_;}
                constexpr size_type length() const {return size_;}

                constexpr size_type max_length() const {return npos - 1;}

                constexpr bool empty() const {return size() == 0;}

                size_type copy(value_type *dest,
                               size_type count,
                               size_type pos = 0) const
                {
                    using namespace std;

                    if (pos > size())
                        throw std::out_of_range("impl::string_view_t - element access is out of bounds");

                    count = min(count, size() - pos);
                    for (size_type idx = pos; idx < pos + count; ++idx)
                        *dest++ = data_[idx];

                    return count;
                }

                constexpr string_view_t substr(size_type pos = 0, size_type count = npos) const
                {
                    return pos > size()? (throw std::out_of_range("impl::string_view_t - element access is out of bounds"), string_view_t()):
                                         string_view_t(data_ + pos, std::min(count, size() - pos));
                }

#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                int compare(string_view_t v) const noexcept
                {
                    using namespace std;

                    for (size_type i = 0; i < min(size(), v.size()); ++i)
                    {
                        if (data_[i] < v[i])
                            return -1;
                        else if (v[i] < data_[i])
                            return 1;
                    }

                    if (size() == v.size())
                        return 0;
                    else if (size() < v.size())
                        return -1;
                    else // (size() > v.size())
                        return 1;
                }
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                int compare(size_type pos1, size_type count1, string_view_t v) const {return substr(pos1, count1).compare(v);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                int compare(size_type pos1, size_type count1, string_view_t v, size_type pos2, size_type count2) const {return substr(pos1, count1).compare(v.substr(pos2, count2));}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                int compare(const_pointer s) const {return compare(string_view_t(s));}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                int compare(size_type pos, size_type count1, const_pointer s) const {return substr(pos, count1).compare(string_view_t(s));}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                int compare(size_type pos, size_type count1, const_pointer s, size_type count2) const {return substr(pos, count1).compare(string_view_t(s, count2));}

#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                bool starts_with(string_view_t v) const noexcept {return size() >= v.size() && compare(0, v.size(), v) == 0;}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                bool starts_with(value_type v) const noexcept {return size() > 0 && compare(0, 1, string_view_t(&v, 1)) == 0;}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                bool starts_with(const_pointer s) const {return starts_with(string_view_t(s));}

#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                bool ends_with(string_view_t v) const noexcept {return size() >= v.size() && compare(size()-v.size(), v.size(), v) == 0;}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                bool ends_with(value_type v) const noexcept {return size() > 0 && compare(size()-1, 1, string_view_t(&v, 1)) == 0;}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                bool ends_with(const_pointer s) const {return ends_with(string_view_t(s));}

#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find(string_view_t v, size_type pos = 0) const noexcept
                {
                    if (v.size() > size())
                        return npos;

                    for (; pos <= size() - v.size(); ++pos)
                        if (compare(pos, v.size(), v) == 0)
                            return pos;

                    return npos;
                }
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find(value_type v, size_type pos = 0) const noexcept {return find(string_view_t(&v, 1), pos);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find(const_pointer v, size_type pos, size_type count) const noexcept {return find(string_view_t(v, count), pos);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find(const_pointer s, size_type pos = 0) const noexcept {return find(string_view_t(s), pos);}

#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type rfind(string_view_t v, size_type pos = npos) const noexcept
                {
                    if (v.size() > size())
                        return npos;

                    for (pos = std::min(pos, size() - v.size()) + 1; pos > 0;)
                    {
                        --pos;
                        if (compare(pos, v.size(), v) == 0)
                            return pos;
                    }

                    return npos;
                }
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type rfind(value_type v, size_type pos = npos) const noexcept {return rfind(string_view_t(&v, 1), pos);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type rfind(const_pointer v, size_type pos, size_type count) const noexcept {return rfind(string_view_t(v, count), pos);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type rfind(const_pointer s, size_type pos = npos) const noexcept {return rfind(string_view_t(s), pos);}

#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_first_of(string_view_t v, size_type pos = 0) const noexcept
                {
                    for (; pos < size(); ++pos)
                        for (const auto &c: v)
                            if (c == data_[pos])
                                return pos;

                    return npos;
                }
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_first_of(value_type v, size_type pos = 0) const noexcept {return find_first_of(string_view_t(&v, 1), pos);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_first_of(const_pointer v, size_type pos, size_type count) const noexcept {return find_first_of(string_view_t(v, count), pos);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_first_of(const_pointer s, size_type pos = 0) const noexcept {return find_first_of(string_view_t(s), pos);}

#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_first_not_of(string_view_t v, size_type pos = 0) const noexcept
                {
                    for (; pos < size(); ++pos)
                    {
                        bool in_set = false;

                        for (const auto &c: v)
                            if (c == data_[pos])
                            {
                                in_set = true;
                                break;
                            }

                        if (!in_set)
                            return pos;
                    }

                    return npos;
                }
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_first_not_of(value_type v, size_type pos = 0) const noexcept {return find_first_not_of(string_view_t(&v, 1), pos);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_first_not_of(const_pointer v, size_type pos, size_type count) const noexcept {return find_first_not_of(string_view_t(v, count), pos);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_first_not_of(const_pointer s, size_type pos = 0) const noexcept {return find_first_not_of(string_view_t(s), pos);}

#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_last_of(string_view_t v, size_type pos = npos) const noexcept
                {
                    for (pos = std::min(pos, size() - 1) + 1; pos > 0;)
                    {
                        --pos;
                        for (const auto &c: v)
                            if (c == data_[pos])
                                return pos;
                    }

                    return npos;
                }
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_last_of(value_type v, size_type pos = npos) const noexcept {return find_last_of(string_view_t(&v, 1), pos);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_last_of(const_pointer v, size_type pos, size_type count) const noexcept {return find_last_of(string_view_t(v, count), pos);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_last_of(const_pointer s, size_type pos = npos) const noexcept {return find_last_of(string_view_t(s), pos);}

#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_last_not_of(string_view_t v, size_type pos = npos) const noexcept
                {
                    for (pos = std::min(pos, size() - 1) + 1; pos > 0;)
                    {
                        bool in_set = false;
                        --pos;

                        for (const auto &c: v)
                            if (c == data_[pos])
                            {
                                in_set = true;
                                break;
                            }

                        if (!in_set)
                            return pos;
                    }

                    return npos;
                }
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_last_not_of(value_type v, size_type pos = npos) const noexcept {return find_last_not_of(string_view_t(&v, 1), pos);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_last_not_of(const_pointer v, size_type pos, size_type count) const noexcept {return find_last_not_of(string_view_t(v, count), pos);}
#ifdef CPPDATALIB_CPP14
                constexpr
#endif
                size_type find_last_not_of(const_pointer s, size_type pos = npos) const noexcept {return find_last_not_of(string_view_t(s), pos);}

                bool operator==(string_view_t b) const {return compare(b) == 0;}
                bool operator!=(string_view_t b) const {return compare(b) != 0;}
                bool operator<=(string_view_t b) const {return compare(b) <= 0;}
                bool operator>=(string_view_t b) const {return compare(b) >= 0;}
                bool operator<(string_view_t b) const {return compare(b) < 0;}
                bool operator>(string_view_t b) const {return compare(b) > 0;}

            private:
                const_pointer data_;
                size_type size_;
            };
        }

        std::string s;
        typedef impl::string_view_t<char> string_view_t;

    } // Namespace core
} // Namespace cppdatalib

inline cppdatalib::core::string_t &operator+=(cppdatalib::core::string_t &lhs, cppdatalib::core::string_view_t rhs) {return lhs.append(rhs.data(), rhs.size());}
inline std::ostream &operator<<(std::ostream &os, cppdatalib::core::string_view_t rhs) {return os.write(rhs.data(), rhs.size());}

namespace cppdatalib
{
    namespace core
    {
#endif // CPPDATALIB_STRING_VIEW_T
#else
        typedef const core::string_t &string_view_t;
#endif // CPPDATALIB_DISABLE_TEMP_STRING
    } // Namespace core
} // Namespace cppdatalib

#endif // CPPDATALIB_GLOBAL_H

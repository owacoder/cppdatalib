/*
 * etl.h
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
 *
 * Disclaimer:
 * Trademarked product names referred to in this file are the property of their
 * respective owners. These trademark owners are not affiliated with the author
 * or copyright holder(s) of this file in any capacity, and do not endorse this
 * software nor the authorship and existence of this file.
 */

#ifndef CPPDATALIB_ETL_H
#define CPPDATALIB_ETL_H

#include <etl/src/array.h>
#include <etl/src/bitset.h>
#include <etl/src/deque.h>
#include <etl/src/vector.h>
#include <etl/src/list.h>
#include <etl/src/forward_list.h>
#include <etl/src/intrusive_list.h>
#include <etl/src/intrusive_forward_list.h>
#include <etl/src/queue.h>
#include <etl/src/stack.h>
#include <etl/src/optional.h>
#include <etl/src/cstring.h>

#include "../core/value_builder.h"

// TODO: implement generic_parser helper specializations for all types

/*
 *  TODO: implement conversions for maps, sets, intrusive_queue and intrusive_stack, and priority_queue
 */

// --------
//  string
// --------

template<size_t N>
class cast_sized_template_to_cppdatalib<etl::string, N>
{
    const etl::string<N> &bind;
public:
    cast_sized_template_to_cppdatalib(const etl::string<N> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_string(cppdatalib::core::string_t(bind.c_str(), bind.size()), cppdatalib::core::clob);
    }
};

template<size_t N>
class cast_sized_template_from_cppdatalib<etl::string, N>
{
    const cppdatalib::core::value &bind;
public:
    cast_sized_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator etl::string<N>() const
    {
        etl::string<N> result;
        convert(result);
        return result;
    }
    void convert(etl::string<N> &dest) const
    {
        cppdatalib::core::string_t stdstr = bind.as_string();
        dest.assign(stdstr.c_str(), std::min(stdstr.size(), N));
    }
};

// ----------
//  optional
// ----------

template<typename... Ts>
class cast_template_to_cppdatalib<etl::optional, Ts...>
{
    const etl::optional<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const etl::optional<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        if (!bind.isSpecified())
            dest.set_null(cppdatalib::core::normal);
        dest = cppdatalib::core::value(bind.value());
    }
};

template<typename T>
class cast_template_from_cppdatalib<etl::optional, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator etl::optional<T>() const
    {
        etl::optional<T> result;
        convert(result);
        return result;
    }
    void convert(etl::optional<T> &dest) const
    {
        if (!bind.is_null())
            dest = bind.operator T();
        else
            dest = {};
    }
};

// -------
//  array
// -------

template<typename T, size_t N>
class cast_array_template_to_cppdatalib<etl::array, T, N>
{
    const etl::array<T, N> &bind;
public:
    cast_array_template_to_cppdatalib(const etl::array<T, N> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename T, size_t N>
class cast_array_template_from_cppdatalib<etl::array, T, N>
{
    const cppdatalib::core::value &bind;
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator etl::array<T, N>() const
    {
        etl::array<T, N> result;
        convert(result);
        return result;
    }
    void convert(etl::array<T, N> &dest) const
    {
        if (bind.is_array())
        {
            size_t i = 0;
            auto it = bind.get_array_unchecked().begin();
            for (; i < std::min(bind.array_size(), N); ++i)
                dest[i] = (*it++).operator T();
            for (; i < N; ++i)
                dest[i] = {};
        }
        else
            dest = {};
    }
};

// --------
//  bitset
// --------

template<size_t N>
class cast_sized_template_to_cppdatalib<etl::bitset, N>
{
    const etl::bitset<N> &bind;
public:
    cast_sized_template_to_cppdatalib(const etl::bitset<N> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<size_t N>
class cast_sized_template_from_cppdatalib<etl::bitset, N>
{
    const cppdatalib::core::value &bind;
public:
    cast_sized_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator etl::bitset<N>() const
    {
        etl::bitset<N> result;
        convert(result);
        return result;
    }
    void convert(etl::bitset<N> &dest) const
    {
        if (bind.is_array())
        {
            size_t i = 0;
            auto it = bind.get_array_unchecked().begin();
            for (; i < std::min(bind.array_size(), N); ++i)
                dest[i] = *it++;
            for (; i < N; ++i)
                dest[i] = {};
        }
        else
            dest = {};
        return result;
    }
};

// -------
//  deque
// -------

template<typename T, size_t N>
class cast_array_template_to_cppdatalib<etl::deque, T, N>
{
    const etl::deque<T, N> &bind;
public:
    cast_array_template_to_cppdatalib(const etl::deque<T, N> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename T, size_t N>
class cast_array_template_from_cppdatalib<etl::deque, T, N>
{
    const cppdatalib::core::value &bind;
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator etl::deque<T, N>() const
    {
        etl::deque<T, N> result;
        convert(result);
        return result;
    }
    void convert(etl::deque<T, N> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
    }
};

// --------
//  vector
// --------

template<typename T, size_t N>
class cast_array_template_to_cppdatalib<etl::vector, T, N>
{
    const etl::vector<T, N> &bind;
public:
    cast_array_template_to_cppdatalib(const etl::vector<T, N> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename T, size_t N>
class cast_array_template_from_cppdatalib<etl::vector, T, N>
{
    const cppdatalib::core::value &bind;
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator etl::vector<T, N>() const
    {
        etl::vector<T, N> result;
        convert(result);
        return result;
    }
    void convert(etl::vector<T, N> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
    }
};

// ------
//  list
// ------

template<typename T, size_t N>
class cast_array_template_to_cppdatalib<etl::list, T, N>
{
    const etl::list<T, N> &bind;
public:
    cast_array_template_to_cppdatalib(const etl::list<T, N> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename T, size_t N>
class cast_array_template_from_cppdatalib<etl::list, T, N>
{
    const cppdatalib::core::value &bind;
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator etl::list<T, N>() const
    {
        etl::list<T, N> result;
        convert(result);
        return result;
    }
    void convert(etl::list<T, N> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
    }
};

// --------------
//  forward_list
// --------------

template<typename T, size_t N>
class cast_array_template_to_cppdatalib<etl::forward_list, T, N>
{
    const etl::forward_list<T, N> &bind;
public:
    cast_array_template_to_cppdatalib(const etl::forward_list<T, N> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename T, size_t N>
class cast_array_template_from_cppdatalib<etl::forward_list, T, N>
{
    const cppdatalib::core::value &bind;
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator etl::forward_list<T, N>() const
    {
        etl::forward_list<T, N> result;
        convert(result);
        return result;
    }
    void convert(etl::forward_list<T, N> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
    }
};

// ----------------
//  intrusive_list
// ----------------

template<typename... Ts>
class cast_template_to_cppdatalib<etl::intrusive_list, Ts...>
{
    const etl::intrusive_list<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const etl::intrusive_list<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

// ------------------------
//  intrusive_forward_list
// ------------------------

template<typename... Ts>
class cast_template_to_cppdatalib<etl::intrusive_forward_list, Ts...>
{
    const etl::intrusive_forward_list<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const etl::intrusive_forward_list<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

// -------
//  queue
// -------

template<typename T, size_t N>
class cast_array_template_to_cppdatalib<etl::queue, T, N>
{
    etl::queue<T, N> bind;
public:
    cast_array_template_to_cppdatalib(etl::queue<T, N> bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        while (!bind.empty())
        {
            dest.push_back(cppdatalib::core::value(bind.top()));
            bind.pop();
        }
    }
};

template<typename T, size_t N>
class cast_array_template_from_cppdatalib<etl::queue, T, N>
{
    const cppdatalib::core::value &bind;
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator etl::queue<T, N>() const
    {
        etl::queue<T, N> result;
        convert(result);
        return result;
    }
    void convert(etl::queue<T, N> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push(item.operator T());
    }
};

// -------
//  stack
// -------

template<typename T, size_t N>
class cast_array_template_to_cppdatalib<etl::stack, T, N>
{
    etl::stack<T, N> bind;
public:
    cast_array_template_to_cppdatalib(etl::stack<T, N> bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        using namespace std;
        dest.set_array({}, cppdatalib::core::normal);
        while (!bind.empty())
        {
            dest.push_back(cppdatalib::core::value(bind.top()));
            bind.pop();
        }
        reverse(dest.get_array_ref().begin(), dest.get_array_ref().end());
    }
};

template<typename T, size_t N>
class cast_array_template_from_cppdatalib<etl::stack, T, N>
{
    const cppdatalib::core::value &bind;
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator etl::stack<T, N>() const
    {
        etl::stack<T, N> result;
        convert(result);
        return result;
    }
    void convert(etl::stack<T, N> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push(item.operator T());
    }
};

#endif // CPPDATALIB_ETL_H

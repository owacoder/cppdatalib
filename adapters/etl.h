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

#include "etl/src/array.h"
#include "etl/src/bitset.h"
#include "etl/src/deque.h"
#include "etl/src/vector.h"
#include "etl/src/list.h"
#include "etl/src/forward_list.h"
#include "etl/src/intrusive_list.h"
#include "etl/src/intrusive_forward_list.h"
#include "etl/src/queue.h"
#include "etl/src/stack.h"
#include "etl/src/optional.h"
#include "etl/src/cstring.h"

#include "../core/value_builder.h"

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
    operator cppdatalib::core::value() const {return cppdatalib::core::string_t(bind.c_str(), bind.size());}
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
        std::string stdstr = bind.as_string();
        result.append(stdstr.c_str(), std::min(stdstr.size(), N));
        return result;
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
        if (!bind.isSpecified())
            return cppdatalib::core::null_t();
        return cppdatalib::core::value(bind.value());
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
        if (!bind.is_null())
            result = bind.operator T();
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        etl::array<T, N> result{};
        if (bind.is_array())
        {
            auto it = bind.get_array_unchecked().begin();
            for (size_t i = 0; i < std::min(bind.array_size(), N); ++i)
                result[i] = (*it++).operator T();
        }
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        etl::bitset<N> result{};
        if (bind.is_array())
        {
            auto it = bind.get_array_unchecked().begin();
            for (size_t i = 0; i < std::min(bind.array_size(), N); ++i)
                result[i] = *it++;
        }
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        etl::deque<T, N> result{};
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        etl::vector<T, N> result{};
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        etl::list<T, N> result{};
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        etl::forward_list<T, N> result{};
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        while (!bind.empty())
        {
            result.push_back(cppdatalib::core::value(bind.top()));
            bind.pop();
        }
        return result;
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
        etl::queue<T, N> result{};
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push(item.operator T());
        return result;
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
        using namespace std;
        cppdatalib::core::value result = cppdatalib::core::array_t();
        while (!bind.empty())
        {
            result.push_back(cppdatalib::core::value(bind.top()));
            bind.pop();
        }
        reverse(result.get_array_ref().begin(), result.get_array_ref().end());
        return result;
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
        etl::stack<T, N> result{};
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push(item.operator T());
        return result;
    }
};

#endif // CPPDATALIB_ETL_H

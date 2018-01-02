/*
 * boost_container.h
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

#ifndef CPPDATALIB_BOOST_CONTAINER_H
#define CPPDATALIB_BOOST_CONTAINER_H

#include <boost/container/vector.hpp>
#include <boost/container/stable_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/slist.hpp>
#include <boost/container/list.hpp>
#include <boost/container/deque.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/map.hpp>
#include <boost/container/set.hpp>
#include <boost/container/string.hpp>

#include "../core/value_builder.h"

// --------
//  vector
// --------

template<typename... Ts>
class cast_template_to_cppdatalib<boost::container::vector, Ts...>
{
    const boost::container::vector<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const boost::container::vector<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::vector, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::vector<T, Ts...>() const
    {
        boost::container::vector<T, Ts...> result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
    }
};

// ---------------
//  stable_vector
// ---------------

template<typename... Ts>
class cast_template_to_cppdatalib<boost::container::stable_vector, Ts...>
{
    const boost::container::stable_vector<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const boost::container::stable_vector<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::stable_vector, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::stable_vector<T, Ts...>() const
    {
        boost::container::stable_vector<T, Ts...> result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
    }
};

// ---------------
//  static_vector
// ---------------

// Read implicitly defined with boost::container::vector conversion (static_vector inherits vector)

template<typename T, size_t N>
class cast_array_template_from_cppdatalib<boost::container::static_vector, T, N>
{
    const cppdatalib::core::value &bind;
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::static_vector<T, N>() const
    {
        boost::container::static_vector<T, N> result;
        if (bind.is_array())
        {
            size_t idx = 0;
            for (const auto &item: bind.get_array_unchecked())
            {
                if (idx++ == N)
                    break;
                result.push_back(item.operator T());
            }
        }
        return result;
    }
};

// --------------
//  small_vector
// --------------

// Read implicitly defined with boost::container::vector conversion (small_vector inherits vector)

template<typename T, size_t N, typename... Ts>
class cast_array_template_from_cppdatalib<boost::container::small_vector, T, N, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::small_vector<T, N, Ts...>() const
    {
        boost::container::small_vector<T, N, Ts...> result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
    }
};

// -------
//  slist
// -------

template<typename... Ts>
class cast_template_to_cppdatalib<boost::container::slist, Ts...>
{
    const boost::container::slist<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const boost::container::slist<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::slist, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::slist<T, Ts...>() const
    {
        boost::container::slist<T, Ts...> result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_front(item.operator T());
        result.reverse();
        return result;
    }
};

// ------
//  list
// ------

template<typename... Ts>
class cast_template_to_cppdatalib<boost::container::list, Ts...>
{
    const boost::container::list<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const boost::container::list<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::list, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::list<T, Ts...>() const
    {
        boost::container::list<T, Ts...> result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
    }
};

// -------
//  deque
// -------

template<typename... Ts>
class cast_template_to_cppdatalib<boost::container::deque, Ts...>
{
    const boost::container::deque<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const boost::container::deque<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::deque, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::deque<T, Ts...>() const
    {
        boost::container::deque<T, Ts...> result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
    }
};

// ----------
//  flat_map
// ----------

template<typename... Ts>
class cast_template_to_cppdatalib<boost::container::flat_map, Ts...>
{
    const boost::container::flat_map<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const boost::container::flat_map<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::object_t();
        for (const auto &item: bind)
            result.add_member_at_end(cppdatalib::core::value(item.first),
                                     cppdatalib::core::value(item.second));
        return result;
    }
};

template<typename K, typename V, typename... Ts>
class cast_template_from_cppdatalib<boost::container::flat_map, K, V, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::flat_map<K, V, Ts...>() const
    {
        boost::container::flat_map<K, V, Ts...> result;
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                result.insert(item.first.operator K(),
                              item.second.operator V());
        return result;
    }
};

// ----------
//  flat_set
// ----------

template<typename... Ts>
class cast_template_to_cppdatalib<boost::container::flat_set, Ts...>
{
    const boost::container::flat_set<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const boost::container::flat_set<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::flat_set, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::flat_set<T, Ts...>() const
    {
        boost::container::flat_set<T, Ts...> result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.insert(item.operator T());
        return result;
    }
};

// -----
//  map
// -----

template<typename... Ts>
class cast_template_to_cppdatalib<boost::container::map, Ts...>
{
    const boost::container::map<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const boost::container::map<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::object_t();
        for (const auto &item: bind)
            result.add_member_at_end(cppdatalib::core::value(item.first),
                                     cppdatalib::core::value(item.second));
        return result;
    }
};

template<typename K, typename V, typename... Ts>
class cast_template_from_cppdatalib<boost::container::map, K, V, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::map<K, V, Ts...>() const
    {
        boost::container::map<K, V, Ts...> result;
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                result.insert(item.first.operator K(),
                              item.second.operator V());
        return result;
    }
};

// -----
//  set
// -----

template<typename... Ts>
class cast_template_to_cppdatalib<boost::container::set, Ts...>
{
    const boost::container::set<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const boost::container::set<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::set, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::set<T, Ts...>() const
    {
        boost::container::set<T, Ts...> result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.insert(item.operator T());
        return result;
    }
};

// --------
//  string
// --------

template<typename... Ts>
class cast_template_to_cppdatalib<boost::container::basic_string, char, Ts...>
{
    const boost::container::basic_string<char, Ts...> &bind;
public:
    cast_template_to_cppdatalib(const boost::container::basic_string<char, Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::string_t(bind.c_str(), bind.size());}
};

template<typename... Ts>
class cast_template_from_cppdatalib<boost::container::basic_string, char, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::basic_string<char, Ts...>() const
    {
        cppdatalib::core::string_t str = bind.as_string();
        return boost::container::basic_string<char, Ts...>(str.c_str(), str.size());
    }
};

#endif // CPPDATALIB_BOOST_CONTAINER_H

/*
 * poco.h
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

#ifndef CPPDATALIB_POCO_H
#define CPPDATALIB_POCO_H

#include <Poco/Dynamic/Pair.h>
#include <Poco/Dynamic/Struct.h>
#include <Poco/Dynamic/Var.h>

#include "../core/value.h"

// ---------------
//  Dynamic::Pair
// ---------------

template<typename... Ts>
class cast_template_to_cppdatalib<Poco::Dynamic::Pair, Ts...>
{
    const Poco::Dynamic::Pair<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const Poco::Dynamic::Pair<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::array_t{cppdatalib::core::value(bind.first()), cppdatalib::core::value(bind.second())};}
};

template<typename T>
class cast_template_from_cppdatalib<Poco::Dynamic::Pair, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Dynamic::Pair<T>() const
    {
        return Poco::Dynamic::Pair<T>{bind.element(0).operator T(), bind.element(1).operator Poco::Dynamic::Var()};
    }
};

// ---------------
//  Dynamic::List
// ---------------

template<>
class cast_to_cppdatalib<Poco::Dynamic::List>
{
    const Poco::Dynamic::List &bind;
public:
    cast_to_cppdatalib(const Poco::Dynamic::List &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
    }
};

template<>
class cast_from_cppdatalib<Poco::Dynamic::List>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Dynamic::List() const
    {
        Poco::Dynamic::List result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator Poco::Dynamic::Var());
        return result;
    }
};

// ----------------
//  Dynamic::Deque
// ----------------

template<>
class cast_to_cppdatalib<Poco::Dynamic::Deque>
{
    const Poco::Dynamic::Deque &bind;
public:
    cast_to_cppdatalib(const Poco::Dynamic::Deque &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
    }
};

template<>
class cast_from_cppdatalib<Poco::Dynamic::Deque>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Dynamic::Deque() const
    {
        Poco::Dynamic::Deque result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator Poco::Dynamic::Var());
        return result;
    }
};

// -----------------
//  Dynamic::Vector
// -----------------

template<>
class cast_to_cppdatalib<Poco::Dynamic::Vector>
{
    const Poco::Dynamic::Vector &bind;
public:
    cast_to_cppdatalib(const Poco::Dynamic::Vector &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
    }
};

template<>
class cast_from_cppdatalib<Poco::Dynamic::Vector>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Dynamic::Vector() const
    {
        Poco::Dynamic::Vector result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator Poco::Dynamic::Var());
        return result;
    }
};

// -----------------
//  Dynamic::Struct
// -----------------

template<typename... Ts>
class cast_template_to_cppdatalib<Poco::Dynamic::Struct, Ts...>
{
    const Poco::Dynamic::Struct<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const Poco::Dynamic::Struct<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::object_t();
        for (auto it = bind.begin(); it != bind.end(); ++it)
            result.add_member(cppdatalib::core::value(it->first),
                              cppdatalib::core::value(it->second));
        return result;
    }
};

template<typename T>
class cast_template_from_cppdatalib<Poco::Dynamic::Struct, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Dynamic::Struct<T>() const
    {
        Poco::Dynamic::Struct<T> result;
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                result.insert(item.first.operator T(),
                              item.second.operator Poco::Dynamic::Var());
        return result;
    }
};

// --------------
//  Dynamic::Var
// --------------

template<>
class cast_to_cppdatalib<Poco::Dynamic::Var>
{
    const Poco::Dynamic::Var &bind;
public:
    cast_to_cppdatalib(const Poco::Dynamic::Var &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;

        if (bind.isEmpty()) result.set_null();
        else if (bind.isBoolean()) result = bind.convert<bool>();
        else if (bind.isDeque() ||
                 bind.isArray() ||
                 bind.isList() ||
                 bind.isVector())
        {
            result = cppdatalib::core::array_t();
            for (const auto &item: bind)
                result.push_back(cppdatalib::core::value(item));
        }
        else if (bind.isString()) result = bind.convert<std::string>();
        else if (bind.isStruct())
        {
            result = cppdatalib::core::object_t();
            for (auto it = bind.begin(); it != bind.end(); ++it)
                result.add_member(cppdatalib::core::value(it->first),
                                  cppdatalib::core::value(it->second));
        }
        else if (bind.isInteger())
        {
            if (bind.isSigned())
                result = bind.convert<Poco::Int64>();
            else
                result = bind.convert<Poco::UInt64>();
        }
        else if (bind.isNumeric()) result = bind.convert<double>();
        else result.set_null();

        return result;
    }
};

template<>
class cast_from_cppdatalib<Poco::Dynamic::Var>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Dynamic::Var() const
    {
        Poco::Dynamic::Var result;
        switch (bind.get_type())
        {
            default:
            case cppdatalib::core::null: result = Poco::Dynamic::Var(); break;
            case cppdatalib::core::boolean: result = bind.get_bool_unchecked(); break;
            case cppdatalib::core::integer: result = Poco::Int64(bind.get_int_unchecked()); break;
            case cppdatalib::core::uinteger: result = Poco::UInt64(bind.get_uint_unchecked()); break;
            case cppdatalib::core::real: result = bind.get_real_unchecked(); break;
            case cppdatalib::core::string: result = bind.get_string_unchecked(); break;
            case cppdatalib::core::array: result = bind.operator Poco::Dynamic::Vector(); break;
            case cppdatalib::core::object: result = bind.operator Poco::Dynamic::Struct<Poco::Dynamic::Var>(); break;
        }
        return result;
    }
};

#endif // CPPDATALIB_POCO_H

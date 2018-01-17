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

#include <Poco/Optional.h>
#include <Poco/Nullable.h>

#include <Poco/HashMap.h>

#include <Poco/Tuple.h>

#include "../core/value_builder.h"

// ----------
//  Optional
// ----------

// TODO: implement generic_parser helper specialization for Poco::Optional

template<typename... Ts>
class cast_template_to_cppdatalib<Poco::Optional, Ts...>
{
    const Poco::Optional<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const Poco::Optional<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        if (!bind.isSpecified())
            return cppdatalib::core::null_t();
        return cppdatalib::core::value(bind.value());
    }
};

template<typename T>
class cast_template_from_cppdatalib<Poco::Optional, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Optional<T>() const
    {
        Poco::Optional<T> result;
        if (!bind.is_null())
            result = bind.operator T();
        return result;
    }
};

// ----------
//  Nullable
// ----------

// TODO: implement generic_parser helper specialization for Poco::Nullable

template<typename... Ts>
class cast_template_to_cppdatalib<Poco::Nullable, Ts...>
{
    const Poco::Nullable<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const Poco::Nullable<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        if (!bind.isSpecified())
            return cppdatalib::core::null_t();
        return cppdatalib::core::value(bind.value());
    }
};

template<typename T>
class cast_template_from_cppdatalib<Poco::Nullable, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Nullable<T>() const
    {
        Poco::Nullable<T> result;
        if (!bind.is_null())
            result = bind.operator T();
        return result;
    }
};

// ---------
//  HashMap
// ---------

// TODO: implement generic_parser helper specialization for Poco::HashMap

template<typename... Ts>
class cast_template_to_cppdatalib<Poco::HashMap, Ts...>
{
    const Poco::HashMap<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const Poco::HashMap<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::object_t();
        result.set_subtype(cppdatalib::core::hash);
        for (const auto &item: bind)
            result.add_member(cppdatalib::core::value(item.first),
                              cppdatalib::core::value(item.second));
        return result;
    }
};

template<typename K, typename V, typename... Ts>
class cast_template_from_cppdatalib<Poco::HashMap, K, V, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::HashMap<K, V, Ts...>() const
    {
        Poco::HashMap<K, V, Ts...> result;
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                result.insert({item.first.operator K(),
                               item.second.operator V()});
        return result;
    }
};

// -----------------
//  LinearHashTable
// -----------------

// TODO: implement generic_parser helper specialization for Poco::LinearHashTable

template<typename... Ts>
class cast_template_to_cppdatalib<Poco::LinearHashTable, Ts...>
{
    const Poco::LinearHashTable<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const Poco::LinearHashTable<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::object_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<Poco::LinearHashTable, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::LinearHashTable<T, Ts...>() const
    {
        Poco::LinearHashTable<T, Ts...> result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.insert(item.operator T());
        return result;
    }
};

// -------
//  Tuple
// -------

// TODO: implement generic_parser helper specialization for Poco::Tuple

namespace cppdatalib
{
    namespace poco
    {
        namespace impl
        {
            template<int tuple_size>
            struct push_back : public push_back<tuple_size - 1>
            {
                template<typename... Ts>
                push_back(const Poco::Tuple<Ts...> &tuple, core::array_t &result) : push_back<tuple_size - 1>(tuple, result) {
                    result.data().push_back(tuple.template get<tuple_size - 1>());
                }
            };

            template<>
            struct push_back<0>
            {
                template<typename... Ts>
                push_back(const Poco::Tuple<Ts...> &, core::array_t &) {}
            };
        }
    }
}

template<typename... Ts>
class cast_template_to_cppdatalib<Poco::Tuple, Ts...>
{
    const Poco::Tuple<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const Poco::Tuple<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::array_t result;
        cppdatalib::poco::impl::push_back<Poco::Tuple<Ts...>::length>(bind, result);
        return result;
    }
};

namespace cppdatalib
{
    namespace poco
    {
        namespace impl
        {
            template<int tuple_size>
            struct tuple_push_back : public tuple_push_back<tuple_size - 1>
            {
                template<typename... Ts>
                tuple_push_back(const core::array_t &list, Poco::Tuple<Ts...> &result) : tuple_push_back<tuple_size - 1>(list, result) {
                    typedef typename Poco::TypeGetter<tuple_size - 1, typename Poco::Tuple<Ts...>::Type>::HeadType element_type;
                    if (tuple_size <= list.size()) // If the list contains enough elements, assign it
                        result.set<tuple_size - 1>(list[tuple_size - 1].operator element_type());
                    else // Otherwise, clear the tuple's element value
                        result.set<tuple_size - 1>(element_type{});
                }
            };

            template<>
            struct tuple_push_back<0>
            {
                template<typename... Ts>
                tuple_push_back(const core::array_t &, Poco::Tuple<Ts...> &) {}
            };
        }
    }
}

template<typename... Ts>
class cast_template_from_cppdatalib<Poco::Tuple, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Tuple<Ts...>() const
    {
        Poco::Tuple<Ts...> result;
        if (bind.is_array())
            cppdatalib::poco::impl::tuple_push_back<Poco::Tuple<Ts...>::length>(bind.get_array_unchecked(), result);
        return result;
    }
};

// ---------------
//  Dynamic::Pair
// ---------------

// TODO: implement generic_parser helper specialization for Poco::Dynamic::Pair

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

// TODO: implement generic_parser helper specialization for Poco::Dynamic::List

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

// TODO: implement generic_parser helper specialization for Poco::Dynamic::Deque

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

// TODO: implement generic_parser helper specialization for Poco::Dynamic::Vector

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

// TODO: implement generic_parser helper specialization for Poco::Dynamic::Struct

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

// TODO: implement generic_parser helper specialization for Poco::Dynamic::Var

template<>
class cast_to_cppdatalib<Poco::Dynamic::Var>
{
    const Poco::Dynamic::Var &bind;

    static void extract_struct(const Poco::Dynamic::Var &v, cppdatalib::core::value &result)
    {
        // Try extracting with string keys first
        try
        {
            const auto &struct_ref = v.extract<Poco::DynamicStruct>();
            result = cppdatalib::core::object_t();
            for (auto it = struct_ref.begin(); it != struct_ref.end(); ++it)
                result.add_member(cppdatalib::core::value(it->first),
                                  cppdatalib::core::value(it->second));
            return;
        }
        catch (Poco::BadCastException) {}

        // If that fails, try Poco::Dynamic::Var keys
        try
        {
            const auto &struct_ref = v.extract<Poco::Dynamic::Struct<Poco::Dynamic::Var>>();
            result = cppdatalib::core::object_t();
            for (auto it = struct_ref.begin(); it != struct_ref.end(); ++it)
                result.add_member(cppdatalib::core::value(it->first),
                                  cppdatalib::core::value(it->second));
            return;
        }
        catch (Poco::BadCastException) {}

        // Otherwise, try int keys
        {
            const auto &struct_ref = v.extract<Poco::Dynamic::Struct<int>>();
            result = cppdatalib::core::object_t();
            for (auto it = struct_ref.begin(); it != struct_ref.end(); ++it)
                result.add_member(cppdatalib::core::value(it->first),
                                  cppdatalib::core::value(it->second));
        }

        // If that fails, there's no hope. I give up!
    }

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
        else if (bind.isStruct()) extract_struct(bind, result);
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

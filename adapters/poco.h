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

/***************************************************************************************
 *
 *
 * TODO: EVERYTHING FROM HERE DOWN NEEDS TO HAVE convert() ADDED AS A MEMBER FUNCTION!
 *
 *
 **************************************************************************************/

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
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        if (!bind.isSpecified())
            dest.set_null(cppdatalib::core::normal);
        else
            dest = cppdatalib::core::value(bind.value());
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
        convert(result);
        return result;
    }
    void convert(Poco::Optional<T> &dest) const
    {
        if (!bind.is_null())
            dest = bind.operator T();
        else
            dest.clear();
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
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        if (!bind.isSpecified())
            dest.set_null(cppdatalib::core::normal);
        else
            dest = cppdatalib::core::value(bind.value());
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
        convert(result);
        return result;
    }
    void convert(Poco::Nullable<T> &dest) const
    {
        if (!bind.is_null())
            dest = bind.operator T();
        else
            dest.clear();
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
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::hash);
        for (const auto &item: bind)
            dest.add_member(cppdatalib::core::value(item.first),
                            cppdatalib::core::value(item.second));
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
        convert(result);
        return result;
    }
    void convert(Poco::HashMap<K, V, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                dest.insert({item.first.operator K(),
                             item.second.operator V()});
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
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
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
        convert(result);
        return result;
    }
    void convert(Poco::LinearHashTable<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.insert(item.operator T());
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
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        cppdatalib::poco::impl::push_back<Poco::Tuple<Ts...>::length>(bind, dest.get_array_ref());
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
        convert(result);
        return result;
    }
    void convert(Poco::Tuple<Ts...> &dest) const
    {
        if (bind.is_array())
            cppdatalib::poco::impl::tuple_push_back<Poco::Tuple<Ts...>::length>(bind.get_array_unchecked(), dest);
        else
            dest = {};
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
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({cppdatalib::core::value(bind.first()), cppdatalib::core::value(bind.second())}, cppdatalib::core::normal);
    }
};

template<typename T>
class cast_template_from_cppdatalib<Poco::Dynamic::Pair, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Dynamic::Pair<T>() const
    {
        Poco::Dynamic::Pair<T> result;
        convert(result);
        return result;
    }
    void convert(Poco::Dynamic::Pair<T> &dest) const
    {
        dest = {bind.element(0).operator T(), bind.element(1).operator Poco::Dynamic::Var()};
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

template<>
class cast_from_cppdatalib<Poco::Dynamic::List>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Dynamic::List() const
    {
        Poco::Dynamic::List result;
        convert(result);
        return result;
    }
    void convert(Poco::Dynamic::List &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator Poco::Dynamic::Var());
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

template<>
class cast_from_cppdatalib<Poco::Dynamic::Deque>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Dynamic::Deque() const
    {
        Poco::Dynamic::Deque result;
        convert(result);
        return result;
    }
    void convert(Poco::Dynamic::Deque &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator Poco::Dynamic::Var());
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

template<>
class cast_from_cppdatalib<Poco::Dynamic::Vector>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator Poco::Dynamic::Vector() const
    {
        Poco::Dynamic::Vector result;
        convert(result);
        return result;
    }
    void convert(Poco::Dynamic::Vector &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator Poco::Dynamic::Var());
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
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::normal);
        for (auto it = bind.begin(); it != bind.end(); ++it)
            dest.add_member(cppdatalib::core::value(it->first),
                            cppdatalib::core::value(it->second));
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
        convert(result);
        return result;
    }
    void convert(Poco::Dynamic::Struct<T> &dest) const
    {
        dest = {};
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                dest.insert(item.first.operator T(),
                            item.second.operator Poco::Dynamic::Var());
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
            result.set_object({}, cppdatalib::core::normal);
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
            result.set_object({}, cppdatalib::core::normal);
            for (auto it = struct_ref.begin(); it != struct_ref.end(); ++it)
                result.add_member(cppdatalib::core::value(it->first),
                                  cppdatalib::core::value(it->second));
            return;
        }
        catch (Poco::BadCastException) {}

        // Otherwise, try int keys
        {
            const auto &struct_ref = v.extract<Poco::Dynamic::Struct<int>>();
            result.set_object({}, cppdatalib::core::normal);
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
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        if (bind.isEmpty()) dest.set_null(cppdatalib::core::normal);
        else if (bind.isBoolean()) dest = bind.convert<bool>();
        else if (bind.isDeque() ||
                 bind.isArray() ||
                 bind.isList() ||
                 bind.isVector())
        {
            dest.set_array({}, cppdatalib::core::normal);
            for (const auto &item: bind)
                dest.push_back(cppdatalib::core::value(item));
        }
        else if (bind.isString()) dest.set_string(bind.convert<std::string>(), cppdatalib::core::clob);
        else if (bind.isStruct()) extract_struct(bind, dest);
        else if (bind.isInteger())
        {
            if (bind.isSigned())
                dest = bind.convert<Poco::Int64>();
            else
                dest = bind.convert<Poco::UInt64>();
        }
        else if (bind.isNumeric()) dest = bind.convert<double>();
        else dest.set_null(cppdatalib::core::normal);
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
        convert(result);
        return result;
    }
    void convert(Poco::Dynamic::Var &dest) const
    {
        switch (bind.get_type())
        {
            case cppdatalib::core::link:
            case cppdatalib::core::null: dest = Poco::Dynamic::Var(); break;
            case cppdatalib::core::boolean: dest = bind.get_bool_unchecked(); break;
            case cppdatalib::core::integer: dest = Poco::Int64(bind.get_int_unchecked()); break;
            case cppdatalib::core::uinteger: dest = Poco::UInt64(bind.get_uint_unchecked()); break;
            case cppdatalib::core::real: dest = bind.get_real_unchecked(); break;
            case cppdatalib::core::string:
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            case cppdatalib::core::temporary_string:
#endif
                dest = static_cast<std::string>(bind.get_string_unchecked());
                break;
            case cppdatalib::core::array: dest = bind.operator Poco::Dynamic::Vector(); break;
            case cppdatalib::core::object: dest = bind.operator Poco::Dynamic::Struct<Poco::Dynamic::Var>(); break;
        }
    }
};

#endif // CPPDATALIB_POCO_H

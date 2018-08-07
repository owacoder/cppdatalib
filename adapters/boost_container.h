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

#include "../core/value_parser.h"

/***************************************************************************************
 *
 *
 * TODO: EVERYTHING FROM HERE DOWN NEEDS TO HAVE convert() ADDED AS A MEMBER FUNCTION!
 *
 *
 **************************************************************************************/

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

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::vector, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::vector<T, Ts...>() const
    {
        boost::container::vector<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(boost::container::vector<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
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

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::stable_vector, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::stable_vector<T, Ts...>() const
    {
        boost::container::stable_vector<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(boost::container::stable_vector<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
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
        convert(result);
        return result;
    }
    void convert(boost::container::static_vector<T, N> &dest) const
    {
        dest.clear();
        if (bind.is_array())
        {
            size_t idx = 0;
            for (const auto &item: bind.get_array_unchecked())
            {
                if (idx++ == N)
                    break;
                dest.push_back(item.operator T());
            }
        }
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
        convert(result);
        return result;
    }
    void convert(boost::container::small_vector<T, N, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
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

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::slist, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::slist<T, Ts...>() const
    {
        boost::container::slist<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(boost::container::slist<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_front(item.operator T());
        dest.reverse();
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

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::list, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::list<T, Ts...>() const
    {
        boost::container::list<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(boost::container::list<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
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

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::deque, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::deque<T, Ts...>() const
    {
        boost::container::deque<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(boost::container::deque<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
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
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.add_member_at_end(cppdatalib::core::value(item.first),
                                   cppdatalib::core::value(item.second));
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
        convert(result);
        return result;
    }
    void convert(boost::container::flat_map<K, V, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                dest.insert(item.first.operator K(),
                            item.second.operator V());
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<boost::container::flat_map, Ts...> : public generic_stream_input
    {
    protected:
        const boost::container::flat_map<Ts...> bind;
        decltype(bind.begin()) iterator;
        bool parsingKey;

    public:
        template_parser(const boost::container::flat_map<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {iterator = bind.begin(); parsingKey = true;}

        void write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_object(core::object_t(), bind->size());
                if (iterator != bind.end())
                    compose_parser(iterator->first);
            }
            else if (iterator != bind.end())
            {
                if (parsingKey)
                    compose_parser(iterator->second);
                else
                    compose_parser((iterator++)->first);
                parsingKey = !parsingKey;
            }
            else
                get_output()->end_object(core::object_t());
        }
    };
}}

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

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::flat_set, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::flat_set<T, Ts...>() const
    {
        boost::container::flat_set<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(boost::container::flat_set<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.insert(item.operator T());
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
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.add_member_at_end(cppdatalib::core::value(item.first),
                                   cppdatalib::core::value(item.second));
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
        convert(result);
        return result;
    }
    void convert(boost::container::map<K, V, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                dest.insert(item.first.operator K(),
                            item.second.operator V());
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<boost::container::map, Ts...> : public generic_stream_input
    {
    protected:
        const boost::container::map<Ts...> bind;
        decltype(bind.begin()) iterator;
        bool parsingKey;

    public:
        template_parser(const boost::container::map<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {iterator = bind.begin(); parsingKey = true;}

        void write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_object(core::object_t(), bind->size());
                if (iterator != bind.end())
                    compose_parser(iterator->first);
            }
            else if (iterator != bind.end())
            {
                if (parsingKey)
                    compose_parser(iterator->second);
                else
                    compose_parser((iterator++)->first);
                parsingKey = !parsingKey;
            }
            else
                get_output()->end_object(core::object_t());
        }
    };
}}

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

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::container::set, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::container::set<T, Ts...>() const
    {
        boost::container::set<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(boost::container::set<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.insert(item.operator T());
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
    operator cppdatalib::core::value() const {return cppdatalib::core::value(cppdatalib::core::string_t(bind.c_str(), bind.size()),
                                                                             cppdatalib::core::clob);}
    void convert(cppdatalib::core::value &dest) const {dest.set_string(cppdatalib::core::string_t(bind.c_str(), bind.size()),
                                                                                      cppdatalib::core::clob);}
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
    void convert(boost::container::basic_string<char, Ts...> &dest) const
    {
        cppdatalib::core::string_t str = bind.as_string();
        dest.assign(str.c_str(), str.size());
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<boost::container::basic_string, char, Ts...> : public generic_stream_input
    {
    protected:
        const boost::container::basic_string<char, Ts...> bind;

    public:
        template_parser(const boost::container::basic_string<char, Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {}

        void write_one_()
        {
            get_output()->write(core::value(bind, core::clob));
        }
    };
}}

// --------
//  TODO: wstring
// --------

#endif // CPPDATALIB_BOOST_CONTAINER_H

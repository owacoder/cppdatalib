/*
 * value.h
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

#ifndef CPPDATALIB_VALUE_H
#define CPPDATALIB_VALUE_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

#include <vector>
#include <map>

#include <stack>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <codecvt>
#include <locale>
#include <cfloat>
#include <limits>
#include <algorithm>

#include <type_traits>

namespace cppdatalib {
    namespace core
    {
        class value;
    }
}

template<typename T> class cast_to_cppdatalib;
template<template<size_t, typename...> class Template, size_t N, typename... Ts> struct cast_sized_template_to_cppdatalib;
template<template<typename...> class Template, typename... Ts> struct cast_template_to_cppdatalib;
template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts> struct cast_array_template_to_cppdatalib;
template<typename T> class cast_from_cppdatalib;
template<template<size_t, typename...> class Template, size_t N, typename... Ts> struct cast_sized_template_from_cppdatalib;
template<template<typename...> class Template, typename... Ts> struct cast_template_from_cppdatalib;
template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts> struct cast_array_template_from_cppdatalib;

namespace cppdatalib
{
    namespace core
    {
        enum type : int8_t
        {
            null,
            boolean,
            integer,
            uinteger,
            real,
            string,
            array,
            object
        };

        enum subtype
        {
            normal = -1,

            // Integers
            timestamp = -10, // Number of seconds since the epoch, Jan 1, 1970

            // Strings
            blob = -20, // A chunk of binary data
            clob = -21, // A chunk of binary data, that should be interpreted as text
            symbol = -22, // A symbolic atom, or identifier
            datetime = -23, // A datetime structure, with unspecified format
            date = -24, // A date structure, with unspecified format
            time = -25, // A time structure, with unspecified format
            bignum = -26, // A high-precision, decimal-encoded, number
            uuid = -27, // A generic UUID value

            // Arrays
            regexp = -30, // A regular expression definition containing two string elements, the first is the definition, the second is the options list
            sexp = -31, // Ordered collection of values, distinct from an array only by name

            // Objects
            map = -40, // A normal object with integral keys
            hash = -41, // A hash lookup (not supported as such in the value class, but can be used as a tag for external variant classes)

            user = 0
        };

        class value_builder;
        class stream_handler;

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
#define CPPDATALIB_REAL_DIG DBL_DIG
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

        class array_t;
        class object_t;
        class array_iterator_t;
        class array_const_iterator_t;
        class object_iterator_t;
        class object_const_iterator_t;

#ifdef CPPDATALIB_SUBTYPE_T
        typedef CPPDATALIB_SUBTYPE_T subtype_t;
#else
        typedef int16_t subtype_t;
#endif

        struct null_t {};

        /* The core value class for all of cppdatalib.
         *
         */

        class value
        {
        public:
            struct traversal_reference;
            struct traversal_ancestry_finder;

        private:
            // Functors should return true if processing should continue
            static bool traverse_node_null(const value *, traversal_ancestry_finder) {return true;}
            static bool traverse_node_mutable_clear(const value *arg, traversal_ancestry_finder) {arg->mutable_clear(); return true;}

            struct traverse_node_prefix_serialize;
            struct traverse_node_postfix_serialize;

            struct traverse_compare_prefix;
            struct traverse_less_than_compare_prefix;
            struct traverse_equality_compare_prefix;
            struct traverse_compare_postfix;

            friend bool operator<(const value &lhs, const value &rhs);
            friend bool operator<=(const value &lhs, const value &rhs);
            friend bool operator==(const value &lhs, const value &rhs);
            friend stream_handler &operator<<(stream_handler &output, const value &input);
            friend void operator<<(stream_handler &&output, const value &input);
            static value &assign(value &dst, const value &src);

        public:
            struct traversal_ancestry_finder
            {
                typedef std::stack<traversal_reference, std::vector<traversal_reference>> container;

                const container &c;

            public:
                traversal_ancestry_finder(const container &c) : c(c) {}

                size_t get_parent_count() const {return c.size();}

                // First element is direct parent, last element is ancestry root
                std::vector<traversal_reference> get_ancestry() const
                {
                    std::vector<traversal_reference> result;
                    container temp = c;

                    while (!temp.empty())
                    {
                        result.push_back(temp.top());
                        temp.pop();
                    }

                    return result;
                }
            };

            // Predicates must be callables with argument type `const core::value *arg, core::value::traversal_ancestry_finder arg_finder` and return value bool
            // If return value is non-zero, processing continues, otherwise processing aborts immediately
            template<typename PrefixPredicate, typename PostfixPredicate>
            void traverse(PrefixPredicate &prefix, PostfixPredicate &postfix) const;

            // Predicates must be callables with argument types `const core::value *arg, core::value::traversal_ancestry_finder arg_finder, bool prefix` and return value bool
            // `prefix` is set to true if the current invocation is for the prefix handling of the specified element
            // If return value is non-zero, processing continues, otherwise processing aborts immediately
            template<typename Predicate>
            void traverse(Predicate &predicate) const;

            // Predicates must be callables with argument type `const core::value *arg, core::value::traversal_ancestry_finder arg_finder` and return value bool
            // If return value is non-zero, processing continues, otherwise processing aborts immediately
            // NOTE: The predicate is only called for object values, not object keys. It's invoked for all other values.
            template<typename PrefixPredicate, typename PostfixPredicate>
            void value_traverse(PrefixPredicate &prefix, PostfixPredicate &postfix) const;

            // Predicates must be callables with argument types `const core::value *arg, core::value::traversal_ancestry_finder arg_finder, bool prefix` and return value bool
            // `prefix` is set to true if the current invocation is for the prefix handling of the specified element
            // If return value is non-zero, processing continues, otherwise processing aborts immediately
            // NOTE: The predicate is only called for object values, not object keys. It's invoked for all other values.
            template<typename Predicate>
            void value_traverse(Predicate &predicate) const;

            // Predicates must be callables with argument type `const core::value *arg, core::value::traversal_ancestry_finder arg_finder` and return value bool
            // If return value is non-zero, processing continues, otherwise processing aborts immediately
            template<typename PrefixPredicate>
            void prefix_traverse(PrefixPredicate &prefix);

            // Predicates must be callables with argument type `const core::value *arg, core::value::traversal_ancestry_finder arg_finder` and return value bool
            // If return value is non-zero, processing continues, otherwise processing aborts immediately
            template<typename PostfixPredicate>
            void postfix_traverse(PostfixPredicate &postfix);

            // Predicates must be callables with argument type `const core::value *arg, const core::value *arg2, core::value::traversal_ancestry_finder arg_finder, core::value::traversal_ancestry_finder arg2_finder`
            // and return value bool. Either argument may be NULL, but both arguments will never be NULL simultaneously.
            // If return value is non-zero, processing continues, otherwise processing aborts immediately
            template<typename PrefixPredicate, typename PostfixPredicate>
            void parallel_traverse(const value &other, PrefixPredicate &prefix, PostfixPredicate &postfix) const;

            // Predicates must be callables with argument type `const core::value *arg, const core::value *arg2, core::value::traversal_ancestry_finder arg_finder, core::value::traversal_ancestry_finder arg2_finder`
            // and return value bool. Either argument may be NULL, but both arguments will never be NULL simultaneously.
            // If return value is non-zero, processing continues, otherwise processing aborts immediately
            template<typename PrefixPredicate, typename PostfixPredicate>
            void parallel_diff_traverse(const value &other, PrefixPredicate &prefix, PostfixPredicate &postfix) const;
            value() : type_(null), subtype_(core::normal) {}
            value(null_t, subtype_t subtype = core::normal) : type_(null), subtype_(subtype) {}
            value(bool_t v, subtype_t subtype = core::normal) {bool_init(subtype, v);}
            value(int_t v, subtype_t subtype = core::normal) {int_init(subtype, v);}
            value(uint_t v, subtype_t subtype = core::normal) {uint_init(subtype, v);}
            value(real_t v, subtype_t subtype = core::normal) {real_init(subtype, v);}
            value(cstring_t v, subtype_t subtype = core::normal) {string_init(subtype, v);}
            value(const string_t &v, subtype_t subtype = core::normal) {string_init(subtype, v);}
            value(string_t &&v, subtype_t subtype = core::normal) {string_init(subtype, std::move(v));}
            value(const array_t &v, subtype_t subtype = core::normal);
            value(array_t &&v, subtype_t subtype = core::normal);
            value(const object_t &v, subtype_t subtype = core::normal);
            value(object_t &&v, subtype_t subtype = core::normal);
            template<typename T, typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
            value(T v, subtype_t subtype = core::normal) {uint_init(subtype, v);}
            template<typename T, typename std::enable_if<std::is_signed<T>::value, int>::type = 0, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
            value(T v, subtype_t subtype = core::normal) {int_init(subtype, v);}
            template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
            value(T v, subtype_t subtype = core::normal) {real_init(subtype, v);}
            template<typename... Ts>
            value(std::initializer_list<Ts...> v, subtype_t subtype = core::normal);
            // Template constructor for simple type
            template<typename T, typename std::enable_if<std::is_class<T>::value, int>::type = 0>
            value(const T &v, subtype_t subtype = core::normal) : type_(null), subtype_(subtype)
            {
                assign(*this, cast_to_cppdatalib<T>(v));
            }
            // Template constructor for simple type with supplied user data
            template<typename T, typename UserData, typename std::enable_if<std::is_class<T>::value, int>::type = 0>
            value(const T &v, UserData userdata, subtype_t subtype) : type_(null), subtype_(subtype)
            {
                assign(*this, cast_to_cppdatalib<T>(v, userdata));
            }

            // Template constructor for template type
            template<template<typename...> class Template, typename... Ts>
            value(const Template<Ts...> &v, subtype_t subtype = core::normal) : type_(null), subtype_(subtype)
            {
                assign(*this, cast_template_to_cppdatalib<Template, Ts...>(v));
            }
            // Template constructor for template type with supplied user data
            template<template<typename...> class Template, typename UserData, typename... Ts>
            value(const Template<Ts...> &v, UserData userdata, subtype_t subtype) : type_(null), subtype_(subtype)
            {
                assign(*this, cast_template_to_cppdatalib<Template, Ts...>(v, userdata));
            }

            // Template constructor for sized array template type
            template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
            value(const Template<T, N, Ts...> &v, subtype_t subtype = core::normal) : type_(null), subtype_(subtype)
            {
                assign(*this, cast_array_template_to_cppdatalib<Template, T, N, Ts...>(v));
            }
            // Template constructor for sized array template type with supplied user data
            template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename UserData, typename... Ts>
            value(const Template<T, N, Ts...> &v, UserData userdata, subtype_t subtype) : type_(null), subtype_(subtype)
            {
                assign(*this, cast_array_template_to_cppdatalib<Template, T, N, Ts...>(v, userdata));
            }

            // Template constructor for sized type
            template<template<size_t, typename...> class Template, size_t N, typename... Ts>
            value(const Template<N, Ts...> &v, subtype_t subtype = core::normal) : type_(null), subtype_(subtype)
            {
                assign(*this, cast_sized_template_to_cppdatalib<Template, N, Ts...>(v));
            }
            // Template constructor for sized type with supplied user data
            template<template<size_t, typename...> class Template, size_t N, typename UserData, typename... Ts>
            value(const Template<N, Ts...> &v, UserData userdata, subtype_t subtype) : type_(null), subtype_(subtype)
            {
                assign(*this, cast_sized_template_to_cppdatalib<Template, N, Ts...>(v, userdata));
            }

            ~value();

            value(const value &other) : type_(null), subtype_(core::normal) {assign(*this, other);}
            value(value &&other);
            value &operator=(const value &other) {return assign(*this, other);}
            value &operator=(value &&other);

            void swap(value &other)
            {
                using std::swap;

                if (type_ == other.type_)
                {
                    swap(subtype_, other.subtype_);
                    switch (type_)
                    {
                        case boolean: swap(bool_, other.bool_); break;
                        case integer: swap(int_, other.int_); break;
                        case uinteger: swap(uint_, other.uint_); break;
                        case real: swap(real_, other.real_); break;
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                        case string: swap(str_, other.str_); break;
#else
                        case string: swap(ptr_, other.ptr_); break;
#endif
                        case array:
                        case object: swap(ptr_, other.ptr_); break;
                        default: break;
                    }
                }
                else
                    std::swap(*this, other);
            }

            subtype_t get_subtype() const {return subtype_;}
            subtype_t &get_subtype_ref() {return subtype_;}
            void set_subtype(subtype_t _type) {subtype_ = _type;}

            type get_type() const {return type_;}
            size_t size() const;
            size_t array_size() const;
            size_t object_size() const;
            size_t string_size() const {return is_string()? str_ref_().size(): 0;}

            bool_t is_null() const {return type_ == null;}
            bool_t is_bool() const {return type_ == boolean;}
            bool_t is_int() const {return type_ == integer;}
            bool_t is_uint() const {return type_ == uinteger;}
            bool_t is_real() const {return type_ == real;}
            bool_t is_string() const {return type_ == string;}
            bool_t is_array() const {return type_ == array;}
            bool_t is_object() const {return type_ == object;}

            // The following seven functions exhibit UNDEFINED BEHAVIOR if the value is not the requested type
            bool_t get_bool_unchecked() const {return bool_;}
            int_t get_int_unchecked() const {return int_;}
            uint_t get_uint_unchecked() const {return uint_;}
            real_t get_real_unchecked() const {return real_;}
            const string_t &get_string_unchecked() const {return str_ref_();}
            const array_t &get_array_unchecked() const {return arr_ref_();}
            const object_t &get_object_unchecked() const {return obj_ref_();}

            bool_t &get_bool_ref() {clear(boolean); return bool_;}
            int_t &get_int_ref() {clear(integer); return int_;}
            uint_t &get_uint_ref() {clear(uinteger); return uint_;}
            real_t &get_real_ref() {clear(real); return real_;}
            string_t &get_string_ref() {clear(string); return str_ref_();}
            array_t &get_array_ref() {clear(array); return arr_ref_();}
            object_t &get_object_ref() {clear(object); return obj_ref_();}

            void set_null() {clear(null);}
            void set_bool(bool_t v) {clear(boolean); bool_ = v;}
            void set_int(int_t v) {clear(integer); int_ = v;}
            void set_uint(uint_t v) {clear(uinteger); uint_ = v;}
            void set_real(real_t v) {clear(real); real_ = v;}
            void set_string(cstring_t v) {clear(string); str_ref_() = v;}
            void set_string(const string_t &v) {clear(string); str_ref_() = v;}
            void set_array(const array_t &v);
            void set_object(const object_t &v);

            void set_null(subtype_t subtype) {clear(null); subtype_ = subtype;}
            void set_bool(bool_t v, subtype_t subtype) {clear(boolean); bool_ = v; subtype_ = subtype;}
            void set_int(int_t v, subtype_t subtype) {clear(integer); int_ = v; subtype_ = subtype;}
            void set_uint(uint_t v, subtype_t subtype) {clear(uinteger); uint_ = v; subtype_ = subtype;}
            void set_real(real_t v, subtype_t subtype) {clear(real); real_ = v; subtype_ = subtype;}
            void set_string(cstring_t v, subtype_t subtype) {clear(string); str_ref_() = v; subtype_ = subtype;}
            void set_string(const string_t &v, subtype_t subtype) {clear(string); str_ref_() = v; subtype_ = subtype;}
            void set_array(const array_t &v, subtype_t subtype);
            void set_object(const object_t &v, subtype_t subtype);

            value operator[](cstring_t key) const;
            value &operator[](cstring_t key);
            value operator[](const string_t &key) const;
            value &operator[](const string_t &key);
            value member(const value &key) const;
            value &member(const value &key);
            const value *member_ptr(const value &key) const;
            bool_t is_member(cstring_t key) const;
            bool_t is_member(const string_t &key) const;
            bool_t is_member(const value &key) const;
            size_t member_count(cstring_t key) const;
            size_t member_count(const string_t &key) const;
            size_t member_count(const value &key) const;
            void erase_member(cstring_t key);
            void erase_member(const string_t &key);
            void erase_member(const value &key);

            value &add_member(const value &key);
            value &add_member(value &&key);
            value &add_member(const value &key, const value &val);
            value &add_member(value &&key, value &&val);

            value &add_member_at_end(const value &key);
            value &add_member_at_end(value &&key);
            value &add_member_at_end(const value &key, const value &val);
            value &add_member_at_end(value &&key, value &&val);

            void push_back(const value &v);
            void push_back(value &&v);
            value operator[](size_t pos) const;
            value &operator[](size_t pos);
            value element(size_t pos) const;
            value &element(size_t pos);
            void erase_element(size_t pos);

            // The following are convenience conversion functions
            bool_t get_bool(bool_t default_ = false) const {return is_bool()? bool_: default_;}
            int_t get_int(int_t default_ = 0) const {return is_int()? int_: default_;}
            uint_t get_uint(uint_t default_ = 0) const {return is_uint()? uint_: default_;}
            real_t get_real(real_t default_ = 0.0) const {return is_real()? real_: default_;}
            cstring_t get_cstring(cstring_t default_ = "") const {return is_string()? str_ref_().c_str(): default_;}
            string_t get_string(const string_t &default_ = string_t()) const {return is_string()? str_ref_(): default_;}
            array_t get_array(const array_t &default_) const;
            object_t get_object(const object_t &default_) const;
            array_t get_array() const;
            object_t get_object() const;

            bool_t as_bool(bool_t default_ = false) const {return value(*this).convert_to(boolean, default_).bool_;}
            int_t as_int(int_t default_ = 0) const {return value(*this).convert_to(integer, default_).int_;}
            uint_t as_uint(uint_t default_ = 0) const {return value(*this).convert_to(uinteger, default_).uint_;}
            real_t as_real(real_t default_ = 0.0) const {return value(*this).convert_to(real, default_).real_;}
            string_t as_string(const string_t &default_ = string_t()) const {return value(*this).convert_to(string, default_).str_ref_();}
            array_t as_array(const array_t &default_) const;
            object_t as_object(const object_t &default_) const;
            array_t as_array() const;
            object_t as_object() const;

            bool_t &convert_to_bool(bool_t default_ = false) {return convert_to(boolean, default_).bool_;}
            int_t &convert_to_int(int_t default_ = 0) {return convert_to(integer, default_).int_;}
            uint_t &convert_to_uint(uint_t default_ = 0) {return convert_to(uinteger, default_).uint_;}
            real_t &convert_to_real(real_t default_ = 0.0) {return convert_to(real, default_).real_;}
            string_t &convert_to_string(const string_t &default_ = string_t()) {return convert_to(string, default_).str_ref_();}
            array_t &convert_to_array(const array_t &default_) {return convert_to(array, default_).arr_ref_();}
            object_t &convert_to_object(const object_t &default_) {return convert_to(object, default_).obj_ref_();}
            array_t &convert_to_array();
            object_t &convert_to_object();

            template<typename T>
            typename std::remove_cv<typename std::remove_reference<T>::type>::type
            cast() const {return cast_from_cppdatalib<typename std::remove_cv<typename std::remove_reference<T>::type>::type>(*this);}

            template<typename T, typename UserData>
            typename std::remove_cv<typename std::remove_reference<T>::type>::type
            cast(UserData userdata) const {return cast_from_cppdatalib<typename std::remove_cv<typename std::remove_reference<T>::type>::type>(*this, userdata);}

            template<template<typename...> class Template, typename... Ts>
            Template<Ts...> cast() const {return cast_template_from_cppdatalib<Template, Ts...>(*this);}

            template<template<typename...> class Template, typename UserData, typename... Ts>
            Template<Ts...> cast(UserData userdata) const {return cast_template_from_cppdatalib<Template, Ts...>(*this, userdata);}

            template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
            Template<T, N, Ts...> cast() const {return cast_array_template_from_cppdatalib<Template, T, N, Ts...>(*this);}

            template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename UserData, typename... Ts>
            Template<T, N, Ts...> cast(UserData userdata) const {return cast_array_template_from_cppdatalib<Template, T, N, Ts...>(*this, userdata);}

            template<template<size_t, typename...> class Template, size_t N, typename... Ts>
            Template<N, Ts...> cast() const {return cast_sized_template_from_cppdatalib<Template, N, Ts...>(*this);}

            template<template<size_t, typename...> class Template, size_t N, typename UserData, typename... Ts>
            Template<N, Ts...> cast(UserData userdata) const {return cast_sized_template_from_cppdatalib<Template, N, Ts...>(*this, userdata);}

            template<typename T>
            operator T() const {return cast_from_cppdatalib<typename std::remove_cv<typename std::remove_reference<T>::type>::type>(*this);}

            template<template<typename...> class Template, typename... Ts>
            operator Template<Ts...>() const {return cast_template_from_cppdatalib<Template, Ts...>(*this);}

            template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
            operator Template<T, N, Ts...>() const {return cast_array_template_from_cppdatalib<Template, T, N, Ts...>(*this);}

            template<template<size_t, typename...> class Template, size_t N, typename... Ts>
            operator Template<N, Ts...>() const {return cast_sized_template_from_cppdatalib<Template, N, Ts...>(*this);}

        private:
            // WARNING: DO NOT CALL mutable_clear() anywhere but the destructor!
            // It violates const-correctness for the sole purpose of allowing
            // complex object keys to be destroyed iteratively, instead of recursively.
            // (They're defined as `const` members in the std::map implementation)
            void mutable_clear() const;
            void shallow_clear() {deinit();}

            void clear(type new_type)
            {
                if (type_ == new_type)
                    return;

                deinit();
                init(new_type, normal);
            }

            void init(type new_type, subtype_t new_subtype);

            template<typename... Args>
            void bool_init(subtype_t new_subtype, Args... args)
            {
                new (&bool_) bool_t(args...);
                type_ = boolean;
                subtype_ = new_subtype;
            }

            template<typename... Args>
            void int_init(subtype_t new_subtype, Args... args)
            {
                new (&int_) int_t(args...);
                type_ = integer;
                subtype_ = new_subtype;
            }

            template<typename... Args>
            void uint_init(subtype_t new_subtype, Args... args)
            {
                new (&uint_) uint_t(args...);
                type_ = uinteger;
                subtype_ = new_subtype;
            }

            template<typename... Args>
            void real_init(subtype_t new_subtype, Args... args)
            {
                new (&real_) real_t(args...);
                type_ = real;
                subtype_ = new_subtype;
            }

            template<typename... Args>
            void string_init(subtype_t new_subtype, Args... args)
            {
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                new (&str_) string_t(args...);
#else
                new (&ptr_) string_t*(); ptr_ = new string_t(args...);
#endif
                type_ = string;
                subtype_ = new_subtype;
            }

            template<typename... Args>
            void array_init(subtype_t new_subtype, Args... args);

            template<typename... Args>
            void object_init(subtype_t new_subtype, Args... args);

            void deinit();

            // TODO: ensure that all conversions are generic enough (for example, string -> bool doesn't just need to be "true")
            value &convert_to(type new_type, const value &default_value)
            {
                if (type_ == new_type)
                    return *this;

                switch (type_)
                {
                    default:
                    case null: *this = default_value; break;
                    case boolean:
                    {
                        switch (new_type)
                        {
                            case integer: set_int((bool) bool_); break;
                            case uinteger: set_uint((bool) bool_); break;
                            case real: set_real((bool) bool_); break;
                            case string: set_string((bool) bool_? "true": "false"); break;
                            default: *this = default_value; break;
                        }
                        break;
                    }
                    case integer:
                    {
                        switch (new_type)
                        {
                            case boolean: set_bool(int_ != 0); break;
                            case uinteger: set_uint(int_ > 0? int_: 0); break;
                            case real: set_real(static_cast<real_t>(int_)); break;
                            case string: set_string(std::to_string(int_)); break;
                            default: *this = default_value; break;
                        }
                        break;
                    }
                    case uinteger:
                    {
                        switch (new_type)
                        {
                            case boolean: set_bool(uint_ != 0); break;
                            case integer: set_int(uint_ <= INT64_MAX? uint_: 0); break;
                            case real: set_real(static_cast<real_t>(uint_)); break;
                            case string: set_string(std::to_string(uint_)); break;
                            default: *this = default_value; break;
                        }
                        break;
                    }
                    case real:
                    {
                        switch (new_type)
                        {
                            case boolean: set_bool(real_ != 0.0); break;
                            case integer: set_int((real_ >= INT64_MIN && real_ <= INT64_MAX)? static_cast<int_t>(trunc(real_)): 0); break;
                            case uinteger: set_uint((real_ >= 0 && real_ <= UINT64_MAX)? static_cast<uint_t>(trunc(real_)): 0); break;
                            case string:
                            {
                                std::ostringstream str;
                                str << std::setprecision(CPPDATALIB_REAL_DIG) << real_;
                                set_string(str.str());
                                break;
                            }
                            default: *this = default_value; break;
                        }
                        break;
                    }
                    case string:
                    {
                        switch (new_type)
                        {
                            case boolean: set_bool(str_ref_() == "true"); break;
                            case integer:
                            {
                                std::istringstream str(str_ref_());
                                clear(integer);
                                str >> int_;
                                if (!str)
                                    int_ = 0;
                                break;
                            }
                            case uinteger:
                            {
                                std::istringstream str(str_ref_());
                                clear(uinteger);
                                str >> uint_;
                                if (!str)
                                    uint_ = 0;
                                break;
                            }
                            case real:
                            {
                                std::istringstream str(str_ref_());
                                clear(real);
                                str >> real_;
                                if (!str)
                                    real_ = 0.0;
                                break;
                            }
                            default: *this = default_value; break;
                        }
                        break;
                    }
                }

                return *this;
            }

            string_t &str_ref_() {
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                return str_;
#else
                return *reinterpret_cast<string_t *>(ptr_);
#endif
            }
            const string_t &str_ref_() const {
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                return str_;
#else
                return *reinterpret_cast<const string_t *>(ptr_);
#endif
            }

            array_t &arr_ref_() {
                return *reinterpret_cast<array_t *>(ptr_);
            }
            const array_t &arr_ref_() const {
                return *reinterpret_cast<const array_t *>(ptr_);
            }

            object_t &obj_ref_() {
                return *reinterpret_cast<object_t *>(ptr_);
            }
            const object_t &obj_ref_() const {
                return *reinterpret_cast<const object_t *>(ptr_);
            }

            union
            {
                bool_t bool_;
                int_t int_;
                uint_t uint_;
                real_t real_;
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                string_t str_;
#endif
                mutable void *ptr_; // Mutable to provide editable traversal access to const destructor
            };
            mutable type type_; // Mutable to provide editable traversal access to const destructor
            subtype_t subtype_;
        };

        namespace impl
        {
            template<typename IteratorType, typename PointedTo>
            struct const_iterator_t
            {
                const_iterator_t(IteratorType iterator = IteratorType()) : it(iterator) {}
                virtual ~const_iterator_t() {}

                IteratorType &data() {return it;}
                const IteratorType &data() const {return it;}

                operator IteratorType &() {return it;}
                operator const IteratorType &() const {return it;}

                const_iterator_t<IteratorType, PointedTo> &operator++() {++it; return *this;}
                const_iterator_t<IteratorType, PointedTo> operator++(int) {const_iterator_t<IteratorType, PointedTo> temp(*this); ++it; return temp;}

                const PointedTo &operator*() const {return *it;}
                const PointedTo *operator->() const {return it.operator->();}

            protected:
                IteratorType it;
            };

            template<typename IteratorType, typename PointedTo>
            bool operator==(const const_iterator_t<IteratorType, PointedTo> &lhs, const const_iterator_t<IteratorType, PointedTo> &rhs)
            {
                return lhs.data() == rhs.data();
            }
            template<typename IteratorType, typename PointedTo>
            bool operator!=(const const_iterator_t<IteratorType, PointedTo> &lhs, const const_iterator_t<IteratorType, PointedTo> &rhs)
            {
                return !(lhs == rhs);
            }

            template<typename IteratorType, typename PointedTo>
            struct iterator_t
            {
                iterator_t(IteratorType iterator = IteratorType()) : it(iterator) {}
                virtual ~iterator_t() {}

                IteratorType &data() {return it;}
                const IteratorType &data() const {return it;}

                operator IteratorType &() {return it;}
                operator const IteratorType &() const {return it;}

                iterator_t<IteratorType, PointedTo> &operator++() {++it; return *this;}
                iterator_t<IteratorType, PointedTo> operator++(int) {iterator_t<IteratorType, PointedTo> temp(*this); ++it; return temp;}

                iterator_t<IteratorType, PointedTo> &operator--() {--it; return *this;}
                iterator_t<IteratorType, PointedTo> operator--(int) {iterator_t<IteratorType, PointedTo> temp(*this); --it; return temp;}

                PointedTo &operator*() {return *it;}
                PointedTo *operator->() {return it.operator->();}

            protected:
                IteratorType it;
            };

            template<typename IteratorType, typename PointedTo>
            bool operator==(const iterator_t<IteratorType, PointedTo> &lhs, const iterator_t<IteratorType, PointedTo> &rhs)
            {
                return lhs.data() == rhs.data();
            }
            template<typename IteratorType, typename PointedTo>
            bool operator!=(const iterator_t<IteratorType, PointedTo> &lhs, const iterator_t<IteratorType, PointedTo> &rhs)
            {
                return !(lhs == rhs);
            }
        }

        class array_t
        {
		public:
#ifdef CPPDATALIB_ARRAY_T
            typedef CPPDATALIB_ARRAY_T container_type;
#else
            typedef std::vector<value> container_type;
#endif
            array_t() {}
            array_t(const container_type &data) : m_data(data) {}
            array_t(std::initializer_list<typename container_type::value_type> il) : m_data(il) {}
            template<typename... Ts>
            array_t(Ts&&... args) : m_data(std::forward<Ts>(args)...) {}

            typedef typename container_type::iterator iterator;
            typedef typename container_type::const_iterator const_iterator;

            bool empty() const {return m_data.empty();}
            size_t size() const {return m_data.size();}

            const value &operator[](size_t idx) const {return m_data[idx];}
            value &operator[](size_t idx) {return m_data[idx];}

            array_iterator_t begin();
            array_const_iterator_t begin() const;
            array_iterator_t end();
            array_const_iterator_t end() const;
            array_const_iterator_t cbegin() const;
            array_const_iterator_t cend() const;

            container_type &data() {return m_data;}
            const container_type &data() const {return m_data;}

            operator container_type &() {return m_data;}
            operator const container_type &() const {return m_data;}

        private:
            container_type m_data;
        };

        class array_iterator_t : public impl::iterator_t<array_t::iterator, typename array_t::container_type::value_type>
        {
		public:
            using impl::iterator_t<array_t::iterator, typename array_t::container_type::value_type>::iterator_t;
        };

        class array_const_iterator_t : public impl::const_iterator_t<array_t::const_iterator, const typename array_t::container_type::value_type>
        {
		public:
            using impl::const_iterator_t<array_t::const_iterator, const typename array_t::container_type::value_type>::const_iterator_t;
        };

        array_iterator_t array_t::begin() {return m_data.begin();}
        array_const_iterator_t array_t::begin() const {return m_data.begin();}
        array_iterator_t array_t::end() {return m_data.end();}
        array_const_iterator_t array_t::end() const {return m_data.end();}
        array_const_iterator_t array_t::cbegin() const {return m_data.cbegin();}
        array_const_iterator_t array_t::cend() const {return m_data.cend();}

        class object_t
        {
		public:
#ifdef CPPDATALIB_OBJECT_T
            typedef CPPDATALIB_OBJECT_T container_type;
#else
            typedef std::multimap<value, value> container_type;
#endif
            object_t() {}
            object_t(const container_type &data) : m_data(data) {}
            object_t(std::initializer_list<typename container_type::value_type> il) : m_data(il) {}
            template<typename... Ts>
            object_t(Ts&&... args) : m_data(std::forward<Ts>(args)...) {}

            typedef typename container_type::iterator iterator;
            typedef typename container_type::const_iterator const_iterator;

            bool empty() const {return m_data.empty();}
            size_t size() const {return m_data.size();}

            object_iterator_t begin();
            object_const_iterator_t begin() const;
            object_iterator_t end();
            object_const_iterator_t end() const;
            object_const_iterator_t cbegin() const;
            object_const_iterator_t cend() const;

            container_type &data() {return m_data;}
            const container_type &data() const {return m_data;}

            operator container_type &() {return m_data;}
            operator const container_type &() const {return m_data;}

        private:
            container_type m_data;
        };

        class object_iterator_t : public impl::iterator_t<object_t::iterator, typename object_t::container_type::value_type>
        {
		public:
            using impl::iterator_t<object_t::iterator, typename object_t::container_type::value_type>::iterator_t;
        };
        class object_const_iterator_t : public impl::const_iterator_t<object_t::const_iterator, typename object_t::container_type::value_type>
        {
		public:
            using impl::const_iterator_t<object_t::const_iterator, typename object_t::container_type::value_type>::const_iterator_t;
        };

        object_iterator_t object_t::begin() {return m_data.begin();}
        object_const_iterator_t object_t::begin() const {return m_data.begin();}
        object_iterator_t object_t::end() {return m_data.end();}
        object_const_iterator_t object_t::end() const {return m_data.end();}
        object_const_iterator_t object_t::cbegin() const {return m_data.cbegin();}
        object_const_iterator_t object_t::cend() const {return m_data.cend();}

        struct value::traversal_reference
        {
            traversal_reference(const value *p, array_const_iterator_t a, object_const_iterator_t o, bool traversed_key, bool frozen = false)
                : p(p)
                , array(a)
                , object(o)
                , traversed_key_already(traversed_key)
                , frozen(frozen)
            {}

            bool is_array() const {return p && p->is_array() && array != array_const_iterator_t() && array != p->get_array_unchecked().end();}
            size_t get_array_index() const {return is_array()? array.data() - p->get_array_unchecked().begin().data(): 0;}
            const core::value *get_array_element() const {return is_array()? std::addressof(*array): NULL;}

            bool is_object() const {return p && p->is_object() && object != object_const_iterator_t() && object != p->get_object_unchecked().end();}
            bool is_object_key() const {return is_object() && !traversed_key_already;}
            const core::value *get_object_key() const {return is_object()? std::addressof(object->first): NULL;}
            const core::value *get_object_value() const {return is_object()? std::addressof(object->second): NULL;}

        private:
            friend class value;
            friend class value_parser;

            const value *p;
            array_const_iterator_t array;
            object_const_iterator_t object;
            bool traversed_key_already;
            bool frozen;
        };

        template<typename... Ts>
        value::value(std::initializer_list<Ts...> v, subtype_t subtype)
        {
            array_init(subtype, core::array_t());
            for (auto const &element: v)
                push_back(element);
        }

        template<typename... Args>
        void value::array_init(subtype_t new_subtype, Args... args)
        {
            new (&ptr_) array_t*(); ptr_ = new array_t(args...);
            type_ = array;
            subtype_ = new_subtype;
        }

        template<typename... Args>
        void value::object_init(subtype_t new_subtype, Args... args)
        {
            new (&ptr_) object_t*(); ptr_ = new object_t(args...);
            type_ = object;
            subtype_ = new_subtype;
        }

        // Predicates must be callables with argument type `const core::value *arg, core::value::traversal_ancestry_finder arg_finder` and return value bool
        // If return value is non-zero, processing continues, otherwise processing aborts immediately
        template<typename PrefixPredicate, typename PostfixPredicate>
        void value::traverse(PrefixPredicate &prefix, PostfixPredicate &postfix) const
        {
            std::stack<traversal_reference, std::vector<traversal_reference>> references;
            const value *p = this;

            while (!references.empty() || p != NULL)
            {
                if (p != NULL)
                {
                    if (!prefix(p, traversal_ancestry_finder(references)))
                        return;

                    if (p->is_array())
                    {
                        references.push(traversal_reference(p, p->get_array_unchecked().begin(), object_const_iterator_t(), false));
                        if (!p->get_array_unchecked().empty())
                            p = std::addressof(*references.top().array++);
                        else
                            p = NULL;
                    }
                    else if (p->is_object())
                    {
                        references.push(traversal_reference(p, array_const_iterator_t(), p->get_object_unchecked().begin(), true));
                        if (!p->get_object_unchecked().empty())
                            p = std::addressof(references.top().object->first);
                        else
                            p = NULL;
                    }
                    else
                    {
                        references.push(traversal_reference(p, array_const_iterator_t(), object_const_iterator_t(), false));
                        p = NULL;
                    }
                }
                else
                {
                    const value *peek = references.top().p;
                    if (peek->is_array() && references.top().array != peek->get_array_unchecked().end())
                        p = std::addressof(*references.top().array++);
                    else if (peek->is_object() && references.top().object != peek->get_object_unchecked().end())
                    {
                        if (!references.top().traversed_key_already)
                            p = std::addressof(references.top().object->first);
                        else
                            p = std::addressof((references.top().object++)->second);

                        references.top().traversed_key_already = !references.top().traversed_key_already;
                    }
                    else
                    {
                        references.pop();
                        if (!postfix(peek, traversal_ancestry_finder(references)))
                            return;
                    }
                }
            }
        }

        // Predicates must be callables with argument types `const core::value *arg, core::value::traversal_ancestry_finder arg_finder, bool prefix` and return value bool
        // `prefix` is set to true if the current invocation is for the prefix handling of the specified element
        // If return value is non-zero, processing continues, otherwise processing aborts immediately
        template<typename Predicate>
        void value::traverse(Predicate &predicate) const
        {
            std::stack<traversal_reference, std::vector<traversal_reference>> references;
            const value *p = this;

            while (!references.empty() || p != NULL)
            {
                if (p != NULL)
                {
                    if (!predicate(p, traversal_ancestry_finder(references), true))
                        return;

                    if (p->is_array())
                    {
                        references.push(traversal_reference(p, p->get_array_unchecked().begin(), object_const_iterator_t(), false));
                        if (!p->get_array_unchecked().empty())
                            p = std::addressof(*references.top().array++);
                        else
                            p = NULL;
                    }
                    else if (p->is_object())
                    {
                        references.push(traversal_reference(p, array_const_iterator_t(), p->get_object_unchecked().begin(), true));
                        if (!p->get_object_unchecked().empty())
                            p = std::addressof(references.top().object->first);
                        else
                            p = NULL;
                    }
                    else
                    {
                        references.push(traversal_reference(p, array_const_iterator_t(), object_const_iterator_t(), false));
                        p = NULL;
                    }
                }
                else
                {
                    const value *peek = references.top().p;
                    if (peek->is_array() && references.top().array != peek->get_array_unchecked().end())
                        p = std::addressof(*references.top().array++);
                    else if (peek->is_object() && references.top().object != peek->get_object_unchecked().end())
                    {
                        if (!references.top().traversed_key_already)
                            p = std::addressof(references.top().object->first);
                        else
                            p = std::addressof((references.top().object++)->second);

                        references.top().traversed_key_already = !references.top().traversed_key_already;
                    }
                    else
                    {
                        references.pop();
                        if (!predicate(peek, traversal_ancestry_finder(references), false))
                            return;
                    }
                }
            }
        }

        // Predicates must be callables with argument type `const core::value *arg, core::value::traversal_ancestry_finder arg_finder` and return value bool
        // If return value is non-zero, processing continues, otherwise processing aborts immediately
        // NOTE: The predicate is only called for object values, not object keys. It's invoked for all other values.
        template<typename PrefixPredicate, typename PostfixPredicate>
        void value::value_traverse(PrefixPredicate &prefix, PostfixPredicate &postfix) const
        {
            std::stack<traversal_reference, std::vector<traversal_reference>> references;
            const value *p = this;

            while (!references.empty() || p != NULL)
            {
                if (p != NULL)
                {
                    if (!prefix(p, traversal_ancestry_finder(references)))
                        return;

                    if (p->is_array())
                    {
                        references.push(traversal_reference(p, p->get_array_unchecked().begin(), object_const_iterator_t(), false));
                        if (!p->get_array_unchecked().empty())
                            p = std::addressof(*references.top().array++);
                        else
                            p = NULL;
                    }
                    else if (p->is_object())
                    {
                        references.push(traversal_reference(p, array_const_iterator_t(), p->get_object_unchecked().begin(), true));
                        if (!p->get_object_unchecked().empty())
                            p = std::addressof((references.top().object++)->second);
                        else
                            p = NULL;
                    }
                    else
                    {
                        references.push(traversal_reference(p, array_const_iterator_t(), object_const_iterator_t(), false));
                        p = NULL;
                    }
                }
                else
                {
                    const value *peek = references.top().p;
                    if (peek->is_array() && references.top().array != peek->get_array_unchecked().end())
                        p = std::addressof(*references.top().array++);
                    else if (peek->is_object() && references.top().object != peek->get_object_unchecked().end())
                        p = std::addressof((references.top().object++)->second);
                    else
                    {
                        references.pop();
                        if (!postfix(peek, traversal_ancestry_finder(references)))
                            return;
                    }
                }
            }
        }

        // Predicates must be callables with argument types `const core::value *arg, core::value::traversal_ancestry_finder arg_finder, bool prefix` and return value bool
        // `prefix` is set to true if the current invocation is for the prefix handling of the specified element
        // If return value is non-zero, processing continues, otherwise processing aborts immediately
        // NOTE: The predicate is only called for object values, not object keys. It's invoked for all other values.
        template<typename Predicate>
        void value::value_traverse(Predicate &predicate) const
        {
            std::stack<traversal_reference, std::vector<traversal_reference>> references;
            const value *p = this;

            while (!references.empty() || p != NULL)
            {
                if (p != NULL)
                {
                    if (!predicate(p, traversal_ancestry_finder(references), true))
                        return;

                    if (p->is_array())
                    {
                        references.push(traversal_reference(p, p->get_array_unchecked().begin(), object_const_iterator_t(), false));
                        if (!p->get_array_unchecked().empty())
                            p = std::addressof(*references.top().array++);
                        else
                            p = NULL;
                    }
                    else if (p->is_object())
                    {
                        references.push(traversal_reference(p, array_const_iterator_t(), p->get_object_unchecked().begin(), true));
                        if (!p->get_object_unchecked().empty())
                            p = std::addressof((references.top().object++)->second);
                        else
                            p = NULL;
                    }
                    else
                    {
                        references.push(traversal_reference(p, array_const_iterator_t(), object_const_iterator_t(), false));
                        p = NULL;
                    }
                }
                else
                {
                    const value *peek = references.top().p;
                    if (peek->is_array() && references.top().array != peek->get_array_unchecked().end())
                        p = std::addressof(*references.top().array++);
                    else if (peek->is_object() && references.top().object != peek->get_object_unchecked().end())
                        p = std::addressof((references.top().object++)->second);
                    else
                    {
                        references.pop();
                        if (!predicate(peek, traversal_ancestry_finder(references), false))
                            return;
                    }
                }
            }
        }

        // Predicates must be callables with argument type `const core::value *arg, core::value::traversal_ancestry_finder arg_finder` and return value bool
        // If return value is non-zero, processing continues, otherwise processing aborts immediately
        template<typename PrefixPredicate>
        void value::prefix_traverse(PrefixPredicate &prefix) {traverse(prefix, traverse_node_null);}

        // Predicates must be callables with argument type `const core::value *arg, core::value::traversal_ancestry_finder arg_finder` and return value bool
        // If return value is non-zero, processing continues, otherwise processing aborts immediately
        template<typename PostfixPredicate>
        void value::postfix_traverse(PostfixPredicate &postfix) {traverse(traverse_node_null, postfix);}

        // Predicates must be callables with argument type `const core::value *arg, const core::value *arg2, core::value::traversal_ancestry_finder arg_finder, core::value::traversal_ancestry_finder arg2_finder`
        // and return value bool. Either argument may be NULL, but both arguments will never be NULL simultaneously.
        // If return value is non-zero, processing continues, otherwise processing aborts immediately
        template<typename PrefixPredicate, typename PostfixPredicate>
        void value::parallel_traverse(const value &other, PrefixPredicate &prefix, PostfixPredicate &postfix) const
        {
            std::stack<traversal_reference, std::vector<traversal_reference>> references;
            std::stack<traversal_reference, std::vector<traversal_reference>> other_references;
            const value *p = this, *other_p = &other;

            while (!references.empty() || !other_references.empty() || p != NULL || other_p != NULL)
            {
                if (p != NULL || other_p != NULL)
                {
                    if (!prefix(p, other_p, traversal_ancestry_finder(references), traversal_ancestry_finder(other_references)))
                        return;

                    if (p != NULL)
                    {
                        if (p->is_array())
                        {
                            references.push(traversal_reference(p, p->get_array_unchecked().begin(), object_const_iterator_t(), false));
                            if (!p->get_array_unchecked().empty())
                                p = std::addressof(*references.top().array++);
                            else
                                p = NULL;
                        }
                        else if (p->is_object())
                        {
                            references.push(traversal_reference(p, array_const_iterator_t(), p->get_object_unchecked().begin(), true));
                            if (!p->get_object_unchecked().empty())
                                p = std::addressof(references.top().object->first);
                            else
                                p = NULL;
                        }
                        else
                        {
                            references.push(traversal_reference(p, array_const_iterator_t(), object_const_iterator_t(), false));
                            p = NULL;
                        }
                    }

                    if (other_p != NULL)
                    {
                        if (other_p->is_array())
                        {
                            other_references.push(traversal_reference(other_p, other_p->get_array_unchecked().begin(), object_const_iterator_t(), false));
                            if (!other_p->get_array_unchecked().empty())
                                other_p = std::addressof(*other_references.top().array++);
                            else
                                other_p = NULL;
                        }
                        else if (other_p->is_object())
                        {
                            other_references.push(traversal_reference(other_p, array_const_iterator_t(), other_p->get_object_unchecked().begin(), true));
                            if (!other_p->get_object_unchecked().empty())
                                other_p = std::addressof(other_references.top().object->first);
                            else
                                other_p = NULL;
                        }
                        else
                        {
                            other_references.push(traversal_reference(other_p, array_const_iterator_t(), object_const_iterator_t(), false));
                            other_p = NULL;
                        }
                    }
                }
                else
                {
                    const value *peek = references.empty()? NULL: references.top().p;
                    const value *other_peek = other_references.empty()? NULL: other_references.top().p;

                    if (peek)
                    {
                        if (peek->is_array() && references.top().array != peek->get_array_unchecked().end())
                            p = std::addressof(*references.top().array++);
                        else if (peek->is_object() && references.top().object != peek->get_object_unchecked().end())
                        {
                            if (!references.top().traversed_key_already)
                                p = std::addressof(references.top().object->first);
                            else
                                p = std::addressof((references.top().object++)->second);

                            references.top().traversed_key_already = !references.top().traversed_key_already;
                        }
                    }

                    if (other_peek)
                    {
                        if (other_peek->is_array() && other_references.top().array != other_peek->get_array_unchecked().end())
                            other_p = std::addressof(*other_references.top().array++);
                        else if (other_peek->is_object() && other_references.top().object != other_peek->get_object_unchecked().end())
                        {
                            if (!other_references.top().traversed_key_already)
                                other_p = std::addressof(other_references.top().object->first);
                            else
                                other_p = std::addressof((other_references.top().object++)->second);

                            other_references.top().traversed_key_already = !other_references.top().traversed_key_already;
                        }
                    }

                    if (p == NULL && other_p == NULL)
                    {
                        if (peek != NULL) references.pop();
                        if (other_peek != NULL) other_references.pop();
                        if (!postfix(peek, other_peek, traversal_ancestry_finder(references), traversal_ancestry_finder(other_references)))
                            return;
                    }
                }
            }
        }

        // Predicates must be callables with argument type `const core::value *arg, const core::value *arg2, core::value::traversal_ancestry_finder arg_finder, core::value::traversal_ancestry_finder arg2_finder`
        // and return value bool. Either argument may be NULL, but both arguments will never be NULL simultaneously.
        // If return value is non-zero, processing continues, otherwise processing aborts immediately
        template<typename PrefixPredicate, typename PostfixPredicate>
        void value::parallel_diff_traverse(const value &other, PrefixPredicate &prefix, PostfixPredicate &postfix) const
        {
            std::stack<traversal_reference, std::vector<traversal_reference>> references;
            std::stack<traversal_reference, std::vector<traversal_reference>> other_references;
            const value *p = this, *other_p = &other;

            while (!references.empty() || !other_references.empty() || p != NULL || other_p != NULL)
            {
                if (p != NULL || other_p != NULL)
                {
                    bool p_frozen = false, other_p_frozen = false;

                    auto save_refs = references;
                    auto save_other_refs = other_references;

                    if (!save_refs.empty() && !save_refs.top().is_array() && !save_refs.top().is_object())
                        save_refs.pop();

                    if (!save_other_refs.empty() && !save_other_refs.top().is_array() && !save_other_refs.top().is_object())
                        save_other_refs.pop();

                    traversal_reference save_ref = save_refs.empty()? traversal_reference(NULL, array_const_iterator_t(), object_const_iterator_t(), false): save_refs.top();
                    traversal_reference save_other_ref = save_other_refs.empty()? traversal_reference(NULL, array_const_iterator_t(), object_const_iterator_t(), false): save_other_refs.top();

                    if (save_ref.is_object() && save_other_ref.is_object())
                    {
                        p_frozen = false;
                        other_p_frozen = false;
                        if (*save_ref.get_object_key() < *save_other_ref.get_object_key())
                            other_p_frozen = true;
                        else if (*save_other_ref.get_object_key() < *save_ref.get_object_key())
                            p_frozen = true;
                    }

                    if (!prefix(p_frozen? NULL: p, other_p_frozen? NULL: other_p, traversal_ancestry_finder(references), traversal_ancestry_finder(other_references)))
                        return;

                    if (p != NULL)
                    {
                        if (p->is_array())
                        {
                            references.push(traversal_reference(p, p->get_array_unchecked().begin(), object_const_iterator_t(), false, p_frozen));
                            if (!p->get_array_unchecked().empty() && !p_frozen)
                                p = std::addressof(*references.top().array++);
                            else
                                p = NULL;
                        }
                        else if (p->is_object())
                        {
                            references.push(traversal_reference(p, array_const_iterator_t(), p->get_object_unchecked().begin(), true, p_frozen));
                            if (!p->get_object_unchecked().empty() && !p_frozen)
                                p = std::addressof(references.top().object->first);
                            else
                                p = NULL;
                        }
                        else
                        {
                            references.push(traversal_reference(p, array_const_iterator_t(), object_const_iterator_t(), false, p_frozen));
                            p = NULL;
                        }
                    }

                    if (other_p != NULL)
                    {
                        if (other_p->is_array())
                        {
                            other_references.push(traversal_reference(other_p, other_p->get_array_unchecked().begin(), object_const_iterator_t(), false, other_p_frozen));
                            if (!other_p->get_array_unchecked().empty() && !other_p_frozen)
                                other_p = std::addressof(*other_references.top().array++);
                            else
                                other_p = NULL;
                        }
                        else if (other_p->is_object())
                        {
                            other_references.push(traversal_reference(other_p, array_const_iterator_t(), other_p->get_object_unchecked().begin(), true, other_p_frozen));
                            if (!other_p->get_object_unchecked().empty() && !other_p_frozen)
                                other_p = std::addressof(other_references.top().object->first);
                            else
                                other_p = NULL;
                        }
                        else
                        {
                            other_references.push(traversal_reference(other_p, array_const_iterator_t(), object_const_iterator_t(), false, other_p_frozen));
                            other_p = NULL;
                        }
                    }
                }
                else // p and other_p are both NULL here
                {
                    const value *peek = references.empty()? NULL: references.top().p;
                    const value *other_peek = other_references.empty()? NULL: other_references.top().p;
                    bool p_was_frozen = peek? references.top().frozen: false;
                    bool other_p_was_frozen = peek? other_references.top().frozen: false;

                    if (peek)
                    {
                        if (references.top().frozen)
                            p = references.top().p;
                        else if (peek->is_array() && references.top().array != peek->get_array_unchecked().end())
                            p = std::addressof(*references.top().array++);
                        else if (peek->is_object() && references.top().object != peek->get_object_unchecked().end())
                        {
                            if (!references.top().traversed_key_already)
                                p = std::addressof(references.top().object->first);
                            else
                                p = std::addressof((references.top().object++)->second);

                            references.top().traversed_key_already = !references.top().traversed_key_already;
                        }
                    }

                    if (other_peek)
                    {
                        if (other_references.top().frozen)
                            other_p = other_references.top().p;
                        else if (other_peek->is_array() && other_references.top().array != other_peek->get_array_unchecked().end())
                            other_p = std::addressof(*other_references.top().array++);
                        else if (other_peek->is_object() && other_references.top().object != other_peek->get_object_unchecked().end())
                        {
                            if (!other_references.top().traversed_key_already)
                                other_p = std::addressof(other_references.top().object->first);
                            else
                                other_p = std::addressof((other_references.top().object++)->second);

                            other_references.top().traversed_key_already = !other_references.top().traversed_key_already;
                        }
                    }

                    if (peek != NULL) references.pop();
                    if (other_peek != NULL) other_references.pop();
                    if (!postfix(p_was_frozen? NULL: peek, other_p_was_frozen? NULL: other_peek, traversal_ancestry_finder(references), traversal_ancestry_finder(other_references)))
                        return;
                }
            }
        }

        value::value(const array_t &v, subtype_t subtype) {array_init(subtype, v);}
        value::value(array_t &&v, subtype_t subtype) {array_init(subtype, std::move(v));}
        value::value(const object_t &v, subtype_t subtype) {object_init(subtype, v);}
        value::value(object_t &&v, subtype_t subtype) {object_init(subtype, std::move(v));}

        value::value(value &&other)
        {
            switch (other.type_)
            {
                case boolean: new (&bool_) bool_t(std::move(other.bool_)); break;
                case integer: new (&int_) int_t(std::move(other.int_)); break;
                case uinteger: new (&uint_) uint_t(std::move(other.uint_)); break;
                case real: new (&real_) real_t(std::move(other.real_)); break;
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                case string: new (&str_) string_t(std::move(other.str_)); break;
#else
                case string: new (&ptr_) string_t*(); ptr_ = new string_t(std::move(other.str_ref_())); break;
#endif
                case array: new (&ptr_) array_t*(); ptr_ = new array_t(std::move(other.arr_ref_())); break;
                case object: new (&ptr_) object_t*(); ptr_ = new object_t(std::move(other.obj_ref_())); break;
                default: break;
            }
            type_ = other.type_;
            subtype_ = other.subtype_;
        }

        value::~value()
        {
            if ((type_ == array && arr_ref_().size() > 0) ||
                (type_ == object && obj_ref_().size() > 0))
                traverse(traverse_node_null, traverse_node_mutable_clear);
            deinit();
        }

        value &value::operator=(value &&other)
        {
            clear(other.type_);
            switch (other.type_)
            {
                case boolean: bool_ = std::move(other.bool_); break;
                case integer: int_ = std::move(other.int_); break;
                case uinteger: uint_ = std::move(other.uint_); break;
                case real: real_ = std::move(other.real_); break;
                case string: str_ref_() = std::move(other.str_ref_()); break;
                case array: arr_ref_() = std::move(other.arr_ref_()); break;
                case object: obj_ref_() = std::move(other.obj_ref_()); break;
                default: break;
            }
            subtype_ = other.subtype_;
            return *this;
        }

        size_t value::size() const {return is_array()? arr_ref_().size(): is_object()? obj_ref_().size(): is_string()? str_ref_().size(): 0;}
        size_t value::array_size() const {return is_array()? arr_ref_().size(): 0;}
        size_t value::object_size() const {return is_object()? obj_ref_().size(): 0;}

        void value::set_array(const array_t &v) {clear(array); arr_ref_() = v;}
        void value::set_object(const object_t &v) {clear(object); obj_ref_() = v;}
        void value::set_array(const array_t &v, subtype_t subtype) {clear(array); arr_ref_() = v; subtype_ = subtype;}
        void value::set_object(const object_t &v, subtype_t subtype) {clear(object); obj_ref_() = v; subtype_ = subtype;}

        value value::operator[](cstring_t key) const {return member(key);}
        value &value::operator[](cstring_t key) {return member(key);}
        value value::operator[](const string_t &key) const {return member(key);}
        value &value::operator[](const string_t &key) {return member(key);}
        value value::member(const value &key) const
        {
            if (is_object())
            {
                auto it = obj_ref_().data().find(key);
                if (it != obj_ref_().end())
                    return it->second;
            }
            return value();
        }
        value &value::member(const value &key)
        {
            clear(object);
            auto it = obj_ref_().data().lower_bound(key);
            if (it != obj_ref_().end() && it->first == key)
                return it->second;
            it = obj_ref_().data().insert(it, {key, null_t()});
            return it->second;
        }
        const value *value::member_ptr(const value &key) const
        {
            if (is_object())
            {
                auto it = obj_ref_().data().find(key);
                if (it != obj_ref_().end())
                    return std::addressof(it->second);
            }
            return NULL;
        }
        bool_t value::is_member(cstring_t key) const {return is_object() && obj_ref_().data().find(key) != obj_ref_().end();}
        bool_t value::is_member(const string_t &key) const {return is_object() && obj_ref_().data().find(key) != obj_ref_().end();}
        bool_t value::is_member(const value &key) const {return is_object() && obj_ref_().data().find(key) != obj_ref_().end();}
        size_t value::member_count(cstring_t key) const {return is_object()? obj_ref_().data().count(key): 0;}
        size_t value::member_count(const string_t &key) const {return is_object()? obj_ref_().data().count(key): 0;}
        size_t value::member_count(const value &key) const {return is_object()? obj_ref_().data().count(key): 0;}
        void value::erase_member(cstring_t key) {if (is_object()) obj_ref_().data().erase(key);}
        void value::erase_member(const string_t &key) {if (is_object()) obj_ref_().data().erase(key);}
        void value::erase_member(const value &key) {if (is_object()) obj_ref_().data().erase(key);}

        value &value::add_member(const value &key)
        {
            clear(object);
            return obj_ref_().data().insert(std::make_pair(key, null_t()))->second;
        }
        value &value::add_member(value &&key)
        {
            clear(object);
            return obj_ref_().data().insert(std::make_pair(std::move(key), null_t()))->second;
        }
        value &value::add_member(const value &key, const value &val)
        {
            clear(object);
            return obj_ref_().data().insert(std::make_pair(key, val))->second;
        }
        value &value::add_member(value &&key, value &&val)
        {
            clear(object);
            return obj_ref_().data().insert(std::make_pair(std::move(key), std::move(val)))->second;
        }

        value &value::add_member_at_end(const value &key)
        {
            clear(object);
            return obj_ref_().data().insert(obj_ref_().end().data(), std::make_pair(key, null_t()))->second;
        }
        value &value::add_member_at_end(value &&key)
        {
            clear(object);
            return obj_ref_().data().insert(obj_ref_().end().data(), std::make_pair(std::move(key), null_t()))->second;
        }
        value &value::add_member_at_end(const value &key, const value &val)
        {
            clear(object);
            return obj_ref_().data().insert(obj_ref_().end().data(), std::make_pair(key, val))->second;
        }
        value &value::add_member_at_end(value &&key, value &&val)
        {
            clear(object);
            return obj_ref_().data().insert(obj_ref_().end().data(), std::make_pair(std::move(key), std::move(val)))->second;
        }

        void value::push_back(const value &v) {clear(array); arr_ref_().data().push_back(v);}
        void value::push_back(value &&v) {clear(array); arr_ref_().data().push_back(v);}
        value value::operator[](size_t pos) const {return element(pos);}
        value &value::operator[](size_t pos) {return element(pos);}
        value value::element(size_t pos) const {return is_array() && pos < arr_ref_().size()? arr_ref_()[pos]: value();}
        value &value::element(size_t pos)
        {
            clear(array);
            if (arr_ref_().size() <= pos)
                arr_ref_().data().insert(arr_ref_().end().data(), pos - arr_ref_().size() + 1, core::null_t());
            return arr_ref_()[pos];
        }
        void value::erase_element(size_t pos) {if (is_array()) arr_ref_().data().erase(arr_ref_().begin().data() + pos);}

        array_t value::get_array(const array_t &default_) const {return is_array()? arr_ref_(): default_;}
        object_t value::get_object(const object_t &default_) const {return is_object()? obj_ref_(): default_;}
        array_t value::get_array() const {return is_array()? arr_ref_(): array_t();}
        object_t value::get_object() const {return is_object()? obj_ref_(): object_t();}

        array_t value::as_array(const array_t &default_) const {return value(*this).convert_to(array, default_).arr_ref_();}
        object_t value::as_object(const object_t &default_) const {return value(*this).convert_to(object, default_).obj_ref_();}
        array_t value::as_array() const {return value(*this).convert_to(array, array_t()).arr_ref_();}
        object_t value::as_object() const {return value(*this).convert_to(object, object_t()).obj_ref_();}

        array_t &value::convert_to_array() {return convert_to(array, array_t()).arr_ref_();}
        object_t &value::convert_to_object() {return convert_to(object, object_t()).obj_ref_();}

        void value::mutable_clear() const
        {
            typedef void *ptr;

            switch (type_)
            {
                case boolean: bool_.~bool_t(); break;
                case integer: int_.~int_t(); break;
                case uinteger: uint_.~uint_t(); break;
                case real: real_.~real_t(); break;
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                case string: str_.~string_t(); break;
#else
                case string: delete reinterpret_cast<string_t*>(ptr_); ptr_.~ptr(); break;
#endif
                case array: delete reinterpret_cast<array_t*>(ptr_); ptr_.~ptr(); break;
                case object: delete reinterpret_cast<object_t*>(ptr_); ptr_.~ptr(); break;
                default: break;
            }
            type_ = null;
        }

        void value::init(type new_type, subtype_t new_subtype)
        {
            switch (new_type)
            {
                case boolean: new (&bool_) bool_t(); break;
                case integer: new (&int_) int_t(); break;
                case uinteger: new (&uint_) uint_t(); break;
                case real: new (&real_) real_t(); break;
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                case string: new (&str_) string_t(); break;
#else
                case string: new (&ptr_) string_t*(); ptr_ = new string_t(); break;
#endif
                case array: new (&ptr_) array_t*(); ptr_ = new array_t(); break;
                case object: new (&ptr_) object_t*(); ptr_ = new object_t(); break;
                default: break;
            }
            type_ = new_type;
            subtype_ = new_subtype;
        }

        void value::deinit()
        {
            typedef void *ptr;

            switch (type_)
            {
                case boolean: bool_.~bool_t(); break;
                case integer: int_.~int_t(); break;
                case uinteger: uint_.~uint_t(); break;
                case real: real_.~real_t(); break;
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                case string: str_.~string_t(); break;
#else
                case string: delete reinterpret_cast<string_t*>(ptr_); ptr_.~ptr(); break;
#endif
                case array: delete reinterpret_cast<array_t*>(ptr_); ptr_.~ptr(); break;
                case object: delete reinterpret_cast<object_t*>(ptr_); ptr_.~ptr(); break;
                default: break;
            }
            type_ = null;
            subtype_ = normal;
        }

        namespace impl
        {
            template<typename UserData>
            class extended_value_cast
            {
                const value &bind;
                UserData userdata;

            public:
                extended_value_cast(const value &bind, UserData userdata)
                    : bind(bind), userdata(userdata)
                {}

                template<typename T>
                operator T() const {return cast_from_cppdatalib<typename std::remove_cv<typename std::remove_reference<T>::type>::type>(bind, userdata);}

                template<template<typename...> class Template, typename... Ts>
                operator Template<Ts...>() const {return cast_template_from_cppdatalib<Template, Ts...>(bind, userdata);}

                template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
                operator Template<T, N, Ts...>() const {return cast_array_template_from_cppdatalib<Template, T, N, Ts...>(bind, userdata);}

                template<template<size_t, typename...> class Template, size_t N, typename... Ts>
                operator Template<N, Ts...>() const {return cast_sized_template_from_cppdatalib<Template, N, Ts...>(bind, userdata);}
            };

            template<typename T, typename U>
            T zero_convert(T min, U val, T max)
            {
                return val < min || max < val? T(0): T(val);
            }
        }

        template<typename T>
        cppdatalib::core::value cast(T val, subtype_t subtype = normal)
        {
            return cppdatalib::core::value(val, subtype);
        }

        const value &cast(const value &val)
        {
            return val;
        }

        template<typename UserData>
        impl::extended_value_cast<UserData &> userdata_cast(const value &bind, UserData &userdata)
        {
            return impl::extended_value_cast<UserData &>(bind, userdata);
        }

        template<typename UserData>
        impl::extended_value_cast<UserData> userdata_cast(const value &bind, const UserData &userdata)
        {
            return impl::extended_value_cast<UserData>(bind, userdata);
        }

        template<typename Bind, typename UserData>
        value userdata_cast(Bind bind, UserData &userdata, subtype_t subtype = normal)
        {
            return value(bind, userdata, subtype);
        }

        template<typename Bind, typename UserData>
        value userdata_cast(Bind bind, const UserData &userdata, subtype_t subtype = normal)
        {
            return value(bind, userdata, subtype);
        }
    }

    void swap(core::value &l, core::value &r) {l.swap(r);}
}

template<typename T>
class cast_to_cppdatalib
{
public:
    cast_to_cppdatalib(T) {}
    operator cppdatalib::core::value() const {return T::You_must_reimplement_cppdatalib_conversions_for_custom_types();}
};

template<template<typename...> class Template, typename... Ts>
struct cast_template_to_cppdatalib
{
public:
    cast_template_to_cppdatalib(const Template<Ts...> &) {}
    operator cppdatalib::core::value() const {return Template<Ts...>::You_must_reimplement_cppdatalib_conversions_for_custom_types();}
};

template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
struct cast_array_template_to_cppdatalib
{
public:
    cast_array_template_to_cppdatalib(const Template<T, N, Ts...> &) {}
    operator cppdatalib::core::value() const {return Template<T, N, Ts...>::You_must_reimplement_cppdatalib_conversions_for_custom_types();}
};

template<template<size_t, typename...> class Template, size_t N, typename... Ts>
struct cast_sized_template_to_cppdatalib
{
public:
    cast_sized_template_to_cppdatalib(const Template<N, Ts...> &) {}
    operator cppdatalib::core::value() const {return Template<N, Ts...>::You_must_reimplement_cppdatalib_conversions_for_custom_types();}
};

template<typename T>
class cast_from_cppdatalib {
public:
    cast_from_cppdatalib(const cppdatalib::core::value &) {}
    operator T() const {return T::You_must_reimplement_cppdatalib_conversions_for_custom_types();}
};

template<template<typename...> class Template, typename... Ts>
struct cast_template_from_cppdatalib
{
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &) {}
    operator Template<Ts...>() const {return Template<Ts...>::You_must_reimplement_cppdatalib_conversions_for_custom_types();}
};

template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
struct cast_array_template_from_cppdatalib
{
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &) {}
    operator Template<T, N, Ts...>() const {return Template<T, N, Ts...>::You_must_reimplement_cppdatalib_conversions_for_custom_types();}
};

template<template<size_t, typename...> class Template, size_t N, typename... Ts>
struct cast_sized_template_from_cppdatalib
{
public:
    cast_sized_template_from_cppdatalib(const cppdatalib::core::value &) {}
    operator Template<N, Ts...>() const {return Template<N, Ts...>::You_must_reimplement_cppdatalib_conversions_for_custom_types();}
};

template<>
class cast_to_cppdatalib<signed char>
{
    signed char bind;
public:
    cast_to_cppdatalib(signed char bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::int_t>(bind));}
};

template<>
class cast_to_cppdatalib<unsigned char>
{
    unsigned char bind;
public:
    cast_to_cppdatalib(unsigned char bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::uint_t>(bind));}
};

template<>
class cast_to_cppdatalib<signed short>
{
    signed short bind;
public:
    cast_to_cppdatalib(signed short bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::int_t>(bind));}
};

template<>
class cast_to_cppdatalib<unsigned short>
{
    unsigned short bind;
public:
    cast_to_cppdatalib(unsigned short bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::uint_t>(bind));}
};

template<>
class cast_to_cppdatalib<signed int>
{
    signed int bind;
public:
    cast_to_cppdatalib(signed int bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::int_t>(bind));}
};

template<>
class cast_to_cppdatalib<unsigned int>
{
    unsigned int bind;
public:
    cast_to_cppdatalib(unsigned int bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::uint_t>(bind));}
};

template<>
class cast_to_cppdatalib<signed long>
{
    signed long bind;
public:
    cast_to_cppdatalib(signed long bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::int_t>(bind));}
};

template<>
class cast_to_cppdatalib<unsigned long>
{
    unsigned long bind;
public:
    cast_to_cppdatalib(unsigned long bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::uint_t>(bind));}
};

template<>
class cast_to_cppdatalib<signed long long>
{
    signed long long bind;
public:
    cast_to_cppdatalib(signed long long bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::int_t>(bind));}
};

template<>
class cast_to_cppdatalib<unsigned long long>
{
    unsigned long long bind;
public:
    cast_to_cppdatalib(unsigned long long bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::uint_t>(bind));}
};

template<>
class cast_to_cppdatalib<float>
{
    float bind;
public:
    cast_to_cppdatalib(float bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::real_t>(bind));}
};

template<>
class cast_to_cppdatalib<double>
{
    double bind;
public:
    cast_to_cppdatalib(double bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::real_t>(bind));}
};

template<>
class cast_to_cppdatalib<long double>
{
    long double bind;
public:
    cast_to_cppdatalib(long double bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(static_cast<cppdatalib::core::real_t>(bind));}
};

template<>
class cast_to_cppdatalib<char *>
{
    char * const &bind;
public:
    cast_to_cppdatalib(char * const &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(cppdatalib::core::string_t(bind));}
};

template<>
class cast_to_cppdatalib<const char *>
{
    const char * const &bind;
public:
    cast_to_cppdatalib(const char * const &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(cppdatalib::core::string_t(bind));}
};

template<>
class cast_to_cppdatalib<cppdatalib::core::string_t>
{
    const cppdatalib::core::string_t &bind;
public:
    cast_to_cppdatalib(const cppdatalib::core::string_t &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(bind);}
};

template<>
class cast_to_cppdatalib<cppdatalib::core::array_t>
{
    const cppdatalib::core::array_t &bind;
public:
    cast_to_cppdatalib(const cppdatalib::core::array_t &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(bind);}
};

template<>
class cast_to_cppdatalib<cppdatalib::core::object_t>
{
    const cppdatalib::core::object_t &bind;
public:
    cast_to_cppdatalib(const cppdatalib::core::object_t &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(bind);}
};

template<>
class cast_from_cppdatalib<bool>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator bool() const {return bind.as_bool();}
};

template<>
class cast_from_cppdatalib<signed char>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator signed char() const {
        return cppdatalib::core::impl::zero_convert(std::numeric_limits<signed char>::min(),
                                      bind.as_int(),
                                      std::numeric_limits<signed char>::max());
    }
};

template<>
class cast_from_cppdatalib<unsigned char>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator unsigned char() const {
        return cppdatalib::core::impl::zero_convert(std::numeric_limits<unsigned char>::min(),
                                      bind.as_uint(),
                                      std::numeric_limits<unsigned char>::max());
    }
};

template<>
class cast_from_cppdatalib<signed short>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator signed short() const {
        return cppdatalib::core::impl::zero_convert(std::numeric_limits<signed short>::min(),
                                      bind.as_int(),
                                      std::numeric_limits<signed short>::max());
    }
};

template<>
class cast_from_cppdatalib<unsigned short>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator unsigned short() const {
        return cppdatalib::core::impl::zero_convert(std::numeric_limits<unsigned short>::min(),
                                      bind.as_uint(),
                                      std::numeric_limits<unsigned short>::max());
    }
};

template<>
class cast_from_cppdatalib<signed int>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator signed int() const {
        return cppdatalib::core::impl::zero_convert(std::numeric_limits<signed int>::min(),
                                      bind.as_int(),
                                      std::numeric_limits<signed int>::max());
    }
};

template<>
class cast_from_cppdatalib<unsigned int>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator unsigned int() const {
        return cppdatalib::core::impl::zero_convert(std::numeric_limits<unsigned int>::min(),
                                      bind.as_uint(),
                                      std::numeric_limits<unsigned int>::max());
    }
};

template<>
class cast_from_cppdatalib<signed long>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator signed long() const {
        return cppdatalib::core::impl::zero_convert(std::numeric_limits<signed long>::min(),
                                      bind.as_int(),
                                      std::numeric_limits<signed long>::max());
    }
};

template<>
class cast_from_cppdatalib<unsigned long>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator unsigned long() const {
        return cppdatalib::core::impl::zero_convert(std::numeric_limits<unsigned long>::min(),
                                      bind.as_uint(),
                                      std::numeric_limits<unsigned long>::max());
    }
};

template<>
class cast_from_cppdatalib<signed long long>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator signed long long() const {
        return cppdatalib::core::impl::zero_convert(std::numeric_limits<signed long long>::min(),
                                      bind.as_int(),
                                      std::numeric_limits<signed long long>::max());
    }
};

template<>
class cast_from_cppdatalib<unsigned long long>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator unsigned long long() const {
        return cppdatalib::core::impl::zero_convert(std::numeric_limits<unsigned long long>::min(),
                                      bind.as_uint(),
                                      std::numeric_limits<unsigned long long>::max());
    }
};

template<>
class cast_from_cppdatalib<float>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator float() const {return static_cast<float>(bind.as_real());}
};

template<>
class cast_from_cppdatalib<double>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator double() const {return bind.as_real();}
};

template<>
class cast_from_cppdatalib<long double>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator long double() const {return bind.as_real();}
};

template<>
class cast_from_cppdatalib<cppdatalib::core::string_t>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator cppdatalib::core::string_t() const {return bind.as_string();}
};

template<typename... Ts>
class cast_template_from_cppdatalib<std::basic_string, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::basic_string<Ts...>() const {return bind.as_string();}
};

template<>
class cast_from_cppdatalib<cppdatalib::core::array_t>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator cppdatalib::core::array_t() const {return bind.as_array();}
};

template<>
class cast_from_cppdatalib<cppdatalib::core::object_t>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator cppdatalib::core::object_t() const {return bind.as_object();}
};

#endif // CPPDATALIB_VALUE_H

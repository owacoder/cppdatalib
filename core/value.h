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

#include "cache_vector.h"
#include "global.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

#include <vector>
#include <map>
#include <set>

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
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            temporary_string,
#endif
            array,
            object,
            link // That is, a link (pointer) to another value structure, that may be linked to other values as well
            // If CPPDATALIB_ENABLE_ATTRIBUTES is defined, the normal null attribute of the target refers to the name of the reference
            // If the target has no normal null attribute, the link object itself is tested for the same value, and it is used as the reference name instead if available.
            // Otherwise, reference names are not supported
        };

        inline type get_domain(type t)
        {
            switch (t)
            {
                default:
                    return t;
                case integer:
                case uinteger:
                    return real;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                case temporary_string:
                    return string;
#endif
            }
        }

        // Value map:
        //     0 to maximum: format- or user-specified subtypes
        //     -9 to -1: generic subtypes applicable to all types
        //     -19 to -10: subtypes applicable to booleans
        //     -29 to -20: subtypes applicable to integers, either signed or unsigned
        //     -39 to -30: subtypes applicable only to signed integers
        //     -49 to -40: subtypes applicable only to unsigned integers
        //     -59 to -50: subtypes applicable to floating-point values
        //     -129 to -60: subtypes applicable to strings or temporary strings, encoded as some form of text
        //     -199 to -130: subtypes applicable to strings or temporary strings, encoded as some form of binary value
        //     -209 to -200: subtypes applicable to arrays
        //     -219 to -200: subtypes applicable to objects
        //     -229 to -220: subtypes applicable to links
        //     -239 to -230: subtypes applicable to nulls
        //     -255 to -240: undefined, reserved
        //     minimum to -256: format-specified reserved subtypes
        enum subtype
        {
            normal = -1, // Normal strings are encoded with valid UTF-8. Use blob or clob for other generic types of strings.
            // Normal links are weak links, that is, they don't own the object they point to

            // The `comparable` types should not be used in actual data values, but are intented to be used to find a specific value in a dataset
            domain_comparable = -2, // Domain comparables are not compared by type or subtype, but are compared by value in a specific domain (i.e. compare by numbers or compare by strings)
            generic_subtype_comparable = -3, // Generic subtype comparables are only compared by type and value, not by subtype.

            // Integers
            unix_timestamp = -29, // Number of seconds since the epoch, Jan 1, 1970, without leap seconds
            utc_timestamp, // Number of seconds since the epoch, Jan 1, 1970, with leap seconds

            // Text strings
            clob = -129, // A chunk of text (unknown encoding, can include random bytes > 0x7f)
            symbol, // A symbolic atom, or identifier (should be interpreted as text, but has unknown encoding, can include random bytes > 0x7f)
            datetime, // A datetime structure, with unspecified format (unknown text encoding)
            date, // A date structure, with unspecified format (unknown text encoding)
            time, // A time structure, with unspecified format (unknown text encoding)
            regexp, // A generic regexp structure, with unspecified text format, encoding, and option flags
            bignum, // A high-precision, decimal-encoded, number (unknown text encoding)
            uuid, // A generic UUID value (unknown text encoding)
            function, // A generic function value (unknown text encoding or language)
            javascript, // A section of executable JavaScript code (unknown text encoding, likely UTF-8)
            comment, // A comment, not to be used as an actual string (where supported by the output format)
            program_directive, // A program instruction or directive, not to be used as an actual string (where supported by the output format)

            // Binary strings
            blob = -199, // A chunk of binary data
            binary_symbol, // A symbolic atom, or identifier (should be interpreted as binary data)
            binary_datetime, // A datetime structure, with unspecified binary format
            binary_date, // A date structure, with unspecified binary format
            binary_time, // A time structure, with unspecified binary format
            binary_regexp, // A generic regexp structure, with unspecified binary format
            binary_pos_bignum, // A high-precision, binary-encoded, positive number (unknown binary encoding)
            binary_neg_bignum, // A high-precision, binary-encoded, negative number (unknown binary encoding)
            binary_uuid, // A generic binary UUID value
            binary_function, // A generic binary function value (unknown language or target)
            binary_object_id, // A 12-byte binary Object ID (used especially for BSON)

            // Arrays
            sexp = -209, // Ordered collection of values, distinct from an array only by name

            // Objects
            map = -219, // A normal object with integral keys
            hash, // A hash lookup (not supported as such in the value class, but can be used as a tag for external variant classes)

            // Links
            strong_link = -229, // A strong (owning) link to another value instance
            parent_link, // A weak (non-owning) link to the parent instance, usually used as an attribute value that points to the instance's parent.

            // Nulls
            undefined = -239, // An undefined value that can be used as normal null if necessary

            // Other reserved values (-2,147,483,392 options)
            reserved = INT32_MIN,
            reserved_max = -256,

            // User-defined values (2,147,483,648 options)
            user = 0,
            user_max = INT32_MAX
        };

        class value_builder;
        class stream_handler;

        class array_t;
        class object_t;
        class array_iterator_t;
        class array_const_iterator_t;
        class object_iterator_t;
        class object_const_iterator_t;

#ifdef CPPDATALIB_SUBTYPE_T
        typedef CPPDATALIB_SUBTYPE_T subtype_t;
#else
        typedef int32_t subtype_t;
#endif

        struct null_t {};

        inline bool subtype_is_reserved(subtype_t subtype, subtype_t *which)
        {
            if (subtype <= reserved_max)
            {
                if (which)
                    *which = subtype - reserved;
                return true;
            }
            return false;
        }

        inline bool subtype_is_user_defined(subtype_t subtype, subtype_t *which)
        {
            if (subtype >= user)
            {
                if (which)
                    *which = subtype - user;
                return true;
            }
            return false;
        }

        inline bool subtype_is_text_string(subtype_t subtype)
        {
            return (subtype > -130 && subtype <= -60) || (subtype > -10 && subtype <= -1);
        }

        inline bool subtype_is_binary_string(subtype_t subtype)
        {
            return (subtype > -200 && subtype <= -130);
        }

        const char *subtype_to_string(subtype_t subtype)
        {
            switch (subtype)
            {
                case normal: return "normal";

                case domain_comparable: return "<domain-comparable>";
                case generic_subtype_comparable: return "<no subtype>";

                case unix_timestamp: return "UNIX timestamp";
                case utc_timestamp: return "UTC timestamp";

                case clob: return "text (unknown encoding)";
                case symbol: return "symbol";
                case datetime: return "date/time";
                case date: return "date";
                case time: return "time";
                case regexp: return "regular expression";
                case bignum: return "bignum";
                case uuid: return "UUID";
                case function: return "function";
                case javascript: return "JavaScript";
                case comment: return "comment";
                case program_directive: return "program directive";

                case blob: return "binary (unknown data)";
                case binary_symbol: return "binary symbol";
                case binary_datetime: return "binary date/time";
                case binary_date: return "binary date";
                case binary_time: return "binary time";
                case binary_regexp: return "binary regular expression";
                case binary_pos_bignum: return "binary positive bignum";
                case binary_neg_bignum: return "binary negative bignum";
                case binary_uuid: return "binary UUID";
                case binary_function: return "binary function";
                case binary_object_id: return "binary ObjectID";

                case sexp: return "S-expression";

                case map: return "map";
                case hash: return "hash";

                case strong_link: return "strong link";
                case parent_link: return "parent link";

                case undefined: return "undefined";

                default:
                    if (subtype <= reserved_max)
                        return "reserved";
                    else if (subtype >= user)
                        return "user";
                    else
                        return "undefined subtype";
            }
        }

        /* The core value class for all of cppdatalib.
         * Provides a sort of tagged union for data storage, includes a type specifier and a subtype specifier.
         * If CPPDATALIB_ENABLE_ATTRIBUTES is defined, it also includes a pointer to an attributes object.
         *
         * This class does not have a call-stack-limited recursion depth for arrays and objects.
         * The recursion depth is only limited to the available memory and addressing capability of the target machine.
         *
         * However, there is no protection against call-stack overflow with a large number of nested links. Links are not automatically
         * followed, but with a series of strong links (strong link -> strong link -> strong link -> etc.) there is
         * *definitely* a risk of buffer overflow when the destructor is called. Watch your strong link recursion depth!
         *
         * Also, there is no protection against attributes having attributes having attributes, etc.
         * If you keep nesting excessively in this manner, stack-overflow is bound to occur.
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
            static value &assign(value &dst, value &&src);

        public:
            struct traversal_ancestry_finder
            {
                typedef std::stack<traversal_reference, core::cache_vector_n<traversal_reference, core::cache_size>> container;

                const container &c;

            public:
                traversal_ancestry_finder(const container &c) : c(c) {}

                size_t get_parent_count() const;

                // First element is direct parent, last element is ancestry root
                std::vector<traversal_reference> get_ancestry() const;
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

            value() : type_(null), subtype_(core::normal)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                , attr_(nullptr)
#endif
            {}

            value(null_t, subtype_t subtype = core::normal) : type_(null), subtype_(subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                , attr_(nullptr)
#endif
            {}

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(value *v, subtype_t subtype = core::normal)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {link_init(subtype, v);}

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(bool_t v, subtype_t subtype = core::normal)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {bool_init(subtype, v);}

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(int_t v, subtype_t subtype = core::normal)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {int_init(subtype, v);}

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(uint_t v, subtype_t subtype = core::normal)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {uint_init(subtype, v);}

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(real_t v, subtype_t subtype = core::normal)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {real_init(subtype, v);}

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(cstring_t v, subtype_t subtype = core::normal, bool make_temporary = false)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {
                (void) make_temporary;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                if (make_temporary)
                {
                    if (*v)
                        temp_string_init(subtype, v);
                    else
                        init(temporary_string, subtype);

                    return;
                }
#endif
                if (*v)
                    string_init(subtype, v);
                else
                    init(string, subtype);
            }

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(cstring_t v, size_t size, subtype_t subtype = core::normal, bool make_temporary = false)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {
                (void) make_temporary;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                if (make_temporary)
                {
                    if (size)
                        temp_string_init(subtype, v, size);
                    else
                        init(temporary_string, subtype);

                    return;
                }
#endif
                if (size)
                    string_init(subtype, v, size);
                else
                    init(string, subtype);
            }

#ifndef CPPDATALIB_DISABLE_TEMP_STRING
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(string_view_t v, subtype_t subtype = core::normal)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {temp_string_init(subtype, v);}
#endif // CPPDATALIB_DISABLE_TEMP_STRING

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const string_t &v, subtype_t subtype = core::normal, bool make_temporary = false)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {
                (void) make_temporary;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                if (make_temporary)
                {
                    if (!v.empty())
                        temp_string_init(subtype, v.data(), v.size());
                    else
                        init(temporary_string, subtype);

                    return;
                }
#endif
                if (!v.empty())
                    string_init(subtype, v);
                else
                    init(string, subtype);
            }

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(string_t &&v, subtype_t subtype = core::normal)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {
                if (!v.empty())
                    string_init(subtype, std::move(v));
                else
                    init(string, subtype);
            }

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const array_t &v, subtype_t subtype = core::normal);

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(array_t &&v, subtype_t subtype = core::normal);

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const object_t &v, subtype_t subtype = core::normal);

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(object_t &&v, subtype_t subtype = core::normal);

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const object_t &v, const object_t &attributes, subtype_t subtype = core::normal);

#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(object_t &&v, object_t &&attributes, subtype_t subtype = core::normal);
#endif

            template<typename T, typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(T v, subtype_t subtype = core::normal)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {uint_init(subtype, v);}

            template<typename T, typename std::enable_if<std::is_signed<T>::value, int>::type = 0, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(T v, subtype_t subtype = core::normal)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {int_init(subtype, v);}

            template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(T v, subtype_t subtype = core::normal)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                : attr_(nullptr)
#endif
            {real_init(subtype, v);}

            template<typename... Ts>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(std::initializer_list<Ts...> v, subtype_t subtype = core::normal);

            template<typename UserData, typename... Ts>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(std::initializer_list<Ts...> v, UserData userdata, subtype_t subtype = core::normal);

#ifndef CPPDATALIB_DISABLE_WEAK_POINTER_CONVERSIONS
            // Template constructor for pointer
            template<typename T>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const T *v, subtype_t subtype = core::normal) : type_(null), subtype_(subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                , attr_(nullptr)
#endif
            {
                if (v)
                    assign(*this, cppdatalib::core::value(*v));
            }

            // Template constructor for pointer with supplied user data
            template<typename T, typename UserData>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const T *v, UserData userdata, subtype_t subtype = core::normal) : type_(null), subtype_(subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                , attr_(nullptr)
#endif
            {
                if (v)
                    assign(*this, cppdatalib::core::value(*v, userdata, subtype));
            }
#endif
            // Template constructor for simple array
            template<typename T, size_t N>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const T (&v)[N], subtype_t subtype = core::normal);

            // Template constructor for simple array with supplied user data
            template<typename T, size_t N, typename UserData>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const T (&v)[N], UserData userdata, subtype_t subtype);

            // Template constructor for simple type
            template<typename T, typename std::enable_if<std::is_class<T>::value, int>::type = 0>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const T &v, subtype_t subtype = core::normal) : type_(null), subtype_(subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                , attr_(nullptr)
#endif
            {
                assign(*this, cast_to_cppdatalib<T>(v));
            }

            // Template constructor for simple type with supplied user data
            template<typename T, typename UserData, typename std::enable_if<std::is_class<T>::value, int>::type = 0>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const T &v, UserData userdata, subtype_t subtype) : type_(null), subtype_(subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                , attr_(nullptr)
#endif
            {
                assign(*this, cast_to_cppdatalib<T>(v, userdata));
            }

            // Template constructor for template type
            template<template<typename...> class Template, typename... Ts>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const Template<Ts...> &v, subtype_t subtype = core::normal) : type_(null), subtype_(subtype)
            {
                assign(*this, cast_template_to_cppdatalib<Template, Ts...>(v));
            }

            // Template constructor for template type with supplied user data
            template<template<typename...> class Template, typename UserData, typename... Ts>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const Template<Ts...> &v, UserData userdata, subtype_t subtype) : type_(null), subtype_(subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                , attr_(nullptr)
#endif
            {
                assign(*this, cast_template_to_cppdatalib<Template, Ts...>(v, userdata));
            }

            // Template constructor for sized array template type
            template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const Template<T, N, Ts...> &v, subtype_t subtype = core::normal) : type_(null), subtype_(subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                , attr_(nullptr)
#endif
            {
                assign(*this, cast_array_template_to_cppdatalib<Template, T, N, Ts...>(v));
            }

            // Template constructor for sized array template type with supplied user data
            template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename UserData, typename... Ts>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const Template<T, N, Ts...> &v, UserData userdata, subtype_t subtype) : type_(null), subtype_(subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                , attr_(nullptr)
#endif
            {
                assign(*this, cast_array_template_to_cppdatalib<Template, T, N, Ts...>(v, userdata));
            }

            // Template constructor for sized type
            template<template<size_t, typename...> class Template, size_t N, typename... Ts>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const Template<N, Ts...> &v, subtype_t subtype = core::normal) : type_(null), subtype_(subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                , attr_(nullptr)
#endif
            {
                assign(*this, cast_sized_template_to_cppdatalib<Template, N, Ts...>(v));
            }

            // Template constructor for sized type with supplied user data
            template<template<size_t, typename...> class Template, size_t N, typename UserData, typename... Ts>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            value(const Template<N, Ts...> &v, UserData userdata, subtype_t subtype) : type_(null), subtype_(subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                , attr_(nullptr)
#endif
            {
                assign(*this, cast_sized_template_to_cppdatalib<Template, N, Ts...>(v, userdata));
            }

            ~value();

            value(const value &other) : type_(null), subtype_(core::normal)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                , attr_(nullptr)
#endif
            {assign(*this, other);}
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
                        case null:
                        case boolean: swap(bool_, other.bool_); break;
                        case integer: swap(int_, other.int_); break;
                        case uinteger: swap(uint_, other.uint_); break;
                        case real: swap(real_, other.real_); break;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                        case temporary_string: swap(tstr_, other.tstr_); break;
#endif
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                        case string: swap(str_, other.str_); break;
#else
                        case string:
#endif
                        case link:
                        case array:
                        case object: swap(ptr_, other.ptr_); break;
                    }

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    swap(attr_, other.attr_);
#endif
                }
                else
                {
                    value temp(std::move(*this));
                    assign(*this, std::move(other));
                    assign(other, std::move(temp));
                }
            }

            subtype_t get_subtype() const {return subtype_;}
            // WARNING: it is highly discouraged to modify the subtype of a link value!
            // Do so at your own risk.
            subtype_t &get_subtype_ref() {return subtype_;}
            void set_subtype(subtype_t _type) {subtype_ = _type;}

            type get_type() const {return type_;}
            size_t size() const;
            size_t array_size() const;
            size_t object_size() const;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            size_t string_size() const {return is_temp_string()? tstr_.size(): is_nonnull_owned_string()? str_ref_().size(): 0;}
#else
            size_t string_size() const {return is_nonnull_owned_string()? str_ref_().size(): 0;}
#endif

            bool_t is_null() const {return type_ == null;}
            bool_t is_bool() const {return type_ == boolean;}
            bool_t is_link() const {return type_ == link;}
            bool_t is_int() const {return type_ == integer;}
            bool_t is_uint() const {return type_ == uinteger;}
            bool_t is_real() const {return type_ == real;}
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            bool_t is_temp_string() const {return type_ == temporary_string;}
            bool_t is_string() const {return is_temp_string() || is_owned_string();}
#else
            bool_t is_string() const {return is_owned_string();}
#endif
            bool_t is_owned_string() const {return type_ == string;}
            bool_t is_array() const {return type_ == array;}
            bool_t is_object() const {return type_ == object;}

            bool_t is_nonnull_owned_string() const
            {
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                return type_ == string && !str_ref_().empty();
#else
                return type_ == string && ptr_ != nullptr && !str_ref_().empty();
#endif
            }
            bool_t is_nonnull_array() const {return type_ == array && ptr_ != nullptr;}
            bool_t is_nonnull_object() const {return type_ == object && ptr_ != nullptr;}
            bool_t is_nonnull_link() const {return type_ == link && ptr_ != nullptr;}

            // The following group of functions exhibit UNDEFINED BEHAVIOR if the value is not the requested type
            bool_t get_bool_unchecked() const {return bool_;}
            value *get_link_unchecked() const {return reinterpret_cast<value *>(ptr_);}
            int_t get_int_unchecked() const {return int_;}
            uint_t get_uint_unchecked() const {return uint_;}
            real_t get_real_unchecked() const {return real_;}
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            string_view_t get_temp_string_unchecked() const {return tstr_;}
            string_view_t get_string_unchecked() const
            {
                return is_temp_string()? get_temp_string_unchecked():
                                         string_view_t(get_owned_string_unchecked().data(), get_owned_string_unchecked().size());
            }
#else
            const string_t &get_string_unchecked() const {return get_owned_string_unchecked();}
#endif
            const string_t &get_owned_string_unchecked() const {return str_ref_();}
            const array_t &get_array_unchecked() const {return arr_ref_();}
            const object_t &get_object_unchecked() const {return obj_ref_();}

            bool_t &get_bool_ref() {clear(boolean); return bool_;}
            int_t &get_int_ref() {clear(integer); return int_;}
            uint_t &get_uint_ref() {clear(uinteger); return uint_;}
            real_t &get_real_ref() {clear(real); return real_;}
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            string_view_t &get_temp_string_ref() {clear(temporary_string); return tstr_;}
#endif
            string_t &get_owned_string_ref() {clear(string); return str_ref_();}
            array_t &get_array_ref() {clear(array); return arr_ref_();}
            object_t &get_object_ref() {clear(object); return obj_ref_();}

            void set_null() {clear(null);}

            // WARNING: weaken_link() and strengthen_link() should only be used by those who know what they're doing
            // Misusing these functions may result in double-frees or memory leaks
            // Only use weaken_links() if you are going to take ownership of the current link (pointer)
            void weaken_link()
            {
                if (is_strong_link())
                {
                    set_subtype(normal);
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    if (is_nonnull_link())
                        deref().erase_attribute(parent_link_id());
#endif
                }
            }
            // WARNING: weaken_link() and strengthen_link() should only be used by those who know what they're doing
            // Misusing these functions may result in double-frees or memory leaks
            // Only use strengthen_link() if you immediately relinquish ownership of the link (pointer) and you know no-one else owns it.
            void strengthen_link()
            {
                if (is_link() && get_subtype() != strong_link)
                    set_strong_link(static_cast<value *>(ptr_));
            }

            // Transfers ownership of the link from the other value (`rhs`) to the current value
            value &transfer_link_from(value &rhs)
            {
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                if (rhs.is_link())
                    set_link(rhs.get_link_unchecked(), rhs.get_subtype());
                else
                    *this = rhs;
#else
                if (rhs.is_strong_link())
                {
                    set_strong_link(rhs.get_link_unchecked());
                    rhs.weaken_link();
                }
                else
                    *this = rhs;
#endif

                return *this;
            }

            // With attributes enabled (and if the special values are not modified):
            //    It is safe to set link_target when a strong link is currently held
            //    It is safe to set link_target equal to the currently held link, whether weak or strong
            //    It is safe to set link_target to the value of `this`
            //    It is unsafe to set link_target as a sub-value of the currently held strong link, although it is safe to do so if the currently held link is weak.
            //    It is unsafe to set link_target as a super-value of the currently held strong link, although it is safe to do so if the currently held link is weak.
            // With attributes disabled:
            //    It is safe to set link_target when a strong link is currently held
            //    It is safe to set link_target equal to the currently held link, whether weak or strong
            //    It is safe to set link_target to the value of `this`
            //    It is unsafe to set link_target as a sub-value of the currently held strong link, although it is safe to do so if the currently held link is weak.
            //    It is unsafe to set link_target as a super-value of the currently held strong link, although it is safe to do so if the currently held link is weak.
            // When attempting to perform one of the above "unsafe" actions, you should weaken the existing link first. However, weakening a link on a "safe" case will result in a memory leak.
            void set_weak_link(value *link_target)
            {
                clear(link);
                // Protect against re-assignment
                if (link_target != ptr_)
                {
                    destroy_strong_link();
                    ptr_ = link_target;
                }
                subtype_ = normal;
            }
            // With attributes enabled (and if the special values are not modified):
            //    It is safe to set link_target when a strong link is currently held
            //    It is safe to set link_target equal to the currently held link, whether weak or strong
            //    It is safe to set link_target to the value of `this`, but the link will decay to a weak link
            //    It is safe to call this function even if another value is strongly linked to link_target
            //    It is safe to set link_target as a sub-value of the currently held link, whether weak or strong
            //    It is unsafe to set link_target as a super-value of the currently held strong link, although it is safe to do so if the currently held link is weak.
            // With attributes disabled:
            //    It is safe to set link_target when a strong link is currently held
            //    It is safe to set link_target equal to the currently held link, whether weak or strong
            //    It is safe to set link_target to the value of `this`, but the link will decay to a weak link
            //    It is unsafe to call this function if another value is strongly linked to link_target
            //    It is unsafe to set link_target as a sub-value of the currently held strong link, although it is safe to do so if the currently held link is weak.
            //    It is unsafe to set link_target as a super-value of the currently held strong link, although it is safe to do so if the currently held link is weak.
            //
            // When attempting to perform one of the above "unsafe" actions, you should weaken the existing link first. However, weakening a link on a "safe" case will result in a memory leak.
            //
            // This value takes ownership of link_target, unless it is equal to the value of `this` (self-assignment).
            void set_strong_link(value *link_target)
            {
                clear(link);
                // Protect against double-frees with sub-value assignment
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                if (link_target)
                {
                    value current_parent = link_target->const_attribute(parent_link_id());
                    if (current_parent.is_nonnull_link())
                        current_parent.get_link_unchecked()->weaken_link();
                }
#endif
                // Protect against re-assignment
                if (link_target != ptr_)
                {
                    destroy_strong_link();
                    ptr_ = link_target;
                }

                // Protect against self-assignment
                subtype_ = link_target == this? normal: strong_link;

                // Reinitialize parent of linked-to value
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                if (link_target)
                    link_target->attribute(parent_link_id()) = core::value(this, core::parent_link);
#endif
            }
            void set_bool(bool_t v) {clear(boolean); bool_ = v;}
            void set_int(int_t v) {clear(integer); int_ = v;}
            void set_uint(uint_t v) {clear(uinteger); uint_ = v;}
            void set_real(real_t v) {clear(real); real_ = v;}
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            void set_temp_string(string_view_t v) {clear(temporary_string); tstr_ = v;}
            void set_temp_string(const string_t &v) {clear(temporary_string); tstr_ = string_view_t(v.data(), v.size());}
#endif
            void set_string(cstring_t v) {clear(string); str_ref_() = v;}
            void set_string(const string_t &v) {clear(string); str_ref_() = v;}
            void set_array(const array_t &v);
            void set_object(const object_t &v);

            void set_null(subtype_t subtype) {clear(null); subtype_ = subtype;}
            void set_link(value *link_target, subtype_t subtype)
            {
                if (subtype == strong_link)
                    set_strong_link(link_target);
                else
                {
                    set_weak_link(link_target);
                    set_subtype(subtype);
                }
            }
            void set_bool(bool_t v, subtype_t subtype) {clear(boolean); bool_ = v; subtype_ = subtype;}
            void set_int(int_t v, subtype_t subtype) {clear(integer); int_ = v; subtype_ = subtype;}
            void set_uint(uint_t v, subtype_t subtype) {clear(uinteger); uint_ = v; subtype_ = subtype;}
            void set_real(real_t v, subtype_t subtype) {clear(real); real_ = v; subtype_ = subtype;}
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            void set_temp_string(string_view_t v, subtype_t subtype) {clear(temporary_string); tstr_ = v; subtype_ = subtype;}
            void set_temp_string(const string_t &v, subtype_t subtype) {clear(temporary_string); tstr_ = string_view_t(v.data(), v.size()); subtype_ = subtype;}
#endif
            void set_string(cstring_t v, subtype_t subtype) {clear(string); str_ref_() = v; subtype_ = subtype;}
            void set_string(const string_t &v, subtype_t subtype) {clear(string); str_ref_() = v; subtype_ = subtype;}
            void set_array(const array_t &v, subtype_t subtype);
            void set_object(const object_t &v, subtype_t subtype);

            value operator[](cstring_t key) const;
            value &operator[](cstring_t key);
            value operator[](string_view_t key) const;
            value &operator[](string_view_t key);
            value const_member(cstring_t key) const;
            value const_member(string_view_t key) const;
            value const_member(const value &key) const;
            value member(cstring_t key) const;
            value member(string_view_t key) const;
            value member(const value &key) const;
            value &member(cstring_t key);
            value &member(string_view_t key);
            value &member(const value &key);
            const value *member_ptr(cstring_t key) const;
            const value *member_ptr(string_view_t key) const;
            const value *member_ptr(const value &key) const;
            bool_t is_member(cstring_t key) const;
            bool_t is_member(string_view_t key) const;
            bool_t is_member(const value &key) const;
            size_t member_count(cstring_t key) const;
            size_t member_count(string_view_t key) const;
            size_t member_count(const value &key) const;
            void erase_member(cstring_t key);
            void erase_member(string_view_t key);
            void erase_member(const value &key);

            value &add_member(const value &key);
            value &add_member(value &&key);
            value &add_member(const value &key, const value &val);
            value &add_member(value &&key, value &&val);
            value &add_member(value &&key, const value &val);
            value &add_member(const value &key, value &&val);

            value &add_member_at_end(const value &key);
            value &add_member_at_end(value &&key);
            value &add_member_at_end(const value &key, const value &val);
            value &add_member_at_end(value &&key, value &&val);
            value &add_member_at_end(value &&key, const value &val);
            value &add_member_at_end(const value &key, value &&val);

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            // WARNING: if you are using links, it is recommended to NOT TOUCH certain attribute values,
            // but to leave them unchanged:
            //
            // The `core::value()` (default null) attribute
            // The `core::value(static_cast<core::value *>(nullptr), core::parent_link)` attribute
            // The `core::value(static_cast<core::value *>(nullptr), core::normal)` attribute
            //
            // Thus, it is recommended to avoid `set_attributes()`, `erase_attributes()`, and modifying those specific values
            // via any other function.
            //
            // All other attribute values are available to the end user.

            const object_t &get_attributes() const {return attr_ref_();}
            void set_attributes(const object_t &attributes);
            void set_attributes(object_t &&attributes);

            size_t attributes_size() const;

            value const_attribute(cstring_t key) const;
            value const_attribute(string_view_t key) const;
            value const_attribute(const value &key) const;
            value attribute(cstring_t key) const;
            value attribute(string_view_t key) const;
            value attribute(const value &key) const;
            value &attribute(cstring_t key);
            value &attribute(string_view_t key);
            value &attribute(const value &key);
            const value *attribute_ptr(const value &key) const;
            bool_t is_attribute(cstring_t key) const;
            bool_t is_attribute(string_view_t key) const;
            bool_t is_attribute(const value &key) const;
            size_t attribute_count(cstring_t key) const;
            size_t attribute_count(string_view_t key) const;
            size_t attribute_count(const value &key) const;
            void erase_attribute(cstring_t key);
            void erase_attribute(string_view_t key);
            void erase_attribute(const value &key);
            void erase_attributes();

            value &add_attribute(const value &key);
            value &add_attribute(value &&key);
            value &add_attribute(const value &key, const value &val);
            value &add_attribute(value &&key, value &&val);
            value &add_attribute(value &&key, const value &val);
            value &add_attribute(const value &key, value &&val);

            value &add_attribute_at_end(const value &key);
            value &add_attribute_at_end(value &&key);
            value &add_attribute_at_end(const value &key, const value &val);
            value &add_attribute_at_end(value &&key, value &&val);
            value &add_attribute_at_end(value &&key, const value &val);
            value &add_attribute_at_end(const value &key, value &&val);
#endif // CPPDATALIB_DISABLE_ATTRIBUTES

            void push_back(const value &v);
            void push_back(value &&v);
            template<typename It>
            void append(It begin, It end);
            value operator[](size_t pos) const;
            value &operator[](size_t pos);
            value const_element(size_t pos) const;
            value element(size_t pos) const;
            const value *element_ptr(size_t pos) const;
            value &element(size_t pos);
            void erase_element(size_t pos);

            const value *operator->() const
            {
                if (is_nonnull_link())
                    return reinterpret_cast<const value *>(ptr_);
                return this;
            }
            value *operator->()
            {
                if (is_nonnull_link())
                    return reinterpret_cast<value *>(ptr_);
                return this;
            }
            const value &operator*() const {return deref();}
            const value &deref() const
            {
                if (is_nonnull_link())
                    return *reinterpret_cast<const value *>(ptr_);
                return *this;
            }
            value &operator*() {return deref();}
            value &deref()
            {
                if (is_nonnull_link())
                    return *reinterpret_cast<value *>(ptr_);
                return *this;
            }
            bool_t is_strong_link() const {return is_link() && get_subtype() == strong_link;}

            const value &deref_strong_links() const
            {
                const value *p = this;
                while (p->is_nonnull_link() && p->is_strong_link())
                    p = &p->deref();
                return *p;
            }
            value &deref_strong_links()
            {
                value *p = this;
                while (p->is_nonnull_link() && p->is_strong_link())
                    p = &p->deref();
                return *p;
            }

            const value &deref_all_links() const
            {
                const value *p = this;
                while (p->is_nonnull_link())
                    p = &p->deref();
                return *p;
            }
            value &deref_all_links()
            {
                value *p = this;
                while (p->is_nonnull_link())
                    p = &p->deref();
                return *p;
            }

            bool_t link_cycle_exists() const
            {
                std::set<const value *> found;
                const value *p = this;
                found.insert(p);
                while (p->is_nonnull_link())
                {
                    p = &p->deref();
                    if (!found.insert(p).second)
                        return true;
                }
                return false;
            }

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            bool_t link_name_is_global() const
            {
                if (is_nonnull_link())
                    return (*this)->is_attribute(core::value());
                return false;
            }
            core::value get_link_name() const
            {
                if (is_nonnull_link())
                {
                    const core::value *ptr = (*this)->attribute_ptr(core::value());
                    if (ptr)
                        return *ptr;
                }

                const core::value *ptr = attribute_ptr(core::value());
                if (ptr)
                    return *ptr;

                return core::value();
            }

            // Returns link to owner
            core::value get_owning_link() const
            {
                return const_attribute(core::value(static_cast<value *>(nullptr), core::parent_link));
            }

            // Note that the local link name is not guaranteed to be used.
            void set_local_link_name(const core::value &link_name)
            {
                attribute(core::value()) = link_name;
            }

            // Note that the global link name will be used before the local link name.
            void set_global_link_name(const core::value &link_name)
            {
                if (is_nonnull_link())
                    (*this)->attribute(core::value()) = link_name;
            }

            // Removes the global link name
            void remove_global_link_name()
            {
                if (is_nonnull_link())
                    (*this)->erase_attribute(core::value());
            }
#endif

            // The following are convenience conversion functions
#ifdef CPPDATALIB_THROW_IF_WRONG_TYPE
            bool_t get_bool(bool_t = false) const
            {
                if (is_bool())
                    return bool_;
                throw core::error("cppdatalib::core::value - get_bool() called on non-boolean value");
            }
            value *get_link(value * = nullptr) const
            {
                if (is_link())
                    return reinterpret_cast<value *>(ptr_);
                throw core::error("cppdatalib::core::value - get_link() called on non-link value");
            }
            int_t get_int(int_t = 0) const
            {
                if (is_int())
                    return int_;
                throw core::error("cppdatalib::core::value - get_int() called on non-integer value");
            }
            uint_t get_uint(uint_t = 0) const
            {
                if (is_uint())
                    return uint_;
                throw core::error("cppdatalib::core::value - get_uint() called on non-uinteger value");
            }
            real_t get_real(real_t = 0.0) const
            {
                if (is_real())
                    return real_;
                throw core::error("cppdatalib::core::value - get_real() called on non-real value");
            }
            cstring_t get_cstring(cstring_t = "") const
            {
                if (is_owned_string())
                    return str_ref_().c_str();
                throw core::error("cppdatalib::core::value - get_cstring() called on non-string value");
            }
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            string_view_t get_temp_string(string_view_t = string_view_t()) const
            {
                if (is_temp_string())
                    return tstr_;
                throw core::error("cppdatalib::core::value - get_temp_string() called on non-temporary-string value");
            }
            string_view_t get_string(string_view_t = string_view_t()) const
            {
                if (is_temp_string())
                    return tstr_;
                else if (is_owned_string())
                    return string_view_t(str_ref_().data(), str_ref_().size());
                throw core::error("cppdatalib::core::value - get_string() called on non-string value");
            }
#else
            string_t get_string(const string_t & = string_t()) const {return get_owned_string();}
#endif
            string_t get_owned_string(const string_t & = string_t()) const
            {
                if (is_owned_string())
                    return is_nonnull_owned_string()? str_ref_(): string_t();
                throw core::error("cppdatalib::core::value - get_owned_string() called on non-string value");
            }
            array_t get_array(const array_t &default_) const;
            object_t get_object(const object_t &default_) const;
            array_t get_array() const;
            object_t get_object() const;
#else
            bool_t get_bool(bool_t default_ = false) const {return is_bool()? bool_: default_;}
            value *get_link(value *default_ = nullptr) const {return is_link()? reinterpret_cast<value *>(ptr_): default_;}
            int_t get_int(int_t default_ = 0) const {return is_int()? int_: default_;}
            uint_t get_uint(uint_t default_ = 0) const {return is_uint()? uint_: default_;}
            real_t get_real(real_t default_ = 0.0) const {return is_real()? real_: default_;}
            cstring_t get_cstring(cstring_t default_ = "") const {return is_string()? str_ref_().c_str(): default_;}
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            string_view_t get_temp_string(string_view_t default_ = string_view_t()) const {return is_temp_string()? tstr_: default_;}
            string_view_t get_string(string_view_t default_ = string_view_t()) const {return is_string()? string_view_t(str_ref_().data(), str_ref_().size()):
                                                                                                               is_temp_string()? get_temp_string(default_): default_;}
#else
            string_t get_string(const string_t &default_ = string_t()) const {return get_owned_string(default_);}
#endif
            string_t get_owned_string(const string_t &default_ = string_t()) const {return is_owned_string()? str_ref_(): default_;}
            array_t get_array(const array_t &default_) const;
            object_t get_object(const object_t &default_) const;
            array_t get_array() const;
            object_t get_object() const;
#endif

#ifdef CPPDATALIB_DISABLE_IMPLICIT_DATA_CONVERSIONS
            bool_t as_bool(bool_t default_ = false) const {return get_bool(default_);}
            value *as_link(value *default_ = nullptr) const {return get_link(default_);}
            int_t as_int(int_t default_ = 0) const {return get_int(default_);}
            uint_t as_uint(uint_t default_ = 0) const {return get_uint(default_);}
            real_t as_real(real_t default_ = 0.0) const {return get_real(default_);}
            string_t as_string(const string_t &default_ = string_t()) const {return static_cast<string_t>(get_string(default_));}
            array_t as_array(const array_t &default_) const;
            object_t as_object(const object_t &default_) const;
            array_t as_array() const;
            object_t as_object() const;
#else
            bool_t as_bool(bool_t default_ = false) const {return value(*this).convert_to(boolean, core::value(default_)).bool_;}
            value *as_link(value *default_ = nullptr) const {return reinterpret_cast<value *>(value(*this).convert_to(link, core::value(default_)).ptr_);}
            int_t as_int(int_t default_ = 0) const {return value(*this).convert_to(integer, core::value(default_)).int_;}
            uint_t as_uint(uint_t default_ = 0) const {return value(*this).convert_to(uinteger, core::value(default_)).uint_;}
            real_t as_real(real_t default_ = 0.0) const {return value(*this).convert_to(real, core::value(default_)).real_;}
            string_t as_string(const string_t &default_ = string_t()) const {return value(*this).convert_to(string, core::value(default_)).str_ref_();}
            array_t as_array(const array_t &default_) const;
            object_t as_object(const object_t &default_) const;
            array_t as_array() const;
            object_t as_object() const;

            bool_t &convert_to_bool(bool_t default_ = false) {return convert_to(boolean, core::value(default_)).bool_;}
            int_t &convert_to_int(int_t default_ = 0) {return convert_to(integer, core::value(default_)).int_;}
            uint_t &convert_to_uint(uint_t default_ = 0) {return convert_to(uinteger, core::value(default_)).uint_;}
            real_t &convert_to_real(real_t default_ = 0.0) {return convert_to(real, core::value(default_)).real_;}
            string_t &convert_to_string(const string_t &default_ = string_t()) {return convert_to(string, core::value(default_)).str_ref_();}
            array_t &convert_to_array(const array_t &default_) {return convert_to(array, core::value(default_)).arr_ref_();}
            object_t &convert_to_object(const object_t &default_) {return convert_to(object, core::value(default_)).obj_ref_();}
            array_t &convert_to_array();
            object_t &convert_to_object();
#endif

			// Use of `this->` silences MSVC
            template<typename T>
            typename std::remove_cv<typename std::remove_reference<T>::type>::type cast() const {return this->operator typename std::remove_cv<typename std::remove_reference<T>::type>::type();}

            template<typename T, typename UserData>
            typename std::remove_cv<typename std::remove_reference<T>::type>::type cast(UserData userdata) const;

            template<typename T>
            typename std::remove_cv<typename std::remove_reference<T>::type>::type as() const {return cast<T>();}

            template<typename T, typename UserData>
            typename std::remove_cv<typename std::remove_reference<T>::type>::type as(UserData userdata) const {return cast<T>(userdata);}

            template<typename T>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            operator T() const {return cast_from_cppdatalib<typename std::remove_cv<typename std::remove_reference<T>::type>::type>(*this);}

#ifndef CPPDATALIB_DISABLE_WEAK_POINTER_CONVERSIONS
            template<typename T>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            operator T*() const {return new T(operator T());}
#endif

            template<template<typename...> class Template, typename... Ts>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            operator Template<Ts...>() const {return cast_template_from_cppdatalib<Template, Ts...>(*this);}

            template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
            operator Template<T, N, Ts...>() const {return cast_array_template_from_cppdatalib<Template, T, N, Ts...>(*this);}

            template<template<size_t, typename...> class Template, size_t N, typename... Ts>
#ifdef CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
            explicit
#endif
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

            static core::value parent_link_id() {return core::value(static_cast<value *>(nullptr), core::parent_link);}

            void destroy_strong_link()
            {
                if (is_strong_link())
                    delete reinterpret_cast<value *>(ptr_);
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
            void link_init(subtype_t new_subtype, Args... args)
            {
                new (&ptr_) value*(args...);
                type_ = link;
                set_link(args..., new_subtype);
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

#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            template<typename... Args>
            void temp_string_init(subtype_t new_subtype, Args... args)
            {
                new (&tstr_) string_view_t(args...);
                type_ = temporary_string;
                subtype_ = new_subtype;
            }
#endif

            template<typename... Args>
            void array_init(subtype_t new_subtype, Args... args);

            void create_array();

            template<typename... Args>
            void object_init(subtype_t new_subtype, Args... args);

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            template<typename... Args>
            void attr_init(Args... args);
#endif

            void create_object();

            void deinit();

#ifndef CPPDATALIB_DISABLE_IMPLICIT_DATA_CONVERSIONS
            // TODO: ensure that all conversions are generic enough (for example, string -> bool doesn't just need to be "true")
            value &convert_to(type new_type, const value &default_value)
            {
                using namespace std;

                if (type_ == new_type)
                    return *this;

                switch (type_)
                {
                    case null:
                    case array:
                    case object:
                    case link:
                        *this = default_value;
                        break;
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
#ifndef CPPDATALIB_DISABLE_CONVERSION_LOSS
                            case boolean: set_bool(int_ != 0); break;
                            case uinteger: set_uint(int_ > 0? int_: 0); break;
                            case real: set_real(static_cast<real_t>(int_)); break;
#else
                            case boolean:
                                if (int_ == 0 || int_ == 1)
                                    set_bool(int_ != 0);
                                else
                                    throw core::error("cppdatalib::core::value - attempt to convert integer to boolean results in data loss");
                                break;
                            case uinteger:
                                if (int_ >= 0)
                                    set_uint(int_);
                                else
                                    throw core::error("cppdatalib::core::value - attempt to convert integer to uinteger results in data loss");
                                break;
                            case real:
                                if (static_cast<int_>(static_cast<real_t>(int_)) == int_)
                                    set_real(static_cast<real_t>(int_));
                                else
                                    throw core::error("cppdatalib::core::value - attempt to convert integer to real results in data loss");
                                break;
#endif
                            case string: set_string(to_string(int_)); break;
                            default: *this = default_value; break;
                        }
                        break;
                    }
                    case uinteger:
                    {
                        switch (new_type)
                        {
#ifndef CPPDATALIB_DISABLE_CONVERSION_LOSS
                            case boolean: set_bool(uint_ != 0); break;
                            case integer: set_int(uint_ <= INT64_MAX? uint_: 0); break;
                            case real: set_real(static_cast<real_t>(uint_)); break;
#else
                            case boolean:
                                if (uint_ == 0 || uint_ == 1)
                                    set_bool(uint_ != 0);
                                else
                                    throw core::error("cppdatalib::core::value - attempt to convert uinteger to boolean results in data loss");
                                break;
                            case integer:
                                if (uint_ <= INT64_MAX)
                                    set_int(uint_);
                                else
                                    throw core::error("cppdatalib::core::value - attempt to convert uinteger to integer results in data loss");
                            case real:
                                if (static_cast<uint_>(static_cast<real_t>(uint_)) == uint_)
                                    set_real(static_cast<real_t>(uint_));
                                else
                                    throw core::error("cppdatalib::core::value - attempt to convert uinteger to real results in data loss");
                                break;
#endif
                            case string: set_string(to_string(uint_)); break;
                            default: *this = default_value; break;
                        }
                        break;
                    }
                    case real:
                    {
                        switch (new_type)
                        {
#ifndef CPPDATALIB_DISABLE_CONVERSION_LOSS
                            case boolean: set_bool(real_ != 0.0); break;
                            case integer: set_int((real_ >= INT64_MIN && real_ <= INT64_MAX)? static_cast<int_t>(trunc(real_)): 0); break;
                            case uinteger: set_uint((real_ >= 0 && real_ <= UINT64_MAX)? static_cast<uint_t>(trunc(real_)): 0); break;
#else
                            case boolean:
                                if (real_ == 0 || real_ == 1)
                                    set_bool(real_ != 0);
                                else
                                    throw core::error("cppdatalib::core::value - attempt to convert real to boolean results in data loss");
                                break;
                            case integer:
                                if (real_ >= INT64_MIN && real_ <= INT64_MAX && trunc(real_) == real_)
                                    set_int(real_);
                                else
                                    throw core::error("cppdatalib::core::value - attempt to convert real to integer results in data loss");
                            case uinteger:
                                if (real_ >= 0 && real_ <= UINT64_MAX && trunc(real_) == real_)
                                    set_real(real_);
                                else
                                    throw core::error("cppdatalib::core::value - attempt to convert real to uinteger results in data loss");
                                break;
#endif
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
#ifndef CPPDATALIB_DISABLE_CONVERSION_LOSS
                                if (!str)
                                    int_ = 0;
#else
                                if (!str)
                                    throw core::error("cppdatalib::core::value - attempt to convert string to integer results in data loss");
#endif
                                break;
                            }
                            case uinteger:
                            {
                                std::istringstream str(str_ref_());
                                clear(uinteger);
                                str >> uint_;
#ifndef CPPDATALIB_DISABLE_CONVERSION_LOSS
                                if (!str)
                                    uint_ = 0;
#else
                                if (!str)
                                    throw core::error("cppdatalib::core::value - attempt to convert string to uinteger results in data loss");
#endif
                                break;
                            }
                            case real:
                            {
                                std::istringstream str(str_ref_());
                                clear(real);
                                str >> real_;
#ifndef CPPDATALIB_DISABLE_CONVERSION_LOSS
                                if (!str)
                                    real_ = 0.0;
#else
                                if (!str)
                                    throw core::error("cppdatalib::core::value - attempt to convert string to real results in data loss");
#endif
                                break;
                            }
                            default: *this = default_value; break;
                        }
                        break;
                    }
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                    case temporary_string:
                    {
                        switch (new_type)
                        {
                            case boolean: set_bool(tstr_ == "true"); break;
                            case integer:
                            {
                                std::istringstream str(static_cast<std::string>(tstr_));
                                clear(integer);
                                str >> int_;
#ifndef CPPDATALIB_DISABLE_CONVERSION_LOSS
                                if (!str)
                                    int_ = 0;
#else
                                if (!str)
                                    throw core::error("cppdatalib::core::value - attempt to convert string to integer results in data loss");
#endif
                                break;
                            }
                            case uinteger:
                            {
                                std::istringstream str(static_cast<std::string>(tstr_));
                                clear(uinteger);
                                str >> uint_;
#ifndef CPPDATALIB_DISABLE_CONVERSION_LOSS
                                if (!str)
                                    uint_ = 0;
#else
                                if (!str)
                                    throw core::error("cppdatalib::core::value - attempt to convert string to uinteger results in data loss");
#endif
                                break;
                            }
                            case real:
                            {
                                std::istringstream str(static_cast<std::string>(tstr_));
                                clear(real);
                                str >> real_;
#ifndef CPPDATALIB_DISABLE_CONVERSION_LOSS
                                if (!str)
                                    real_ = 0.0;
#else
                                if (!str)
                                    throw core::error("cppdatalib::core::value - attempt to convert string to real results in data loss");
#endif
                                break;
                            }
                            case string:
                            {
                                core::string_t str(tstr_.data(), tstr_.size());
                                clear(string);
                                if (str.size())
                                    str_ref_() = std::move(str);
                                break;
                            }
                            default: *this = default_value; break;
                        }
                        break;
                    }
#endif
                }

                return *this;
            }
#endif // CPPDATALIB_DISABLE_IMPLICIT_DATA_CONVERSIONS

            string_t &str_ref_();
            const string_t &str_ref_() const;

            array_t &arr_ref_();
            const array_t &arr_ref_() const;

            object_t &obj_ref_();
            const object_t &obj_ref_() const;

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            object_t &attr_ref_();
            const object_t &attr_ref_() const;
#endif

            union
            {
                bool_t bool_;
                int_t int_;
                uint_t uint_;
                real_t real_;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                string_view_t tstr_;
#endif
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                string_t str_;
#endif
                mutable void *ptr_; // Mutable to provide editable traversal access to const destructor
            };

            mutable type type_; // Mutable to provide editable traversal access to const destructor
            subtype_t subtype_;

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            mutable object_t *attr_;
#endif
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

        inline array_iterator_t array_t::begin() {return m_data.begin();}
        inline array_const_iterator_t array_t::begin() const {return m_data.begin();}
        inline array_iterator_t array_t::end() {return m_data.end();}
        inline array_const_iterator_t array_t::end() const {return m_data.end();}
        inline array_const_iterator_t array_t::cbegin() const {return m_data.cbegin();}
        inline array_const_iterator_t array_t::cend() const {return m_data.cend();}

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

        inline object_iterator_t object_t::begin() {return m_data.begin();}
        inline object_const_iterator_t object_t::begin() const {return m_data.begin();}
        inline object_iterator_t object_t::end() {return m_data.end();}
        inline object_const_iterator_t object_t::end() const {return m_data.end();}
        inline object_const_iterator_t object_t::cbegin() const {return m_data.cbegin();}
        inline object_const_iterator_t object_t::cend() const {return m_data.cend();}

        struct value::traversal_reference
        {
            traversal_reference()
                : p(nullptr)
                , traversed_key_already(false)
                , frozen(false)
            {}

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
            bool is_object_key() const {return is_object() && traversed_key_already;}
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

        inline size_t value::traversal_ancestry_finder::get_parent_count() const {return c.size();}

        // First element is direct parent, last element is ancestry root
        inline std::vector<value::traversal_reference> value::traversal_ancestry_finder::get_ancestry() const
        {
            std::vector<value::traversal_reference> result;
            container temp = c;

            while (!temp.empty())
            {
                result.push_back(temp.top());
                temp.pop();
            }

            return result;
        }

        template<typename... Ts>
        value::value(std::initializer_list<Ts...> v, subtype_t subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            : attr_(nullptr)
#endif
        {
            if (v.begin() != v.end())
            {
                array_init(subtype, core::array_t());
                for (auto const &element: v)
                    push_back(value(element));
            }
            else
                init(array, subtype);
        }

        template<typename UserData, typename... Ts>
        value::value(std::initializer_list<Ts...> v, UserData userdata, subtype_t subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            : attr_(nullptr)
#endif
        {
            if (v.begin() != v.end())
            {
                array_init(subtype, core::array_t());
                for (auto const &element: v)
                    push_back(value(element, userdata, core::normal));
            }
            else
                init(array, subtype);
        }

        template<typename T, size_t N>
        value::value(const T (&v)[N], subtype_t subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            : attr_(nullptr)
#endif
        {
            if (N)
            {
                array_init(subtype, core::array_t());
                for (size_t i = 0; i < N; ++i)
                    push_back(value(v[i]));
            }
            else
                init(array, subtype);
        }

        template<typename T, size_t N, typename UserData>
        value::value(const T (&v)[N], UserData userdata, subtype_t subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            : attr_(nullptr)
#endif
        {
            if (N)
            {
                array_init(subtype, core::array_t());
                for (size_t i = 0; i < N; ++i)
                    push_back(value(v[i], userdata, core::normal));
            }
            else
                init(array, subtype);
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

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
        inline void value::set_attributes(const object_t &attributes) {attr_ref_() = attributes;}
        inline void value::set_attributes(object_t &&attributes) {attr_ref_() = std::move(attributes);}

        inline size_t value::attributes_size() const {return attr_ != nullptr? attr_ref_().size(): 0;}

        template<typename... Args>
        void value::attr_init(Args... args)
        {
            attr_ = new object_t(args...);
        }
#endif

        // Predicates must be callables with argument type `const core::value *arg, core::value::traversal_ancestry_finder arg_finder` and return value bool
        // If return value is non-zero, processing continues, otherwise processing aborts immediately
        template<typename PrefixPredicate, typename PostfixPredicate>
        void value::traverse(PrefixPredicate &prefix, PostfixPredicate &postfix) const
        {
            std::stack<traversal_reference, core::cache_vector_n<traversal_reference, core::cache_size>> references;
            const value *p = this;

            while (true)
            {
                if (p != NULL)
                {
                    if (!prefix(p, traversal_ancestry_finder(references)))
                        return;

                    switch (p->get_type())
                    {
                        case array:
                            references.push(traversal_reference(p, p->get_array_unchecked().begin(), object_const_iterator_t(), false));
                            if (!p->get_array_unchecked().empty())
                                p = std::addressof(*references.top().array++);
                            else
                                p = NULL;
                            break;
                        case object:
                            references.push(traversal_reference(p, array_const_iterator_t(), p->get_object_unchecked().begin(), true));
                            if (!p->get_object_unchecked().empty())
                                p = std::addressof(references.top().object->first);
                            else
                                p = NULL;
                            break;
                        default:
                            references.push(traversal_reference(p, array_const_iterator_t(), object_const_iterator_t(), false));
                            p = NULL;
                            break;
                    }
                }
                else if (!references.empty())
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
                else
                    break;
            }
        }

        // Predicates must be callables with argument types `const core::value *arg, core::value::traversal_ancestry_finder arg_finder, bool prefix` and return value bool
        // `prefix` is set to true if the current invocation is for the prefix handling of the specified element
        // If return value is non-zero, processing continues, otherwise processing aborts immediately
        template<typename Predicate>
        void value::traverse(Predicate &predicate) const
        {
            std::stack<traversal_reference, core::cache_vector_n<traversal_reference, core::cache_size>> references;
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
            std::stack<traversal_reference, core::cache_vector_n<traversal_reference, core::cache_size>> references;
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
            std::stack<traversal_reference, core::cache_vector_n<traversal_reference, core::cache_size>> references;
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
            std::stack<traversal_reference, core::cache_vector_n<traversal_reference, core::cache_size>> references;
            std::stack<traversal_reference, core::cache_vector_n<traversal_reference, core::cache_size>> other_references;
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

        inline value::value(const array_t &v, subtype_t subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            : attr_(nullptr)
#endif
        {
            if (!v.empty())
                array_init(subtype, v);
            else
                init(array, subtype);
        }
        inline value::value(array_t &&v, subtype_t subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            : attr_(nullptr)
#endif
        {
            if (!v.empty())
                array_init(subtype, std::move(v));
            else
                init(array, subtype);
        }
        inline value::value(const object_t &v, subtype_t subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            : attr_(nullptr)
#endif
        {
            if (!v.empty())
                object_init(subtype, v);
            else
                init(object, subtype);
        }
        inline value::value(object_t &&v, subtype_t subtype)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            : attr_(nullptr)
#endif
        {
            if (!v.empty())
                object_init(subtype, std::move(v));
            else
                init(object, subtype);
        }

        inline value::value(value &&other) : type_(other.type_), subtype_(other.subtype_)
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            , attr_(nullptr)
#endif
        {
            switch (other.type_)
            {
                case null: break;
                case boolean: new (&bool_) bool_t(std::move(other.bool_)); break;
                case integer: new (&int_) int_t(std::move(other.int_)); break;
                case uinteger: new (&uint_) uint_t(std::move(other.uint_)); break;
                case real: new (&real_) real_t(std::move(other.real_)); break;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                case temporary_string: new (&tstr_) string_view_t(other.tstr_); break;
#endif
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                case string: new (&str_) string_t(std::move(other.str_)); break;
#else
                case string:
                    new (&ptr_) string_t*();
                    ptr_ = other.ptr_;
                    other.ptr_ = nullptr;
                    break;
#endif
                case array:
                    new (&ptr_) array_t*();
                    ptr_ = other.ptr_;
                    other.ptr_ = nullptr;
                    break;
                case object:
                    new (&ptr_) object_t*();
                    ptr_ = other.ptr_;
                    other.ptr_ = nullptr;
                    break;
                case link:
                    new (&ptr_) value*();
                    ptr_ = other.ptr_;
                    other.ptr_ = nullptr;
                    break;
            }
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            std::swap(attr_, other.attr_);
#endif
        }

        inline value::~value()
        {
            if ((is_nonnull_array() && arr_ref_().size() > 0) ||
                (is_nonnull_object() && obj_ref_().size() > 0))
                traverse(traverse_node_null, traverse_node_mutable_clear);

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            if (attr_)
                for (auto &item: *attr_)
                {
                    item.first.traverse(traverse_node_null, traverse_node_mutable_clear);
                    item.second.traverse(traverse_node_null, traverse_node_mutable_clear);
                }
#endif

            deinit();
        }

        inline string_t &value::str_ref_() {
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
            return str_;
#else
            if (ptr_ == nullptr)
            {
                new (&ptr_) string_t*();
                ptr_ = new string_t();
            }
            return *reinterpret_cast<string_t *>(ptr_);
#endif
        }
        inline const string_t &value::str_ref_() const {
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
            return str_;
#else
            if (ptr_ == nullptr)
                ptr_ = new string_t();
            return *reinterpret_cast<const string_t *>(ptr_);
#endif
        }

        inline array_t &value::arr_ref_() {
            if (ptr_ == nullptr)
                ptr_ = new array_t();
            return *reinterpret_cast<array_t *>(ptr_);
        }
        inline const array_t &value::arr_ref_() const {
            if (ptr_ == nullptr)
                ptr_ = new array_t();
            return *reinterpret_cast<const array_t *>(ptr_);
        }

        inline object_t &value::obj_ref_() {
            if (ptr_ == nullptr)
                ptr_ = new object_t();
            return *reinterpret_cast<object_t *>(ptr_);
        }
        inline const object_t &value::obj_ref_() const {
            if (ptr_ == nullptr)
                ptr_ = new object_t();
            return *reinterpret_cast<const object_t *>(ptr_);
        }

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
        inline object_t &value::attr_ref_()
        {
            if (attr_ == nullptr)
                attr_ = new object_t();
            return *attr_;
        }
        inline const object_t &value::attr_ref_() const
        {
            if (attr_ == nullptr)
                attr_ = new object_t();
            return *attr_;
        }
#endif

        inline value &value::operator=(value &&other) {return assign(*this, std::move(other));}

        inline size_t value::size() const
        {
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            if (is_temp_string())
                return tstr_.size();
#endif
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
            if (is_owned_string())
                return str_ref_().size();
#else
            if (is_owned_string())
                return ptr_ == nullptr? 0: str_ref_().size();
#endif

            if (is_array())
                return ptr_ == nullptr? 0: arr_ref_().size();
            else if (is_object())
                return ptr_ == nullptr? 0: obj_ref_().size();

            return 0;
        }
        inline size_t value::array_size() const {return is_nonnull_array()? arr_ref_().size(): 0;}
        inline size_t value::object_size() const {return is_nonnull_object()? obj_ref_().size(): 0;}

        inline void value::set_array(const array_t &v) {clear(array); arr_ref_() = v;}
        inline void value::set_object(const object_t &v) {clear(object); obj_ref_() = v;}
        inline void value::set_array(const array_t &v, subtype_t subtype) {clear(array); arr_ref_() = v; subtype_ = subtype;}
        inline void value::set_object(const object_t &v, subtype_t subtype) {clear(object); obj_ref_() = v; subtype_ = subtype;}

        inline value value::operator[](cstring_t key) const {return member(value(key, domain_comparable));}
        inline value &value::operator[](cstring_t key) {return member(value(key, domain_comparable));}
        inline value value::operator[](string_view_t key) const {return member(value(key, domain_comparable));}
        inline value &value::operator[](string_view_t key) {return member(value(key, domain_comparable));}
        inline value value::const_member(cstring_t key) const {return const_member(value(key, domain_comparable));}
        inline value value::const_member(string_view_t key) const {return const_member(value(key, domain_comparable));}
        inline value value::const_member(const value &key) const
        {
            if (is_nonnull_object())
            {
                auto it = obj_ref_().data().find(key);
                if (it != obj_ref_().end())
                    return it->second;
            }
            return value();
        }
        inline value value::member(cstring_t key) const {return const_member(key);}
        inline value value::member(string_view_t key) const {return const_member(key);}
        inline value value::member(const value &key) const {return const_member(key);}
        inline value &value::member(cstring_t key) {return member(value(key, domain_comparable));}
        inline value &value::member(string_view_t key) {return member(value(key, domain_comparable));}
        inline value &value::member(const value &key)
        {
            clear(object);
            auto it = obj_ref_().data().lower_bound(key);
            if (it != obj_ref_().end() && it->first == key)
                return it->second;
            it = obj_ref_().data().insert(it, {key, null_t()});
            return it->second;
        }
        inline const value *value::member_ptr(cstring_t key) const {return member_ptr(value(key, domain_comparable));}
        inline const value *value::member_ptr(string_view_t key) const {return member_ptr(value(key, domain_comparable));}
        inline const value *value::member_ptr(const value &key) const
        {
            if (is_nonnull_object())
            {
                auto it = obj_ref_().data().find(key);
                if (it != obj_ref_().end())
                    return std::addressof(it->second);
            }
            return NULL;
        }
        inline bool_t value::is_member(cstring_t key) const {return is_nonnull_object() && obj_ref_().data().find(value(key, domain_comparable)) != obj_ref_().end();}
        inline bool_t value::is_member(string_view_t key) const {return is_nonnull_object() && obj_ref_().data().find(value(key, domain_comparable)) != obj_ref_().end();}
        inline bool_t value::is_member(const value &key) const {return is_nonnull_object() && obj_ref_().data().find(key) != obj_ref_().end();}
        inline size_t value::member_count(cstring_t key) const {return is_nonnull_object()? obj_ref_().data().count(value(key, domain_comparable)): 0;}
        inline size_t value::member_count(string_view_t key) const {return is_nonnull_object()? obj_ref_().data().count(value(key, domain_comparable)): 0;}
        inline size_t value::member_count(const value &key) const {return is_nonnull_object()? obj_ref_().data().count(key): 0;}
        inline void value::erase_member(cstring_t key) {if (is_nonnull_object()) obj_ref_().data().erase(value(key, domain_comparable));}
        inline void value::erase_member(string_view_t key) {if (is_nonnull_object()) obj_ref_().data().erase(value(key, domain_comparable));}
        inline void value::erase_member(const value &key) {if (is_nonnull_object()) obj_ref_().data().erase(key);}

        inline value &value::add_member(const value &key)
        {
            clear(object);
            return obj_ref_().data().insert(std::make_pair(key, null_t()))->second;
        }
        inline value &value::add_member(value &&key)
        {
            clear(object);
            return obj_ref_().data().insert(std::make_pair(std::move(key), null_t()))->second;
        }
        inline value &value::add_member(const value &key, const value &val)
        {
            clear(object);
            return obj_ref_().data().insert(std::make_pair(key, val))->second;
        }
        inline value &value::add_member(value &&key, value &&val)
        {
            clear(object);
            return obj_ref_().data().insert(std::make_pair(std::move(key), std::move(val)))->second;
        }
        inline value &value::add_member(value &&key, const value &val)
        {
            clear(object);
            return obj_ref_().data().insert(std::make_pair(std::move(key), val))->second;
        }
        inline value &value::add_member(const value &key, value &&val)
        {
            clear(object);
            return obj_ref_().data().insert(std::make_pair(key, std::move(val)))->second;
        }

        inline value &value::add_member_at_end(const value &key)
        {
            clear(object);
            return obj_ref_().data().insert(obj_ref_().end().data(), std::make_pair(key, null_t()))->second;
        }
        inline value &value::add_member_at_end(value &&key)
        {
            clear(object);
            return obj_ref_().data().insert(obj_ref_().end().data(), std::make_pair(std::move(key), null_t()))->second;
        }
        inline value &value::add_member_at_end(const value &key, const value &val)
        {
            clear(object);
            return obj_ref_().data().insert(obj_ref_().end().data(), std::make_pair(key, val))->second;
        }
        inline value &value::add_member_at_end(value &&key, value &&val)
        {
            clear(object);
            return obj_ref_().data().insert(obj_ref_().end().data(), std::make_pair(std::move(key), std::move(val)))->second;
        }
        inline value &value::add_member_at_end(value &&key, const value &val)
        {
            clear(object);
            return obj_ref_().data().insert(obj_ref_().end().data(), std::make_pair(std::move(key), val))->second;
        }
        inline value &value::add_member_at_end(const value &key, value &&val)
        {
            clear(object);
            return obj_ref_().data().insert(obj_ref_().end().data(), std::make_pair(key, std::move(val)))->second;
        }

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
        inline value value::const_attribute(cstring_t key) const {return const_attribute(core::value(key));}
        inline value value::const_attribute(string_view_t key) const {return const_attribute(core::value(static_cast<string_t>(key)));}
        inline value value::const_attribute(const value &key) const
        {
            auto it = attr_ref_().data().find(key);
            if (it != attr_ref_().end())
                return it->second;
            return value();
        }
        inline value value::attribute(cstring_t key) const {return const_attribute(value(key));}
        inline value value::attribute(string_view_t key) const {return const_attribute(value(static_cast<string_t>(key)));}
        inline value value::attribute(const value &key) const {return const_attribute(key);}
        inline value &value::attribute(cstring_t key) {return attribute(value(key));}
        inline value &value::attribute(string_view_t key) {return attribute(value(static_cast<string_t>(key)));}
        inline value &value::attribute(const value &key)
        {
            auto it = attr_ref_().data().lower_bound(key);
            if (it != attr_ref_().end() && it->first == key)
                return it->second;
            it = attr_ref_().data().insert(it, {key, null_t()});
            return it->second;
        }
        inline const value *value::attribute_ptr(const value &key) const
        {
            auto it = attr_ref_().data().find(key);
            if (it != attr_ref_().end())
                return std::addressof(it->second);
            return NULL;
        }
        inline bool_t value::is_attribute(cstring_t key) const {return attr_ref_().data().find(value(key)) != attr_ref_().end();}
        inline bool_t value::is_attribute(string_view_t key) const {return attr_ref_().data().find(value(static_cast<string_t>(key))) != attr_ref_().end();}
        inline bool_t value::is_attribute(const value &key) const {return attr_ref_().data().find(key) != attr_ref_().end();}
        inline size_t value::attribute_count(cstring_t key) const {return attr_ref_().data().count(value(key));}
        inline size_t value::attribute_count(string_view_t key) const {return attr_ref_().data().count(value(static_cast<string_t>(key)));}
        inline size_t value::attribute_count(const value &key) const {return attr_ref_().data().count(key);}
        inline void value::erase_attribute(cstring_t key) {attr_ref_().data().erase(value(key));}
        inline void value::erase_attribute(string_view_t key) {attr_ref_().data().erase(value(static_cast<string_t>(key)));}
        inline void value::erase_attribute(const value &key) {attr_ref_().data().erase(key);}
        inline void value::erase_attributes() {attr_ref_().data().clear();}

        inline value &value::add_attribute(const value &key)
        {
            return attr_ref_().data().insert(std::make_pair(key, null_t()))->second;
        }
        inline value &value::add_attribute(value &&key)
        {
            return attr_ref_().data().insert(std::make_pair(std::move(key), null_t()))->second;
        }
        inline value &value::add_attribute(const value &key, const value &val)
        {
            return attr_ref_().data().insert(std::make_pair(key, val))->second;
        }
        inline value &value::add_attribute(value &&key, value &&val)
        {
            return attr_ref_().data().insert(std::make_pair(std::move(key), std::move(val)))->second;
        }
        inline value &value::add_attribute(value &&key, const value &val)
        {
            return attr_ref_().data().insert(std::make_pair(std::move(key), val))->second;
        }
        inline value &value::add_attribute(const value &key, value &&val)
        {
            return attr_ref_().data().insert(std::make_pair(key, std::move(val)))->second;
        }

        inline value &value::add_attribute_at_end(const value &key)
        {
            return attr_ref_().data().insert(attr_ref_().end().data(), std::make_pair(key, null_t()))->second;
        }
        inline value &value::add_attribute_at_end(value &&key)
        {
            return attr_ref_().data().insert(attr_ref_().end().data(), std::make_pair(std::move(key), null_t()))->second;
        }
        inline value &value::add_attribute_at_end(const value &key, const value &val)
        {
            return attr_ref_().data().insert(attr_ref_().end().data(), std::make_pair(key, val))->second;
        }
        inline value &value::add_attribute_at_end(value &&key, value &&val)
        {
            return attr_ref_().data().insert(attr_ref_().end().data(), std::make_pair(std::move(key), std::move(val)))->second;
        }
        inline value &value::add_attribute_at_end(value &&key, const value &val)
        {
            return attr_ref_().data().insert(attr_ref_().end().data(), std::make_pair(std::move(key), val))->second;
        }
        inline value &value::add_attribute_at_end(const value &key, value &&val)
        {
            return attr_ref_().data().insert(attr_ref_().end().data(), std::make_pair(key, std::move(val)))->second;
        }
#endif // CPPDATALIB_DISABLE_ATTRIBUTES

        inline void value::push_back(const value &v) {clear(array); arr_ref_().data().push_back(v);}
        inline void value::push_back(value &&v) {clear(array); arr_ref_().data().push_back(std::move(v));}

        template<typename It>
        void value::append(It begin, It end)
        {
            clear(array);
            if (begin != end)
            {
                array_t &ref = arr_ref_();
                ref.data().insert(ref.data().end(), begin, end);
            }
        }

        inline value value::operator[](size_t pos) const {return element(pos);}
        inline value &value::operator[](size_t pos) {return element(pos);}
        inline value value::const_element(size_t pos) const {return is_nonnull_array() && pos < arr_ref_().size()? arr_ref_()[pos]: value();}
        inline value value::element(size_t pos) const {return const_element(pos);}
        inline const value *value::element_ptr(size_t pos) const
        {
            if (is_nonnull_array() && pos < arr_ref_().size())
                return std::addressof(arr_ref_().data()[pos]);
            return NULL;
        }
        inline value &value::element(size_t pos)
        {
            clear(array);
            if (arr_ref_().size() <= pos)
                arr_ref_().data().insert(arr_ref_().end().data(), pos - arr_ref_().size() + 1, core::null_t());
            return arr_ref_()[pos];
        }
        inline void value::erase_element(size_t pos) {if (is_nonnull_array()) arr_ref_().data().erase(arr_ref_().begin().data() + pos);}

#ifdef CPPDATALIB_THROW_IF_WRONG_TYPE
        inline array_t value::get_array(const array_t &) const {return get_array();}
        inline object_t value::get_object(const object_t &) const {return get_object();}
        inline array_t value::get_array() const
        {
            if (is_array())
                return is_nonnull_array()? arr_ref_(): array_t();
            throw core::error("cppdatalib::core::value - get_array() called on non-array value");
        }
        inline object_t value::get_object() const
        {
            if (is_object())
                return is_nonnull_object()? obj_ref_(): object_t();
            throw core::error("cppdatalib::core::value - get_object() called on non-object value");
        }
#else
        inline array_t value::get_array(const array_t &default_) const {return is_array()? arr_ref_(): default_;}
        inline object_t value::get_object(const object_t &default_) const {return is_object()? obj_ref_(): default_;}
        inline array_t value::get_array() const {return is_nonnull_array()? arr_ref_(): array_t();}
        inline object_t value::get_object() const {return is_nonnull_object()? obj_ref_(): object_t();}
#endif

#ifdef CPPDATALIB_DISABLE_IMPLICIT_DATA_CONVERSIONS
        inline array_t value::as_array(const array_t &default_) const {return get_array(default_);}
        inline object_t value::as_object(const object_t &default_) const {return get_object(default_);}
        inline array_t value::as_array() const {return get_array();}
        inline object_t value::as_object() const {return get_object();}
#else
        inline array_t value::as_array(const array_t &default_) const {return value(*this).convert_to(array, core::value(default_)).arr_ref_();}
        inline object_t value::as_object(const object_t &default_) const {return value(*this).convert_to(object, core::value(default_)).obj_ref_();}
        inline array_t value::as_array() const {return value(*this).convert_to(array, core::value(array_t())).arr_ref_();}
        inline object_t value::as_object() const {return value(*this).convert_to(object, core::value(object_t())).obj_ref_();}

        inline array_t &value::convert_to_array() {return convert_to(array, core::value(array_t())).arr_ref_();}
        inline object_t &value::convert_to_object() {return convert_to(object, core::value(object_t())).obj_ref_();}
#endif

        inline void value::mutable_clear() const
        {
            typedef void *ptr;

            switch (type_)
            {
                case null:
                    break;
                case link:
                    if (is_strong_link())
                        delete reinterpret_cast<value *>(ptr_);
                    ptr_.~ptr();
                    break;
                case boolean: bool_.~bool_t(); break;
                case integer: int_.~int_t(); break;
                case uinteger: uint_.~uint_t(); break;
                case real: real_.~real_t(); break;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                case temporary_string: tstr_.~string_view_t(); break;
#endif
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                case string: str_.~string_t(); break;
#else
                case string: delete reinterpret_cast<string_t*>(ptr_); ptr_.~ptr(); break;
#endif
                case array: delete reinterpret_cast<array_t*>(ptr_); ptr_.~ptr(); break;
                case object: delete reinterpret_cast<object_t*>(ptr_); ptr_.~ptr(); break;
            }
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            delete attr_; attr_ = nullptr;
#endif
            type_ = null;
        }

        inline void value::init(type new_type, subtype_t new_subtype)
        {
            switch (new_type)
            {
                case null: break;
                case boolean: new (&bool_) bool_t(); break;
                case integer: new (&int_) int_t(); break;
                case uinteger: new (&uint_) uint_t(); break;
                case real: new (&real_) real_t(); break;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                case temporary_string: new (&tstr_) string_view_t(); break;
#endif
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                case string: new (&str_) string_t(); break;
#else
                case string: new (&ptr_) string_t*(); ptr_ = nullptr; break;
#endif
                case array: new (&ptr_) array_t*(); ptr_ = nullptr; break;
                case object: new (&ptr_) object_t*(); ptr_ = nullptr; break;
                case link: new (&ptr_) value*(); ptr_ = nullptr; break;
            }
            type_ = new_type;
            subtype_ = new_subtype;
        }

        inline void value::deinit()
        {
            typedef void *ptr;

            switch (type_)
            {
                case null:
                    break;
                case link:
                    destroy_strong_link();
                    ptr_.~ptr();
                    break;
                case boolean: bool_.~bool_t(); break;
                case integer: int_.~int_t(); break;
                case uinteger: uint_.~uint_t(); break;
                case real: real_.~real_t(); break;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                case temporary_string: tstr_.~string_view_t(); break;
#endif
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                case string: str_.~string_t(); break;
#else
                case string: delete reinterpret_cast<string_t*>(ptr_); ptr_.~ptr(); break;
#endif
                case array: delete reinterpret_cast<array_t*>(ptr_); ptr_.~ptr(); break;
                case object: delete reinterpret_cast<object_t*>(ptr_); ptr_.~ptr(); break;
            }
            type_ = null;
            subtype_ = normal;
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            delete attr_; attr_ = nullptr;
#endif
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
        typename std::remove_cv<typename std::remove_reference<T>::type>::type cast(const cppdatalib::core::value &val)
        {
            return val.operator T();
        }

        template<typename T>
        cppdatalib::core::value cast(T val, subtype_t subtype = normal)
        {
            return cppdatalib::core::value(val, subtype);
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

        template<typename T, typename UserData>
        typename std::remove_cv<typename std::remove_reference<T>::type>::type value::cast(UserData userdata) const
        {
            return userdata_cast(*this, userdata).operator typename std::remove_cv<typename std::remove_reference<T>::type>::type();
        }
    }

    inline void swap(core::value &l, core::value &r) {l.swap(r);}
}

template<typename T>
class cast_to_cppdatalib
{
    typedef typename T::You_must_reimplement_cppdatalib_conversions_for_custom_types type;
public:
    cast_to_cppdatalib(T) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value();}
};

template<template<typename...> class Template, typename... Ts>
struct cast_template_to_cppdatalib
{
    typedef typename Template<Ts...>::You_must_reimplement_cppdatalib_conversions_for_custom_types type;
public:
    cast_template_to_cppdatalib(const Template<Ts...> &) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value();}
};

template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
struct cast_array_template_to_cppdatalib
{
    typedef typename Template<T, N, Ts...>::You_must_reimplement_cppdatalib_conversions_for_custom_types type;
public:
    cast_array_template_to_cppdatalib(const Template<T, N, Ts...> &) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value();}
};

template<template<size_t, typename...> class Template, size_t N, typename... Ts>
struct cast_sized_template_to_cppdatalib
{
    typedef typename Template<N, Ts...>::You_must_reimplement_cppdatalib_conversions_for_custom_types type;
public:
    cast_sized_template_to_cppdatalib(const Template<N, Ts...> &) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value();}
};

template<typename T>
class cast_from_cppdatalib {
    typedef typename T::You_must_reimplement_cppdatalib_conversions_for_custom_types type;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &) {}
    operator T() const {return T();}
};

template<template<typename...> class Template, typename... Ts>
struct cast_template_from_cppdatalib
{
    typedef typename Template<Ts...>::You_must_reimplement_cppdatalib_conversions_for_custom_types type;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &) {}
    operator Template<Ts...>() const {return Template<Ts...>();}
};

template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
struct cast_array_template_from_cppdatalib
{
    typedef typename Template<T, N, Ts...>::You_must_reimplement_cppdatalib_conversions_for_custom_types type;
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &) {}
    operator Template<T, N, Ts...>() const {return Template<T, N, Ts...>();}
};

template<template<size_t, typename...> class Template, size_t N, typename... Ts>
struct cast_sized_template_from_cppdatalib
{
    typedef typename Template<N, Ts...>::You_must_reimplement_cppdatalib_conversions_for_custom_types type;
public:
    cast_sized_template_from_cppdatalib(const cppdatalib::core::value &) {}
    operator Template<N, Ts...>() const {return Template<N, Ts...>();}
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
class cast_to_cppdatalib<char>
{
    unsigned char bind;
public:
    cast_to_cppdatalib(char bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(cppdatalib::core::string_t(1, bind));}
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

template<typename T>
class cast_to_cppdatalib<T *>
{
    const T *bind;
public:
    cast_to_cppdatalib(const T *bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        if (bind)
            return *bind;
        return cppdatalib::core::null_t();
    }
};

template<size_t N>
class cast_to_cppdatalib<char[N]>
{
    const char *bind;
public:
    cast_to_cppdatalib(const char (&bind)[N]) : bind(&bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::string_t(bind, N);}
};

template<typename T, size_t N>
class cast_to_cppdatalib<T[N]>
{
    const T *bind;
public:
    cast_to_cppdatalib(const T (&bind)[N]) : bind(&bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (size_t i = 0; i < N; ++i)
            result.push_back(bind[i]);
        return result;
    }
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

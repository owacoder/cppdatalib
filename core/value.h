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
#include <list>
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

namespace cppdatalib
{
    namespace core
    {
        enum type
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
            normal,

            // Integers
            timestamp, // Number of seconds since the epoch, Jan 1, 1970

            // Strings
            blob, // A chunk of binary data
            clob, // A chunk of binary data, that should be interpreted as text
            symbol, // A symbolic atom, or identifier
            datetime, // A datetime structure, with unspecified format
            date, // A date structure, with unspecified format
            time, // A time structure, with unspecified format
            bignum, // A high-precision, decimal-encoded, number

            // Arrays
            regexp, // A regular expression definition containing two string elements, the first is the definition, the second is the options list
            sexp, // Ordered collection of values, distinct from an array only by name

            // Objects
            map, // A normal object with integral keys

            user = 16
        };

        class value;
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

#ifdef CPPDATALIB_ARRAY_T
        typedef CPPDATALIB_ARRAY_T array_t;
#else
        typedef std::vector<value> array_t;
#endif

// TODO: FAST_IO_OBJECT is not fast at all. Fix?
//#define CPPDATALIB_FAST_IO_OBJECT

#ifdef CPPDATALIB_OBJECT_T
        typedef CPPDATALIB_OBJECT_T object_t;
#elif defined(CPPDATALIB_FAST_IO_OBJECT)
        template<typename K, typename V>
        class fast_object
        {
        public:
            typedef K key_type;
            typedef V mapped_type;
            typedef std::pair<K, V> value_type;
            typedef value_type &reference;
            typedef const value_type &const_reference;
            typedef value_type *pointer;
            typedef const value_type *const_pointer;
            typedef typename std::list<value_type>::iterator iterator;
            typedef typename std::list<value_type>::const_iterator const_iterator;
            typedef typename std::list<value_type>::reverse_iterator reverse_iterator;
            typedef typename std::list<value_type>::const_reverse_iterator const_reverse_iterator;
            typedef typename std::list<value_type>::difference_type difference_type;
            typedef typename std::list<value_type>::size_type size_type;
            typedef std::less<value> key_compare;
            struct value_compare
            {
                bool operator()(const value_type &l, const value_type &r) const
                {
                    return l.first < r.first;
                }

                bool operator()(const value_type &l, const key_type &r) const
                {
                    return l.first < r;
                }

                bool operator()(const key_type &l, const value_type &r) const
                {
                    return l < r.first;
                }
            };

            key_compare key_comp() const {return key_compare();}
            value_compare value_comp() const {return value_compare();}

            iterator begin() {return d_.begin();}
            const_iterator begin() const {return d_.begin();}
            iterator end() {return d_.end();}
            const_iterator end() const {return d_.end();}

            reverse_iterator rbegin() {return d_.rbegin();}
            const_reverse_iterator rbegin() const {return d_.rbegin();}
            reverse_iterator rend() {return d_.rend();}
            const_reverse_iterator rend() const {return d_.rend();}

            const_iterator cbegin() const {return d_.cbegin();}
            const_iterator cend() const {return d_.cend();}
            const_reverse_iterator crbegin() const {return d_.crbegin();}
            const_reverse_iterator crend() const {return d_.crend();}

            bool empty() const {return d_.empty();}
            size_type size() const {return d_.size();}
            size_type max_size() const {return d_.max_size();}

            iterator insert(const value_type &val)
            {
                return d_.insert(upper_bound(val.first), val);
            }
            template<typename P> iterator insert(P &&val)
            {
                return d_.insert(upper_bound(val.first), val);
            }

            // TODO: accelerate placement insert (use hint)
            iterator insert(const_iterator insert, const value_type &val)
            {
                (void) insert;
                return this->insert(val);
            }
            template<typename P> iterator insert(const_iterator insert, P &&val)
            {
                (void) insert;
                return this->insert(val);
            }

            size_type count(const key_type &k) const
            {
                size_type n = 0;
                const_iterator it = find(k);
                while (it != end() && it->first == k)
                    ++n, ++it;
                return n;
            }

            void clear() {d_.clear();}

            iterator find(const key_type &k)
            {
                auto it = std::lower_bound(d_.begin(), d_.end(), k, value_compare());
                if (it == end() || k < it->first)
                    return end();
                return it;
            }

            const_iterator find(const key_type &k) const
            {
                auto it = std::lower_bound(d_.cbegin(), d_.cend(), k, value_compare());
                if (it == end() || k < it->first)
                    return end();
                return it;
            }

            iterator lower_bound(const key_type &k) {return std::lower_bound(d_.begin(), d_.end(), k, value_compare());}
            const_iterator lower_bound(const key_type &k) const {return std::lower_bound(d_.begin(), d_.end(), k, value_compare());}
            iterator upper_bound(const key_type &k) {return std::upper_bound(d_.begin(), d_.end(), k, value_compare());}
            const_iterator upper_bound(const key_type &k) const {return std::upper_bound(d_.begin(), d_.end(), k, value_compare());}

            iterator erase(const_iterator position) {return d_.erase(position);}
            size_type erase(const key_type &k)
            {
                size_type n = 0;
                iterator first = find(k);
                iterator last = first;

                while (last != end() && last->first == k)
                    ++last, ++n;

                if (first != last)
                    d_.erase(first, last);

                return n;
            }
            iterator erase(const_iterator first, const_iterator last) {return d_.erase(first, last);}

        private:
            std::list<value_type> d_;
        };

        typedef fast_object<value, value> object_t;
#else
        typedef std::multimap<value, value> object_t;
#endif

#ifdef CPPDATALIB_SUBTYPE_T
        typedef CPPDATALIB_SUBTYPE_T subtype_t;
#else
        typedef long subtype_t;
#endif

        struct null_t {};

        template<typename T> core::value to_cppdatalib(T v);
        template<typename T> T from_cppdatalib(const core::value &v);

        /* The core value class for all of cppdatalib.
         *
         * To marshal and unmarshal from user-defined types, define the following two functions:
         *
         *     void from_cppdatalib(const cppdatalib::core::value &src);
         *     void to_cppdatalib(cppdatalib::core::value &dst) const;
         *
         * `from_cppdatalib()` should assign the value in `src` to the specified variable, and `to_cppdatalib()` should
         * serialize the value into `dst` (Note: dst will always be a null object when passed to `to_cppdatalib()`).
         *
         * Once these functions are defined, you can use the generic `operator=` in both directions; `value = user_type` or `user_type = value`
         *
         */

        class value
        {
        public:
            struct traversal_reference
            {
                traversal_reference(const value *p, array_t::const_iterator a, object_t::const_iterator o, bool traversed_key, bool frozen = false)
                    : p(p)
                    , array(a)
                    , object(o)
                    , traversed_key_already(traversed_key)
                    , frozen(frozen)
                {}

                bool is_array() const {return p && array != array_t::const_iterator() && array != p->get_array().end();}
                size_t get_array_index() const {return is_array()? array - p->get_array().begin(): 0;}
                const core::value *get_array_element() const {return is_array()? std::addressof(*array): NULL;}

                bool is_object() const {return p && object != object_t::const_iterator() && object != p->get_object().end();}
                bool is_object_key() const {return is_object() && !traversed_key_already;}
                const core::value *get_object_key() const {return is_object()? std::addressof(object->first): NULL;}
                const core::value *get_object_value() const {return is_object()? std::addressof(object->second): NULL;}

            private:
                friend class value;

                const value *p;
                array_t::const_iterator array;
                object_t::const_iterator object;
                bool traversed_key_already;
                bool frozen;
            };

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
            void traverse(PrefixPredicate &prefix, PostfixPredicate &postfix) const
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
                            references.push(traversal_reference(p, p->get_array().begin(), object_t::const_iterator(), false));
                            if (!p->get_array().empty())
                                p = std::addressof(*references.top().array++);
                            else
                                p = NULL;
                        }
                        else if (p->is_object())
                        {
                            references.push(traversal_reference(p, array_t::const_iterator(), p->get_object().begin(), true));
                            if (!p->get_object().empty())
                                p = std::addressof(references.top().object->first);
                            else
                                p = NULL;
                        }
                        else
                        {
                            references.push(traversal_reference(p, array_t::const_iterator(), object_t::const_iterator(), false));
                            p = NULL;
                        }
                    }
                    else
                    {
                        const value *peek = references.top().p;
                        if (peek->is_array() && references.top().array != peek->get_array().end())
                            p = std::addressof(*references.top().array++);
                        else if (peek->is_object() && references.top().object != peek->get_object().end())
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
            void traverse(Predicate &predicate) const
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
                            references.push(traversal_reference(p, p->get_array().begin(), object_t::const_iterator(), false));
                            if (!p->get_array().empty())
                                p = std::addressof(*references.top().array++);
                            else
                                p = NULL;
                        }
                        else if (p->is_object())
                        {
                            references.push(traversal_reference(p, array_t::const_iterator(), p->get_object().begin(), true));
                            if (!p->get_object().empty())
                                p = std::addressof(references.top().object->first);
                            else
                                p = NULL;
                        }
                        else
                        {
                            references.push(traversal_reference(p, array_t::const_iterator(), object_t::const_iterator(), false));
                            p = NULL;
                        }
                    }
                    else
                    {
                        const value *peek = references.top().p;
                        if (peek->is_array() && references.top().array != peek->get_array().end())
                            p = std::addressof(*references.top().array++);
                        else if (peek->is_object() && references.top().object != peek->get_object().end())
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
            void value_traverse(PrefixPredicate &prefix, PostfixPredicate &postfix) const
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
                            references.push(traversal_reference(p, p->get_array().begin(), object_t::const_iterator(), false));
                            if (!p->get_array().empty())
                                p = std::addressof(*references.top().array++);
                            else
                                p = NULL;
                        }
                        else if (p->is_object())
                        {
                            references.push(traversal_reference(p, array_t::const_iterator(), p->get_object().begin(), true));
                            if (!p->get_object().empty())
                                p = std::addressof((references.top().object++)->second);
                            else
                                p = NULL;
                        }
                        else
                        {
                            references.push(traversal_reference(p, array_t::const_iterator(), object_t::const_iterator(), false));
                            p = NULL;
                        }
                    }
                    else
                    {
                        const value *peek = references.top().p;
                        if (peek->is_array() && references.top().array != peek->get_array().end())
                            p = std::addressof(*references.top().array++);
                        else if (peek->is_object() && references.top().object != peek->get_object().end())
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
            void value_traverse(Predicate &predicate) const
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
                            references.push(traversal_reference(p, p->get_array().begin(), object_t::const_iterator(), false));
                            if (!p->get_array().empty())
                                p = std::addressof(*references.top().array++);
                            else
                                p = NULL;
                        }
                        else if (p->is_object())
                        {
                            references.push(traversal_reference(p, array_t::const_iterator(), p->get_object().begin(), true));
                            if (!p->get_object().empty())
                                p = std::addressof((references.top().object++)->second);
                            else
                                p = NULL;
                        }
                        else
                        {
                            references.push(traversal_reference(p, array_t::const_iterator(), object_t::const_iterator(), false));
                            p = NULL;
                        }
                    }
                    else
                    {
                        const value *peek = references.top().p;
                        if (peek->is_array() && references.top().array != peek->get_array().end())
                            p = std::addressof(*references.top().array++);
                        else if (peek->is_object() && references.top().object != peek->get_object().end())
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
            void prefix_traverse(PrefixPredicate &prefix) {traverse(prefix, traverse_node_null);}

            // Predicates must be callables with argument type `const core::value *arg, core::value::traversal_ancestry_finder arg_finder` and return value bool
            // If return value is non-zero, processing continues, otherwise processing aborts immediately
            template<typename PostfixPredicate>
            void postfix_traverse(PostfixPredicate &postfix) {traverse(traverse_node_null, postfix);}

            // Predicates must be callables with argument type `const core::value *arg, const core::value *arg2, core::value::traversal_ancestry_finder arg_finder, core::value::traversal_ancestry_finder arg2_finder`
            // and return value bool. Either argument may be NULL, but both arguments will never be NULL simultaneously.
            // If return value is non-zero, processing continues, otherwise processing aborts immediately
            template<typename PrefixPredicate, typename PostfixPredicate>
            void parallel_traverse(const value &other, PrefixPredicate &prefix, PostfixPredicate &postfix) const
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
                                references.push(traversal_reference(p, p->get_array().begin(), object_t::const_iterator(), false));
                                if (!p->get_array().empty())
                                    p = std::addressof(*references.top().array++);
                                else
                                    p = NULL;
                            }
                            else if (p->is_object())
                            {
                                references.push(traversal_reference(p, array_t::const_iterator(), p->get_object().begin(), true));
                                if (!p->get_object().empty())
                                    p = std::addressof(references.top().object->first);
                                else
                                    p = NULL;
                            }
                            else
                            {
                                references.push(traversal_reference(p, array_t::const_iterator(), object_t::const_iterator(), false));
                                p = NULL;
                            }
                        }

                        if (other_p != NULL)
                        {
                            if (other_p->is_array())
                            {
                                other_references.push(traversal_reference(other_p, other_p->get_array().begin(), object_t::const_iterator(), false));
                                if (!other_p->get_array().empty())
                                    other_p = std::addressof(*other_references.top().array++);
                                else
                                    other_p = NULL;
                            }
                            else if (other_p->is_object())
                            {
                                other_references.push(traversal_reference(other_p, array_t::const_iterator(), other_p->get_object().begin(), true));
                                if (!other_p->get_object().empty())
                                    other_p = std::addressof(other_references.top().object->first);
                                else
                                    other_p = NULL;
                            }
                            else
                            {
                                other_references.push(traversal_reference(other_p, array_t::const_iterator(), object_t::const_iterator(), false));
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
                            if (peek->is_array() && references.top().array != peek->get_array().end())
                                p = std::addressof(*references.top().array++);
                            else if (peek->is_object() && references.top().object != peek->get_object().end())
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
                            if (other_peek->is_array() && other_references.top().array != other_peek->get_array().end())
                                other_p = std::addressof(*other_references.top().array++);
                            else if (other_peek->is_object() && other_references.top().object != other_peek->get_object().end())
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
            void parallel_diff_traverse(const value &other, PrefixPredicate &prefix, PostfixPredicate &postfix) const
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

                        traversal_reference save_ref = save_refs.empty()? traversal_reference(NULL, array_t::const_iterator(), object_t::const_iterator(), false): save_refs.top();
                        traversal_reference save_other_ref = save_other_refs.empty()? traversal_reference(NULL, array_t::const_iterator(), object_t::const_iterator(), false): save_other_refs.top();

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
                                references.push(traversal_reference(p, p->get_array().begin(), object_t::const_iterator(), false, p_frozen));
                                if (!p->get_array().empty() && !p_frozen)
                                    p = std::addressof(*references.top().array++);
                                else
                                    p = NULL;
                            }
                            else if (p->is_object())
                            {
                                references.push(traversal_reference(p, array_t::const_iterator(), p->get_object().begin(), true, p_frozen));
                                if (!p->get_object().empty() && !p_frozen)
                                    p = std::addressof(references.top().object->first);
                                else
                                    p = NULL;
                            }
                            else
                            {
                                references.push(traversal_reference(p, array_t::const_iterator(), object_t::const_iterator(), false, p_frozen));
                                p = NULL;
                            }
                        }

                        if (other_p != NULL)
                        {
                            if (other_p->is_array())
                            {
                                other_references.push(traversal_reference(other_p, other_p->get_array().begin(), object_t::const_iterator(), false, other_p_frozen));
                                if (!other_p->get_array().empty() && !other_p_frozen)
                                    other_p = std::addressof(*other_references.top().array++);
                                else
                                    other_p = NULL;
                            }
                            else if (other_p->is_object())
                            {
                                other_references.push(traversal_reference(other_p, array_t::const_iterator(), other_p->get_object().begin(), true, other_p_frozen));
                                if (!other_p->get_object().empty() && !other_p_frozen)
                                    other_p = std::addressof(other_references.top().object->first);
                                else
                                    other_p = NULL;
                            }
                            else
                            {
                                other_references.push(traversal_reference(other_p, array_t::const_iterator(), object_t::const_iterator(), false, other_p_frozen));
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
                            else if (peek->is_array() && references.top().array != peek->get_array().end())
                                p = std::addressof(*references.top().array++);
                            else if (peek->is_object() && references.top().object != peek->get_object().end())
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
                            else if (other_peek->is_array() && other_references.top().array != other_peek->get_array().end())
                                other_p = std::addressof(*other_references.top().array++);
                            else if (other_peek->is_object() && other_references.top().object != other_peek->get_object().end())
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

            value() : type_(null), subtype_(0) {}
            value(null_t, subtype_t subtype = 0) : type_(null), subtype_(subtype) {}
            value(bool_t v, subtype_t subtype = 0) : type_(boolean), subtype_(subtype), bool_(v) {}
            value(int_t v, subtype_t subtype = 0) : type_(integer), subtype_(subtype), int_(v) {}
            value(uint_t v, subtype_t subtype = 0) : type_(uinteger), subtype_(subtype), uint_(v) {}
            value(real_t v, subtype_t subtype = 0) : type_(real), subtype_(subtype), real_(v) {}
            value(cstring_t v, subtype_t subtype = 0) : type_(string), subtype_(subtype), str_(v) {}
            value(const string_t &v, subtype_t subtype = 0) : type_(string), subtype_(subtype), str_(v) {}
            value(string_t &&v, subtype_t subtype = 0) : type_(string), subtype_(subtype), str_(std::move(v)) {}
            value(const array_t &v, subtype_t subtype = 0) : type_(array), subtype_(subtype), arr_(v) {}
            value(array_t &&v, subtype_t subtype = 0) : type_(array), subtype_(subtype), arr_(std::move(v)) {}
            value(const object_t &v, subtype_t subtype = 0) : type_(object), subtype_(subtype), obj_(v) {}
            value(object_t &&v, subtype_t subtype = 0) : type_(object), subtype_(subtype), obj_(std::move(v)) {}
            template<typename T, typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
            value(T v, subtype_t subtype = 0) : type_(uinteger), subtype_(subtype), uint_(v) {}
            template<typename T, typename std::enable_if<std::is_signed<T>::value, int>::type = 0>
            value(T v, subtype_t subtype = 0) : type_(integer), subtype_(subtype), int_(v) {}
            template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
            value(T v, subtype_t subtype = 0) : type_(real), subtype_(subtype), real_(v) {}
            template<typename T>
            value(std::initializer_list<T> v, subtype_t subtype = 0) : type_(null), subtype_(subtype)
            {
                for (auto const &element: v)
                    push_back(element);
            }
            template<typename T>
            value(const std::vector<T> &v, subtype_t subtype = 0) : type_(null), subtype_(subtype)
            {
                for (auto const &element: v)
                    push_back(element);
            }
            template<typename T, typename U>
            value(const std::map<T, U> &v, subtype_t subtype = 0) : type_(null), subtype_(subtype)
            {
                for (auto const &element: v)
                    add_member(element.first, element.second);
            }
            template<typename T, typename U>
            value(const std::multimap<T, U> &v, subtype_t subtype = 0) : type_(null), subtype_(subtype)
            {
                for (auto const &element: v)
                    add_member(element.first, element.second);
            }
            template<typename T, typename std::enable_if<std::is_class<T>::value, int>::type = 0>
            value(const T &v, subtype_t subtype = 0) : type_(null), subtype_(subtype)
            {
                assign(*this, to_cppdatalib(v));
            }

            ~value()
            {
                if ((type_ == array && arr_.size() > 0) ||
                    (type_ == object && obj_.size() > 0))
                    traverse(traverse_node_null, traverse_node_mutable_clear);
            }

            value(const value &other) : type_(null), subtype_(0) {assign(*this, other);}
            value(value &&other) = default;
            value &operator=(const value &other) {return assign(*this, other);}

            subtype_t get_subtype() const {return subtype_;}
            subtype_t &get_subtype() {return subtype_;}
            void set_subtype(subtype_t _type) {subtype_ = _type;}

            type get_type() const {return type_;}
            size_t size() const {return type_ == array? arr_.size(): type_ == object? obj_.size(): type_ == string? str_.size(): 0;}

            bool_t is_null() const {return type_ == null;}
            bool_t is_bool() const {return type_ == boolean;}
            bool_t is_int() const {return type_ == integer;}
            bool_t is_uint() const {return type_ == uinteger;}
            bool_t is_real() const {return type_ == real;}
            bool_t is_string() const {return type_ == string;}
            bool_t is_array() const {return type_ == array;}
            bool_t is_object() const {return type_ == object;}

            bool_t get_bool() const {return bool_;}
            int_t get_int() const {return int_;}
            uint_t get_uint() const {return uint_;}
            real_t get_real() const {return real_;}
            cstring_t get_cstring() const {return str_.c_str();}
            const string_t &get_string() const {return str_;}
            const array_t &get_array() const {return arr_;}
            const object_t &get_object() const {return obj_;}

            bool_t &get_bool() {clear(boolean); return bool_;}
            int_t &get_int() {clear(integer); return int_;}
            uint_t &get_uint() {clear(uinteger); return uint_;}
            real_t &get_real() {clear(real); return real_;}
            string_t &get_string() {clear(string); return str_;}
            array_t &get_array() {clear(array); return arr_;}
            object_t &get_object() {clear(object); return obj_;}

            void set_null() {clear(null);}
            void set_bool(bool_t v) {clear(boolean); bool_ = v;}
            void set_int(int_t v) {clear(integer); int_ = v;}
            void set_uint(uint_t v) {clear(uinteger); uint_ = v;}
            void set_real(real_t v) {clear(real); real_ = v;}
            void set_string(cstring_t v) {clear(string); str_ = v;}
            void set_string(const string_t &v) {clear(string); str_ = v;}
            void set_array(const array_t &v) {clear(array); arr_ = v;}
            void set_object(const object_t &v) {clear(object); obj_ = v;}

            void set_null(subtype_t subtype) {clear(null); subtype_ = subtype;}
            void set_bool(bool_t v, subtype_t subtype) {clear(boolean); bool_ = v; subtype_ = subtype;}
            void set_int(int_t v, subtype_t subtype) {clear(integer); int_ = v; subtype_ = subtype;}
            void set_uint(uint_t v, subtype_t subtype) {clear(uinteger); uint_ = v; subtype_ = subtype;}
            void set_real(real_t v, subtype_t subtype) {clear(real); real_ = v; subtype_ = subtype;}
            void set_string(cstring_t v, subtype_t subtype) {clear(string); str_ = v; subtype_ = subtype;}
            void set_string(const string_t &v, subtype_t subtype) {clear(string); str_ = v; subtype_ = subtype;}
            void set_array(const array_t &v, subtype_t subtype) {clear(array); arr_ = v; subtype_ = subtype;}
            void set_object(const object_t &v, subtype_t subtype) {clear(object); obj_ = v; subtype_ = subtype;}

            value operator[](cstring_t key) const {return member(key);}
            value &operator[](cstring_t key) {return member(key);}
            value operator[](const string_t &key) const {return member(key);}
            value &operator[](const string_t &key) {return member(key);}
            value member(const value &key) const
            {
                auto it = obj_.find(key);
                if (it != obj_.end())
                    return it->second;
                return value();
            }
            value &member(const value &key)
            {
                clear(object);
                auto it = obj_.lower_bound(key);
                if (it->first == key)
                    return it->second;
                it = obj_.insert(it, {key, null_t()});
                return it->second;
            }
            const value *member_ptr(const value &key) const
            {
                auto it = obj_.find(key);
                if (it != obj_.end())
                    return std::addressof(it->second);
                return NULL;
            }
            bool_t is_member(cstring_t key) const {return obj_.find(key) != obj_.end();}
            bool_t is_member(const string_t &key) const {return obj_.find(key) != obj_.end();}
            bool_t is_member(const value &key) const {return obj_.find(key) != obj_.end();}
            size_t member_count(cstring_t key) const {return obj_.count(key);}
            size_t member_count(const string_t &key) const {return obj_.count(key);}
            size_t member_count(const value &key) const {return obj_.count(key);}
            void erase_member(cstring_t key) {obj_.erase(key);}
            void erase_member(const string_t &key) {obj_.erase(key);}
            void erase_member(const value &key) {obj_.erase(key);}

            value &add_member(const value &key)
            {
                clear(object);
                return obj_.insert(decltype(obj_)::value_type(key, null_t()))->second;
            }
            value &add_member(value &&key)
            {
                clear(object);
                return obj_.insert(decltype(obj_)::value_type(key, null_t()))->second;
            }
            value &add_member(const value &key, const value &val)
            {
                clear(object);
                return obj_.insert(decltype(obj_)::value_type(key, val))->second;
            }
            value &add_member(value &&key, value &&val)
            {
                clear(object);
                return obj_.insert(decltype(obj_)::value_type(key, val))->second;
            }

            void push_back(const value &v) {clear(array); arr_.push_back(v);}
            void push_back(value &&v) {clear(array); arr_.push_back(v);}
            const value &operator[](size_t pos) const {return element(pos);}
            value &operator[](size_t pos) {return element(pos);}
            const value &element(size_t pos) const {return arr_[pos];}
            value &element(size_t pos) {return arr_[pos];}
            void erase_element(int_t pos) {arr_.erase(arr_.begin() + pos);}

            // The following are convenience conversion functions
            bool_t get_bool(bool_t default_) const {return is_bool()? bool_: default_;}
            int_t get_int(int_t default_) const {return is_int()? int_: default_;}
            uint_t get_uint(uint_t default_) const {return is_uint()? uint_: default_;}
            real_t get_real(real_t default_) const {return is_real()? real_: default_;}
            cstring_t get_string(cstring_t default_) const {return is_string()? str_.c_str(): default_;}
            string_t get_string(const string_t &default_) const {return is_string()? str_: default_;}
            array_t get_array(const array_t &default_) const {return is_array()? arr_: default_;}
            object_t get_object(const object_t &default_) const {return is_object()? obj_: default_;}

            bool_t as_bool(bool_t default_ = false) const {return value(*this).convert_to(boolean, default_).bool_;}
            int_t as_int(int_t default_ = 0) const {return value(*this).convert_to(integer, default_).int_;}
            uint_t as_uint(uint_t default_ = 0) const {return value(*this).convert_to(uinteger, default_).uint_;}
            real_t as_real(real_t default_ = 0.0) const {return value(*this).convert_to(real, default_).real_;}
            string_t as_string(const string_t &default_ = string_t()) const {return value(*this).convert_to(string, default_).str_;}
            array_t as_array(const array_t &default_ = array_t()) const {return value(*this).convert_to(array, default_).arr_;}
            object_t as_object(const object_t &default_ = object_t()) const {return value(*this).convert_to(object, default_).obj_;}

            bool_t &convert_to_bool(bool_t default_ = false) {return convert_to(boolean, default_).bool_;}
            int_t &convert_to_int(int_t default_ = 0) {return convert_to(integer, default_).int_;}
            uint_t &convert_to_uint(uint_t default_ = 0) {return convert_to(uinteger, default_).uint_;}
            real_t &convert_to_real(real_t default_ = 0.0) {return convert_to(real, default_).real_;}
            string_t &convert_to_string(const string_t &default_ = string_t()) {return convert_to(string, default_).str_;}
            array_t &convert_to_array(const array_t &default_ = array_t()) {return convert_to(array, default_).arr_;}
            object_t &convert_to_object(const object_t &default_ = object_t()) {return convert_to(object, default_).obj_;}

            template<typename T>
            operator T() const {return from_cppdatalib<T>(*this);}

        private:
            // WARNING: DO NOT CALL mutable_clear() anywhere but the destructor!
            // It violates const-correctness for the sole purpose of allowing
            // complex object keys to be destroyed iteratively, instead of recursively.
            // (They're defined as `const` members in the std::map implementation)
            void mutable_clear() const
            {
                arr_.clear();
                obj_.clear();
                type_ = null;
            }

            void shallow_clear()
            {
                str_.clear();
                arr_.clear();
                obj_.clear();
                type_ = null;
            }

            void clear(type new_type)
            {
                if (type_ == new_type)
                    return;

                str_.clear(); str_.shrink_to_fit();
                arr_.clear(); arr_.shrink_to_fit();
                obj_.clear();
                type_ = new_type;
                subtype_ = 0;
            }

            // TODO: ensure that all conversions are generic enough (for example, string -> bool doesn't just need to be "true")
            value &convert_to(type new_type, const value &default_value)
            {
                if (type_ == new_type)
                    return *this;

                switch (type_)
                {
                    case null: *this = default_value; break;
                    case boolean:
                    {
                        clear(new_type);
                        switch (new_type)
                        {
                            case integer: int_ = bool_; break;
                            case uinteger: uint_ = bool_; break;
                            case real: real_ = bool_; break;
                            case string: str_ = bool_? "true": "false"; break;
                            default: *this = default_value; break;
                        }
                        break;
                    }
                    case integer:
                    {
                        clear(new_type);
                        switch (new_type)
                        {
                            case boolean: bool_ = int_ != 0; break;
                            case uinteger: uint_ = int_ > 0? int_: 0; break;
                            case real: real_ = int_; break;
                            case string: str_ = std::to_string(int_); break;
                            default: *this = default_value; break;
                        }
                        break;
                    }
                    case uinteger:
                    {
                        clear(new_type);
                        switch (new_type)
                        {
                            case boolean: bool_ = uint_ != 0; break;
                            case integer: int_ = uint_ <= INT64_MAX? uint_: 0; break;
                            case real: real_ = uint_; break;
                            case string: str_ = std::to_string(uint_); break;
                            default: *this = default_value; break;
                        }
                        break;
                    }
                    case real:
                    {
                        clear(new_type);
                        switch (new_type)
                        {
                            case boolean: bool_ = real_ != 0.0; break;
                            case integer: int_ = (real_ >= INT64_MIN && real_ <= INT64_MAX)? trunc(real_): 0; break;
                            case uinteger: uint_ = (real_ >= 0 && real_ <= UINT64_MAX)? trunc(real_): 0; break;
                            case string:
                            {
                                std::ostringstream str;
                                str << std::setprecision(CPPDATALIB_REAL_DIG) << real_;
                                str_ = str.str();
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
                            case boolean: bool_ = str_ == "true"; break;
                            case integer:
                            {
                                std::istringstream str(str_);
                                str >> int_;
                                if (!str)
                                    int_ = 0;
                                break;
                            }
                            case uinteger:
                            {
                                std::istringstream str(str_);
                                str >> uint_;
                                if (!str)
                                    uint_ = 0;
                                break;
                            }
                            case real:
                            {
                                std::istringstream str(str_);
                                str >> real_;
                                if (!str)
                                    real_ = 0.0;
                                break;
                            }
                            default: *this = default_value; break;
                        }
                        clear(new_type);
                        break;
                    }
                    default: break;
                }

                return *this;
            }

            mutable type type_; // Mutable to provide editable traversal access to const destructor
            subtype_t subtype_;
#ifndef CPPDATALIB_NON_POD_LAYOUT
            union
            {
                bool_t bool_;
                int_t int_;
                uint_t uint_;
                real_t real_;
            };
#else
            bool_t bool_;
            int_t int_;
            uint_t uint_;
            real_t real_;
#endif
            string_t str_;
            mutable array_t arr_; // Mutable to provide editable traversal access to const destructor
            mutable object_t obj_; // Mutable to provide editable traversal access to const destructor
        };

        namespace impl
        {
            template<typename T, typename U>
            T zero_convert(T min, U val, T max)
            {
                return val < min || max < val? T(0): T(val);
            }
        }

        core::value to_cppdatalib(signed char v) {return v;}
        core::value to_cppdatalib(unsigned char v) {return v;}
        core::value to_cppdatalib(signed short v) {return v;}
        core::value to_cppdatalib(unsigned short v) {return v;}
        core::value to_cppdatalib(signed int v) {return v;}
        core::value to_cppdatalib(unsigned int v) {return v;}
        core::value to_cppdatalib(signed long v) {return v;}
        core::value to_cppdatalib(unsigned long v) {return v;}
        core::value to_cppdatalib(signed long long v) {return v;}
        core::value to_cppdatalib(unsigned long long v) {return v;}
        core::value to_cppdatalib(float v) {return core::real_t(v);}
        core::value to_cppdatalib(double v) {return v;}
        core::value to_cppdatalib(long double v) {return core::real_t(v);}
        core::value to_cppdatalib(char *v) {return v;}
        core::value to_cppdatalib(const char *v) {return v;}
        core::value to_cppdatalib(const std::string &v) {return v;}
        template<typename T> core::value to_cppdatalib(std::initializer_list<T> v) {return v;}

        template<typename T>
        core::value to_cppdatalib(const std::vector<T> &v)
        {
            core::value result;
            for (auto const &element: v)
                result.push_back(element);
            return result;
        }

        template<typename T, typename U>
        core::value to_cppdatalib(const std::map<T, U> &v)
        {
            core::value result;
            for (auto const &element: v)
                result.add_member(element.first) = element.second;
            return result;
        }

        template<typename T, typename U>
        core::value to_cppdatalib(const std::multimap<T, U> &v)
        {
            core::value result;
            for (auto const &element: v)
                result.add_member(element.first) = element.second;
            return result;
        }

        template<> bool from_cppdatalib<bool>(const core::value &v) {return v.as_bool();}
        template<> signed char from_cppdatalib<signed char>(const core::value &v)
        {
            return impl::zero_convert(std::numeric_limits<signed char>::min(),
                                      v.as_int(),
                                      std::numeric_limits<signed char>::max());
        }
        template<> unsigned char from_cppdatalib<unsigned char>(const core::value &v)
        {
            return impl::zero_convert(std::numeric_limits<unsigned char>::min(),
                                      v.as_uint(),
                                      std::numeric_limits<unsigned char>::max());
        }
        template<> signed short from_cppdatalib<signed short>(const core::value &v)
        {
            return impl::zero_convert(std::numeric_limits<signed short>::min(),
                                      v.as_int(),
                                      std::numeric_limits<signed short>::max());
        }
        template<> unsigned short from_cppdatalib<unsigned short>(const core::value &v)
        {
            return impl::zero_convert(std::numeric_limits<unsigned short>::min(),
                                      v.as_uint(),
                                      std::numeric_limits<unsigned short>::max());
        }
        template<> signed int from_cppdatalib<signed int>(const core::value &v)
        {
            return impl::zero_convert(std::numeric_limits<signed int>::min(),
                                      v.as_int(),
                                      std::numeric_limits<signed int>::max());
        }
        template<> unsigned int from_cppdatalib<unsigned int>(const core::value &v)
        {
            return impl::zero_convert(std::numeric_limits<unsigned int>::min(),
                                      v.as_uint(),
                                      std::numeric_limits<unsigned int>::max());
        }
        template<> signed long from_cppdatalib<signed long>(const core::value &v)
        {
            return impl::zero_convert(std::numeric_limits<signed long>::min(),
                                      v.as_int(),
                                      std::numeric_limits<signed long>::max());
        }
        template<> unsigned long from_cppdatalib<unsigned long>(const core::value &v)
        {
            return impl::zero_convert(std::numeric_limits<unsigned long>::min(),
                                      v.as_uint(),
                                      std::numeric_limits<unsigned long>::max());
        }
        template<> signed long long from_cppdatalib<signed long long>(const core::value &v)
        {
            return impl::zero_convert(std::numeric_limits<signed long long>::min(),
                                      v.as_int(),
                                      std::numeric_limits<signed long long>::max());
        }
        template<> unsigned long long from_cppdatalib<unsigned long long>(const core::value &v)
        {
            return impl::zero_convert(std::numeric_limits<unsigned long long>::min(),
                                      v.as_uint(),
                                      std::numeric_limits<unsigned long long>::max());
        }

        template<> float from_cppdatalib<float>(const core::value &v) {return v.as_real();}
        template<> double from_cppdatalib<double>(const core::value &v) {return v.as_real();}
        template<> long double from_cppdatalib<long double>(const core::value &v) {return v.as_real();}

        template<> std::string from_cppdatalib<std::string>(const core::value &v) {return v.as_string();}
    }
}

#endif // CPPDATALIB_VALUE_H

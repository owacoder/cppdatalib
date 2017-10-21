#ifndef CPPDATALIB_VALUE_H
#define CPPDATALIB_VALUE_H

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

namespace cppdatalib
{
    namespace core
    {
        enum type
        {
            null,
            boolean,
            integer,
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
            map, // A normal object with integral keys (they're still stored as strings, but SHOULD all be valid decimal integers)

            user = 16
        };

        class value;
        class value_builder;
        class stream_handler;

        typedef bool bool_t;
        typedef int64_t int_t;
        typedef double real_t;
#define CPPDATALIB_REAL_DIG DBL_DIG
        typedef const char *cstring_t;
        typedef std::string string_t;
        typedef std::vector<value> array_t;
        typedef std::map<value, value> object_t;
        typedef long subtype_t;

        struct error
        {
            error(cstring_t reason) : what_(reason) {}

            cstring_t what() const {return what_;}

        private:
            cstring_t what_;
        };

        class value
        {
            friend stream_handler &convert(const value &v, stream_handler &handler);
            friend value &assign(value &dest, const value &src);

            struct traversal_reference
            {
                traversal_reference(value *p, array_t::iterator a, object_t::iterator o)
                    : p(p)
                    , array(a)
                    , object(o)
                {}

                value *p;
                array_t::iterator array;
                object_t::iterator object;
            };

            struct const_traversal_reference
            {
                const_traversal_reference(const value *p, array_t::const_iterator a, object_t::const_iterator o, bool traversed_key)
                    : p(p)
                    , array(a)
                    , object(o)
                    , traversed_key_already(traversed_key)
                {}

                const value *p;
                array_t::const_iterator array;
                object_t::const_iterator object;
                bool traversed_key_already;
            };

            template<typename PrefixPredicate, typename PostfixPredicate>
            void traverse_and_edit(PrefixPredicate prefix, PostfixPredicate postfix)
            {
                std::stack<traversal_reference, std::vector<traversal_reference>> references;
                value *p = this;

                while (!references.empty() || p != NULL)
                {
                    if (p != NULL)
                    {
                        prefix(p);

                        if (p->is_array())
                        {
                            references.push(traversal_reference(p, p->get_array().begin(), object_t::iterator()));
                            if (!p->get_array().empty())
                                p = std::addressof(*references.top().array++);
                            else
                                p = NULL;
                        }
                        else if (p->is_object())
                        {
                            references.push(traversal_reference(p, array_t::iterator(), p->get_object().begin()));
                            if (!p->get_object().empty())
                                p = std::addressof((references.top().object++)->second);
                            else
                                p = NULL;
                        }
                        else
                        {
                            references.push(traversal_reference(p, array_t::iterator(), object_t::iterator()));
                            p = NULL;
                        }
                    }
                    else
                    {
                        value *peek = references.top().p;
                        if (peek->is_array() && references.top().array != peek->get_array().end())
                            p = std::addressof(*references.top().array++);
                        else if (peek->is_object() && references.top().object != peek->get_object().end())
                            p = std::addressof((references.top().object++)->second);
                        else
                        {
                            references.pop();
                            postfix(peek);
                        }
                    }
                }
            }

            // TODO: this traversal algorithm does not traverse object keys, since they are declared `const` inside
            // the `std::map` definition. Using complex keys may overflow the stack when the destructor is called.
            // Using simple scalar keys will not be an issue
            template<typename PrefixPredicate, typename PostfixPredicate>
            void traverse(PrefixPredicate prefix, PostfixPredicate postfix) const
            {
                std::stack<const_traversal_reference, std::vector<const_traversal_reference>> references;
                const value *p = this;

                while (!references.empty() || p != NULL)
                {
                    if (p != NULL)
                    {
                        prefix(p);

                        if (p->is_array())
                        {
                            references.push(const_traversal_reference(p, p->get_array().begin(), object_t::const_iterator(), false));
                            if (!p->get_array().empty())
                                p = std::addressof(*references.top().array++);
                            else
                                p = NULL;
                        }
                        else if (p->is_object())
                        {
                            references.push(const_traversal_reference(p, array_t::const_iterator(), p->get_object().begin(), true));
                            if (!p->get_object().empty())
                                p = std::addressof(references.top().object->first);
                            else
                                p = NULL;
                        }
                        else
                        {
                            references.push(const_traversal_reference(p, array_t::const_iterator(), object_t::const_iterator(), false));
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
                            postfix(peek);
                        }
                    }
                }
            }

            static void traverse_node_null(value *) {}
            static void traverse_node_clear(value *arg) {arg->shallow_clear();}
            struct traverse_node_prefix_serialize;
            struct traverse_node_postfix_serialize;

        public:
            value() : type_(null), subtype_(0) {}
            value(bool_t v, subtype_t subtype = 0) : type_(boolean), bool_(v), subtype_(subtype) {}
            value(int_t v, subtype_t subtype = 0) : type_(integer), int_(v), subtype_(subtype) {}
            value(real_t v, subtype_t subtype = 0) : type_(real), real_(v), subtype_(subtype) {}
            value(cstring_t v, subtype_t subtype = 0) : type_(string), str_(v), subtype_(subtype) {}
            value(const string_t &v, subtype_t subtype = 0) : type_(string), str_(v), subtype_(subtype) {}
            value(string_t &&v, subtype_t subtype = 0) : type_(string), str_(v), subtype_(subtype) {}
            value(const array_t &v, subtype_t subtype = 0) : type_(array), arr_(v), subtype_(subtype) {}
            value(array_t &&v, subtype_t subtype = 0) : type_(array), arr_(v), subtype_(subtype) {}
            value(const object_t &v, subtype_t subtype = 0) : type_(object), obj_(v), subtype_(subtype) {}
            value(object_t &&v, subtype_t subtype = 0) : type_(object), obj_(v), subtype_(subtype) {}
            template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
            value(T v, subtype_t subtype = 0) : type_(integer), int_(v), subtype_(subtype) {}
            template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
            value(T v, subtype_t subtype = 0) : type_(real), real_(v), subtype_(subtype) {}
            ~value()
            {
                if ((type_ == array && arr_.size() > 0) ||
                    (type_ == object && obj_.size() > 0))
                    traverse_and_edit(traverse_node_null, traverse_node_clear);
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
            bool_t is_real() const {return type_ == real || type_ == integer;}
            bool_t is_string() const {return type_ == string;}
            bool_t is_array() const {return type_ == array;}
            bool_t is_object() const {return type_ == object;}

            bool_t get_bool() const {return bool_;}
            int_t get_int() const {return int_;}
            real_t get_real() const {return type_ == integer? int_: real_;}
            cstring_t get_cstring() const {return str_.c_str();}
            const string_t &get_string() const {return str_;}
            const array_t &get_array() const {return arr_;}
            const object_t &get_object() const {return obj_;}

            bool_t &get_bool() {clear(boolean); return bool_;}
            int_t &get_int() {clear(integer); return int_;}
            real_t &get_real() {clear(real); return real_;}
            string_t &get_string() {clear(string); return str_;}
            array_t &get_array() {clear(array); return arr_;}
            object_t &get_object() {clear(object); return obj_;}

            void set_null() {clear(null);}
            void set_bool(bool_t v) {clear(boolean); bool_ = v;}
            void set_int(int_t v) {clear(integer); int_ = v;}
            void set_real(real_t v) {clear(real); real_ = v;}
            void set_string(cstring_t v) {clear(string); str_ = v;}
            void set_string(const string_t &v) {clear(string); str_ = v;}
            void set_array(const array_t &v) {clear(array); arr_ = v;}
            void set_object(const object_t &v) {clear(object); obj_ = v;}

            void set_null(subtype_t subtype) {clear(null); subtype_ = subtype;}
            void set_bool(bool_t v, subtype_t subtype) {clear(boolean); bool_ = v; subtype_ = subtype;}
            void set_int(int_t v, subtype_t subtype) {clear(integer); int_ = v; subtype_ = subtype;}
            void set_real(real_t v, subtype_t subtype) {clear(real); real_ = v; subtype_ = subtype;}
            void set_string(cstring_t v, subtype_t subtype) {clear(string); str_ = v; subtype_ = subtype;}
            void set_string(const string_t &v, subtype_t subtype) {clear(string); str_ = v; subtype_ = subtype;}
            void set_array(const array_t &v, subtype_t subtype) {clear(array); arr_ = v; subtype_ = subtype;}
            void set_object(const object_t &v, subtype_t subtype) {clear(object); obj_ = v; subtype_ = subtype;}

            value operator[](const string_t &key) const
            {
                auto it = obj_.find(key);
                if (it != obj_.end())
                    return it->second;
                return value();
            }
            value &operator[](const string_t &key) {clear(object); return obj_[key];}
            value member(const value &key) const
            {
                auto it = obj_.find(key);
                if (it != obj_.end())
                    return it->second;
                return value();
            }
            value &member(const value &key) {clear(object); return obj_[key];}
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
            void erase_member(cstring_t key) {obj_.erase(key);}
            void erase_member(const string_t &key) {obj_.erase(key);}
            void erase_member(const value &key) {obj_.erase(key);}

            void push_back(const value &v) {clear(array); arr_.push_back(v);}
            void push_back(value &&v) {clear(array); arr_.push_back(v);}
            const value &operator[](size_t pos) const {return arr_[pos];}
            value &operator[](size_t pos) {return arr_[pos];}
            void erase_element(int_t pos) {arr_.erase(arr_.begin() + pos);}

            // The following are convenience conversion functions
            bool_t get_bool(bool_t default_) const {return is_bool()? bool_: default_;}
            int_t get_int(int_t default_) const {return is_int()? int_: default_;}
            real_t get_real(real_t default_) const {return is_real()? real_: default_;}
            cstring_t get_string(cstring_t default_) const {return is_string()? str_.c_str(): default_;}
            string_t get_string(const string_t &default_) const {return is_string()? str_: default_;}
            array_t get_array(const array_t &default_) const {return is_array()? arr_: default_;}
            object_t get_object(const object_t &default_) const {return is_object()? obj_: default_;}

            bool_t as_bool(bool_t default_ = false) const {return value(*this).convert_to(boolean, default_).bool_;}
            int_t as_int(int_t default_ = 0) const {return value(*this).convert_to(integer, default_).int_;}
            real_t as_real(real_t default_ = 0.0) const {return value(*this).convert_to(real, default_).real_;}
            string_t as_string(const string_t &default_ = string_t()) const {return value(*this).convert_to(string, default_).str_;}
            array_t as_array(const array_t &default_ = array_t()) const {return value(*this).convert_to(array, default_).arr_;}
            object_t as_object(const object_t &default_ = object_t()) const {return value(*this).convert_to(object, default_).obj_;}

            bool_t &convert_to_bool(bool_t default_ = false) {return convert_to(boolean, default_).bool_;}
            int_t &convert_to_int(int_t default_ = 0) {return convert_to(integer, default_).int_;}
            real_t &convert_to_real(real_t default_ = 0.0) {return convert_to(real, default_).real_;}
            string_t &convert_to_string(const string_t &default_ = string_t()) {return convert_to(string, default_).str_;}
            array_t &convert_to_array(const array_t &default_ = array_t()) {return convert_to(array, default_).arr_;}
            object_t &convert_to_object(const object_t &default_ = object_t()) {return convert_to(object, default_).obj_;}

        private:
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
                            case real: real_ = int_; break;
                            case string: str_ = std::to_string(int_); break;
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

            type type_;
            union
            {
                bool_t bool_;
                int_t int_;
                real_t real_;
            };
            string_t str_;
            array_t arr_;
            object_t obj_;
            subtype_t subtype_;
        };

        struct null_t : value {null_t() {}};

        // TODO: comparisons need to be non-recursive, iterative versions for stack overflow protection

        inline bool operator<(const value &lhs, const value &rhs)
        {
            if (lhs.get_type() < rhs.get_type())
                return true;
            else if (rhs.get_type() < lhs.get_type())
                return false;

            switch (lhs.get_type())
            {
                case null: return false;
                case boolean: return lhs.get_bool() < rhs.get_bool();
                case integer: return lhs.get_int() < rhs.get_int();
                case real: return lhs.get_real() < rhs.get_real();
                case string: return lhs.get_string() < rhs.get_string();
                case array: return lhs.get_array() < rhs.get_array();
                case object: return lhs.get_object() < rhs.get_object();
                default: return false;
            }
        }

        inline bool operator==(const value &lhs, const value &rhs)
        {
            if (lhs.get_type() != rhs.get_type())
                return false;

            switch (lhs.get_type())
            {
                case null: return true;
                case boolean: return lhs.get_bool() == rhs.get_bool();
                case integer: return lhs.get_int() == rhs.get_int();
                case real: return lhs.get_real() == rhs.get_real() || (std::isnan(lhs.get_real()) && std::isnan(rhs.get_real()));
                case string: return lhs.get_string() == rhs.get_string();
                case array: return lhs.get_array() == rhs.get_array();
                case object: return lhs.get_object() == rhs.get_object();
                default: return false;
            }
        }

        inline bool operator!=(const value &lhs, const value &rhs)
        {
            return !(lhs == rhs);
        }
    }
}

#endif // CPPDATALIB_VALUE_H

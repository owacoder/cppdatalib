#ifndef JSON_H
#define JSON_H

#include <string>
#include <vector>
#include <map>
#include <stack>
#include <iostream>
#include <sstream>
#include <codecvt>
#include <locale>
#include <type_traits>
#include <cmath>
#include <float.h>
#include <string.h>

namespace cppdatalib
{
    namespace hex
    {
        inline std::ostream &write(std::ostream &stream, unsigned char c)
        {
            const char alpha[] = "0123456789ABCDEF";

            return stream << alpha[(c & 0xff) >> 4] << alpha[c & 0xf];
        }

        inline std::ostream &write(std::ostream &stream, const std::string &str)
        {
            for (auto c: str)
                write(stream, c);

            return stream;
        }

        inline std::string encode(const std::string &str)
        {
            std::ostringstream stream;
            write(stream, str);
            return stream.str();
        }
    }

    namespace base64
    {
        inline std::ostream &write(std::ostream &stream, const std::string &str)
        {
            const char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            uint32_t temp;

            for (size_t i = 0; i < str.size();)
            {
                temp = (uint32_t) (str[i++] & 0xFF) << 16;
                if (i + 2 <= str.size())
                {
                    temp |= (uint32_t) (str[i++] & 0xFF) << 8;
                    temp |= (uint32_t) (str[i++] & 0xFF);
                    stream << alpha[(temp >> 18) & 0x3F]
                           << alpha[(temp >> 12) & 0x3F]
                           << alpha[(temp >>  6) & 0x3F]
                           << alpha[ temp        & 0x3F];
                }
                else if (i + 1 == str.size())
                {
                    temp |= (uint32_t) (str[i++] & 0xFF) << 8;
                    stream << alpha[(temp >> 18) & 0x3F]
                           << alpha[(temp >> 12) & 0x3F]
                           << alpha[(temp >>  6) & 0x3F]
                           << '=';
                }
                else if (i == str.size())
                {
                    stream << alpha[(temp >> 18) & 0x3F]
                           << alpha[(temp >> 12) & 0x3F]
                           << "==";
                }
            }

            return stream;
        }

        inline std::string encode(const std::string &str)
        {
            std::ostringstream stream;
            write(stream, str);
            return stream.str();
        }

        inline std::string decode(const std::string &str)
        {
            const std::string alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string result;
            size_t input = 0;
            uint32_t temp = 0;

            for (size_t i = 0; i < str.size(); ++i)
            {
                size_t pos = alpha.find(str[i]);
                if (pos == std::string::npos)
                    continue;
                temp |= (uint32_t) pos << (18 - 6 * input++);
                if (input == 4)
                {
                    result.push_back((temp >> 16) & 0xFF);
                    result.push_back((temp >>  8) & 0xFF);
                    result.push_back( temp        & 0xFF);
                    input = 0;
                    temp = 0;
                }
            }

            if (input > 1) result.push_back((temp >> 16) & 0xFF);
            if (input > 2) result.push_back((temp >>  8) & 0xFF);

            return result;
        }
    }

    namespace core
    {
        inline uint32_t float_cast_to_ieee_754(float f)
        {
            union
            {
                uint32_t output;
                float input;
            } result;

            result.input = f;
            return result.output;
        }

        inline float float_cast_from_ieee_754(uint32_t f)
        {
            union
            {
                uint32_t input;
                float output;
            } result;

            result.input = f;
            return result.output;
        }

        inline float float_from_ieee_754_half(uint16_t f)
        {
            const int32_t mantissa_mask = 0x3ff;
            const int32_t exponent_offset = 10;
            const int32_t exponent_mask = 0x1f;
            const int32_t sign_offset = 15;

            float result;
            int32_t exp, mantissa;

            // Extract exponent and mantissa
            exp = (f >> exponent_offset) & exponent_mask;
            mantissa = f & mantissa_mask;

            // Handle special cases
            if (exp == 0 && mantissa == 0) // 0, -0
                result = 0;
            else if (exp == exponent_mask) // +/- Infinity, NaN
                result = mantissa == 0? INFINITY: NAN;
            else
            {
                const int32_t normal = exp != 0;

                // `mantissa | (normal << exponent_offset)` evaluates to 1.mantissa if normalized number, and 0.mantissa if denormalized
                // `exp - exponent_offset - 14 - normal` is the adjusted exponent, removing the mantissa bias and the exponent bias (15)
                // (The mantissa bias is determined by exponent_offset and normal, equal to the binary length of the mantissa, plus the implicit leading 1
                //  for a normalized number. Denormalized numbers only have a leading 0.)

                result = std::ldexp(static_cast<float>(mantissa | (normal << exponent_offset)),
                                    exp - exponent_offset - 14 /* sic: the bias is adjusted for the following subtraction */ - normal);
            }

            return f >> sign_offset? -result: result;
        }

        inline uint16_t float_to_ieee_754_half(float f)
        {
            uint16_t result = 0;
            int exp;

            // Enter sign in result
            result |= static_cast<uint16_t>(std::signbit(f)) << 15;
            f = std::fabs(f);

            // Handle special cases
            if (f == 0)
                return result;
            else if (std::isinf(f))
                return result | (0x1ful << 10);
            else if (std::isnan(f))
                return result | (0x3ful << 9);

            // Then get exponent and significand, base 2
            f = std::frexp(f, &exp);

            // Enter exponent in result
            if (exp > -14) // Normalized number
            {
                if (exp + 14 >= 0x1f)
                    return result | (0x1fu << 10);

                result |= static_cast<uint16_t>(exp + 14) << 10;
                exp = 0;
            }
            else // Denormalized number, adjust shift count
                exp += 13;

            // Bias significand so we can extract it as an integer
            f *= std::exp2(11 + exp); // exp will actually be negative here, so this is effectively a subtraction
            result |= static_cast<uint16_t>(std::round(f)) & 0x3ff;

            return result;
        }

        inline float float_from_ieee_754(uint32_t f)
        {
            const int32_t mantissa_mask = 0x7fffff;
            const int32_t exponent_offset = 23;
            const int32_t exponent_mask = 0xff;
            const int32_t sign_offset = 31;

            float result;
            int32_t exp, mantissa;

            // Extract exponent and mantissa
            exp = (f >> exponent_offset) & exponent_mask;
            mantissa = f & mantissa_mask;

            // Handle special cases
            if (exp == 0 && mantissa == 0) // 0, -0
                result = 0;
            else if (exp == exponent_mask) // +/- Infinity, NaN
                result = mantissa == 0? INFINITY: NAN;
            else
            {
                const int32_t normal = exp != 0;

                // `mantissa | (normal << exponent_offset)` evaluates to 1.mantissa if normalized number, and 0.mantissa if denormalized
                // `exp - exponent_offset - 126 - normal` is the adjusted exponent, removing the mantissa bias and the exponent bias (127)
                // (The mantissa bias is determined by exponent_offset and normal, equal to the binary length of the mantissa, plus the implicit leading 1
                //  for a normalized number. Denormalized numbers only have a leading 0.)

                result = std::ldexp(static_cast<float>(mantissa | (normal << exponent_offset)),
                                    exp - exponent_offset - 126 /* sic: the bias is adjusted for the following subtraction */ - normal);
            }

            return f >> sign_offset? -result: result;
        }

        inline uint32_t float_to_ieee_754(float f)
        {
            uint32_t result = 0;
            int exp;

            // Enter sign in result
            result |= static_cast<uint32_t>(std::signbit(f)) << 31;
            f = std::fabs(f);

            // Handle special cases
            if (f == 0)
                return result;
            else if (std::isinf(f))
                return result | (0xfful << 23);
            else if (std::isnan(f))
                return result | (0x1fful << 22);

            // Then get exponent and significand, base 2
            f = std::frexp(f, &exp);

            // Enter exponent in result
            if (exp > -126) // Normalized number
            {
                if (exp + 126 >= 0xff)
                    return result | (0xfful << 23);

                result |= static_cast<uint32_t>((exp + 126) & 0xff) << 23;
                exp = 0;
            }
            else // Denormalized number, adjust shift count
                exp += 125;

            // Bias significand so we can extract it as an integer
            f *= std::exp2(24 + exp); // exp will actually be negative here, so this is effectively a subtraction
            result |= static_cast<uint32_t>(std::round(f)) & 0x7fffff;

            return result;
        }

        inline uint64_t double_cast_to_ieee_754(double d)
        {
            union
            {
                uint64_t output;
                double input;
            } result;

            result.input = d;
            return result.output;
        }

        inline double double_cast_from_ieee_754(uint64_t d)
        {
            union
            {
                uint64_t input;
                double output;
            } result;

            result.input = d;
            return result.output;
        }

        inline double double_from_ieee_754(uint64_t f)
        {
            const int64_t mantissa_mask = 0xfffffffffffff;
            const int64_t exponent_offset = 52;
            const int64_t exponent_mask = 0x7ff;
            const int64_t sign_offset = 63;

            double result;
            int64_t exp, mantissa;

            // Extract exponent and mantissa
            exp = (f >> exponent_offset) & exponent_mask;
            mantissa = f & mantissa_mask;

            // Handle special cases
            if (exp == 0 && mantissa == 0) // 0, -0
                result = 0;
            else if (exp == exponent_mask) // +/- Infinity, NaN
                result = mantissa == 0? INFINITY: NAN;
            else
            {
                const int64_t normal = exp != 0;

                // `mantissa | (normal << exponent_offset)` evaluates to 1.mantissa if normalized number, and 0.mantissa if denormalized
                // `exp - exponent_offset - 1022 - normal` is the adjusted exponent, removing the mantissa bias and the exponent bias (1023)
                // (The mantissa bias is determined by exponent_offset and normal, equal to the binary length of the mantissa, plus the implicit leading 1
                //  for a normalized number. Denormalized numbers only have a leading 0.)

                result = std::ldexp(static_cast<double>(mantissa | (normal << exponent_offset)),
                                    exp - exponent_offset - 1022 /* sic: the bias is adjusted for the following subtraction */ - normal);
            }

            return f >> sign_offset? -result: result;
        }

        inline uint64_t double_to_ieee_754(double d)
        {
            uint64_t result = 0;
            int exp;

            // Enter sign in result
            result |= static_cast<uint64_t>(std::signbit(d)) << 63;
            d = std::fabs(d);

            // Handle special cases
            if (d == 0)
                return result;
            else if (std::isinf(d))
                return result | (0x7ffull << 52);
            else if (std::isnan(d))
                return result | (0xfffull << 51);

            // Then get exponent and significand, base 2
            d = std::frexp(d, &exp);

            // Enter exponent in result
            if (exp > -1022) // Normalized number
            {
                if (exp + 1022 >= 0x7ff)
                    return result | (0x7ffull << 52);

                result |= static_cast<uint64_t>((exp + 1022) & 0x7ff) << 52;
                exp = 0;
            }
            else // Denormalized number, adjust shift count
                exp += 1021;

            // Bias significand so we can extract it as an integer
            d *= std::exp2(53 + exp); // exp will actually be negative here, so this is effectively a subtraction
            result |= static_cast<uint64_t>(std::round(d)) & ((1ull << 52) - 1);

            return result;
        }

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
        typedef const char *cstring_t;
        typedef std::string string_t;
        typedef std::vector<value> array_t;
        typedef std::map<value, value> object_t;

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
                traversal_reference(value *p, array_t::iterator a, object_t::iterator o, bool traversed_key)
                    : p(p)
                    , array(a)
                    , object(o)
                    , traversed_key_already(traversed_key)
                {}

                value *p;
                array_t::iterator array;
                object_t::iterator object;
                bool traversed_key_already;
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

            // TODO: this traversal algorithm does not traverse object keys, since they are declared `const` inside
            // the `std::map` definition. Using complex keys may overflow the stack when the destructor is called.
            // Using simple scalar keys will not be an issue
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
                            references.push(traversal_reference(p, p->get_array().begin(), object_t::iterator(), false));
                            if (!p->get_array().empty())
                                p = std::addressof(*references.top().array++);
                            else
                                p = NULL;
                        }
                        else if (p->is_object())
                        {
                            references.push(traversal_reference(p, array_t::iterator(), p->get_object().begin(), false));
                            if (!p->get_object().empty())
                                p = std::addressof((references.top().object++)->second);
                            else
                                p = NULL;
                        }
                        else
                        {
                            references.push(traversal_reference(p, array_t::iterator(), object_t::iterator(), false));
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
            value(bool_t v, long subtype = 0) : type_(boolean), bool_(v), subtype_(subtype) {}
            value(int_t v, long subtype = 0) : type_(integer), int_(v), subtype_(subtype) {}
            value(real_t v, long subtype = 0) : type_(real), real_(v), subtype_(subtype) {}
            value(cstring_t v, long subtype = 0) : type_(string), str_(v), subtype_(subtype) {}
            value(const string_t &v, long subtype = 0) : type_(string), str_(v), subtype_(subtype) {}
            value(string_t &&v, long subtype = 0) : type_(string), str_(v), subtype_(subtype) {}
            value(const array_t &v, long subtype = 0) : type_(array), arr_(v), subtype_(subtype) {}
            value(array_t &&v, long subtype = 0) : type_(array), arr_(v), subtype_(subtype) {}
            value(const object_t &v, long subtype = 0) : type_(object), obj_(v), subtype_(subtype) {}
            value(object_t &&v, long subtype = 0) : type_(object), obj_(v), subtype_(subtype) {}
            template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
            value(T v, long subtype = 0) : type_(integer), int_(v), subtype_(subtype) {}
            template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
            value(T v, long subtype = 0) : type_(real), real_(v), subtype_(subtype) {}
            ~value()
            {
                if (get_type() == array || get_type() == object)
                    traverse_and_edit(traverse_node_null, traverse_node_clear);
            }

            value(const value &other) : type_(null), subtype_(0) {assign(*this, other);}
            value(value &&other) = default;
            value &operator=(const value &other) {return assign(*this, other);}

            long get_subtype() const {return subtype_;}
            long &get_subtype() {return subtype_;}
            void set_subtype(long _type) {subtype_ = _type;}

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

            void set_null(long subtype) {clear(null); subtype_ = subtype;}
            void set_bool(bool_t v, long subtype) {clear(boolean); bool_ = v; subtype_ = subtype;}
            void set_int(int_t v, long subtype) {clear(integer); int_ = v; subtype_ = subtype;}
            void set_real(real_t v, long subtype) {clear(real); real_ = v; subtype_ = subtype;}
            void set_string(cstring_t v, long subtype) {clear(string); str_ = v; subtype_ = subtype;}
            void set_string(const string_t &v, long subtype) {clear(string); str_ = v; subtype_ = subtype;}
            void set_array(const array_t &v, long subtype) {clear(array); arr_ = v; subtype_ = subtype;}
            void set_object(const object_t &v, long subtype) {clear(object); obj_ = v; subtype_ = subtype;}

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

            value &convert_to(type new_type, value default_value)
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
                            case string: str_ = std::to_string(real_); break;
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
            long subtype_;
        };

        struct null_t : value {null_t() {}};

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

        inline bool stream_starts_with(std::istream &stream, const char *str)
        {
            int c;
            while (*str)
            {
                c = stream.get();
                if ((*str++ & 0xff) != c) return false;
            }
            return true;
        }

        class stream_writer
        {
        protected:
            std::ostream &output_stream;

        public:
            stream_writer(std::ostream &stream) : output_stream(stream) {}

            std::ostream &stream() {return output_stream;}
        };

        class stream_handler
        {
        protected:
            struct scope_data
            {
                scope_data(type t, bool parsed_key = false)
                    : type_(t)
                    , parsed_key_(parsed_key)
                    , items_(0)
                {}

                type get_type() const {return type_;}
                size_t items_parsed() const {return items_;}
                bool key_was_parsed() const {return parsed_key_;}

                type type_; // The type of container that is being parsed
                bool parsed_key_; // false if the object key needs to be or is being parsed, true if it has already been parsed but the value associated with it has not
                size_t items_; // The number of items parsed into this container
            };

        public:
            enum {
                unknown_size = -1
            };

            void begin()
            {
                while (!nested_scopes.empty())
                    nested_scopes.pop();
                begin_();
            }
            void end()
            {
                if (!nested_scopes.empty())
                    throw error("cppdatalib::stream_handler - unexpected end of stream");
                end_();
            }

        protected:
            virtual void begin_() {}
            virtual void end_() {}

        public:
            size_t nesting_depth() const {return nested_scopes.size();}

            type current_container() const
            {
                if (nested_scopes.empty())
                    return null;
                return nested_scopes.top().get_type();
            }

            size_t current_container_size() const
            {
                if (nested_scopes.empty())
                    return 0;
                return nested_scopes.top().items_parsed();
            }

            bool container_key_was_just_parsed() const
            {
                if (nested_scopes.empty())
                    return false;
                return nested_scopes.top().key_was_parsed();
            }

            // An API must call this when a scalar value is encountered.
            // Returns true if value was handled, false otherwise
            bool write(const value &v)
            {
                const bool is_key =
                       !nested_scopes.empty() &&
                        nested_scopes.top().type_ == object &&
                       !nested_scopes.top().key_was_parsed();

                if (is_key)
                    begin_key_(v);
                else
                    begin_item_(v);
                begin_scalar_(v, is_key);

                switch (v.get_type())
                {
                    case null: begin_null_(v); null_(v); end_null_(v); break;
                    case boolean: begin_bool_(v); bool_(v); end_bool_(v); break;
                    case integer: begin_integer_(v); integer_(v); end_integer_(v); break;
                    case real: begin_real_(v); real_(v); end_real_(v); break;
                    case string: begin_string_(v, v.size(), is_key); string_data_(v); end_string_(v, is_key); break;
                    default: return false;
                }

                end_scalar_(v, is_key);
                if (is_key)
                    end_key_(v);
                else
                    end_item_(v);

                if (!nested_scopes.empty())
                {
                    if (nested_scopes.top().get_type() == object)
                    {
                        nested_scopes.top().items_ += !is_key;
                        nested_scopes.top().parsed_key_ = !nested_scopes.top().key_was_parsed();
                    }
                    else
                        ++nested_scopes.top().items_;
                }

                return true;
            }

        protected:
            // Called when any non-key item is parsed
            virtual void begin_item_(const core::value &v) {(void) v;}
            virtual void end_item_(const core::value &v) {(void) v;}

            // Called when any non-array, non-object item is parsed
            virtual void begin_scalar_(const core::value &v, bool is_key) {(void) v; (void) is_key;}
            virtual void end_scalar_(const core::value &v, bool is_key) {(void) v; (void) is_key;}

            // Called when object keys are parsed. Keys may be complex, and have other calls within these events.
            virtual void begin_key_(const core::value &v) {(void) v;}
            virtual void end_key_(const core::value &v) {(void) v;}

            // Called when a scalar null is parsed
            virtual void begin_null_(const core::value &v) {(void) v;}
            virtual void null_(const core::value &v) {(void) v;}
            virtual void end_null_(const core::value &v) {(void) v;}

            // Called when a scalar boolean is parsed
            virtual void begin_bool_(const core::value &v) {(void) v;}
            virtual void bool_(const core::value &v) {(void) v;}
            virtual void end_bool_(const core::value &v) {(void) v;}

            // Called when a scalar integer is parsed
            virtual void begin_integer_(const core::value &v) {(void) v;}
            virtual void integer_(const core::value &v) {(void) v;}
            virtual void end_integer_(const core::value &v) {(void) v;}

            // Called when a scalar real is parsed
            virtual void begin_real_(const core::value &v) {(void) v;}
            virtual void real_(const core::value &v) {(void) v;}
            virtual void end_real_(const core::value &v) {(void) v;}

            // Called when a scalar string is parsed
            virtual void begin_string_(const core::value &v, core::int_t size, bool is_key) {(void) v; (void) size; (void) is_key;}
            virtual void string_data_(const core::value &v) {(void) v;}
            virtual void end_string_(const core::value &v, bool is_key) {(void) v; (void) is_key;}

        public:
            // An API must call these when a long string is parsed. The number of bytes is passed in size, if possible
            // size < 0 means unknown size
            void begin_string(const core::value &v, core::int_t size)
            {
                if (!nested_scopes.empty() &&
                     nested_scopes.top().type_ == object &&
                    !nested_scopes.top().key_was_parsed())
                {
                    begin_key_(v);
                    begin_string_(v, size, true);
                }
                else
                {
                    begin_item_(v);
                    begin_string_(v, size, false);
                }

                if (v.size())
                    string_data_(v);

                nested_scopes.push(string);
            }
            // An API must call these when a long string is parsed. The number of bytes of this chunk is passed in size, if possible
            void append_to_string(const core::value &v)
            {
                if (nested_scopes.empty() || nested_scopes.top().get_type() != string)
                    throw error("cppdatalib::stream_handler - attempted to append to string that was never begun");

                string_data_(v);
                nested_scopes.top().items_ += v.get_string().size();
            }
            void end_string(const core::value &v)
            {
                if (nested_scopes.empty() || nested_scopes.top().get_type() != string)
                    throw error("cppdatalib::stream_handler - attempted to end string that was never begun");

                // Pop the top scope off so we can look at the parent scope, but put it back on for the callbacks
                auto save = nested_scopes.top();
                nested_scopes.pop();

                if (!nested_scopes.empty() &&
                     nested_scopes.top().get_type() == object &&
                    !nested_scopes.top().key_was_parsed())
                {
                    nested_scopes.push(save);
                    end_string_(v, true);
                    end_key_(v);
                }
                else
                {
                    nested_scopes.push(save);
                    end_string_(v, false);
                    end_item_(v);
                }
                nested_scopes.pop();

                if (!nested_scopes.empty())
                {
                    if (nested_scopes.top().get_type() == object)
                    {
                        nested_scopes.top().items_ += nested_scopes.top().key_was_parsed();
                        nested_scopes.top().parsed_key_ = !nested_scopes.top().key_was_parsed();
                    }
                    else
                        ++nested_scopes.top().items_;
                }
            }

            // An API must call these when an array is parsed. The number of elements is passed in size, if possible
            // size < 0 means unknown size
            void begin_array(const core::value &v, core::int_t size)
            {
                if (!nested_scopes.empty() &&
                     nested_scopes.top().type_ == object &&
                    !nested_scopes.top().key_was_parsed())
                {
                    begin_key_(v);
                    begin_array_(v, size, true);
                }
                else
                {
                    begin_item_(v);
                    begin_array_(v, size, false);
                }

                nested_scopes.push(array);
            }
            void end_array(const core::value &v)
            {
                if (nested_scopes.empty() || nested_scopes.top().get_type() != array)
                    throw error("cppdatalib::stream_handler - attempted to end array that was never begun");

                // Pop the top scope off so we can look at the parent scope, but put it back on for the callbacks
                auto save = nested_scopes.top();
                nested_scopes.pop();

                if (!nested_scopes.empty() &&
                     nested_scopes.top().get_type() == object &&
                    !nested_scopes.top().key_was_parsed())
                {
                    nested_scopes.push(save);
                    end_array_(v, true);
                    end_key_(v);
                }
                else
                {
                    nested_scopes.push(save);
                    end_array_(v, false);
                    end_item_(v);
                }
                nested_scopes.pop();

                if (!nested_scopes.empty())
                {
                    if (nested_scopes.top().get_type() == object)
                    {
                        nested_scopes.top().items_ += nested_scopes.top().key_was_parsed();
                        nested_scopes.top().parsed_key_ = !nested_scopes.top().key_was_parsed();
                    }
                    else
                        ++nested_scopes.top().items_;
                }
            }

            // An API must call these when an object is parsed. The number of key/value pairs is passed in size, if possible
            // size < 0 means unknown size
            void begin_object(const core::value &v, core::int_t size)
            {
                if (!nested_scopes.empty() &&
                     nested_scopes.top().type_ == object &&
                    !nested_scopes.top().key_was_parsed())
                {
                    begin_key_(v);
                    begin_object_(v, size, true);
                }
                else
                {
                    begin_item_(v);
                    begin_object_(v, size, false);
                }

                nested_scopes.push(object);
            }
            void end_object(const core::value &v)
            {
                if (nested_scopes.empty() || nested_scopes.top().get_type() != object)
                    throw error("cppdatalib::stream_handler - attempted to end object that was never begun");
                if (nested_scopes.top().key_was_parsed())
                    throw error("cppdatalib::stream_handler - attempted to end object before final value was written");

                // Pop the top scope off so we can look at the parent scope, but put it back on for the callbacks
                auto save = nested_scopes.top();
                nested_scopes.pop();

                if (!nested_scopes.empty() &&
                     nested_scopes.top().get_type() == object &&
                    !nested_scopes.top().key_was_parsed())
                {
                    nested_scopes.push(save);
                    end_object_(v, true);
                    end_key_(v);
                }
                else
                {
                    nested_scopes.push(save);
                    end_object_(v, false);
                    end_item_(v);
                }
                nested_scopes.pop();

                if (!nested_scopes.empty())
                {
                    if (nested_scopes.top().get_type() == object)
                    {
                        nested_scopes.top().items_ += nested_scopes.top().key_was_parsed();
                        nested_scopes.top().parsed_key_ = !nested_scopes.top().key_was_parsed();
                    }
                    else
                        ++nested_scopes.top().items_;
                }
            }

        protected:
            // Overloads to detect beginnings and ends of arrays
            virtual void begin_array_(const core::value &v, core::int_t size, bool is_key) {(void) v; (void) size; (void) is_key;}
            virtual void end_array_(const core::value &v, bool is_key) {(void) v; (void) is_key;}

            // Overloads to detect beginnings and ends of objects
            virtual void begin_object_(const core::value &v, core::int_t size, bool is_key) {(void) v; (void) size; (void) is_key;}
            virtual void end_object_(const core::value &v, bool is_key) {(void) v; (void) is_key;}

            std::stack<scope_data, std::vector<scope_data>> nested_scopes;
        };

        class value_builder : public core::stream_handler
        {
            core::value &v;

            std::stack<core::value, std::vector<core::value>> keys;
            std::stack<core::value *, std::vector<core::value *>> references;

        public:
            value_builder(core::value &bind) : v(bind) {}

            // begin_() clears the bound value to null and pushes a reference to it
            void begin_()
            {
                while (!keys.empty())
                    keys.pop();
                while (!references.empty())
                    references.pop();

                v.set_null();
                references.push(&this->v);
            }

            const core::value &value() const {return v;}

            // begin_key_() just queues a new object key in the stack
            void begin_key_(const core::value &v)
            {
                keys.push(v);
                references.push(&keys.top());
            }
            void end_key_(const core::value &)
            {
                references.pop();
            }

            // begin_scalar_() pushes the item to the array if the object to be modified is an array,
            // adds a member with the specified key, or simply assigns if not in a container
            void begin_scalar_(const core::value &v, bool is_key)
            {
                if (!is_key && current_container() == array)
                    references.top()->push_back(v);
                else if (!is_key && current_container() == object)
                {
                    references.top()->member(keys.top()) = v;
                    keys.pop();
                }
                else if (v.get_type() != string)
                    *references.top() = v;
            }

            void string_data_(const core::value &v)
            {
                references.top()->get_string() += v.get_string();
            }

            // begin_container() operates similarly to begin_scalar_(), but pushes a reference to the container as well
            void begin_container(const core::value &v, core::int_t size, bool is_key)
            {
                (void) size;

                if (!is_key && current_container() == array)
                {
                    references.top()->push_back(core::null_t());
                    references.push(&references.top()->get_array().back());
                }
                else if (!is_key && current_container() == object)
                {
                    references.top()->member(keys.top());
                    references.push(&references.top()->member(keys.top()));
                    keys.pop();
                }

                // WARNING: If one tries to perform the following assignment `*references.top() = v` here,
                // an infinite recursion will result, because the `core::value` assignment operator and the
                // `core::value` copy constructor use this class to build complex (array or object) types.
                if (v.is_array())
                    references.top()->set_array(core::array_t(), v.get_subtype());
                else if (v.is_object())
                    references.top()->set_object(core::object_t(), v.get_subtype());
            }

            // end_container_() just removes a container from the stack, because nothing more needs to be done
            void end_container(bool is_key)
            {
                if (!is_key)
                    references.pop();
            }

            void begin_array_(const core::value &v, core::int_t size, bool is_key) {begin_container(v, size, is_key);}
            void end_array_(const core::value &, bool is_key) {end_container(is_key);}
            void begin_object_(const core::value &v, core::int_t size, bool is_key) {begin_container(v, size, is_key);}
            void end_object_(const core::value &, bool is_key) {end_container(is_key);}
        };

        struct value::traverse_node_prefix_serialize
        {
            traverse_node_prefix_serialize(stream_handler &handler) : stream(handler) {}

            void operator()(const value *arg)
            {
                if (arg->is_array())
                    stream.begin_array(*arg, stream_handler::unknown_size);
                else if (arg->is_object())
                    stream.begin_object(*arg, stream_handler::unknown_size);
            }

        private:
            stream_handler &stream;
        };

        struct value::traverse_node_postfix_serialize
        {
            traverse_node_postfix_serialize(stream_handler &handler) : stream(handler) {}

            void operator()(const value *arg)
            {
                if (arg->is_array())
                    stream.end_array(*arg);
                else if (arg->is_object())
                    stream.end_object(*arg);
                else
                    stream.write(*arg);
            }

        private:
            stream_handler &stream;
        };

        inline stream_handler &convert(const value &v, stream_handler &handler)
        {
            value::traverse_node_prefix_serialize prefix(handler);
            value::traverse_node_postfix_serialize postfix(handler);

            handler.begin();
            v.traverse(prefix, postfix);
            handler.end();

            return handler;
        }

        inline value &assign(value &dst, const value &src)
        {
            switch (src.get_type())
            {
                case null: dst.set_null(); return dst;
                case boolean: dst.set_bool(src.get_bool(), src.get_subtype()); return dst;
                case integer: dst.set_int(src.get_int(), src.get_subtype()); return dst;
                case real: dst.set_real(src.get_real(), src.get_subtype()); return dst;
                case string: dst.set_string(src.get_string(), src.get_subtype()); return dst;
                case array:
                case object:
                {
                    value_builder builder(dst);
                    convert(src, builder);
                    return dst;
                }
                default: dst.set_null(); return dst;
            }
        }
    }

    namespace json
    {
        inline std::istream &read_string(std::istream &stream, core::stream_handler &writer)
        {
            static const std::string hex = "0123456789ABCDEF";
            int c;
            std::string buffer;

            {char chr; stream >> chr; c = chr;} // Skip initial whitespace
            if (c != '"') throw core::error("JSON - expected string");

            writer.begin_string(core::string_t(), core::stream_handler::unknown_size);
            while (c = stream.get(), c != '"')
            {
                if (c == EOF) throw core::error("JSON - unexpected end of string");

                if (c == '\\')
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("JSON - unexpected end of string");

                    switch (c)
                    {
                        case 'b': buffer.push_back('\b'); break;
                        case 'f': buffer.push_back('\f'); break;
                        case 'n': buffer.push_back('\n'); break;
                        case 'r': buffer.push_back('\r'); break;
                        case 't': buffer.push_back('\t'); break;
                        case 'u':
                        {
                            uint32_t code = 0;
                            for (int i = 0; i < 4; ++i)
                            {
                                c = stream.get();
                                if (c == EOF) throw core::error("JSON - unexpected end of string");
                                size_t pos = hex.find(toupper(c));
                                if (pos == std::string::npos) throw core::error("JSON - invalid character escape sequence");
                                code = (code << 4) | pos;
                            }

                            std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
                            buffer += utf8.to_bytes(code);

                            break;
                        }
                        default:
                            buffer.push_back(c);
                            break;
                    }
                }
                else
                    buffer.push_back(c);

                if (buffer.size() >= 65536)
                    writer.append_to_string(buffer), buffer.clear();
            }

            //writer.append_to_string(buffer);
            writer.end_string(core::string_t());
            return stream;
        }

        inline std::ostream &write_string(std::ostream &stream, const std::string &str)
        {
            for (size_t i = 0; i < str.size(); ++i)
            {
                int c = str[i] & 0xff;

                if (c == '"' || c == '\\')
                    stream << '\\' << static_cast<char>(c);
                else
                {
                    switch (c)
                    {
                        case '"':
                        case '\\': stream << '\\' << static_cast<char>(c); break;
                        case '\b': stream << "\\b"; break;
                        case '\f': stream << "\\f"; break;
                        case '\n': stream << "\\n"; break;
                        case '\r': stream << "\\r"; break;
                        case '\t': stream << "\\t"; break;
                        default:
                            if (iscntrl(c))
                                hex::write(stream << "\\u00", c);
                            else
                                stream << str[i];
                            break;
                    }
                }
            }

            return stream;
        }

        inline std::istream &convert(std::istream &stream, core::stream_handler &writer)
        {
            bool delimiter_required = false;
            char chr;

            writer.begin();

            while (stream >> std::skipws >> chr, stream.unget(), stream.good() && !stream.eof())
            {
                if (writer.nesting_depth() == 0 && delimiter_required)
                    break;

                if (delimiter_required && !strchr(",:]}", chr))
                    throw core::error("JSON - expected ',' separating array or object entries");

                switch (chr)
                {
                    case 'n':
                        if (!core::stream_starts_with(stream, "null")) throw core::error("JSON - expected 'null' value");
                        writer.write(core::null_t());
                        delimiter_required = true;
                        break;
                    case 't':
                        if (!core::stream_starts_with(stream, "true")) throw core::error("JSON - expected 'true' value");
                        writer.write(true);
                        delimiter_required = true;
                        break;
                    case 'f':
                        if (!core::stream_starts_with(stream, "false")) throw core::error("JSON - expected 'false' value");
                        writer.write(false);
                        delimiter_required = true;
                        break;
                    case '"':
                        read_string(stream, writer);
                        delimiter_required = true;
                        break;
                    case ',':
                        stream >> chr; // Eat ','
                        if (writer.current_container_size() == 0 || writer.container_key_was_just_parsed())
                            throw core::error("JSON - invalid ',' does not separate array or object entries");
                        stream >> chr; stream.unget(); // Peek ahead
                        if (!stream || chr == ',' || chr == ']' || chr == '}')
                            throw core::error("JSON - invalid ',' does not separate array or object entries");
                        delimiter_required = false;
                        break;
                    case ':':
                        stream >> chr; // Eat ':'
                        if (!writer.container_key_was_just_parsed())
                            throw core::error("JSON - invalid ':' does not separate a key and value pair");
                        delimiter_required = false;
                        break;
                    case '[':
                        stream >> chr; // Eat '['
                        writer.begin_array(core::array_t(), core::stream_handler::unknown_size);
                        delimiter_required = false;
                        break;
                    case ']':
                        stream >> chr; // Eat ']'
                        writer.end_array(core::array_t());
                        delimiter_required = true;
                        break;
                    case '{':
                        stream >> chr; // Eat '{'
                        writer.begin_object(core::object_t(), core::stream_handler::unknown_size);
                        delimiter_required = false;
                        break;
                    case '}':
                        stream >> chr; // Eat '}'
                        writer.end_object(core::object_t());
                        delimiter_required = true;
                        break;
                    default:
                        if (isdigit(chr) || chr == '-')
                        {
                            core::real_t r;
                            stream >> r;
                            if (!stream) throw core::error("JSON - invalid number");

                            if (r == trunc(r) && r >= INT64_MIN && r <= INT64_MAX)
                                writer.write(static_cast<core::int_t>(r));
                            else
                                writer.write(r);

                            delimiter_required = true;
                        }
                        else
                            throw core::error("JSON - expected value");
                        break;
                }
            }

            writer.end();
            return stream;
        }

        class stream_writer : public core::stream_handler, public core::stream_writer
        {
        public:
            stream_writer(std::ostream &output) : core::stream_writer(output) {}

        protected:
            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    output_stream << ':';
                else if (current_container_size() > 0)
                    output_stream << ',';
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    output_stream << ',';

                if (!v.is_string())
                    throw core::error("JSON - cannot write non-string key");
            }

            void null_(const core::value &) {output_stream << "null";}
            void bool_(const core::value &v) {output_stream << (v.get_bool()? "true": "false");}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void real_(const core::value &v) {output_stream << v.get_real();}
            void begin_string_(const core::value &, core::int_t, bool) {output_stream << '"';}
            void string_data_(const core::value &v) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &, bool) {output_stream << '"';}

            void begin_array_(const core::value &, core::int_t, bool) {output_stream << '[';}
            void end_array_(const core::value &, bool) {output_stream << ']';}

            void begin_object_(const core::value &, core::int_t, bool) {output_stream << '{';}
            void end_object_(const core::value &, bool) {output_stream << '}';}
        };

        class pretty_stream_writer : public core::stream_handler, public core::stream_writer
        {
            size_t indent_width;
            size_t current_indent;

        public:
            pretty_stream_writer(std::ostream &output, size_t indent_width)
                : core::stream_writer(output)
                , indent_width(indent_width)
                , current_indent(0)
            {}

            size_t indent() {return indent_width;}

        protected:
            void begin_() {current_indent = 0;}

            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    output_stream << ": ";
                else if (current_container_size() > 0)
                    output_stream << ',';

                if (current_container() == core::array)
                    output_stream << '\n' << std::string(current_indent, ' ');
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    output_stream << ',';

                output_stream << '\n' << std::string(current_indent, ' ');

                if (!v.is_string())
                    throw core::error("JSON - cannot write non-string key");
            }

            void null_(const core::value &) {output_stream << "null";}
            void bool_(const core::value &v) {output_stream << (v.get_bool()? "true": "false");}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void real_(const core::value &v) {output_stream << v.get_real();}
            void begin_string_(const core::value &, core::int_t, bool) {output_stream << '"';}
            void string_data_(const core::value &v) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &, bool) {output_stream << '"';}

            void begin_array_(const core::value &, core::int_t, bool)
            {
                output_stream << '[';
                current_indent += indent_width;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream << '\n' << std::string(current_indent, ' ');

                output_stream << ']';
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                output_stream << '{';
                current_indent += indent_width;
            }
            void end_object_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream << '\n' << std::string(current_indent, ' ');

                output_stream << '}';
            }
        };

        inline std::istream &operator>>(std::istream &stream, core::value &v)
        {
            core::value_builder builder(v);
            convert(stream, builder);
            return stream;
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            stream_writer writer(stream);
            core::convert(v, writer);
            return stream;
        }

        inline std::istream &input(std::istream &stream, core::value &v) {return stream >> v;}
        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline std::ostream &pretty_print(std::ostream &stream, const core::value &v, size_t indent_width)
        {
            pretty_stream_writer writer(stream, indent_width);
            core::convert(v, writer);
            return stream;
        }

        inline core::value from_json(const std::string &json)
        {
            std::istringstream stream(json);
            core::value v;
            stream >> v;
            return v;
        }

        inline std::string to_json(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }

        inline std::string to_pretty_json(const core::value &v, size_t indent_width)
        {
            std::ostringstream stream;
            pretty_stream_writer writer(stream, indent_width);
            core::convert(v, writer);
            return stream.str();
        }
    }

    namespace bencode
    {
        inline std::istream &convert(std::istream &stream, core::stream_handler &writer)
        {
            char chr;

            writer.begin();

            while (stream >> std::skipws >> chr, stream.unget(), stream.good() && !stream.eof())
            {
                switch (chr)
                {
                    case 'i':
                    {
                        stream >> chr; // Eat 'i'

                        core::int_t i;
                        stream >> i;
                        if (!stream) throw core::error("Bencode - expected 'integer' value");

                        writer.write(i);
                        if (stream.get() != 'e') throw core::error("Bencode - invalid 'integer' value");
                        break;
                    }
                    case 'e':
                        stream >> chr; // Eat 'e'
                        switch (writer.current_container())
                        {
                            case core::array: writer.end_array(core::array_t()); break;
                            case core::object: writer.end_object(core::object_t()); break;
                            default: throw core::error("Bencode - attempt to end element does not exist"); break;
                        }
                        break;
                    case 'l':
                        stream >> chr; // Eat 'l'
                        writer.begin_array(core::array_t(), core::stream_handler::unknown_size);
                        break;
                    case 'd':
                        stream >> chr; // Eat 'd'
                        writer.begin_object(core::object_t(), core::stream_handler::unknown_size);
                        break;
                    default:
                        if (isdigit(chr))
                        {
                            core::int_t size;

                            stream >> size;
                            if (size < 0) throw core::error("Bencode - expected string size");
                            if (stream.get() != ':') throw core::error("Bencode - expected ':' separating string size and data");

                            writer.begin_string(core::string_t(), size);
                            while (size--)
                            {
                                chr = stream.get();
                                if (chr == EOF) throw core::error("Bencode - unexpected end of string");
                                writer.append_to_string(core::string_t(1, chr));
                            }
                            writer.end_string(core::string_t());
                        }
                        else
                            throw core::error("Bencode - expected value");
                        break;
                }

                if (writer.nesting_depth() == 0)
                    break;
            }

            writer.end();
            return stream;
        }

        class stream_writer : public core::stream_handler, public core::stream_writer
        {
        public:
            stream_writer(std::ostream &output) : core::stream_writer(output) {}

        protected:
            void begin_key_(const core::value &v)
            {
                if (!v.is_string())
                    throw core::error("Bencode - cannot write non-string key");
            }

            void null_(const core::value &) {throw core::error("Bencode - 'null' value not allowed in output");}
            void bool_(const core::value &) {throw core::error("Bencode - 'boolean' value not allowed in output");}
            void integer_(const core::value &v) {output_stream << 'i' << v.get_int() << 'e';}
            void real_(const core::value &) {throw core::error("Bencode - 'real' value not allowed in output");}
            void begin_string_(const core::value &, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("Bencode - 'string' value does not have size specified");
                output_stream << size << ':';
            }
            void string_data_(const core::value &v) {output_stream << v.get_string();}

            void begin_array_(const core::value &, core::int_t, bool) {output_stream << 'l';}
            void end_array_(const core::value &, bool) {output_stream << 'e';}

            void begin_object_(const core::value &, core::int_t, bool) {output_stream << 'd';}
            void end_object_(const core::value &, bool) {output_stream << 'e';}
        };

        inline std::istream &operator>>(std::istream &stream, core::value &v)
        {
            core::value_builder builder(v);
            convert(stream, builder);
            return stream;
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            stream_writer writer(stream);
            core::convert(v, writer);
            return stream;
        }

        inline std::istream &input(std::istream &stream, core::value &v) {return stream >> v;}
        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline core::value from_bencode(const std::string &bencode)
        {
            std::istringstream stream(bencode);
            core::value v;
            stream >> v;
            return v;
        }

        inline std::string to_bencode(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }
    }

    namespace plain_text_property_list
    {
        inline std::istream &read_string(std::istream &stream, core::stream_handler &writer)
        {
            static const std::string hex = "0123456789ABCDEF";
            int c;

            {char chr; stream >> chr; c = chr;} // Skip initial whitespace
            if (c != '"') throw core::error("Plain Text Property List - expected string");

            writer.begin_string(core::string_t(), core::stream_handler::unknown_size);
            while (c = stream.get(), c != '"')
            {
                if (c == EOF) throw core::error("Plain Text Property List - unexpected end of string");

                if (c == '\\')
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("Plain Text Property List - unexpected end of string");

                    switch (c)
                    {
                        case 'b': writer.append_to_string(std::string(1, '\b')); break;
                        case 'n': writer.append_to_string(std::string(1, '\n')); break;
                        case 'r': writer.append_to_string(std::string(1, '\r')); break;
                        case 't': writer.append_to_string(std::string(1, '\t')); break;
                        case 'U':
                        {
                            uint32_t code = 0;
                            for (int i = 0; i < 4; ++i)
                            {
                                c = stream.get();
                                if (c == EOF) throw core::error("Plain Text Property List - unexpected end of string");
                                size_t pos = hex.find(toupper(c));
                                if (pos == std::string::npos) throw core::error("Plain Text Property List - invalid character escape sequence");
                                code = (code << 4) | pos;
                            }

                            std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
                            writer.append_to_string(utf8.to_bytes(code));

                            break;
                        }
                        default:
                        {
                            if (isdigit(c))
                            {
                                uint32_t code = 0;
                                stream.unget();
                                for (int i = 0; i < 3; ++i)
                                {
                                    c = stream.get();
                                    if (c == EOF) throw core::error("Plain Text Property List - unexpected end of string");
                                    if (!isdigit(c) || c == '8' || c == '9') throw core::error("Plain Text Property List - invalid character escape sequence");
                                    code = (code << 3) | (c - '0');
                                }

                                std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
                                writer.append_to_string(utf8.to_bytes(code));
                            }
                            else
                                writer.append_to_string(std::string(1, c));

                            break;
                        }
                    }
                }
                else
                    writer.append_to_string(std::string(1, c));
            }

            writer.end_string(core::string_t());
            return stream;
        }

        inline std::ostream &write_string(std::ostream &stream, const std::string &str)
        {
            for (size_t i = 0; i < str.size(); ++i)
            {
                int c = str[i] & 0xff;

                if (c == '"' || c == '\\')
                    stream << '\\' << static_cast<char>(c);
                else
                {
                    switch (c)
                    {
                        case '"':
                        case '\\': stream << '\\' << static_cast<char>(c); break;
                        case '\b': stream << "\\b"; break;
                        case '\n': stream << "\\n"; break;
                        case '\r': stream << "\\r"; break;
                        case '\t': stream << "\\t"; break;
                        default:
                            if (iscntrl(c))
                                stream << '\\' << (c >> 6) << ((c >> 3) & 0x7) << (c & 0x7);
                            else if (static_cast<unsigned char>(str[i]) > 0x7f)
                            {
                                std::string utf8_string;
                                size_t j;
                                for (j = i; j < str.size() && static_cast<unsigned char>(str[j]) > 0x7f; ++j)
                                    utf8_string.push_back(str[j]);

                                if (j < str.size())
                                    utf8_string.push_back(str[j]);

                                i = j;

                                std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf8;
                                std::u16string wstr = utf8.from_bytes(utf8_string);

                                for (j = 0; j < wstr.size(); ++j)
                                {
                                    uint16_t c = wstr[j];

                                    hex::write(stream << "\\U", c >> 8);
                                    hex::write(stream, c & 0xff);
                                }
                            }
                            else
                                stream << str[i];
                            break;
                    }
                }
            }

            return stream;
        }

        inline std::istream &convert(std::istream &stream, core::stream_handler &writer)
        {
            const std::string hex = "0123456789ABCDEF";
            bool delimiter_required = false;
            char chr;

            writer.begin();

            while (stream >> std::skipws >> chr, stream.unget(), stream.good() && !stream.eof())
            {
                if (writer.nesting_depth() == 0 && delimiter_required)
                    break;

                if (delimiter_required && !strchr(",=)}", chr))
                    throw core::error("Plain Text Property List - expected ',' separating array or object entries");

                switch (chr)
                {
                    case '<':
                        stream >> chr; // Eat '<'
                        stream >> chr;
                        if (!stream) throw core::error("Plain Text Property List - expected '*' after '<' in value");

                        if (chr != '*')
                        {
                            const core::value value_type(core::string_t(), core::blob);
                            writer.begin_string(value_type, core::stream_handler::unknown_size);

                            unsigned int t = 0;
                            bool have_first_nibble = false;

                            while (stream && chr != '>')
                            {
                                t <<= 4;
                                size_t p = hex.find(toupper(static_cast<unsigned char>(chr)));
                                if (p == std::string::npos) throw core::error("Plain Text Property List - expected hexadecimal-encoded binary data in value");
                                t |= p;

                                if (have_first_nibble)
                                    writer.append_to_string(core::string_t(1, t));

                                have_first_nibble = !have_first_nibble;
                                stream >> chr;
                            }

                            if (have_first_nibble) throw core::error("Plain Text Property List - unfinished byte in binary data");

                            writer.end_string(value_type);
                            break;
                        }

                        stream >> chr;
                        if (!stream || !strchr("BIRD", chr))
                            throw core::error("Plain Text Property List - expected type specifier after '<*' in value");

                        if (chr == 'B')
                        {
                            stream >> chr;
                            if (!stream || (chr != 'Y' && chr != 'N'))
                                throw core::error("Plain Text Property List - expected 'boolean' value after '<*B' in value");

                            writer.write(chr == 'Y');
                        }
                        else if (chr == 'I')
                        {
                            core::int_t i;
                            stream >> i;
                            if (!stream)
                                throw core::error("Plain Text Property List - expected 'integer' value after '<*I' in value");

                            writer.write(i);
                        }
                        else if (chr == 'R')
                        {
                            core::real_t r;
                            stream >> r;
                            if (!stream)
                                throw core::error("Plain Text Property List - expected 'real' value after '<*R' in value");

                            writer.write(r);
                        }
                        else if (chr == 'D')
                        {
                            int c;
                            const core::value value_type(core::string_t(), core::datetime);
                            writer.begin_string(value_type, core::stream_handler::unknown_size);
                            while (c = stream.get(), c != '>')
                            {
                                if (c == EOF) throw core::error("Plain Text Property List - expected '>' after value");

                                writer.append_to_string(core::string_t(1, c));
                            }
                            stream.unget();
                            writer.end_string(value_type);
                        }

                        stream >> chr;
                        if (chr != '>') throw core::error("Plain Text Property List - expected '>' after value");
                        break;
                    case '"':
                        read_string(stream, writer);
                        delimiter_required = true;
                        break;
                    case ',':
                        stream >> chr; // Eat ','
                        if (writer.current_container_size() == 0 || writer.container_key_was_just_parsed())
                            throw core::error("Plain Text Property List - invalid ',' does not separate array or object entries");
                        stream >> chr; stream.unget(); // Peek ahead
                        if (!stream || chr == ',' || chr == ']' || chr == '}')
                            throw core::error("Plain Text Property List - invalid ',' does not separate array or object entries");
                        delimiter_required = false;
                        break;
                    case '=':
                        stream >> chr; // Eat '='
                        if (!writer.container_key_was_just_parsed())
                            throw core::error("Plain Text Property List - invalid '=' does not separate a key and value pair");
                        delimiter_required = false;
                        break;
                    case '(':
                        stream >> chr; // Eat '('
                        writer.begin_array(core::array_t(), core::stream_handler::unknown_size);
                        delimiter_required = false;
                        break;
                    case ')':
                        stream >> chr; // Eat ')'
                        writer.end_array(core::array_t());
                        delimiter_required = true;
                        break;
                    case '{':
                        stream >> chr; // Eat '{'
                        writer.begin_object(core::object_t(), core::stream_handler::unknown_size);
                        delimiter_required = false;
                        break;
                    case '}':
                        stream >> chr; // Eat '}'
                        writer.end_object(core::object_t());
                        delimiter_required = true;
                        break;
                    default:
                        throw core::error("Plain Text Property List - expected value");
                        break;
                }
            }

            writer.end();
            return stream;
        }

        class stream_writer : public core::stream_handler, public core::stream_writer
        {
        public:
            stream_writer(std::ostream &output) : core::stream_writer(output) {}

        protected:
            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    output_stream << '=';
                else if (current_container_size() > 0)
                    output_stream << ',';
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    output_stream << ',';

                if (!v.is_string())
                    throw core::error("Plain Text Property List - cannot write non-string key");
            }

            void null_(const core::value &) {throw core::error("Plain Text Property List - 'null' value not allowed in output");}
            void bool_(const core::value &v) {output_stream << "<*B" << (v.get_bool()? 'Y': 'N') << '>';}
            void integer_(const core::value &v) {output_stream << "<*I" << v.get_int() << '>';}
            void real_(const core::value &v) {output_stream << "<*R" << v.get_real() << '>';}
            void begin_string_(const core::value &v, core::int_t, bool)
            {
                switch (v.get_subtype())
                {
                    case core::date:
                    case core::time:
                    case core::datetime: output_stream << "<*D"; break;
                    case core::blob: output_stream << '<'; break;
                    default: output_stream << '"'; break;
                }
            }
            void string_data_(const core::value &v)
            {
                if (v.get_subtype() == core::blob)
                    hex::write(output_stream, v.get_string());
                else
                    write_string(output_stream, v.get_string());
            }
            void end_string_(const core::value &v, bool)
            {
                switch (v.get_subtype())
                {
                    case core::date:
                    case core::time:
                    case core::datetime:
                    case core::blob: output_stream << '>'; break;
                    default: output_stream << '"'; break;
                }
            }

            void begin_array_(const core::value &, core::int_t, bool) {output_stream << '(';}
            void end_array_(const core::value &, bool) {output_stream << ')';}

            void begin_object_(const core::value &, core::int_t, bool) {output_stream << '{';}
            void end_object_(const core::value &, bool) {output_stream << '}';}
        };

        class pretty_stream_writer : public core::stream_handler, public core::stream_writer
        {
            size_t indent_width;
            size_t current_indent;

        public:
            pretty_stream_writer(std::ostream &output, size_t indent_width)
                : core::stream_writer(output)
                , indent_width(indent_width)
                , current_indent(0)
            {}

            size_t indent() {return indent_width;}

        protected:
            void begin_() {current_indent = 0;}

            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    output_stream << " = ";
                else if (current_container_size() > 0)
                    output_stream << ',';

                if (current_container() == core::array)
                    output_stream << '\n' << std::string(current_indent, ' ');
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    output_stream << ',';

                output_stream << '\n' << std::string(current_indent, ' ');

                if (!v.is_string())
                    throw core::error("Plain Text Property List - cannot write non-string key");
            }

            void null_(const core::value &) {throw core::error("Plain Text Property List - 'null' value not allowed in output");}
            void bool_(const core::value &v) {output_stream << "<*B" << (v.get_bool()? 'Y': 'N') << '>';}
            void integer_(const core::value &v) {output_stream << "<*I" << v.get_int() << '>';}
            void real_(const core::value &v) {output_stream << "<*R" << v.get_real() << '>';}
            void begin_string_(const core::value &v, core::int_t, bool)
            {
                switch (v.get_subtype())
                {
                    case core::date:
                    case core::time:
                    case core::datetime: output_stream << "<*D"; break;
                    case core::blob: output_stream << '<'; break;
                    default: output_stream << '"'; break;
                }
            }
            void string_data_(const core::value &v)
            {
                if (v.get_subtype() == core::blob)
                    hex::write(output_stream, v.get_string());
                else
                    write_string(output_stream, v.get_string());
            }
            void end_string_(const core::value &v, bool)
            {
                switch (v.get_subtype())
                {
                    case core::date:
                    case core::time:
                    case core::datetime:
                    case core::blob: output_stream << '>'; break;
                    default: output_stream << '"'; break;
                }
            }

            void begin_array_(const core::value &, core::int_t, bool)
            {
                output_stream << '(';
                current_indent += indent_width;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream << '\n' << std::string(current_indent, ' ');

                output_stream << ')';
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                output_stream << '{';
                current_indent += indent_width;
            }
            void end_object_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream << '\n' << std::string(current_indent, ' ');

                output_stream << '}';
            }
        };

        inline std::istream &operator>>(std::istream &stream, core::value &v)
        {
            core::value_builder builder(v);
            convert(stream, builder);
            return stream;
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            stream_writer writer(stream);
            core::convert(v, writer);
            return stream;
        }

        inline std::ostream &pretty_print(std::ostream &stream, const core::value &v, size_t indent_width)
        {
            pretty_stream_writer writer(stream, indent_width);
            core::convert(v, writer);
            return stream;
        }

        inline std::istream &input(std::istream &stream, core::value &v) {return stream >> v;}
        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline core::value from_plain_text_property_list(const std::string &property_list)
        {
            std::istringstream stream(property_list);
            core::value v;
            stream >> v;
            return v;
        }

        inline std::string to_plain_text_property_list(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }

        inline std::string to_pretty_plain_text_property_list(const core::value &v, size_t indent_width)
        {
            std::ostringstream stream;
            pretty_print(stream, v, indent_width);
            return stream.str();
        }
    }

    namespace xml_property_list
    {
        inline std::ostream &write_string(std::ostream &stream, const std::string &str)
        {
            for (size_t i = 0; i < str.size(); ++i)
            {
                int c = str[i] & 0xff;

                switch (c)
                {
                    case '"': stream << "&quot;"; break;
                    case '&': stream << "&amp;"; break;
                    case '\'': stream << "&apos;"; break;
                    case '<': stream << "&lt;"; break;
                    case '>': stream << "&gt;"; break;
                    default:
                        if (iscntrl(c))
                            stream << "&#" << c << ';';
                        else
                            stream << str[i];
                        break;
                }
            }

            return stream;
        }

        class stream_writer : public core::stream_handler, public core::stream_writer
        {
        public:
            stream_writer(std::ostream &output) : core::stream_writer(output) {}

        protected:
            void begin_key_(const core::value &v)
            {
                if (!v.is_string())
                    throw core::error("XML Property List - cannot write non-string key");
            }

            void null_(const core::value &) {throw core::error("XML Property List - 'null' value not allowed in output");}
            void bool_(const core::value &v) {output_stream << '<' << (v.get_bool()? "true": "false") << "/>";}
            void integer_(const core::value &v) {output_stream << "<integer>" << v.get_int() << "</integer>";}
            void real_(const core::value &v) {output_stream << "<real>" << v.get_real() << "</real>";}
            void begin_string_(const core::value &v, core::int_t, bool is_key)
            {
                if (is_key)
                    output_stream << "<key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: output_stream << "<date>"; break;
                        case core::blob: output_stream << "<data>"; break;
                        default: output_stream << "<string>"; break;
                    }
            }
            void string_data_(const core::value &v)
            {
                if (v.get_subtype() == core::blob)
                    base64::write(output_stream, v.get_string());
                else
                    write_string(output_stream, v.get_string());
            }
            void end_string_(const core::value &v, bool is_key)
            {
                if (is_key)
                    output_stream << "</key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: output_stream << "</date>"; break;
                        case core::blob: output_stream << "</data>"; break;
                        default: output_stream << "</string>"; break;
                    }
            }

            void begin_array_(const core::value &, core::int_t, bool) {output_stream << "<array>";}
            void end_array_(const core::value &, bool) {output_stream << "</array>";}

            void begin_object_(const core::value &, core::int_t, bool) {output_stream << "<dict>";}
            void end_object_(const core::value &, bool) {output_stream << "</dict>";}
        };

        class pretty_stream_writer : public core::stream_handler, public core::stream_writer
        {
            size_t indent_width;
            size_t current_indent;

        public:
            pretty_stream_writer(std::ostream &output, size_t indent_width)
                : core::stream_writer(output)
                , indent_width(indent_width)
                , current_indent(0)
            {}

            size_t indent() {return indent_width;}

        protected:
            void begin_() {current_indent = 0;}

            void begin_item_(const core::value &)
            {
                if (current_container() != core::null)
                    output_stream << '\n' << std::string(current_indent, ' ');
            }
            void begin_key_(const core::value &v)
            {
                output_stream << '\n' << std::string(current_indent, ' ');

                if (!v.is_string())
                    throw core::error("XML Property List - cannot write non-string key");
            }

            void bool_(const core::value &v) {output_stream << '<' << (v.get_bool()? "true": "false") << "/>";}
            void integer_(const core::value &v)
            {
                output_stream << "<integer>"
                              << '\n' << std::string(current_indent + indent_width, ' ')
                              << v.get_int()
                              << '\n' << std::string(current_indent, ' ')
                              << "</integer>";
            }
            void real_(const core::value &v)
            {
                output_stream << "<real>"
                              << '\n' << std::string(current_indent + indent_width, ' ')
                              << v.get_real()
                              << '\n' << std::string(current_indent, ' ')
                              << "</real>";
            }
            void begin_string_(const core::value &v, core::int_t, bool is_key)
            {
                if (is_key)
                    output_stream << "<key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: output_stream << "<date>"; break;
                        case core::blob: output_stream << "<data>"; break;
                        default: output_stream << "<string>"; break;
                    }
            }
            void string_data_(const core::value &v)
            {
                if (current_container_size() == 0)
                    output_stream << '\n' << std::string(current_indent + indent_width, ' ');

                if (v.get_subtype() == core::blob)
                    base64::write(output_stream, v.get_string());
                else
                    write_string(output_stream, v.get_string());
            }
            void end_string_(const core::value &v, bool is_key)
            {
                if (current_container_size() > 0)
                    output_stream << '\n' << std::string(current_indent, ' ');

                if (is_key)
                    output_stream << "</key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: output_stream << "</date>"; break;
                        case core::blob: output_stream << "</data>"; break;
                        default: output_stream << "</string>"; break;
                    }
            }

            void begin_array_(const core::value &, core::int_t, bool)
            {
                output_stream << "<array>";
                current_indent += indent_width;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream << '\n' << std::string(current_indent, ' ');

                output_stream << "</array>";
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                output_stream << "<dict>";
                current_indent += indent_width;
            }
            void end_object_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream << '\n' << std::string(current_indent, ' ');

                output_stream << "</dict>";
            }
        };

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            stream_writer writer(stream);
            core::convert(v, writer);
            return stream;
        }

        inline std::ostream &pretty_print(std::ostream &stream, const core::value &v, size_t indent_width)
        {
            pretty_stream_writer writer(stream, indent_width);
            core::convert(v, writer);
            return stream;
        }

        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline std::string to_xml_property_list(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }

        inline std::string to_pretty_xml_property_list(const core::value &v, size_t indent_width)
        {
            std::ostringstream stream;
            pretty_print(stream, v, indent_width);
            return stream.str();
        }
    }

    namespace xml_rpc
    {
        inline std::ostream &write_string(std::ostream &stream, const std::string &str)
        {
            for (size_t i = 0; i < str.size(); ++i)
            {
                int c = str[i] & 0xff;

                switch (c)
                {
                    case '"': stream << "&quot;"; break;
                    case '&': stream << "&amp;"; break;
                    case '\'': stream << "&apos;"; break;
                    case '<': stream << "&lt;"; break;
                    case '>': stream << "&gt;"; break;
                    default:
                        if (iscntrl(c))
                            stream << "&#" << c << ';';
                        else
                            stream << str[i];
                        break;
                }
            }

            return stream;
        }

        class stream_writer : public core::stream_handler, public core::stream_writer
        {
        public:
            stream_writer(std::ostream &output) : core::stream_writer(output) {}

        protected:
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    output_stream << "</member>";

                if (!v.is_string())
                    throw core::error("XML RPC - cannot write non-string key");

                output_stream << "<member>";
            }

            void null_(const core::value &) {throw core::error("XML RPC - 'null' value not allowed in output");}
            void bool_(const core::value &v) {output_stream << "<value><boolean>" << v.as_int() << "</boolean></value>";}
            void integer_(const core::value &v) {output_stream << "<value><int>" << v.get_int() << "</int></value>";}
            void real_(const core::value &v) {output_stream << "<value><double>" << v.get_real() << "</double></value>";}
            void begin_string_(const core::value &, core::int_t, bool is_key)
            {
                if (is_key)
                    output_stream << "<name>";
                else
                    output_stream << "<value><string>";
            }
            void string_data_(const core::value &v) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &, bool is_key)
            {
                if (is_key)
                    output_stream << "</name>";
                else
                    output_stream << "</string></value>";
            }

            void begin_array_(const core::value &, core::int_t, bool) {output_stream << "<value><array><data>";}
            void end_array_(const core::value &, bool) {output_stream << "</data></array></value>";}

            void begin_object_(const core::value &, core::int_t, bool) {output_stream << "<value><struct>";}
            void end_object_(const core::value &, bool)
            {
                if (current_container_size() > 0)
                    output_stream << "</member>";
                output_stream << "</struct></value>";
            }
        };

        class pretty_stream_writer : public core::stream_handler, public core::stream_writer
        {
            size_t indent_width;
            size_t current_indent;

        public:
            pretty_stream_writer(std::ostream &output, size_t indent_width) : core::stream_writer(output), indent_width(indent_width) {}

        protected:
            void begin_() {current_indent = 0;}

            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                {
                    current_indent -= indent_width;
                    output_stream << '\n' << std::string(current_indent, ' ')
                                  << "</member>"
                                  << '\n' << std::string(current_indent, ' ');
                }

                if (!v.is_string())
                    throw core::error("XML RPC - cannot write non-string key");

                output_stream << "<member>";
                current_indent += indent_width;
                output_stream << '\n' << std::string(current_indent, ' ');
            }
            void begin_item_(const core::value &)
            {
                if (current_container_size() > 0 || current_container() == core::object)
                    output_stream << '\n' << std::string(current_indent, ' ');
            }

            void null_(const core::value &) {throw core::error("XML RPC - 'null' value not allowed in output");}
            void bool_(const core::value &v)
            {
                output_stream << "<value>"
                              << '\n' << std::string(current_indent + indent_width, ' ')
                              << "<boolean>"
                              << '\n' << std::string(current_indent + indent_width * 2, ' ')
                              << v.as_int()
                              << '\n' << std::string(current_indent + indent_width, ' ')
                              << "</boolean>"
                              << '\n' << std::string(current_indent, ' ')
                              << "</value>";
            }
            void integer_(const core::value &v)
            {
                output_stream << "<value>"
                              << '\n' << std::string(current_indent + indent_width, ' ')
                              << "<int>"
                              << '\n' << std::string(current_indent + indent_width * 2, ' ')
                              << v.get_int()
                              << '\n' << std::string(current_indent + indent_width, ' ')
                              << "</int>"
                              << '\n' << std::string(current_indent, ' ')
                              << "</value>";
            }
            void real_(const core::value &v)
            {
                output_stream << "<value>"
                              << '\n' << std::string(current_indent + indent_width, ' ')
                              << "<double>"
                              << '\n' << std::string(current_indent + indent_width * 2, ' ')
                              << v.get_real()
                              << '\n' << std::string(current_indent + indent_width, ' ')
                              << "</double>"
                              << '\n' << std::string(current_indent, ' ')
                              << "</value>";
            }
            void begin_string_(const core::value &, core::int_t, bool is_key)
            {
                if (is_key)
                    output_stream << "<name>";
                else
                {
                    current_indent += indent_width;

                    output_stream << "<value>"
                                  << '\n' << std::string(current_indent, ' ')
                                  << "<string>";
                }
            }
            void string_data_(const core::value &v)
            {
                if (current_container_size() == 0)
                    output_stream << '\n' << std::string(current_indent + indent_width, ' ');

                write_string(output_stream, v.get_string());
            }
            void end_string_(const core::value &, bool is_key)
            {
                if (current_container_size() > 0)
                    output_stream << '\n' << std::string(current_indent, ' ');

                if (is_key)
                    output_stream << "</name>";
                else
                {
                    current_indent -= indent_width;

                    output_stream << "</string>"
                                  << '\n' << std::string(current_indent, ' ')
                                  << "</value>";
                }
            }

            void begin_array_(const core::value &, core::int_t, bool)
            {
                output_stream << "<value>"
                              << '\n' << std::string(current_indent + indent_width, ' ')
                              << "<array>"
                              << '\n' << std::string(current_indent + indent_width * 2, ' ')
                              << "<data>"
                              << '\n' << std::string(current_indent + indent_width * 3, ' ');
                current_indent += indent_width * 3;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width * 3;

                output_stream << '\n' << std::string(current_indent + indent_width * 2, ' ')
                              << "</data>"
                              << '\n' << std::string(current_indent + indent_width, ' ')
                              << "</array>"
                              << '\n' << std::string(current_indent, ' ')
                              << "</value>";
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                output_stream << "<value>"
                              << '\n' << std::string(current_indent + indent_width, ' ')
                              << "<struct>"
                              << '\n' << std::string(current_indent + indent_width * 2, ' ');
                current_indent += indent_width * 2;
            }
            void end_object_(const core::value &, bool)
            {
                if (current_container_size() > 0)
                {
                    current_indent -= indent_width;
                    output_stream << '\n' << std::string(current_indent, ' ')
                                  << "</member>";
                }

                current_indent -= indent_width * 2;
                output_stream << '\n' << std::string(current_indent + indent_width, ' ')
                              << "</struct>"
                              << '\n' << std::string(current_indent, ' ')
                              << "</value>";
            }
        };

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            stream_writer writer(stream);
            core::convert(v, writer);
            return stream;
        }

        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline std::string to_xml_rpc(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }
    }

    namespace csv
    {
        inline std::ostream &write_string(std::ostream &stream, const std::string &str)
        {
            for (size_t i = 0; i < str.size(); ++i)
            {
                int c = str[i] & 0xff;

                if (c == '"')
                    stream << '"';

                stream << str[i];
            }

            return stream;
        }

        class row_writer : public core::stream_handler, public core::stream_writer
        {
        public:
            row_writer(std::ostream &output) : core::stream_writer(output) {}

        protected:
            void begin_item_(const core::value &)
            {
                if (current_container_size() > 0)
                    output_stream << ',';
            }

            void bool_(const core::value &v) {output_stream << (v.get_bool()? "true": "false");}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void real_(const core::value &v) {output_stream << v.get_real();}
            void begin_string_(const core::value &, core::int_t, bool) {output_stream << '"';}
            void string_data_(const core::value &v) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &, bool) {output_stream << '"';}

            void begin_array_(const core::value &, core::int_t, bool) {throw core::error("CSV - 'array' value not allowed in row output");}
            void begin_object_(const core::value &, core::int_t, bool) {throw core::error("CSV - 'object' value not allowed in output");}
        };

        class stream_writer : public core::stream_handler, public core::stream_writer
        {
        public:
            stream_writer(std::ostream &output) : core::stream_writer(output) {}

        protected:
            void begin_item_(const core::value &)
            {
                if (current_container_size() > 0)
                    output_stream << (nesting_depth() == 1? '\n': ',');
            }

            void bool_(const core::value &v) {output_stream << (v.get_bool()? "true": "false");}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void real_(const core::value &v) {output_stream << v.get_real();}
            void begin_string_(const core::value &, core::int_t, bool) {output_stream << '"';}
            void string_data_(const core::value &v) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &, bool) {output_stream << '"';}

            void begin_array_(const core::value &, core::int_t, bool)
            {
                if (nesting_depth() == 2)
                    throw core::error("CSV - 'array' value not allowed in row output");
            }
            void begin_object_(const core::value &, core::int_t, bool) {throw core::error("CSV - 'object' value not allowed in output");}
        };

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            stream_writer writer(stream);
            core::convert(v, writer);
            return stream;
        }

        inline std::ostream &print_table(std::ostream &stream, const core::value &v) {return stream << v;}
        inline std::ostream &print_row(std::ostream &stream, const core::value &v)
        {
            row_writer writer(stream);
            core::convert(v, writer);
            return stream;
        }

        inline std::string to_csv_row(const core::value &v)
        {
            std::ostringstream stream;
            print_row(stream, v);
            return stream.str();
        }

        inline std::string to_csv_table(const core::value &v)
        {
            std::ostringstream stream;
            print_table(stream, v);
            return stream.str();
        }

        inline std::ostream &print(std::ostream &stream, const core::value &v) {return print_table(stream, v);}
        inline std::string to_csv(const core::value &v) {return to_csv_table(v);}
    }

    namespace ubjson
    {
        inline char size_specifier(core::int_t min, core::int_t max)
        {
            if (min >= 0 && max <= UINT8_MAX)
                return 'U';
            else if (min >= INT8_MIN && max <= INT8_MAX)
                return 'i';
            else if (min >= INT16_MIN && max <= INT16_MAX)
                return 'I';
            else if (min >= INT32_MIN && max <= INT32_MAX)
                return 'l';
            else
                return 'L';
        }

        inline std::istream &read_int(std::istream &stream, core::int_t &i, char specifier)
        {
            uint64_t temp;
            bool negative = false;
            int c = stream.get();

            if (c == EOF) throw core::error("UBJSON - expected integer value after type specifier");
            temp = c & 0xff;

            switch (specifier)
            {
                case 'U': // Unsigned byte
                    break;
                case 'i': // Signed byte
                    negative = c >> 7;
                    temp |= negative * 0xffffffffffffff00ull;
                    break;
                case 'I': // Signed word
                    negative = c >> 7;

                    c = stream.get();
                    if (c == EOF) throw core::error("UBJSON - expected integer value after type specifier");

                    temp = (temp << 8) | (c & 0xff);
                    temp |= negative * 0xffffffffffff0000ull;
                    break;
                case 'l': // Signed double-word
                    negative = c >> 7;

                    for (int i = 0; i < 3; ++i)
                    {
                        c = stream.get();
                        if (c == EOF) throw core::error("UBJSON - expected integer value after type specifier");

                        temp = (temp << 8) | (c & 0xff);
                    }
                    temp |= negative * 0xffffffff00000000ull;
                    break;
                case 'L': // Signed double-word
                    negative = c >> 7;

                    for (int i = 0; i < 7; ++i)
                    {
                        c = stream.get();
                        if (c == EOF) throw core::error("UBJSON - expected integer value after type specifier");

                        temp = (temp << 8) | (c & 0xff);
                    }
                    break;
            }

            if (negative)
            {
                i = (~temp + 1) & ((1ull << 63) - 1);
                if (i == 0)
                    i = INT64_MIN;
                else
                    i = -i;
            }
            else
                i = temp;

            return stream;
        }

        inline std::istream &read_float(std::istream &stream, core::real_t &r, char specifier)
        {
            uint64_t temp;
            int c = stream.get();

            if (c == EOF) throw core::error("UBJSON - expected integer value after type specifier");
            temp = c & 0xff;

            if (specifier == 'd')
            {
                for (int i = 0; i < 3; ++i)
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("UBJSON - expected floating-point value after type specifier");

                    temp = (temp << 8) | (c & 0xff);
                }
            }
            else
            {
                for (int i = 0; i < 7; ++i)
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("UBJSON - expected floating-point value after type specifier");

                    temp = (temp << 8) | (c & 0xff);
                }
            }

            if (specifier == 'd')
                r = core::float_from_ieee_754(temp);
            else
                r = core::double_from_ieee_754(temp);

            return stream;
        }

        inline std::istream &read_string(std::istream &stream, core::string_t &s, char specifier, long &subtype)
        {
            int c = stream.get();

            if (c == EOF) throw core::error("UBJSON - expected string value after type specifier");

            subtype = core::normal;

            s.clear();
            if (specifier == 'C')
                s.push_back(c);
            else if (specifier == 'H')
            {
                core::int_t size;

                subtype = core::bignum;
                read_int(stream, size, c);
                if (size < 0) throw core::error("UBJSON - invalid negative size specified for high-precision number");

                while (size-- > 0)
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("UBJSON - expected high-precision number value after type specifier");

                    s.push_back(c);
                }
            }
            else
            {
                core::int_t size;

                read_int(stream, size, c);
                if (size < 0) throw core::error("UBJSON - invalid negative size specified for string");

                while (size-- > 0)
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("UBJSON - expected string value after type specifier");

                    s.push_back(c);
                }
            }

            return stream;
        }

        inline std::ostream &write_int(std::ostream &stream, core::int_t i, bool add_specifier, char force_specifier = 0)
        {
            const std::string specifiers = "UiIlL";
            size_t force_bits = specifiers.find(force_specifier);

            if (force_bits == std::string::npos)
                force_bits = 0;

            if (force_bits == 0 && (i >= 0 && i <= UINT8_MAX))
                return stream << (add_specifier? "U": "") << static_cast<unsigned char>(i);
            else if (force_bits <= 1 && (i >= INT8_MIN && i < 0))
                return stream << (add_specifier? "i": "") << static_cast<unsigned char>(0x80 | ((~std::abs(i) & 0xff) + 1));
            else if (force_bits <= 2 && (i >= INT16_MIN && i <= INT16_MAX))
            {
                uint16_t t;

                if (i < 0)
                    t = 0x8000u | ((~std::abs(i) & 0xffffu) + 1);
                else
                    t = i;

                return stream << (add_specifier? "I": "") << static_cast<unsigned char>(t >> 8) <<
                                                             static_cast<unsigned char>(t & 0xff);
            }
            else if (force_bits <= 3 && (i >= INT32_MIN && i <= INT32_MAX))
            {
                uint32_t t;

                if (i < 0)
                    t = 0x80000000u | ((~std::abs(i) & 0xffffffffu) + 1);
                else
                    t = i;

                return stream << (add_specifier? "l": "") << static_cast<unsigned char>(t >> 24) <<
                                                             static_cast<unsigned char>((t >> 16) & 0xff) <<
                                                             static_cast<unsigned char>((t >>  8) & 0xff) <<
                                                             static_cast<unsigned char>((t      ) & 0xff);
            }
            else
            {
                uint64_t t;

                if (i < 0)
                    t = 0x8000000000000000u | ((~std::abs(i) & 0xffffffffffffffffu) + 1);
                else
                    t = i;

                return stream << (add_specifier? "L": "") << static_cast<unsigned char>(t >> 56) <<
                                                             static_cast<unsigned char>((t >> 48) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 40) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 32) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 24) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 16) & 0xff) <<
                                                             static_cast<unsigned char>((t >>  8) & 0xff) <<
                                                             static_cast<unsigned char>((t      ) & 0xff);
            }
        }

        inline std::ostream &write_float(std::ostream &stream, core::real_t f, bool add_specifier, char force_specifier = 0)
        {
            const std::string specifiers = "dD";
            size_t force_bits = specifiers.find(force_specifier);

            if (force_bits == std::string::npos)
                force_bits = 0;

            if (force_bits == 0 && (core::float_from_ieee_754(core::float_to_ieee_754(f)) == f || std::isnan(f)))
            {
                uint32_t t = core::float_to_ieee_754(f);

                return stream << (add_specifier? "d": "") << static_cast<unsigned char>(t >> 24) <<
                                                             static_cast<unsigned char>((t >> 16) & 0xff) <<
                                                             static_cast<unsigned char>((t >>  8) & 0xff) <<
                                                             static_cast<unsigned char>((t      ) & 0xff);
            }
            else
            {
                uint64_t t = core::double_to_ieee_754(f);

                return stream << (add_specifier? "L": "") << static_cast<unsigned char>(t >> 56) <<
                                                             static_cast<unsigned char>((t >> 48) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 40) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 32) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 24) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 16) & 0xff) <<
                                                             static_cast<unsigned char>((t >>  8) & 0xff) <<
                                                             static_cast<unsigned char>((t      ) & 0xff);
            }
        }

        inline std::ostream &write_string(std::ostream &stream, const std::string &str, bool add_specifier, long subtype)
        {
            if (subtype == core::normal && str.size() == 1 && static_cast<unsigned char>(str[0]) < 128)
                return stream << (add_specifier? "C": "") << str[0];

            if (add_specifier)
                stream << (subtype == core::bignum? 'H': 'S');

            write_int(stream, str.size(), true);

            return stream << str;
        }

        inline std::istream &input(std::istream &stream, core::value &v, char specifier = 0)
        {
            struct null_exception {};

            while (true)
            {
                int c = specifier? specifier: stream.get();

                switch (c)
                {
                    case 'Z': v.set_null(); return stream;
                    case 'T': v.set_bool(true); return stream;
                    case 'F': v.set_bool(false); return stream;
                    case 'U':
                    case 'i':
                    case 'I':
                    case 'l':
                    case 'L': return read_int(stream, v.get_int(), c);
                    case 'd':
                    case 'D': return read_float(stream, v.get_real(), c);
                    case 'C':
                    case 'H':
                    case 'S': return read_string(stream, v.get_string(), c, v.get_subtype());
                    case 'N': break;
                    case '[':
                    {
                        int type = 0;
                        core::int_t size = 0;

                        c = stream.get();
                        if (c == EOF) throw core::error("UBJSON - expected array value after '['");

                        if (c == '$') // Type specified
                        {
                            c = stream.get();
                            if (c == EOF) throw core::error("UBJSON - expected type specifier after '$'");
                            type = c;
                            c = stream.get();
                            if (c == EOF) throw core::error("UBJSON - unexpected end of array");
                        }

                        v.set_array(core::array_t());
                        if (c == '#') // Count specified
                        {
                            c = stream.get();
                            if (c == EOF) throw core::error("UBJSON - expected count specifier after '#'");

                            read_int(stream, size, c);
                            if (size < 0) throw core::error("UBJSON - invalid negative size specified for array");

                            while (size-- > 0)
                            {
                                core::value item;
                                input(stream, item, type);
                                v.push_back(item);
                            }
                            return stream;
                        }

                        while (c != ']')
                        {
                            core::value item;
                            input(stream, item, c);
                            v.push_back(item);

                            c = stream.get();
                            if (c == EOF) throw core::error("UBJSON - unexpected end of array");
                        }

                        return stream;
                    }
                    case '{':
                    {
                        int type = 0;
                        core::int_t size = 0;

                        c = stream.get();
                        if (c == EOF) throw core::error("UBJSON - expected object value after '{'");

                        if (c == '$') // Type specified
                        {
                            c = stream.get();
                            if (c == EOF) throw core::error("UBJSON - expected type specifier after '$'");
                            type = c;
                            c = stream.get();
                            if (c == EOF) throw core::error("UBJSON - unexpected end of object");
                        }

                        v.set_object(core::object_t());
                        if (c == '#') // Count specified
                        {
                            c = stream.get();
                            if (c == EOF) throw core::error("UBJSON - expected count specifier after '#'");

                            read_int(stream, size, c);
                            if (size < 0) throw core::error("UBJSON - invalid negative size specified for object");

                            while (size-- > 0)
                            {
                                core::value item;
                                std::string key;
                                long dummy;

                                read_string(stream, key, 'S', dummy);
                                input(stream, item, type);
                                v[key] = item;
                            }
                            return stream;
                        }

                        while (c != '}')
                        {
                            core::value item;
                            std::string key;
                            long dummy;

                            stream.unget();
                            read_string(stream, key, 'S', dummy);
                            input(stream, item);
                            v[key] = item;

                            c = stream.get();
                            if (c == EOF) throw core::error("UBJSON - unexpected end of object");
                        }

                        return stream;
                    }
                }

                throw core::error("UBJSON - expected value");
            }
        }

        inline std::ostream &print(std::ostream &stream, const core::value &v, bool add_specifier = true, char force_specifier = 0)
        {
            switch (v.get_type())
            {
                case core::null: return add_specifier? stream << 'Z': stream;
                case core::boolean: return add_specifier? stream << (v.get_bool()? 'T': 'F'): stream;
                case core::integer: return write_int(stream, v.get_int(), add_specifier, force_specifier);
                case core::real: return write_float(stream, v.get_real(), add_specifier, force_specifier);
                case core::string: return write_string(stream, v.get_string(), add_specifier, v.get_subtype());
                case core::array:
                {
                    core::type type = core::null;
                    char forced_type = 0;
                    bool same_types = true;

                    bool bool_val = false;
                    bool strings_can_be_chars = true;
                    bool reals_can_be_floats = true;
                    core::int_t int_min = 0, int_max = 0;

                    if (v.size())
                    {
                        auto it = *v.get_array().begin();
                        type = it.get_type();
                        int_min = int_max = it.as_int();
                        bool_val = it.as_bool();
                    }

                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                    {
                        if (it->get_type() != type || (type == core::boolean && bool_val != it->get_bool()))
                        {
                            same_types = false;
                            break;
                        }

                        if (it->is_int())
                        {
                            if (it->get_int() < int_min)
                                int_min = it->get_int();
                            else if (it->get_int() > int_max)
                                int_max = it->get_int();
                        }
                        else if (it->is_real() && reals_can_be_floats)
                        {
                            if (it->get_real() != core::float_from_ieee_754(core::float_to_ieee_754(it->get_real())) && !std::isnan(it->get_real()))
                                reals_can_be_floats = false;
                        }
                        else if (it->is_string() && strings_can_be_chars)
                        {
                            if (it->get_string().size() != 1 || static_cast<unsigned char>(it->get_string()[0]) >= 128)
                                strings_can_be_chars = false;
                        }
                    }

                    if (add_specifier)
                        stream << '[';

                    if (same_types && v.size() > 1)
                    {
                        stream << '$';
                        switch (type)
                        {
                            case core::null: stream << 'Z'; break;
                            case core::boolean: stream << (bool_val? 'T': 'F'); break;
                            case core::integer: stream << (forced_type = size_specifier(int_min, int_max)); break;
                            case core::real: stream << (forced_type = (reals_can_be_floats? 'd': 'D')); break;
                            case core::string: stream << (strings_can_be_chars? 'C': 'S'); break;
                            case core::array: stream << '['; break;
                            case core::object: stream << '{'; break;
                        }

                        write_int(stream << '#', v.size(), true);

                        if (type != core::null && type != core::boolean)
                        {
                            for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                                print(stream, *it, false, forced_type);
                        }

                        return stream;
                    }
                    else
                    {
                        for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                            print(stream, *it);

                        return stream << ']';
                    }
                }
                case core::object:
                {
                    core::type type = core::null;
                    char forced_type = 0;
                    bool same_types = true;

                    bool bool_val = false;
                    bool strings_can_be_chars = true;
                    bool reals_can_be_floats = true;
                    core::int_t int_min = 0, int_max = 0;

                    if (v.size())
                    {
                        auto it = v.get_object().begin();
                        type = it->second.get_type();
                        int_min = int_max = it->second.as_int();
                        bool_val = it->second.as_bool();
                    }

                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                    {
                        if (it->second.get_type() != type || (type == core::boolean && bool_val != it->second.get_bool()))
                        {
                            same_types = false;
                            break;
                        }

                        if (it->second.is_int())
                        {
                            if (it->second.get_int() < int_min)
                                int_min = it->second.get_int();
                            else if (it->second.get_int() > int_max)
                                int_max = it->second.get_int();
                        }
                        else if (it->second.is_real() && reals_can_be_floats)
                        {
                            if (it->second.get_real() != core::float_from_ieee_754(core::float_to_ieee_754(it->second.get_real())) && !std::isnan(it->second.get_real()))
                                reals_can_be_floats = false;
                        }
                        else if (it->second.is_string() && strings_can_be_chars)
                        {
                            if (it->second.get_string().size() != 1 || static_cast<unsigned char>(it->second.get_string()[0]) >= 128)
                                strings_can_be_chars = false;
                        }
                    }

                    if (add_specifier)
                        stream << '{';

                    if (same_types && v.size() > 1)
                    {
                        stream << '$';
                        switch (type)
                        {
                            case core::null: stream << 'Z'; break;
                            case core::boolean: stream << (bool_val? 'T': 'F'); break;
                            case core::integer: stream << (forced_type = size_specifier(int_min, int_max)); break;
                            case core::real: stream << (forced_type = (reals_can_be_floats? 'd': 'D')); break;
                            case core::string: stream << (strings_can_be_chars? 'C': 'S'); break;
                            case core::array: stream << '['; break;
                            case core::object: stream << '{'; break;
                        }

                        write_int(stream << '#', v.size(), true);

                        for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        {
                            if (!it->first.is_string())
                                throw core::error("UBJSON - object key is not a string");

                            print(write_int(stream, it->first.size(), true) << it->first.get_string(), it->second, false, forced_type);
                        }

                        return stream;
                    }
                    else
                    {
                        for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        {
                            if (!it->first.is_string())
                                throw core::error("UBJSON - object key is not a string");

                            print(write_int(stream, it->first.size(), true) << it->first.get_string(), it->second);
                        }

                        return stream << '}';
                    }
                }
            }

            // Control will never get here
            return stream;
        }

        inline std::istream &operator>>(std::istream &stream, core::value &v) {return input(stream, v);}
        inline std::ostream &operator<<(std::ostream &stream, const core::value &v) {return print(stream, v);}

        inline core::value from_ubjson(const std::string &ubjson)
        {
            std::istringstream stream(ubjson);
            core::value v;
            stream >> v;
            return v;
        }

        inline std::string to_ubjson(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }
    }

    namespace binn
    {
        inline std::ostream &write_type(std::ostream &stream, unsigned int type, unsigned int subtype, int *written = NULL)
        {
            char c;

            if (written != NULL)
                *written = 1 + (subtype > 15);

            c = (type & 0x7) << 5;
            if (subtype > 15)
            {
                c = (c | 0x10) | ((subtype >> 8) & 0xf);
                return stream << c << static_cast<char>(subtype & 0xff);
            }

            c = c | subtype;
            return stream << c;
        }

        inline std::ostream &write_size(std::ostream &stream, uint64_t size, int *written = NULL)
        {
            if (written != NULL)
                *written = 1 + 3 * (size < 128);

            if (size < 128)
                return stream << static_cast<char>(size);

            return stream << static_cast<char>(((size >> 24) & 0xff) | 0x80)
                          << static_cast<char>((size >> 16) & 0xff)
                          << static_cast<char>((size >>  8) & 0xff)
                          << static_cast<char>( size        & 0xff);
        }

        inline size_t get_size(const core::value &v)
        {
            switch (v.get_type())
            {
                case core::null:
                case core::boolean: return 1 + (v.get_subtype() >= core::user && v.get_subtype() > 15);
                case core::integer:
                {
                    size_t size = 1; // one byte for type specifier

                    if (v.get_subtype() >= core::user && v.get_subtype() > 15)
                        ++size; // type specifier requires another byte

                    if (v.get_int() >= INT8_MIN && v.get_int() <= UINT8_MAX)
                        size += 1;
                    else if (v.get_int() >= INT16_MIN && v.get_int() <= UINT16_MAX)
                        size += 2;
                    else if (v.get_int() >= INT32_MIN && v.get_int() <= UINT32_MAX)
                        size += 4;
                    else
                        size += 8;

                    return size;
                }
                case core::real:
                {
                    size_t size = 5; // one byte for type specifier, minimum of four bytes for data

                    // A user-specified subtype is not available for reals
                    // (because when the data is read again, the IEEE-754 representation will be put into an integer instead of a real,
                    // since there is nothing to show that the data should be read as a floating point number)
                    // To prevent the loss of data, the subtype is discarded and the value stays the same

                    if (core::float_from_ieee_754(core::float_to_ieee_754(v.get_real())) != v.get_real() && !std::isnan(v.get_real()))
                        size += 4; // requires more than 32-bit float to losslessly encode

                    return size;
                }
                case core::string:
                {
                    size_t size = 3; // one byte for the type specifier, one for the minimum size specifier of one byte, one for trailing nul

                    if (v.get_subtype() >= core::user && v.get_subtype() > 15)
                        ++size; // type specifier requires another byte

                    if (size + v.size() >= 128)
                        size += 3; // requires a four-byte size specifier

                    return size + v.get_string().size();
                }
                case core::array:
                {
                    size_t size = 3; // one byte for the type specifier,
                                     // one for the minimum size specifier of one byte,
                                     // and one for the minimum count specifier of one byte

                    if (v.get_subtype() >= core::user && v.get_subtype() > 15)
                        ++size; // type specifier requires another byte

                    if (v.size() >= 128)
                        size += 3; // requires a four-byte count specifier

                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        size += get_size(*it);

                    if (size >= 128)
                        size += 3; // requires a four-byte size specifier

                    return size;
                }
                case core::object:
                {
                    size_t size = 3; // one byte for the type specifier,
                                     // one for the minimum size specifier of one byte,
                                     // and one for the minimum count specifier of one byte

                    // A user-specified subtype is not available for objects
                    // (because when the data is read again, there is no way to determine the type of structure the container holds)
                    // To prevent the loss of data, the subtype is discarded and the value stays the same

                    if (v.size() >= 128)
                        size += 3; // requires a four-byte count specifier

                    if (v.get_subtype() == core::map)
                    {
                        for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                            size += 4 /* map ID */ + get_size(it->second) /* value size */;
                    }
                    else
                    {
                        for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                            size += 1 /* key size specifier */ + it->first.size() /* key size */ + get_size(it->second) /* value size */;
                    }

                    if (size >= 128)
                        size += 3; // requires a four-byte size specifier

                    return size;
                }
            }

            // Control will never get here
            return 0;
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            enum types
            {
                nobytes,
                byte,
                word,
                dword,
                qword,
                string,
                blob,
                container
            };

            enum subtypes
            {
                null = 0,
                yes, // true
                no, // false

                uint8 = 0,
                int8,

                uint16 = 0,
                int16,

                uint32 = 0,
                int32,
                single_float,

                uint64 = 0,
                int64,
                double_float,

                text = 0,
                datetime,
                date,
                time,
                decimal_str,

                blob_data = 0,

                list = 0,
                map,
                object
            };

            switch (v.get_type())
            {
                case core::null: return write_type(stream, nobytes, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): null);
                case core::boolean: return write_type(stream, nobytes, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): v.get_bool()? yes: no);
                case core::integer:
                {
                    uint64_t out = std::abs(v.get_int());
                    if (v.get_int() < 0)
                        out = ~out + 1;

                    if (v.get_int() >= INT8_MIN && v.get_int() <= UINT8_MAX)
                        return write_type(stream, byte, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): v.get_int() < 0? int8: uint8) << static_cast<char>(out);
                    else if (v.get_int() >= INT16_MIN && v.get_int() <= UINT16_MAX)
                        return write_type(stream, word, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): v.get_int() < 0? int16: uint16)
                                << static_cast<char>(out >> 8)
                                << static_cast<char>(out & 0xff);
                    else if (v.get_int() >= INT32_MIN && v.get_int() <= UINT32_MAX)
                        return write_type(stream, dword, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): v.get_int() < 0? int32: uint32)
                                << static_cast<char>(out >> 24)
                                << static_cast<char>((out >> 16) & 0xff)
                                << static_cast<char>((out >> 8) & 0xff)
                                << static_cast<char>(out & 0xff);
                    else
                        return write_type(stream, qword, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): v.get_int() < 0? int64: uint64)
                                << static_cast<char>(out >> 56)
                                << static_cast<char>((out >> 48) & 0xff)
                                << static_cast<char>((out >> 40) & 0xff)
                                << static_cast<char>((out >> 32) & 0xff)
                                << static_cast<char>((out >> 24) & 0xff)
                                << static_cast<char>((out >> 16) & 0xff)
                                << static_cast<char>((out >> 8) & 0xff)
                                << static_cast<char>(out & 0xff);
                }
                case core::real:
                {
                    uint64_t out;

                    if (core::float_from_ieee_754(core::float_to_ieee_754(v.get_real())) == v.get_real() || std::isnan(v.get_real()))
                    {
                        out = core::float_to_ieee_754(v.get_real());
                        return write_type(stream, dword, single_float)
                                << static_cast<char>(out >> 24)
                                << static_cast<char>((out >> 16) & 0xff)
                                << static_cast<char>((out >> 8) & 0xff)
                                << static_cast<char>(out & 0xff);
                    }
                    else
                    {
                        out = core::double_to_ieee_754(v.get_real());
                        return write_type(stream, qword, double_float)
                                << static_cast<char>(out >> 56)
                                << static_cast<char>((out >> 48) & 0xff)
                                << static_cast<char>((out >> 40) & 0xff)
                                << static_cast<char>((out >> 32) & 0xff)
                                << static_cast<char>((out >> 24) & 0xff)
                                << static_cast<char>((out >> 16) & 0xff)
                                << static_cast<char>((out >> 8) & 0xff)
                                << static_cast<char>(out & 0xff);
                    }
                }
                case core::string:
                {
                    switch (v.get_subtype())
                    {
                        case core::date: write_type(stream, string, date); break;
                        case core::time: write_type(stream, string, time); break;
                        case core::datetime: write_type(stream, string, datetime); break;
                        case core::bignum: write_type(stream, string, decimal_str); break;
                        case core::blob: write_type(stream, blob, blob_data); break;
                        default: write_type(stream, string, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): text); break;
                    }

                    return write_size(stream, v.get_string().size()) << v.get_string() << static_cast<char>(0);
                }
                case core::array:
                {
                    write_type(stream, container, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): list);
                    write_size(stream, get_size(v));
                    write_size(stream, v.size());

                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        stream << *it;

                    return stream;
                }
                case core::object:
                {
                    write_type(stream, container, v.get_subtype() == core::map? map: object);
                    write_size(stream, get_size(v));
                    write_size(stream, v.size());

                    if (v.get_subtype() == core::map)
                    {
                        for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        {
                            int64_t key = it->first.get_int();
                            uint32_t out;

                            if (!it->first.is_int())
                                throw core::error("Binn - map key is not an integer");
                            else if (key < INT32_MIN || key > INT32_MAX)
                                throw core::error("Binn - map key is out of range");

                            out = std::abs(key);
                            if (key < 0)
                                out = ~out + 1;

                            stream << static_cast<char>(out >> 24) <<
                                      static_cast<char>((out >> 16) & 0xff) <<
                                      static_cast<char>((out >>  8) & 0xff) <<
                                      static_cast<char>(out & 0xff) <<
                                      it->second;
                        }
                    }
                    else
                    {
                        for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        {
                            if (!it->first.is_string())
                                throw core::error("Binn - object key is not a string");

                            if (it->first.size() > 255)
                                throw core::error("Binn - object key is larger than limit of 255 bytes");

                            stream << static_cast<char>(it->first.size()) << it->first.get_string() << it->second;
                        }
                    }

                    return stream;
                }
            }

            // Control will never get here
            return stream;
        }

        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline std::string to_binn(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }
    }

    namespace netstrings
    {
        inline size_t get_size(const core::value &v)
        {
            switch (v.get_type())
            {
                case core::null: return 3; // "0:,"
                case core::boolean: return 7 + v.get_bool(); // "4:true," or "5:false,"
                case core::integer:
                {
                    std::ostringstream str;
                    str << ":" << v.get_int() << ',';
                    str << (str.str().size() - 2);
                    return str.str().size();
                }
                case core::real:
                {
                    std::ostringstream str;
                    str << ":" << v.get_real() << ',';
                    str << (str.str().size() - 2);
                    return str.str().size();
                }
                case core::string:
                    return floor(log10(std::max(v.size(), size_t(1))) + 1) + v.size() + 2;
                case core::array:
                {
                    size_t size = 0;

                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        size += get_size(*it);

                    return floor(log10(std::max(size, size_t(1))) + 1) + size + 2;
                }
                case core::object:
                {
                    size_t size = 0;

                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        size += get_size(it->first) + get_size(it->second);

                    return floor(log10(std::max(size, size_t(1))) + 1) + size + 2;
                }
            }

            // Control will never get here
            return 0;
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            switch (v.get_type())
            {
                case core::null: return stream << "0:,";
                case core::boolean: return stream << (v.get_bool()? "4:true,": "5:false,");
                case core::integer: return stream << std::to_string(v.get_int()).size() << ':' << v.get_int() << ',';
                case core::real: return stream << std::to_string(v.get_real()).size() << ':' << v.get_real() << ',';
                case core::string: return stream << v.size() << ':' << v.get_string() << ',';
                case core::array:
                {
                    size_t size = 0;

                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        size += get_size(*it);

                    stream << size << ':';

                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        stream << *it;

                    return stream << ',';
                }
                case core::object:
                {
                    size_t size = 0;

                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        size += get_size(it->first) + get_size(it->second);

                    stream << size << ':';

                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        stream << it->first << it->second;

                    return stream << ',';
                }
            }

            // Control will never get here
            return stream;
        }

        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline std::string to_netstrings(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }
    }
}

#endif // JSON_H

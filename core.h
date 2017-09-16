#ifndef JSON_H
#define JSON_H

#include <string>
#include <vector>
#include <map>
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
    namespace base64
    {
        inline std::string encode(const std::string &str)
        {
            const char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string result;
            uint32_t temp;

            for (size_t i = 0; i < str.size();)
            {
                temp = (uint32_t) (str[i++] & 0xFF) << 16;
                if (i + 2 <= str.size())
                {
                    temp |= (uint32_t) (str[i++] & 0xFF) << 8;
                    temp |= (uint32_t) (str[i++] & 0xFF);
                    result.push_back(alpha[(temp >> 18) & 0x3F]);
                    result.push_back(alpha[(temp >> 12) & 0x3F]);
                    result.push_back(alpha[(temp >>  6) & 0x3F]);
                    result.push_back(alpha[ temp        & 0x3F]);
                }
                else if (i + 1 == str.size())
                {
                    temp |= (uint32_t) (str[i++] & 0xFF) << 8;
                    result.push_back(alpha[(temp >> 18) & 0x3F]);
                    result.push_back(alpha[(temp >> 12) & 0x3F]);
                    result.push_back(alpha[(temp >>  6) & 0x3F]);
                    result.push_back('=');
                }
                else if (i == str.size())
                {
                    result.push_back(alpha[(temp >> 18) & 0x3F]);
                    result.push_back(alpha[(temp >> 12) & 0x3F]);
                    result.push_back('=');
                    result.push_back('=');
                }
            }

            return result;
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
                    return result | (0x1ful << 10);

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
                    return result | (0x7fful << 52);

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

        class value;

        typedef bool bool_t;
        typedef int64_t int_t;
        typedef double real_t;
        typedef const char *cstring_t;
        typedef std::string string_t;
        typedef std::vector<value> array_t;
        typedef std::map<string_t, value> object_t;

        struct error
        {
            error(cstring_t reason) : what_(reason) {}

            cstring_t what() const {return what_;}

        private:
            cstring_t what_;
        };

        class value
        {
        public:
            value() : type_(null) {}
            value(bool_t v) : type_(boolean), bool_(v) {}
            value(int_t v) : type_(integer), int_(v) {}
            value(real_t v) : type_(real), real_(v) {}
            value(cstring_t v) : type_(string), str_(v) {}
            value(const string_t &v) : type_(string), str_(v) {}
            value(const array_t &v) : type_(array), arr_(v) {}
            value(const object_t &v) : type_(object), obj_(v) {}
            template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
            value(T v) : type_(integer), int_(v) {}
            template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
            value(T v) : type_(real), real_(v) {}

            type get_type() const {return type_;}
            size_t size() const {return type_ == array? arr_.size(): type_ == object? obj_.size(): 0;}

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

            value operator[](const string_t &key) const
            {
                auto it = obj_.find(key);
                if (it != obj_.end())
                    return it->second;
                return value();
            }
            value &operator[](const string_t &key) {clear(object); return obj_[key];}
            bool_t is_member(cstring_t key) const {return obj_.find(key) != obj_.end();}
            bool_t is_member(const string_t &key) const {return obj_.find(key) != obj_.end();}
            void erase(const string_t &key) {obj_.erase(key);}

            void push_back(const value &v) {clear(array); arr_.push_back(v);}
            void push_back(value &&v) {clear(array); arr_.push_back(v);}
            const value &operator[](size_t pos) const {return arr_[pos];}
            value &operator[](size_t pos) {return arr_[pos];}
            void erase(int_t pos) {arr_.erase(arr_.begin() + pos);}

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
            void clear(type new_type)
            {
                if (type_ == new_type)
                    return;

                str_.clear(); str_.shrink_to_fit();
                arr_.clear(); arr_.shrink_to_fit();
                obj_.clear();
                type_ = new_type;
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
        };

        struct null_t : value {null_t() {}};

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
    }

    namespace json
    {
        inline std::istream &read_string(std::istream &stream, std::string &str)
        {
            static const std::string hex = "0123456789ABCDEF";
            int c;

            c = stream.get();
            if (c != '"') throw core::error("expected string");

            str.clear();
            while (c = stream.get(), c != '"')
            {
                if (c == EOF) throw core::error("unexpected end of string");

                if (c == '\\')
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("unexpected end of string");

                    switch (c)
                    {
                        case 'b': str.push_back('\b'); break;
                        case 'f': str.push_back('\f'); break;
                        case 'n': str.push_back('\n'); break;
                        case 'r': str.push_back('\r'); break;
                        case 't': str.push_back('\t'); break;
                        case 'u':
                        {
                            uint32_t code = 0;
                            for (int i = 0; i < 4; ++i)
                            {
                                c = stream.get();
                                if (c == EOF) throw core::error("unexpected end of string");
                                size_t pos = hex.find(toupper(c));
                                if (pos == std::string::npos) throw core::error("invalid character escape sequence");
                                code = (code << 4) | pos;
                            }

                            std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
                            str += utf8.to_bytes(code);

                            break;
                        }
                        default:
                            str.push_back(c); break;
                    }
                }
                else
                    str.push_back(c);
            }

            return stream;
        }

        inline std::ostream &write_string(std::ostream &stream, const std::string &str)
        {
            static const char hex[] = "0123456789ABCDEF";
            stream << '"';
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
                                stream << "\\u00" << hex[c >> 4] << hex[c & 0xf];
                            else
                                stream << str[i];
                            break;
                    }
                }
            }

            return stream << '"';
        }

        inline std::istream &operator>>(std::istream &stream, core::value &v)
        {
            char chr;

            stream >> std::skipws >> chr, stream.unget(); // Peek at next character past whitespace

            if (stream.good())
            {
                switch (chr)
                {
                    case 'n':
                        if (!core::stream_starts_with(stream, "null")) throw core::error("expected 'null' value");
                        v.set_null();
                        return stream;
                    case 't':
                        if (!core::stream_starts_with(stream, "true")) throw core::error("expected 'true' value");
                        v.set_bool(true);
                        return stream;
                    case 'f':
                        if (!core::stream_starts_with(stream, "false")) throw core::error("expected 'false' value");
                        v.set_bool(false);
                        return stream;
                    case '"':
                    {
                        std::string str;
                        read_string(stream, str);
                        v.set_string(str);
                        return stream;
                    }
                    case '[':
                        stream >> chr; // Eat '['
                        v.set_array(core::array_t());

                        // Peek at next character past whitespace
                        stream >> chr;
                        if (!stream) throw core::error("expected ']' ending array");
                        else if (chr == ']') return stream;

                        stream.unget(); // Replace character we peeked at
                        do
                        {
                            core::value item;
                            stream >> item;
                            v.push_back(item);

                            stream >> chr;
                            if (!stream || (chr != ',' && chr != ']'))
                                throw core::error("expected ',' separating array elements or ']' ending array");
                        } while (stream && chr != ']');

                        return stream;
                    case '{':
                        stream >> chr; // Eat '{'
                        v.set_object(core::object_t());

                        // Peek at next character past whitespace
                        stream >> chr;
                        if (!stream) throw core::error("expected '}' ending object");
                        else if (chr == '}') return stream;

                        stream.unget(); // Replace character we peeked at
                        do
                        {
                            std::string key;
                            core::value item;

                            read_string(stream, key);
                            stream >> chr;
                            if (chr != ':') throw core::error("expected ':' separating key and value in object");
                            stream >> item;
                            v[key] = item;

                            stream >> chr;
                            if (!stream || (chr != ',' && chr != '}'))
                                throw core::error("expected ',' separating key value pairs or '}' ending object");
                        } while (stream && chr != '}');

                        return stream;
                    default:
                        if (isdigit(chr) || chr == '-')
                        {
                            core::real_t r;
                            stream >> r;
                            if (!stream) throw core::error("invalid number");

                            if (r == trunc(r) && r >= INT64_MIN && r <= INT64_MAX)
                                v.set_int(static_cast<core::int_t>(r));
                            else
                                v.set_real(r);

                            return stream;
                        }
                        break;
                }
            }

            throw core::error("expected JSON value");
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            switch (v.get_type())
            {
                case core::null: return stream << "null";
                case core::boolean: return stream << (v.get_bool()? "true": "false");
                case core::integer: return stream << v.get_int();
                case core::real: return stream << v.get_real();
                case core::string: return write_string(stream, v.get_string());
                case core::array:
                {
                    stream << '[';
                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                    {
                        if (it != v.get_array().begin())
                            stream << ',';
                        stream << *it;
                    }
                    return stream << ']';
                }
                case core::object:
                {
                    stream << '{';
                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                    {
                        if (it != v.get_object().begin())
                            stream << ',';
                        write_string(stream, it->first) << ':' << it->second;
                    }
                    return stream << '}';
                }
            }

            // Control will never get here
            return stream;
        }

        inline std::istream &input(std::istream &stream, core::value &v) {return stream >> v;}
        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline std::ostream &pretty_print(std::ostream &stream, const core::value &v, size_t indent_width, size_t start_indent = 0)
        {
            switch (v.get_type())
            {
                case core::null: return stream << "null";
                case core::boolean: return stream << (v.get_bool()? "true": "false");
                case core::integer: return stream << v.get_int();
                case core::real: return stream << v.get_real();
                case core::string: return write_string(stream, v.get_string());
                case core::array:
                {
                    if (v.get_array().empty())
                        return stream << "[]";

                    stream << "[\n";
                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                    {
                        if (it != v.get_array().begin())
                            stream << ",\n";
                        stream << std::string(indent_width * (start_indent + 1), ' ');
                        pretty_print(stream, *it, indent_width, start_indent + 1);
                    }
                    return stream << '\n' << std::string(indent_width * start_indent, ' ') << ']';
                }
                case core::object:
                {
                    if (v.get_object().empty())
                        return stream << "{}";

                    stream << "{\n";
                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                    {
                        if (it != v.get_object().begin())
                            stream << ",\n";
                        stream << std::string(indent_width * (start_indent + 1), ' ');
                        pretty_print(write_string(stream, it->first) << ": ", it->second, indent_width, start_indent + 1);
                    }
                    return stream << '\n' << std::string(indent_width * start_indent, ' ') << '}';
                }
            }

            // Control will never get here
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
            pretty_print(stream, v, indent_width);
            return stream.str();
        }
    }

    namespace bencode
    {
        inline std::istream &operator>>(std::istream &stream, core::value &v)
        {
            int chr = stream.peek(); // Peek at next character

            if (chr != EOF)
            {
                switch (chr)
                {
                    case 'i':
                    {
                        stream.get(); // Eat 'i'

                        core::int_t i;
                        stream >> i;
                        if (!stream) throw core::error("expected 'integer' value");

                        v.set_int(i);
                        if (stream.get() != 'e') throw core::error("invalid 'integer' value");

                        return stream;
                    }
                    case 'l':
                        stream.get(); // Eat '['
                        v.set_array(core::array_t());

                        // Peek at next character
                        chr = stream.get();
                        if (!stream) throw core::error("expected 'e' ending list");
                        else if (chr == 'e') return stream;

                        do
                        {
                            stream.unget(); // Replace character we peeked at

                            core::value item;
                            stream >> item;
                            v.push_back(item);

                            chr = stream.get();
                            if (chr == EOF)
                                throw core::error("expected 'e' ending list");
                        } while (stream && chr != 'e');

                        return stream;
                    case 'd':
                        stream.get(); // Eat 'd'
                        v.set_object(core::object_t());

                        // Peek at next character
                        chr = stream.get();
                        if (!stream) throw core::error("expected 'e' ending dictionary");
                        else if (chr == 'e') return stream;

                        do
                        {
                            stream.unget(); // Replace character we peeked at

                            core::value key;
                            core::value item;

                            stream >> key >> item;

                            if (!key.is_string()) throw core::error("dictionary key is not a string");
                            v[key.get_string()] = item;

                            chr = stream.get();
                            if (chr == EOF)
                                throw core::error("expected 'e' ending dictionary");
                        } while (stream && chr != 'e');

                        return stream;
                    default:
                        if (isdigit(chr))
                        {
                            core::int_t size;
                            v.set_string(core::string_t());

                            stream >> size;
                            if (size < 0) throw core::error("expected string size");
                            if (stream.get() != ':') throw core::error("expected ':' separating string size and data");

                            // TODO: read string
                            v.get_string().reserve(size);
                            while (size--)
                            {
                                chr = stream.get();
                                if (chr == EOF) throw core::error("unexpected end of string");
                                v.get_string().push_back(chr);
                            }

                            return stream;
                        }
                        break;
                }
            }

            throw core::error("expected Bencode value");
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            switch (v.get_type())
            {
                case core::null: throw core::error("'null' value not allowed in Bencode output");
                case core::boolean: throw core::error("'boolean' value not allowed in Bencode output");
                case core::integer: return stream << 'i' << v.get_int() << 'e';
                case core::real: throw core::error("'real' value not allowed in Bencode output");
                case core::string: return stream << v.get_string().size() << ':' << v.get_string();
                case core::array:
                {
                    stream << 'l';
                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        stream << *it;
                    return stream << 'e';
                }
                case core::object:
                {
                    stream << 'd';
                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        stream << it->first.size() << ':' << it->first << it->second;
                    return stream << 'e';
                }
            }

            // Control will never get here
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
        inline std::istream &read_string(std::istream &stream, std::string &str)
        {
            static const std::string hex = "0123456789ABCDEF";
            int c;

            c = stream.get();
            if (c != '"') throw core::error("expected string");

            str.clear();
            while (c = stream.get(), c != '"')
            {
                if (c == EOF) throw core::error("unexpected end of string");

                if (c == '\\')
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("unexpected end of string");

                    switch (c)
                    {
                        case 'b': str.push_back('\b'); break;
                        case 'n': str.push_back('\n'); break;
                        case 'r': str.push_back('\r'); break;
                        case 't': str.push_back('\t'); break;
                        case 'U':
                        {
                            uint32_t code = 0;
                            for (int i = 0; i < 4; ++i)
                            {
                                c = stream.get();
                                if (c == EOF) throw core::error("unexpected end of string");
                                size_t pos = hex.find(toupper(c));
                                if (pos == std::string::npos) throw core::error("invalid character escape sequence");
                                code = (code << 4) | pos;
                            }

                            std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
                            str += utf8.to_bytes(code);

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
                                    if (c == EOF) throw core::error("unexpected end of string");
                                    if (!isdigit(c) || c == '8' || c == '9') throw core::error("invalid character escape sequence");
                                    code = (code << 3) | (c - '0');
                                }

                                std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
                                str += utf8.to_bytes(code);
                            }
                            else
                                str.push_back(c);

                            break;
                        }
                    }
                }
                else
                    str.push_back(c);
            }

            return stream;
        }

        inline std::ostream &write_string(std::ostream &stream, const std::string &str)
        {
            static const char hex[] = "0123456789ABCDEF";
            stream << '"';
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

                                    stream << "\\U" << hex[c >> 12] << hex[(c >> 8) & 0xf] << hex[(c >> 4) & 0xf] << hex[c & 0xf];
                                }
                            }
                            else
                                stream << str[i];
                            break;
                    }
                }
            }

            return stream << '"';
        }

        inline std::istream &operator>>(std::istream &stream, core::value &v)
        {
            static const std::string hex = "0123456789ABCDEF";
            char chr;

            stream >> std::skipws >> chr, stream.unget(); // Peek at next character past whitespace

            if (stream.good())
            {
                switch (chr)
                {
                    case '<':
                        stream >> chr; // Eat '<'
                        stream >> chr;
                        if (!stream) throw core::error("expected '*' after '<' in value");

                        if (chr != '*')
                        {
                            v.set_string(core::string_t());

                            unsigned int t = 0;
                            bool have_first_nibble = false;

                            while (stream && chr != '>')
                            {
                                t <<= 4;
                                size_t p = hex.find(toupper(static_cast<unsigned char>(chr)));
                                if (p == std::string::npos) throw core::error("expected hexadecimal-encoded binary data in value");
                                t |= p;

                                if (have_first_nibble)
                                    v.get_string().push_back(t);

                                have_first_nibble = !have_first_nibble;
                                stream >> chr;
                            }

                            if (have_first_nibble) throw core::error("unfinished byte in binary data");
                            return stream;
                        }

                        stream >> chr;
                        if (!stream || (chr != 'B' && chr != 'I' && chr != 'R'))
                            throw core::error("expected type specifier after '<*' in value");

                        if (chr == 'B')
                        {
                            stream >> chr;
                            if (!stream || (chr != 'Y' && chr != 'N'))
                                throw core::error("expected 'boolean' value after '<*B' in value");

                            v.set_bool(chr == 'Y');
                        }
                        else if (chr == 'I')
                        {
                            core::int_t i;
                            stream >> i;
                            if (!stream)
                                throw core::error("expected 'integer' value after '<*I' in value");

                            v.set_int(i);
                        }
                        else if (chr == 'R')
                        {
                            core::real_t r;
                            stream >> r;
                            if (!stream)
                                throw core::error("expected 'real' value after '<*R' in value");

                            v.set_real(r);
                        }

                        if (stream.get() != '>') throw core::error("expected '>' after value");
                        return stream;
                    case '"':
                    {
                        std::string str;
                        read_string(stream, str);
                        v.set_string(str);
                        return stream;
                    }
                    case '(':
                        stream >> chr; // Eat '('
                        v.set_array(core::array_t());

                        // Peek at next character past whitespace
                        stream >> chr;
                        if (!stream) throw core::error("expected ')' ending array");
                        else if (chr == ')') return stream;

                        stream.unget(); // Replace character we peeked at
                        do
                        {
                            core::value item;
                            stream >> item;
                            v.push_back(item);

                            stream >> chr;
                            if (!stream || (chr != ',' && chr != ')'))
                                throw core::error("expected ',' separating array elements or ')' ending array");
                        } while (stream && chr != ')');

                        return stream;
                    case '{':
                        stream >> chr; // Eat '{'
                        v.set_object(core::object_t());

                        // Peek at next character past whitespace
                        stream >> chr;
                        if (!stream) throw core::error("expected '}' ending object");
                        else if (chr == '}') return stream;

                        stream.unget(); // Replace character we peeked at
                        do
                        {
                            std::string key;
                            core::value item;

                            read_string(stream, key);
                            stream >> chr;
                            if (chr != '=') throw core::error("expected '=' separating key and value in object");
                            stream >> item;
                            v[key] = item;

                            stream >> chr;
                            if (!stream || chr != ';')
                                throw core::error("expected ';' after value in object");

                            stream >> chr, stream.unget();
                        } while (stream && chr != '}');

                        stream.get(); // Eat '}'
                        return stream;
                    default:
                        break;
                }
            }

            throw core::error("expected plain text property list value");
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            switch (v.get_type())
            {
                case core::null: throw core::error("'null' value not allowed in property list output");
                case core::boolean: return stream << "<*B" << (v.get_bool()? 'Y': 'N') << '>';
                case core::integer: return stream << "<*I" << v.get_int() << '>';
                case core::real: return stream << "<*R" << v.get_real() << '>';
                case core::string: return write_string(stream, v.get_string());
                case core::array:
                {
                    stream << '(';
                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                    {
                        if (it != v.get_array().begin())
                            stream << ',';
                        stream << *it;
                    }
                    return stream << ')';
                }
                case core::object:
                {
                    stream << '{';
                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        write_string(stream, it->first) << '=' << it->second << ';';
                    return stream << '}';
                }
            }

            // Control will never get here
            return stream;
        }

        inline std::ostream &pretty_print(std::ostream &stream, const core::value &v, size_t indent_width, size_t start_indent = 0)
        {
            switch (v.get_type())
            {
                case core::null: throw core::error("'null' value not allowed in property list output");
                case core::boolean: return stream << "<*B" << (v.get_bool()? 'Y': 'N') << '>';
                case core::integer: return stream << "<*I" << v.get_int() << '>';
                case core::real: return stream << "<*R" << v.get_real() << '>';
                case core::string: return write_string(stream, v.get_string());
                case core::array:
                {
                    if (v.get_array().empty())
                        return stream << "()";

                    stream << "(\n";
                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                    {
                        if (it != v.get_array().begin())
                            stream << ",\n";
                        stream << std::string(indent_width * (start_indent + 1), ' ');
                        pretty_print(stream, *it, indent_width, start_indent + 1);
                    }
                    return stream << '\n' << std::string(indent_width * start_indent, ' ') << ')';
                }
                case core::object:
                {
                    if (v.get_object().empty())
                        return stream << "{}";

                    stream << "{\n";
                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                    {
                        stream << std::string(indent_width * (start_indent + 1), ' ');
                        pretty_print(write_string(stream, it->first) << " = ", it->second, indent_width, start_indent + 1) << ";\n";
                    }
                    return stream << std::string(indent_width * start_indent, ' ') << '}';
                }
            }

            // Control will never get here
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

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            switch (v.get_type())
            {
                case core::null: throw core::error("'null' value not allowed in property list output");
                case core::boolean: return stream << "<" << (v.get_bool()? "true": "false") << "/>";
                case core::integer: return stream << "<integer>" << v.get_int() << "</integer>";
                case core::real: return stream << "<real>" << v.get_real() << "</real>";
                case core::string: return write_string(stream << "<string>", v.get_string()) << "</string>";
                case core::array:
                {
                    stream << "<array>";
                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        stream << *it;
                    return stream << "</array>";
                }
                case core::object:
                {
                    stream << "<dict>";
                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        write_string(stream << "<key>", it->first) << "</key>" << it->second;
                    return stream << "</dict>";
                }
            }

            // Control will never get here
            return stream;
        }

        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline std::string to_xml_property_list(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
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

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            switch (v.get_type())
            {
                case core::null: throw core::error("'null' value not allowed in XML-RPC output");
                case core::boolean: return stream << "<value><boolean>" << v.as_int() << "</boolean></value>";
                case core::integer: return stream << "<value><int>" << v.get_int() << "</int></value>";
                case core::real: return stream << "<value><double>" << v.get_real() << "</double></value>";
                case core::string: return write_string(stream << "<value><string>", v.get_string()) << "</string></value>";
                case core::array:
                {
                    stream << "<value><array><data>";
                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        stream << *it;
                    return stream << "</data></array></value>";
                }
                case core::object:
                {
                    stream << "<value><struct>";
                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        write_string(stream << "<member><name>", it->first) << "</name>" << it->second << "</member>";
                    return stream << "</struct></value>";
                }
            }

            // Control will never get here
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
            if (str.find_first_of("\t\r\n ") != std::string::npos || str.find('"') != std::string::npos) // Contains whitespace or double quote
            {
                stream << '"';
                for (size_t i = 0; i < str.size(); ++i)
                {
                    int c = str[i] & 0xff;

                    if (c == '"')
                        stream << '"';

                    stream << str[i];
                }
                return stream << '"';
            }

            return stream << str;
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            switch (v.get_type())
            {
                case core::null: return stream;
                case core::boolean: return stream << (v.get_bool()? "true": "false");
                case core::integer: return stream << v.get_int();
                case core::real: return stream << v.get_real();
                case core::string: return write_string(stream, v.get_string());
                case core::array:
                {
                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                    {
                        if (it != v.get_array().begin())
                            stream << ',';
                        stream << *it;
                    }
                    return stream;
                }
                case core::object: throw core::error("'object' value not allowed in CSV output");
            }

            // Control will never get here
            return stream;
        }

        inline std::ostream &print_row(std::ostream &stream, const core::value &v) {return stream << v;}
        inline std::ostream &print_table(std::ostream &stream, const core::value &v)
        {
            if (v.is_array())
            {
                for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                {
                    if (it != v.get_array().begin())
                        stream << '\n';
                    stream << *it;
                }
                return stream;
            }
            else
                return stream << v;
        }

        inline std::string to_csv_row(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }

        inline std::string to_csv_table(const core::value &v)
        {
            std::ostringstream stream;
            print_table(stream, v);
            return stream.str();
        }
    }

    namespace ubjson
    {
        inline char size_specifier(core::int_t min, core::int_t max)
        {
            if (min >= 0 && max < 256)
                return 'U';
            else if (min >= -128 && max < 128)
                return 'i';
            else if (min >= -32768 && max < 32768)
                return 'I';
            else if (min >= -2147483648 && max < 2147483648)
                return 'l';
            else
                return 'L';
        }

        inline std::istream &read_int(std::istream &stream, core::int_t &i, char specifier)
        {
            uint64_t temp;
            bool negative = false;
            int c = stream.get();

            if (c == EOF) throw core::error("expected integer value after type specifier");
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
                    if (c == EOF) throw core::error("expected integer value after type specifier");

                    temp = (temp << 8) | (c & 0xff);
                    temp |= negative * 0xffffffffffff0000ull;
                    break;
                case 'l': // Signed double-word
                    negative = c >> 7;

                    for (int i = 0; i < 3; ++i)
                    {
                        c = stream.get();
                        if (c == EOF) throw core::error("expected integer value after type specifier");

                        temp = (temp << 8) | (c & 0xff);
                    }
                    temp |= negative * 0xffffffff00000000ull;
                    break;
                case 'L': // Signed double-word
                    negative = c >> 7;

                    for (int i = 0; i < 7; ++i)
                    {
                        c = stream.get();
                        if (c == EOF) throw core::error("expected integer value after type specifier");

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

            if (c == EOF) throw core::error("expected integer value after type specifier");
            temp = c & 0xff;

            if (specifier == 'd')
            {
                for (int i = 0; i < 3; ++i)
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("expected floating-point value after type specifier");

                    temp = (temp << 8) | (c & 0xff);
                }
            }
            else
            {
                for (int i = 0; i < 7; ++i)
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("expected floating-point value after type specifier");

                    temp = (temp << 8) | (c & 0xff);
                }
            }

            if (specifier == 'd')
                r = core::float_from_ieee_754(temp);
            else
                r = core::double_from_ieee_754(temp);

            return stream;
        }

        inline std::istream &read_string(std::istream &stream, core::string_t &s, char specifier)
        {
            int c = stream.get();

            if (c == EOF) throw core::error("expected string value after type specifier");

            s.clear();
            if (specifier == 'C')
                s.push_back(c);
            else
            {
                core::int_t size;

                read_int(stream, size, c);
                if (size < 0) throw core::error("invalid negative size specified for string");

                while (size-- > 0)
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("expected string value after type specifier");

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

            if (force_bits == 0 && (i >= 0 && i < 256))
                return stream << (add_specifier? "U": "") << static_cast<unsigned char>(i);
            else if (force_bits <= 1 && (i >= -128 && i < 0))
                return stream << (add_specifier? "i": "") << static_cast<unsigned char>(0x80 | ((~std::abs(i) & 0xff) + 1));
            else if (force_bits <= 2 && (i >= -32768 && i < 32768))
            {
                uint16_t t;

                if (i < 0)
                    t = 0x8000u | ((~std::abs(i) & 0xffffu) + 1);
                else
                    t = i;

                return stream << (add_specifier? "I": "") << static_cast<unsigned char>(t >> 8) <<
                                                             static_cast<unsigned char>(t & 0xff);
            }
            else if (force_bits <= 3 && (i >= -2147483648 && i < 2147483648))
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

        inline std::ostream &write_string(std::ostream &stream, const std::string &str, bool add_specifier)
        {
            if (str.size() == 1 && static_cast<unsigned char>(str[0]) < 128)
                return stream << (add_specifier? "C": "") << str[0];

            if (add_specifier)
                stream << 'S';

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
                    case 'S': return read_string(stream, v.get_string(), c);
                    case 'N': break;
                    case '[':
                    {
                        int type = 0;
                        core::int_t size = 0;

                        c = stream.get();
                        if (c == EOF) throw core::error("expected array value after '['");

                        if (c == '$') // Type specified
                        {
                            c = stream.get();
                            if (c == EOF) throw core::error("expected type specifier after '$'");
                            type = c;
                            c = stream.get();
                            if (c == EOF) throw core::error("unexpected end of array");
                        }

                        v.set_array(core::array_t());
                        if (c == '#') // Count specified
                        {
                            c = stream.get();
                            if (c == EOF) throw core::error("expected count specifier after '#'");

                            read_int(stream, size, c);
                            if (size < 0) throw core::error("invalid negative size specified for array");

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
                            if (c == EOF) throw core::error("unexpected end of array");
                        }

                        return stream;
                    }
                    case '{':
                    {
                        int type = 0;
                        core::int_t size = 0;

                        c = stream.get();
                        if (c == EOF) throw core::error("expected object value after '{'");

                        if (c == '$') // Type specified
                        {
                            c = stream.get();
                            if (c == EOF) throw core::error("expected type specifier after '$'");
                            type = c;
                            c = stream.get();
                            if (c == EOF) throw core::error("unexpected end of object");
                        }

                        v.set_object(core::object_t());
                        if (c == '#') // Count specified
                        {
                            c = stream.get();
                            if (c == EOF) throw core::error("expected count specifier after '#'");

                            read_int(stream, size, c);
                            if (size < 0) throw core::error("invalid negative size specified for object");

                            while (size-- > 0)
                            {
                                core::value item;
                                std::string key;

                                read_string(stream, key, 'S');
                                input(stream, item, type);
                                v[key] = item;
                            }
                            return stream;
                        }

                        while (c != '}')
                        {
                            core::value item;
                            std::string key;

                            read_string(stream, key, 'S');
                            input(stream, item, c);
                            v[key] = item;

                            c = stream.get();
                            if (c == EOF) throw core::error("unexpected end of object");
                        }

                        return stream;
                    }
                }

                throw core::error("expected UBJSON value");
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
                case core::string: return write_string(stream, v.get_string(), add_specifier);
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
                            if (it->get_real() != core::float_from_ieee_754(core::float_to_ieee_754(it->get_real())))
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
                            if (it->second.get_real() != core::float_from_ieee_754(core::float_to_ieee_754(it->second.get_real())))
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
                            print(write_int(stream, it->first.size(), true) << it->first, it->second, false, forced_type);

                        return stream;
                    }
                    else
                    {
                        for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                            print(write_int(stream, it->first.size(), true) << it->first, it->second);

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
}

#endif // JSON_H

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
#include <math.h>

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
                case real: return lhs.get_real() == rhs.get_real();
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
                    return stream << '\n' << std::string(indent_width * start_indent, ' ') << "]";
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
                    return stream << '\n' << std::string(indent_width * start_indent, ' ') << "}";
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
                                stream << "\\U00" << hex[c >> 4] << hex[c & 0xf];
                            else
                                stream << str[i];
                            break;
                    }
                }
            }

            return stream << '"';
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

        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline std::string to_plain_text_property_list(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
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
                case core::boolean: return stream << "<" << (v.get_bool()? "true": "false") << " />";
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

        inline std::ostream &write_string(std::ostream &stream, const std::string &str, bool add_specifier)
        {
            if (str.size() == 1 && static_cast<unsigned char>(str[0]) < 128)
                return stream << (add_specifier? "C": "") << str[0];

            if (add_specifier)
                stream << 'S';

            write_int(stream, str.size(), true);

            return stream << str;
        }

        inline std::ostream &print(std::ostream &stream, const core::value &v, bool add_specifier = true, char force_specifier = 0)
        {
            switch (v.get_type())
            {
                case core::null: return add_specifier? stream << 'Z': stream;
                case core::boolean: return add_specifier? stream << (v.get_bool()? 'T': 'F'): stream;
                case core::integer: return write_int(stream, v.get_int(), add_specifier, force_specifier);
                case core::real: throw core::error("'real' values are not yet implemented in UBJSON output"); // TODO
                case core::string: return write_string(stream, v.get_string(), add_specifier);
                case core::array:
                {
                    core::type type = core::null;
                    char forced_type = 0;
                    bool same_types = true;

                    bool bool_val = false;
                    bool strings_can_be_chars = true;
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
                        else if (it->is_string() && strings_can_be_chars)
                        {
                            if (it->get_string().size() != 1 || static_cast<unsigned char>(it->get_string()[0]) >= 128)
                                strings_can_be_chars = false;
                        }
                    }

                    if (add_specifier)
                        stream << '[';

                    if (same_types)
                    {
                        stream << '$';
                        switch (type)
                        {
                            case core::null: stream << 'Z'; break;
                            case core::boolean: stream << (bool_val? 'T': 'F'); break;
                            case core::integer: stream << (forced_type = size_specifier(int_min, int_max)); break;
                            case core::real: break; // TODO
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
                        else if (it->second.is_string() && strings_can_be_chars)
                        {
                            if (it->second.get_string().size() != 1 || static_cast<unsigned char>(it->second.get_string()[0]) >= 128)
                                strings_can_be_chars = false;
                        }
                    }

                    if (add_specifier)
                        stream << '{';

                    if (same_types)
                    {
                        stream << '$';
                        switch (type)
                        {
                            case core::null: stream << 'Z'; break;
                            case core::boolean: stream << (bool_val? 'T': 'F'); break;
                            case core::integer: stream << (forced_type = size_specifier(int_min, int_max)); break;
                            case core::real: break; // TODO
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

                        return stream << ']';
                    }
                }
            }

            // Control will never get here
            return stream;
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v) {return print(stream, v);}

        inline std::string to_ubjson(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }
    }
}

#endif // JSON_H

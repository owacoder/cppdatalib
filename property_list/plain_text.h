#ifndef CPPDATALIB_PLAIN_TEXT_PROPERTY_LIST_H
#define CPPDATALIB_PLAIN_TEXT_PROPERTY_LIST_H

#include "../core/value_builder.h"

namespace cppdatalib
{
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
                        stream.get(); // Eat '<'
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
                        stream.get(); // Eat ','
                        if (writer.current_container_size() == 0 || writer.container_key_was_just_parsed())
                            throw core::error("Plain Text Property List - invalid ',' does not separate array or object entries");
                        stream >> chr; stream.unget(); // Peek ahead
                        if (!stream || chr == ',' || chr == ']' || chr == '}')
                            throw core::error("Plain Text Property List - invalid ',' does not separate array or object entries");
                        delimiter_required = false;
                        break;
                    case '=':
                        stream.get(); // Eat '='
                        if (!writer.container_key_was_just_parsed())
                            throw core::error("Plain Text Property List - invalid '=' does not separate a key and value pair");
                        delimiter_required = false;
                        break;
                    case '(':
                        stream.get(); // Eat '('
                        writer.begin_array(core::array_t(), core::stream_handler::unknown_size);
                        delimiter_required = false;
                        break;
                    case ')':
                        stream.get(); // Eat ')'
                        writer.end_array(core::array_t());
                        delimiter_required = true;
                        break;
                    case '{':
                        stream.get(); // Eat '{'
                        writer.begin_object(core::object_t(), core::stream_handler::unknown_size);
                        delimiter_required = false;
                        break;
                    case '}':
                        stream.get(); // Eat '}'
                        writer.end_object(core::object_t());
                        delimiter_required = true;
                        break;
                    default:
                        throw core::error("Plain Text Property List - expected value");
                        break;
                }
            }

            if (!delimiter_required)
                throw core::error("Plain Text Property List - expected value");

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

            void output_padding(size_t padding)
            {
                while (padding-- > 0)
                    output_stream << ' ';
            }

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
                    output_stream << '\n', output_padding(current_indent);
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    output_stream << ',';

                output_stream << '\n', output_padding(current_indent);

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
                    output_stream << '\n', output_padding(current_indent);

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
                    output_stream << '\n', output_padding(current_indent);

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
}

#endif // CPPDATALIB_PLAIN_TEXT_PROPERTY_LIST_H

/*
 * plain_text.h
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

#ifndef CPPDATALIB_PLAIN_TEXT_PROPERTY_LIST_H
#define CPPDATALIB_PLAIN_TEXT_PROPERTY_LIST_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace plain_text_property_list
    {
        class parser : public core::stream_parser
        {
            std::unique_ptr<char []> buffer;

        private:
            core::istream &read_string(core::stream_handler &writer)
            {
                static const std::string hex = "0123456789ABCDEF";

                int c;
                char *write = buffer.get();

                writer.begin_string(core::string_t(), core::stream_handler::unknown_size);
                while (c = input_stream.get(), c != '"' && c != EOF)
                {
                    if (c == '\\')
                    {
                        c = input_stream.get();
                        if (c == EOF) throw core::error("Plain Text Property List - unexpected end of string");

                        switch (c)
                        {
                            case 'b': *write++ = ('\b'); break;
                            case 'n': *write++ = ('\n'); break;
                            case 'r': *write++ = ('\r'); break;
                            case 't': *write++ = ('\t'); break;
                            case 'U':
                            {
                                uint32_t code = 0;
                                for (int i = 0; i < 4; ++i)
                                {
                                    c = input_stream.get();
                                    if (c == EOF) throw core::error("Plain Text Property List - unexpected end of string");
                                    size_t pos = hex.find(toupper(c));
                                    if (pos == std::string::npos) throw core::error("Plain Text Property List - invalid character escape sequence");
                                    code = (code << 4) | pos;
                                }

                                std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
                                std::string bytes = utf8.to_bytes(code);

                                memcpy(write, bytes.c_str(), bytes.size());
                                write += bytes.size();

                                break;
                            }
                            default:
                            {
                                if (isdigit(c))
                                {
                                    uint32_t code = 0;
                                    input_stream.unget();
                                    for (int i = 0; i < 3; ++i)
                                    {
                                        c = input_stream.get();
                                        if (c == EOF) throw core::error("Plain Text Property List - unexpected end of string");
                                        if (!isdigit(c) || c == '8' || c == '9') throw core::error("Plain Text Property List - invalid character escape sequence");
                                        code = (code << 3) | (c - '0');
                                    }

                                    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
                                    std::string bytes = utf8.to_bytes(code);

                                    memcpy(write, bytes.c_str(), bytes.size());
                                    write += bytes.size();
                                }
                                else
                                    *write++ = c;

                                break;
                            }
                        }
                    }
                    else
                        *write++ = c;

                    if (write - buffer.get() >= core::buffer_size)
                    {
                        *write = 0;
                        writer.append_to_string(buffer.get());
                        write = buffer.get();
                    }
                }

                if (c == EOF)
                    throw core::error("Plain Text Property List - unexpected end of string");

                if (write != buffer.get())
                {
                    *write = 0;
                    writer.append_to_string(buffer.get());
                }
                writer.end_string(core::string_t());
                return input_stream;
            }

        public:
            parser(core::istream &input)
                : core::stream_parser(input)
                , buffer(new char [core::buffer_size + core::max_utf8_code_sequence_size + 1])
            {}

            core::stream_input &convert(core::stream_handler &writer)
            {
                const std::string hex = "0123456789ABCDEF";
                bool delimiter_required = false;
                char chr;

                while (input_stream >> std::skipws >> chr, input_stream.good() && !input_stream.eof())
                {
                    if (writer.nesting_depth() == 0 && delimiter_required)
                        break;

                    if (delimiter_required && !strchr(",=)}", chr))
                        throw core::error("Plain Text Property List - expected ',' separating array or object entries");

                    switch (chr)
                    {
                        case '<':
                            input_stream >> chr;
                            if (!input_stream) throw core::error("Plain Text Property List - expected '*' after '<' in value");

                            if (chr != '*')
                            {
                                const core::value value_type(core::string_t(), core::blob);
                                writer.begin_string(value_type, core::stream_handler::unknown_size);

                                unsigned int t = 0;
                                bool have_first_nibble = false;

                                while (input_stream && chr != '>')
                                {
                                    t <<= 4;
                                    size_t p = hex.find(toupper(static_cast<unsigned char>(chr)));
                                    if (p == std::string::npos) throw core::error("Plain Text Property List - expected hexadecimal-encoded binary data in value");
                                    t |= p;

                                    if (have_first_nibble)
                                        writer.append_to_string(core::string_t(1, t));

                                    have_first_nibble = !have_first_nibble;
                                    input_stream >> chr;
                                }

                                if (have_first_nibble) throw core::error("Plain Text Property List - unfinished byte in binary data");

                                writer.end_string(value_type);
                                break;
                            }

                            input_stream >> chr;
                            if (!input_stream || !strchr("BIRD", chr))
                                throw core::error("Plain Text Property List - expected type specifier after '<*' in value");

                            if (chr == 'B')
                            {
                                input_stream >> chr;
                                if (!input_stream || (chr != 'Y' && chr != 'N'))
                                    throw core::error("Plain Text Property List - expected 'boolean' value after '<*B' in value");

                                writer.write(chr == 'Y');
                            }
                            else if (chr == 'I')
                            {
                                core::int_t i;
                                input_stream >> i;
                                if (!input_stream)
                                    throw core::error("Plain Text Property List - expected 'integer' value after '<*I' in value");

                                writer.write(i);
                            }
                            else if (chr == 'R')
                            {
                                core::real_t r;
                                input_stream >> r;
                                if (!input_stream)
                                    throw core::error("Plain Text Property List - expected 'real' value after '<*R' in value");

                                writer.write(r);
                            }
                            else if (chr == 'D')
                            {
                                int c;
                                const core::value value_type(core::string_t(), core::datetime);
                                writer.begin_string(value_type, core::stream_handler::unknown_size);
                                while (c = input_stream.get(), c != '>')
                                {
                                    if (c == EOF) throw core::error("Plain Text Property List - expected '>' after value");

                                    writer.append_to_string(core::string_t(1, c));
                                }
                                input_stream.unget();
                                writer.end_string(value_type);
                            }

                            input_stream >> chr;
                            if (chr != '>') throw core::error("Plain Text Property List - expected '>' after value");
                            break;
                        case '"':
                            read_string(writer);
                            delimiter_required = true;
                            break;
                        case ',':
                            if (writer.current_container_size() == 0 || writer.container_key_was_just_parsed())
                                throw core::error("Plain Text Property List - invalid ',' does not separate array or object entries");
                            input_stream >> chr; input_stream.unget(); // Peek ahead
                            if (!input_stream || chr == ',' || chr == ']' || chr == '}')
                                throw core::error("Plain Text Property List - invalid ',' does not separate array or object entries");
                            delimiter_required = false;
                            break;
                        case '=':
                            if (!writer.container_key_was_just_parsed())
                                throw core::error("Plain Text Property List - invalid '=' does not separate a key and value pair");
                            delimiter_required = false;
                            break;
                        case '(':
                            writer.begin_array(core::array_t(), core::stream_handler::unknown_size);
                            delimiter_required = false;
                            break;
                        case ')':
                            writer.end_array(core::array_t());
                            delimiter_required = true;
                            break;
                        case '{':
                            writer.begin_object(core::object_t(), core::stream_handler::unknown_size);
                            delimiter_required = false;
                            break;
                        case '}':
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

                return *this;
            }
        };

        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(core::ostream &stream) : core::stream_writer(stream) {}

            protected:
                core::ostream &write_string(core::ostream &stream, const std::string &str)
                {
                    for (size_t i = 0; i < str.size(); ++i)
                    {
                        int c = str[i] & 0xff;

                        if (c == '"' || c == '\\')
                        {
                            stream.put('\\');
                            stream.put(c);
                        }
                        else
                        {
                            switch (c)
                            {
                                case '"':
                                case '\\': stream.put('\\'); stream.put(c); break;
                                case '\b': stream.write("\\b", 2); break;
                                case '\n': stream.write("\\n", 2); break;
                                case '\r': stream.write("\\r", 2); break;
                                case '\t': stream.write("\\t", 2); break;
                                default:
                                    if (iscntrl(c))
                                        stream.put('\\').put(c >> 6).put((c >> 3) & 0x7).put(c & 0x7);
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

                                            hex::write(stream.write("\\U", 2), c >> 8);
                                            hex::write(stream, c & 0xff);
                                        }
                                    }
                                    else
                                        stream.put(str[i]);
                                    break;
                            }
                        }
                    }

                    return stream;
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
        public:
            stream_writer(core::ostream &output) : impl::stream_writer_base(output) {}

        protected:
            void begin_() {output_stream.precision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    output_stream.put('=');
                else if (current_container_size() > 0)
                    output_stream.put(',');
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    output_stream.put(',');

                if (!v.is_string())
                    throw core::error("Plain Text Property List - cannot write non-string key");
            }

            void null_(const core::value &) {throw core::error("Plain Text Property List - 'null' value not allowed in output");}
            void bool_(const core::value &v)
            {
                output_stream << "<*B";
                output_stream.put(v.get_bool()? 'Y': 'N');
                output_stream.put('>');
            }
            void integer_(const core::value &v)
            {
                output_stream << "<*I"
                              << v.get_int();
                output_stream.put('>');
            }
            void uinteger_(const core::value &v)
            {
                output_stream << "<*I"
                              << v.get_uint();
                output_stream.put('>');
            }
            void real_(const core::value &v)
            {
                output_stream << "<*R"
                              << v.get_real();
                output_stream.put('>');
            }
            void begin_string_(const core::value &v, core::int_t, bool)
            {
                switch (v.get_subtype())
                {
                    case core::date:
                    case core::time:
                    case core::datetime: output_stream << "<*D"; break;
                    case core::blob:
                    case core::clob: output_stream.put('<'); break;
                    default: output_stream.put('"'); break;
                }
            }
            void string_data_(const core::value &v, bool)
            {
                if (v.get_subtype() == core::blob || v.get_subtype() == core::clob)
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
                    case core::blob:
                    case core::clob: output_stream.put('>'); break;
                    default: output_stream.put('"'); break;
                }
            }

            void begin_array_(const core::value &, core::int_t, bool) {output_stream.put('(');}
            void end_array_(const core::value &, bool) {output_stream.put(')');}

            void begin_object_(const core::value &, core::int_t, bool) {output_stream.put('{');}
            void end_object_(const core::value &, bool) {output_stream.put('}');}
        };

        class pretty_stream_writer : public impl::stream_writer_base
        {
            std::unique_ptr<char []> buffer;
            size_t indent_width;
            size_t current_indent;

            void output_padding(size_t padding)
            {
                while (padding > 0)
                {
                    size_t size = std::min(size_t(core::buffer_size-1), padding);

                    memset(buffer.get(), ' ', size);

                    output_stream.write(buffer.get(), size);
                    padding -= size;
                }
            }

        public:
            pretty_stream_writer(core::ostream &output, size_t indent_width)
                : impl::stream_writer_base(output)
                , buffer(new char [core::buffer_size])
                , indent_width(indent_width)
                , current_indent(0)
            {}

            size_t indent() {return indent_width;}

        protected:
            void begin_() {current_indent = 0; output_stream.precision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    output_stream << " = ";
                else if (current_container_size() > 0)
                    output_stream.put(',');

                if (current_container() == core::array)
                    output_stream.put('\n'), output_padding(current_indent);
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    output_stream.put(',');

                output_stream.put('\n'), output_padding(current_indent);

                if (!v.is_string())
                    throw core::error("Plain Text Property List - cannot write non-string key");
            }

            void null_(const core::value &) {throw core::error("Plain Text Property List - 'null' value not allowed in output");}
            void bool_(const core::value &v)
            {
                output_stream << "<*B";
                output_stream.put(v.get_bool()? 'Y': 'N');
                output_stream.put('>');
            }
            void integer_(const core::value &v)
            {
                output_stream << "<*I"
                              << v.get_int();
                output_stream.put('>');
            }
            void uinteger_(const core::value &v)
            {
                output_stream << "<*I"
                              << v.get_uint();
                output_stream.put('>');
            }
            void real_(const core::value &v)
            {
                output_stream << "<*R"
                              << v.get_real();
                output_stream.put('>');
            }
            void begin_string_(const core::value &v, core::int_t, bool)
            {
                switch (v.get_subtype())
                {
                    case core::date:
                    case core::time:
                    case core::datetime: output_stream << "<*D"; break;
                    case core::blob:
                    case core::clob: output_stream.put('<'); break;
                    default: output_stream.put('"'); break;
                }
            }
            void string_data_(const core::value &v, bool)
            {
                if (v.get_subtype() == core::blob || v.get_subtype() == core::clob)
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
                    case core::blob:
                    case core::clob: output_stream.put('>'); break;
                    default: output_stream.put('"'); break;
                }
            }

            void begin_array_(const core::value &, core::int_t, bool)
            {
                output_stream.put('(');
                current_indent += indent_width;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream.put('\n'), output_padding(current_indent);

                output_stream.put(')');
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                output_stream.put('{');
                current_indent += indent_width;
            }
            void end_object_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream.put('\n'), output_padding(current_indent);

                output_stream.put('}');
            }
        };

        inline core::value from_plain_text_property_list(const std::string &property_list)
        {
            core::istring_wrapper_stream stream(property_list);
            parser p(stream);
            core::value v;
            p >> v;
            return v;
        }

        inline std::string to_plain_text_property_list(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer writer(stream);
            writer << v;
            return stream.str();
        }

        inline std::string to_pretty_plain_text_property_list(const core::value &v, size_t indent_width)
        {
            core::ostringstream stream;
            pretty_stream_writer writer(stream, indent_width);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_PLAIN_TEXT_PROPERTY_LIST_H

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
            bool delimiter_required;

        private:
            core::istream &read_string(core::stream_handler &writer)
            {
                static const std::string hex = "0123456789ABCDEF";

                int c;
                char *write = buffer.get();

                writer.begin_string(core::string_t(), core::stream_handler::unknown_size);
                while (c = stream().get(), c != '"' && c != EOF)
                {
                    if (c == '\\')
                    {
                        c = stream().get();
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
                                    c = stream().get();
                                    if (c == EOF) throw core::error("Plain Text Property List - unexpected end of string");
                                    size_t pos = hex.find(toupper(c));
                                    if (pos == std::string::npos) throw core::error("Plain Text Property List - invalid character escape sequence");
                                    code = (code << 4) | pos;
                                }

#ifdef CPPDATALIB_MSVC
								std::wstring_convert<std::codecvt_utf8<unsigned int>, unsigned int> utf8;
#else
								std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
#endif
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
                                    stream().unget();
                                    for (int i = 0; i < 3; ++i)
                                    {
                                        c = stream().get();
                                        if (c == EOF) throw core::error("Plain Text Property List - unexpected end of string");
                                        if (!isdigit(c) || c == '8' || c == '9') throw core::error("Plain Text Property List - invalid character escape sequence");
                                        code = (code << 3) | (c - '0');
                                    }

#ifdef CPPDATALIB_MSVC
									std::wstring_convert<std::codecvt_utf8<unsigned int>, unsigned int> utf8;
#else
									std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
#endif
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
                return stream();
            }

        public:
            parser(core::istream_handle input)
                : core::stream_parser(input)
                , buffer(new char [core::buffer_size + core::max_utf8_code_sequence_size + 1])
            {
                reset();
            }            

        protected:
            void reset_()
            {
                delimiter_required = false;
                stream() >> std::skipws;
            }

            void write_one_()
            {
                const std::string hex = "0123456789ABCDEF";
                bool written = false;
                char chr;

                if (stream() >> chr, stream())
                {
                    if (get_output()->nesting_depth() == 0 && delimiter_required)
                        goto finish;

                    if (delimiter_required && !strchr(",=)}", chr))
                        throw core::error("Plain Text Property List - expected ',' separating array or object entries");

                    switch (chr)
                    {
                        case '<':
                            stream() >> chr;
                            if (!stream()) throw core::error("Plain Text Property List - expected '*' after '<' in value");

                            if (chr != '*')
                            {
                                const core::value value_type(core::string_t(), core::blob);
                                get_output()->begin_string(value_type, core::stream_handler::unknown_size);

                                unsigned int t = 0;
                                bool have_first_nibble = false;

                                while (stream() && chr != '>')
                                {
                                    t <<= 4;
                                    size_t p = hex.find(toupper(static_cast<unsigned char>(chr)));
                                    if (p == std::string::npos) throw core::error("Plain Text Property List - expected hexadecimal-encoded binary data in value");
                                    t |= p;

                                    if (have_first_nibble)
                                        get_output()->append_to_string(core::string_t(1, t));

                                    have_first_nibble = !have_first_nibble;
                                    stream() >> chr;
                                }

                                if (have_first_nibble) throw core::error("Plain Text Property List - unfinished byte in binary data");

                                get_output()->end_string(value_type);
                                break;
                            }

                            stream() >> chr;
                            if (!stream() || !strchr("BIRD", chr))
                                throw core::error("Plain Text Property List - expected type specifier after '<*' in value");

                            if (chr == 'B')
                            {
                                stream() >> chr;
                                if (!stream() || (chr != 'Y' && chr != 'N'))
                                    throw core::error("Plain Text Property List - expected 'boolean' value after '<*B' in value");

                                get_output()->write(chr == 'Y');
                            }
                            else if (chr == 'I')
                            {
                                core::int_t i;
                                stream() >> i;
                                if (!stream())
                                    throw core::error("Plain Text Property List - expected 'integer' value after '<*I' in value");

                                get_output()->write(i);
                            }
                            else if (chr == 'R')
                            {
                                core::real_t r;
                                stream() >> r;
                                if (!stream())
                                    throw core::error("Plain Text Property List - expected 'real' value after '<*R' in value");

                                get_output()->write(r);
                            }
                            else if (chr == 'D')
                            {
                                int c;
                                const core::value value_type(core::string_t(), core::datetime);
                                get_output()->begin_string(value_type, core::stream_handler::unknown_size);
                                while (c = stream().get(), c != '>')
                                {
                                    if (c == EOF) throw core::error("Plain Text Property List - expected '>' after value");

                                    get_output()->append_to_string(core::string_t(1, c));
                                }
                                stream().unget();
                                get_output()->end_string(value_type);
                            }

                            stream() >> chr;
                            if (chr != '>') throw core::error("Plain Text Property List - expected '>' after value");
                            break;
                        case '"':
                            read_string(*get_output());
                            delimiter_required = true;
                            break;
                        case ',':
                            if (get_output()->current_container_size() == 0 || get_output()->container_key_was_just_parsed())
                                throw core::error("Plain Text Property List - invalid ',' does not separate array or object entries");
                            stream() >> chr; stream().unget(); // Peek ahead
                            if (!stream() || chr == ',' || chr == ']' || chr == '}')
                                throw core::error("Plain Text Property List - invalid ',' does not separate array or object entries");
                            delimiter_required = false;
                            break;
                        case '=':
                            if (!get_output()->container_key_was_just_parsed())
                                throw core::error("Plain Text Property List - invalid '=' does not separate a key and value pair");
                            delimiter_required = false;
                            break;
                        case '(':
                            get_output()->begin_array(core::array_t(), core::stream_handler::unknown_size);
                            delimiter_required = false;
                            break;
                        case ')':
                            get_output()->end_array(core::array_t());
                            delimiter_required = true;
                            break;
                        case '{':
                            get_output()->begin_object(core::object_t(), core::stream_handler::unknown_size);
                            delimiter_required = false;
                            break;
                        case '}':
                            get_output()->end_object(core::object_t());
                            delimiter_required = true;
                            break;
                        default:
                            throw core::error("Plain Text Property List - expected value");
                            break;
                    }

                    written = true;
                }

finish:
                if (!written)
                    throw core::error("Plain Text Property List - unexpected end of stream");
            }
        };

        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(core::ostream_handle &stream) : core::stream_writer(stream) {}

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
            stream_writer(core::ostream_handle output) : impl::stream_writer_base(output) {}

        protected:
            void begin_() {stream().precision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    stream().put('=');
                else if (current_container_size() > 0)
                    stream().put(',');
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    stream().put(',');

                if (!v.is_string())
                    throw core::error("Plain Text Property List - cannot write non-string key");
            }

            void null_(const core::value &) {throw core::error("Plain Text Property List - 'null' value not allowed in output");}
            void bool_(const core::value &v)
            {
                stream() << "<*B";
                stream().put(v.get_bool_unchecked()? 'Y': 'N');
                stream().put('>');
            }
            void integer_(const core::value &v)
            {
                stream() << "<*I"
                              << v.get_int_unchecked();
                stream().put('>');
            }
            void uinteger_(const core::value &v)
            {
                stream() << "<*I"
                              << v.get_uint_unchecked();
                stream().put('>');
            }
            void real_(const core::value &v)
            {
                stream() << "<*R"
                              << v.get_real_unchecked();
                stream().put('>');
            }
            void begin_string_(const core::value &v, core::int_t, bool)
            {
                switch (v.get_subtype())
                {
                    case core::date:
                    case core::time:
                    case core::datetime: stream() << "<*D"; break;
                    case core::blob:
                    case core::clob: stream().put('<'); break;
                    default: stream().put('"'); break;
                }
            }
            void string_data_(const core::value &v, bool)
            {
                if (v.get_subtype() == core::blob || v.get_subtype() == core::clob)
                    hex::write(stream(), v.get_string_unchecked());
                else
                    write_string(stream(), v.get_string_unchecked());
            }
            void end_string_(const core::value &v, bool)
            {
                switch (v.get_subtype())
                {
                    case core::date:
                    case core::time:
                    case core::datetime:
                    case core::blob:
                    case core::clob: stream().put('>'); break;
                    default: stream().put('"'); break;
                }
            }

            void begin_array_(const core::value &, core::int_t, bool) {stream().put('(');}
            void end_array_(const core::value &, bool) {stream().put(')');}

            void begin_object_(const core::value &, core::int_t, bool) {stream().put('{');}
            void end_object_(const core::value &, bool) {stream().put('}');}
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

                    stream().write(buffer.get(), size);
                    padding -= size;
                }
            }

        public:
            pretty_stream_writer(core::ostream_handle output, size_t indent_width)
                : impl::stream_writer_base(output)
                , buffer(new char [core::buffer_size])
                , indent_width(indent_width)
                , current_indent(0)
            {}

            size_t indent() {return indent_width;}

        protected:
            void begin_() {current_indent = 0; stream().precision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    stream() << " = ";
                else if (current_container_size() > 0)
                    stream().put(',');

                if (current_container() == core::array)
                    stream().put('\n'), output_padding(current_indent);
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    stream().put(',');

                stream().put('\n'), output_padding(current_indent);

                if (!v.is_string())
                    throw core::error("Plain Text Property List - cannot write non-string key");
            }

            void null_(const core::value &) {throw core::error("Plain Text Property List - 'null' value not allowed in output");}
            void bool_(const core::value &v)
            {
                stream() << "<*B";
                stream().put(v.get_bool_unchecked()? 'Y': 'N');
                stream().put('>');
            }
            void integer_(const core::value &v)
            {
                stream() << "<*I"
                              << v.get_int_unchecked();
                stream().put('>');
            }
            void uinteger_(const core::value &v)
            {
                stream() << "<*I"
                              << v.get_uint_unchecked();
                stream().put('>');
            }
            void real_(const core::value &v)
            {
                stream() << "<*R"
                              << v.get_real_unchecked();
                stream().put('>');
            }
            void begin_string_(const core::value &v, core::int_t, bool)
            {
                switch (v.get_subtype())
                {
                    case core::date:
                    case core::time:
                    case core::datetime: stream() << "<*D"; break;
                    case core::blob:
                    case core::clob: stream().put('<'); break;
                    default: stream().put('"'); break;
                }
            }
            void string_data_(const core::value &v, bool)
            {
                if (v.get_subtype() == core::blob || v.get_subtype() == core::clob)
                    hex::write(stream(), v.get_string_unchecked());
                else
                    write_string(stream(), v.get_string_unchecked());
            }
            void end_string_(const core::value &v, bool)
            {
                switch (v.get_subtype())
                {
                    case core::date:
                    case core::time:
                    case core::datetime:
                    case core::blob:
                    case core::clob: stream().put('>'); break;
                    default: stream().put('"'); break;
                }
            }

            void begin_array_(const core::value &, core::int_t, bool)
            {
                stream().put('(');
                current_indent += indent_width;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    stream().put('\n'), output_padding(current_indent);

                stream().put(')');
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                stream().put('{');
                current_indent += indent_width;
            }
            void end_object_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    stream().put('\n'), output_padding(current_indent);

                stream().put('}');
            }
        };

        inline core::value from_plain_text_property_list(core::istream_handle stream)
        {
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

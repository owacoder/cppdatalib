/*
 * json.h
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

#ifndef CPPDATALIB_JSON_H
#define CPPDATALIB_JSON_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace json
    {
        class parser : public core::stream_parser
        {
            std::unique_ptr<char []> buffer;
            bool delimiter_required;

        private:
            // Opening quote should already be read
            core::istream &read_string(core::istream &stream, core::stream_handler &writer)
            {
                static const std::string hex = "0123456789ABCDEF";

                core::value str_type((const char *) "", core::normal, true);
                core::istream::int_type c;
                char *write = buffer.get();

                writer.begin_string(str_type, core::stream_handler::unknown_size());
                while (c = stream.get(), c != '"' && c != EOF)
                {
                    if (c == '\\')
                    {
                        c = stream.get();
                        if (c == EOF) throw core::error("JSON - unexpected end of string");

                        switch (c)
                        {
                            case 'b': *write++ = ('\b'); break;
                            case 'f': *write++ = ('\f'); break;
                            case 'n': *write++ = ('\n'); break;
                            case 'r': *write++ = ('\r'); break;
                            case 't': *write++ = ('\t'); break;
                            case 'u':
                            {
                                uint32_t code = 0;
                                for (int i = 0; i < 4; ++i)
                                {
                                    c = stream.get();
                                    if (c == EOF) throw core::error("JSON - unexpected end of string");
                                    size_t pos = hex.find(toupper(c));
                                    if (pos == std::string::npos) throw core::error("JSON - invalid character escape sequence");
                                    code = (code << 4) | uint32_t(pos);
                                }

                                if (code >= 0xd800 && code <= 0xdbff)
                                {
                                    char buf[2];
                                    if (!stream.read(buf, 2) || memcmp(buf, "\\u", 2))
                                        throw core::error("JSON - invalid character escape, expected low surrogate of UTF-16 pair");

                                    uint32_t code2 = 0;
                                    for (int i = 0; i < 4; ++i)
                                    {
                                        c = stream.get();
                                        if (c == EOF) throw core::error("JSON - unexpected end of string");
                                        size_t pos = hex.find(toupper(c));
                                        if (pos == std::string::npos) throw core::error("JSON - invalid character escape sequence");
                                        code2 = (code2 << 4) | uint32_t(pos);
                                    }

                                    if (code2 < 0xdc00 || code2 > 0xdfff)
                                        throw core::error("JSON - invalid character escape, expected low surrogate of UTF-16 pair");

                                    code = ((code << 16) | code2) - 0xd7ffdc00; // Subtract metadata value of both surrogates, and add 0x10000
                                }

                                core::string_t bytes = core::ucs_to_utf8(code);

                                memcpy(write, bytes.c_str(), bytes.size());
                                write += bytes.size();

                                break;
                            }
                            default:
                                *write++ = c;
                                break;
                        }
                    }
                    else
                        *write++ = c;

                    if (write - buffer.get() >= core::buffer_size)
                    {
                        *write = 0;
                        writer.append_to_string(core::value(buffer.get(), write - buffer.get(), core::normal, true));
                        write = buffer.get();
                    }
                }

                if (c == EOF)
                    throw core::error("JSON - unexpected end of string");

                if (write != buffer.get())
                {
                    *write = 0;
                    writer.append_to_string(core::value(buffer.get(), write - buffer.get(), core::normal, true));
                }
                writer.end_string(str_type);
                return stream;
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
                char chr;

                if (stream() >> chr, stream().good())
                {
                    if (delimiter_required)
                    {
                        if (get_output()->nesting_depth() == 0)
                            throw core::error("JSON - unexpected end of stream");
                        else if (!strchr(",:]}", chr))
                            throw core::error("JSON - expected ',' separating array or object entries");
                    }

                    switch (chr)
                    {
                        case 'n':
                            if (!core::stream_starts_with(stream(), "ull")) throw core::error("JSON - expected 'null' value");
                            get_output()->write(core::null_t());
                            delimiter_required = true;
                            break;
                        case 't':
                            if (!core::stream_starts_with(stream(), "rue")) throw core::error("JSON - expected 'true' value");
                            get_output()->write(core::value(true));
                            delimiter_required = true;
                            break;
                        case 'f':
                            if (!core::stream_starts_with(stream(), "alse")) throw core::error("JSON - expected 'false' value");
                            get_output()->write(core::value(false));
                            delimiter_required = true;
                            break;
                        case '"':
                            read_string(stream(), *get_output());
                            delimiter_required = true;
                            break;
                        case ',':
                            if (get_output()->current_container_size() == 0 || get_output()->container_key_was_just_parsed())
                                throw core::error("JSON - invalid ',' does not separate array or object entries");
                            stream() >> chr; // Peek ahead
                            if (!stream() || chr == ',' || chr == ']' || chr == '}')
                                throw core::error("JSON - invalid ',' does not separate array or object entries");
                            stream().unget();
                            delimiter_required = false;
                            break;
                        case ':':
                            if (!get_output()->container_key_was_just_parsed())
                                throw core::error("JSON - invalid ':' does not separate a key and value pair");
                            delimiter_required = false;
                            break;
                        case '[':
                            get_output()->begin_array(core::array_t(), core::stream_handler::unknown_size());
                            delimiter_required = false;
                            break;
                        case ']':
                            get_output()->end_array(core::array_t());
                            delimiter_required = true;
                            break;
                        case '{':
                            get_output()->begin_object(core::object_t(), core::stream_handler::unknown_size());
                            delimiter_required = false;
                            break;
                        case '}':
                            get_output()->end_object(core::object_t());
                            delimiter_required = true;
                            break;
                        default:
                            if (isdigit(chr) || chr == '-')
                            {
                                if (get_output()->current_container() == core::object && !get_output()->container_key_was_just_parsed()) // This is the key?
                                    throw core::error("JSON - invalid number cannot be used as an object key");

                                bool is_float = false;
                                std::string buffer = std::string(1, chr);

                                core::istream::int_type c;
                                while (c = stream().get(), c != EOF && c < 0x80 && (isdigit(c) || strchr(".eE+-", c)))
                                {
                                    buffer.push_back(c);
                                    if (!is_float && (c == '.' || c == 'e' || c == 'E'))
                                        is_float = true;
                                }
                                stream().unget();
                                delimiter_required = true;

                                if (!is_float)
                                {
                                    // Attempt to read as an integer
                                    {
                                        core::istring_wrapper_stream temp_stream(buffer);
                                        core::int_t value = 0;
                                        temp_stream >> value;
                                        if (!temp_stream.fail() && temp_stream.get() == EOF)
                                        {
                                            get_output()->write(core::value(value));
                                            break; // break switch
                                        }
                                    }

                                    // Attempt to read as an unsigned integer
                                    {
                                        core::istring_wrapper_stream temp_stream(buffer);
                                        core::uint_t value = 0;
                                        temp_stream >> value;
                                        if (!temp_stream.fail() && temp_stream.get() == EOF)
                                        {
                                            get_output()->write(core::value(value));
                                            break; // break switch
                                        }
                                    }
                                }
                                // Attempt to read as a real
                                else
                                {
                                    core::istring_wrapper_stream temp_stream(buffer);
                                    core::real_t value = 0.0;
                                    temp_stream >> value;
                                    if (!temp_stream.fail() && temp_stream.get() == EOF)
                                    {
                                        get_output()->write(core::value(value));
                                        break; // break switch
                                    }
                                }

                                // Revert to bignum
                                get_output()->write(core::value(std::move(buffer), core::bignum));
                            }
                            else
                                throw core::error("JSON - expected value");
                            break;
                    }
                }
                else
                    throw core::error("JSON - unexpected end of stream");
            }
        };

        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(core::ostream_handle &stream) : core::stream_writer(stream) {}

            protected:
                core::ostream &write_string(core::ostream &stream, core::string_view_t str)
                {
                    for (size_t i = 0; i < str.size();)
                    {
                        uint32_t c = core::utf8_to_ucs(str, i, i);

                        switch (c)
                        {
                            case '"':
                            case '\\':
                                stream.put('\\');
                                stream.put(c);
                                break;
                            default:
                                if (c < 0x80)
                                {
                                    if (iscntrl(c))
                                    {
                                        switch (c)
                                        {
                                            case '\b': stream.write("\\b", 2); break;
                                            case '\f': stream.write("\\f", 2); break;
                                            case '\n': stream.write("\\n", 2); break;
                                            case '\r': stream.write("\\r", 2); break;
                                            case '\t': stream.write("\\t", 2); break;
                                            default:
                                                hex::write(stream.write("\\u00", 4), c);
                                                break;
                                        }
                                    }
                                    else
                                        stream.put(c);
                                }
                                else if (c >= 0xd800 && c <= 0xdfff)
                                    throw core::error("JSON - invalid UTF-8 string");
                                else // c >= 0x80
                                {
                                    char buf[16];

                                    buf[0] = '\\';
                                    buf[1] = 'u';

                                    if (c <= 0xffff)
#ifdef CPPDATALIB_MSVC
                                        sprintf_s(buf+2, 5, "%04x", (unsigned) c);
#else
										std::sprintf(buf+2, "%04x", (unsigned) c);
#endif
                                    else
                                    {
                                        c -= 0x10000;
                                        c = ((c << 6) & 0x3ff0000) | (c & 0x3ff); // Place top ten and lower ten bits in proper positions
                                        c += 0xd800dc00; // Add UTF-16 surrogate values

#ifdef CPPDATALIB_MSVC
										sprintf_s(buf+2, 5, "%04x", (unsigned) (c >> 16));
										sprintf_s(buf+8, 5, "%04x", (unsigned) (c & 0xffff));
#else
                                        std::sprintf(buf+2, "%04x", (unsigned) (c >> 16));
                                        std::sprintf(buf+8, "%04x", (unsigned) (c & 0xffff));
#endif

                                        // This assignments must be done afterward because the first sprintf will write a NUL to buf[6]
                                        buf[6] = '\\';
                                        buf[7] = 'u';
                                    }

                                    stream << buf;
                                }
                                break;
                            case uint32_t(-1):
                                throw core::error("JSON - invalid UTF-8 string");
                        }
                    }

                    return stream;
                }

                core::ostream &write_blob_string(core::ostream &stream, core::string_view_t str)
                {
                    for (size_t i = 0; i < str.size(); ++i)
                    {
                        int c = str[i] & 0xff;

                        switch (c)
                        {
                            case '"':
                            case '\\':
                                stream.put('\\');
                                stream.put(c);
                                break;
                            default:
                                if (c > 0x7f || iscntrl(c))
                                {
                                    switch (c)
                                    {
                                        case '\b': stream.write("\\b", 2); break;
                                        case '\f': stream.write("\\f", 2); break;
                                        case '\n': stream.write("\\n", 2); break;
                                        case '\r': stream.write("\\r", 2); break;
                                        case '\t': stream.write("\\t", 2); break;
                                        default: hex::write(stream.write("\\u00", 4), c); break;
                                    }
                                }
                                else
                                    stream.put(c);
                                break;
                        }
                    }

                    return stream;
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
        public:
            stream_writer(core::ostream_handle output) : stream_writer_base(output) {}

            std::string name() const {return "cppdatalib::json::stream_writer";}

        protected:
            void begin_() {stream().precision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    stream().put(':');
                else if (current_container_size() > 0)
                    stream().put(',');
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    stream().put(',');

                if (!v.is_string())
                    throw core::error("JSON - cannot write non-string key");
            }

            void null_(const core::value &) {stream() << "null";}
            void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true": "false");}
            void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
            void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
            void real_(const core::value &v)
            {
                if (!std::isfinite(v.get_real_unchecked()))
                    throw core::error("JSON - cannot write 'NaN' or 'Infinity' values");
                stream() << v.get_real_unchecked();
            }
            void begin_string_(const core::value &v, core::optional_size, bool is_key) {if (v.get_subtype() != core::bignum || is_key) stream().put('"');}
            void string_data_(const core::value &v, bool)
            {
                if (!core::subtype_is_text_string(current_container_subtype()))
                    write_blob_string(stream(), v.get_string_unchecked());
                else
                    write_string(stream(), v.get_string_unchecked());
            }
            void end_string_(const core::value &v, bool is_key) {if (v.get_subtype() != core::bignum || is_key) stream().put('"');}

            void begin_array_(const core::value &, core::optional_size, bool) {stream().put('[');}
            void end_array_(const core::value &, bool) {stream().put(']');}

            void begin_object_(const core::value &, core::optional_size, bool) {stream().put('{');}
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
                : stream_writer_base(output)
                , buffer(new char [core::buffer_size])
                , indent_width(indent_width)
                , current_indent(0)
            {}

            size_t indent() {return indent_width;}

            std::string name() const {return "cppdatalib::json::pretty_stream_writer";}

        protected:
            void begin_() {current_indent = 0; stream().precision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    stream() << ": ";
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
                    throw core::error("JSON - cannot write non-string key");
            }

            void null_(const core::value &) {stream() << "null";}
            void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true": "false");}
            void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
            void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
            void real_(const core::value &v)
            {
                if (!std::isfinite(v.get_real_unchecked()))
                    throw core::error("JSON - cannot write 'NaN' or 'Infinity' values");
                stream() << v.get_real_unchecked();
            }
            void begin_string_(const core::value &v, core::optional_size, bool is_key) {if (v.get_subtype() != core::bignum || is_key) stream().put('"');}
            void string_data_(const core::value &v, bool) {write_string(stream(), v.get_string_unchecked());}
            void end_string_(const core::value &v, bool is_key) {if (v.get_subtype() != core::bignum || is_key) stream().put('"');}

            void begin_array_(const core::value &, core::optional_size, bool)
            {
                stream().put('[');
                current_indent += indent_width;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    stream().put('\n'), output_padding(current_indent);

                stream().put(']');
            }

            void begin_object_(const core::value &, core::optional_size, bool)
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

        inline core::value from_json(core::istream_handle stream)
        {
            parser reader(stream);
            core::value v;
            reader >> v;
            return v;
        }

        inline core::value operator "" _json(const char *stream, size_t size)
        {
            core::istringstream wrap(std::string(stream, size));
            return from_json(wrap);
        }

        inline std::string to_json(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer writer(stream);
            writer << v;
            return stream.str();
        }

        inline std::string to_pretty_json(const core::value &v, size_t indent_width)
        {
            core::ostringstream stream;
            pretty_stream_writer writer(stream, indent_width);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_JSON_H

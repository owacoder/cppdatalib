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

        private:
            // Opening quote should already be read
            core::istream &read_string(core::istream &stream, core::stream_handler &writer)
            {
                static const std::string hex = "0123456789ABCDEF";

                int c;
                char *write = buffer.get();

                writer.begin_string(core::string_t(), core::stream_handler::unknown_size);
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
                                    code = (code << 4) | pos;
                                }

                                std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
                                std::string bytes = utf8.to_bytes(code);

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
                        writer.append_to_string(buffer.get());
                        write = buffer.get();
                    }
                }

                if (c == EOF)
                    throw core::error("JSON - unexpected end of string");

                if (write != buffer.get())
                {
                    *write = 0;
                    writer.append_to_string(buffer.get());
                }
                writer.end_string(core::string_t());
                return stream;
            }

        public:
            parser(core::istream &input)
                : core::stream_parser(input)
                , buffer(new char [core::buffer_size + core::max_utf8_code_sequence_size + 1])
            {}

            core::stream_input &convert(core::stream_handler &writer)
            {
                bool delimiter_required = false, get_char = true;
                char chr;

                input_stream >> std::skipws;
                while (get_char? (input_stream >> chr, input_stream.good()): true)
                {
                    get_char = true;

                    if (delimiter_required)
                    {
                        if (writer.nesting_depth() == 0)
                            break;
                        else if (!strchr(",:]}", chr))
                            throw core::error("JSON - expected ',' separating array or object entries");
                    }

                    switch (chr)
                    {
                        case 'n':
                            if (!core::stream_starts_with(input_stream, "ull")) throw core::error("JSON - expected 'null' value");
                            writer.write(core::null_t());
                            delimiter_required = true;
                            break;
                        case 't':
                            if (!core::stream_starts_with(input_stream, "rue")) throw core::error("JSON - expected 'true' value");
                            writer.write(true);
                            delimiter_required = true;
                            break;
                        case 'f':
                            if (!core::stream_starts_with(input_stream, "alse")) throw core::error("JSON - expected 'false' value");
                            writer.write(false);
                            delimiter_required = true;
                            break;
                        case '"':
                            read_string(input_stream, writer);
                            delimiter_required = true;
                            break;
                        case ',':
                            if (writer.current_container_size() == 0 || writer.container_key_was_just_parsed())
                                throw core::error("JSON - invalid ',' does not separate array or object entries");
                            input_stream >> chr; get_char = false; // Peek ahead
                            if (!input_stream || chr == ',' || chr == ']' || chr == '}')
                                throw core::error("JSON - invalid ',' does not separate array or object entries");
                            delimiter_required = false;
                            break;
                        case ':':
                            if (!writer.container_key_was_just_parsed())
                                throw core::error("JSON - invalid ':' does not separate a key and value pair");
                            delimiter_required = false;
                            break;
                        case '[':
                            writer.begin_array(core::array_t(), core::stream_handler::unknown_size);
                            delimiter_required = false;
                            break;
                        case ']':
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
                            if (isdigit(chr) || chr == '-')
                            {
                                if (writer.current_container() == core::object && !writer.container_key_was_just_parsed()) // This is the key?
                                    throw core::error("JSON - invalid number cannot be used as an object key");

                                bool is_float = false;
                                std::string buffer = std::string(1, chr);

                                while (chr = input_stream.get(), chr != EOF && strchr("0123456789.eE+-", chr))
                                {
                                    buffer.push_back(chr);
                                    is_float = chr == '.' || tolower(chr) == 'e';
                                }
                                get_char = chr == EOF; // This forces the loop to end if we're at EOF
                                delimiter_required = true;

                                if (!is_float)
                                {
                                    // Attempt to read as an integer
                                    {
                                        core::istring_wrapper_stream temp_stream(buffer);
                                        core::int_t value;
                                        temp_stream >> value;
                                        if (!temp_stream.fail() && temp_stream.get() == EOF)
                                        {
                                            writer.write(value);
                                            break; // break switch
                                        }
                                    }

                                    // Attempt to read as an unsigned integer
                                    {
                                        core::istring_wrapper_stream temp_stream(buffer);
                                        core::uint_t value;
                                        temp_stream >> value;
                                        if (!temp_stream.fail() && temp_stream.get() == EOF)
                                        {
                                            writer.write(value);
                                            break; // break switch
                                        }
                                    }
                                }

                                // Attempt to read as a real
                                if (buffer.find_first_of("eE.") != std::string::npos)
                                {
                                    core::istring_wrapper_stream temp_stream(buffer);
                                    core::real_t value;
                                    temp_stream >> value;
                                    if (!temp_stream.fail() && temp_stream.get() == EOF)
                                    {
                                        writer.write(value);
                                        break; // break switch
                                    }
                                }

                                // Revert to bignum
                                writer.write(core::value(buffer, core::bignum));
                            }
                            else
                                throw core::error("JSON - expected value");
                            break;
                    }
                }

                if (!delimiter_required)
                    throw core::error("JSON - expected value");

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
                        int c = str[i];

                        switch (c)
                        {
                            case '"':
                            case '\\':
                                stream.put('\\');
                                stream.put(c);
                                break;
                            default:
                                if (iscntrl(c))
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
            stream_writer(core::ostream &output) : stream_writer_base(output) {}

        protected:
            void begin_() {output_stream.precision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    output_stream.put(':');
                else if (current_container_size() > 0)
                    output_stream.put(',');
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    output_stream.put(',');

                if (!v.is_string())
                    throw core::error("JSON - cannot write non-string key");
            }

            void null_(const core::value &) {output_stream << "null";}
            void bool_(const core::value &v) {output_stream << (v.get_bool()? "true": "false");}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void uinteger_(const core::value &v) {output_stream << v.get_uint();}
            void real_(const core::value &v)
            {
                if (!std::isfinite(v.get_real()))
                    throw core::error("JSON - cannot write 'NaN' or 'Infinity' values");
                output_stream << v.get_real();
            }
            void begin_string_(const core::value &v, core::int_t, bool is_key) {if (v.get_subtype() != core::bignum || is_key) output_stream.put('"');}
            void string_data_(const core::value &v, bool) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &v, bool is_key) {if (v.get_subtype() != core::bignum || is_key) output_stream.put('"');}

            void begin_array_(const core::value &, core::int_t, bool) {output_stream.put('[');}
            void end_array_(const core::value &, bool) {output_stream.put(']');}

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
                : stream_writer_base(output)
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
                    output_stream << ": ";
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
                    throw core::error("JSON - cannot write non-string key");
            }

            void null_(const core::value &) {output_stream << "null";}
            void bool_(const core::value &v) {output_stream << (v.get_bool()? "true": "false");}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void uinteger_(const core::value &v) {output_stream << v.get_uint();}
            void real_(const core::value &v)
            {
                if (!std::isfinite(v.get_real()))
                    throw core::error("JSON - cannot write 'NaN' or 'Infinity' values");
                output_stream << v.get_real();
            }
            void begin_string_(const core::value &v, core::int_t, bool is_key) {if (v.get_subtype() != core::bignum || is_key) output_stream.put('"');}
            void string_data_(const core::value &v, bool) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &v, bool is_key) {if (v.get_subtype() != core::bignum || is_key) output_stream.put('"');}

            void begin_array_(const core::value &, core::int_t, bool)
            {
                output_stream.put('[');
                current_indent += indent_width;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream.put('\n'), output_padding(current_indent);

                output_stream.put(']');
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

        inline core::value from_json(const std::string &json)
        {
            core::istring_wrapper_stream stream(json);
            parser reader(stream);
            core::value v;
            reader >> v;
            return v;
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

/*
 * csv.h
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

#ifndef CPPDATALIB_CSV_H
#define CPPDATALIB_CSV_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace csv
    {
        class parser : public core::stream_parser
        {
        public:
            enum options
            {
                convert_fields_by_deduction,
                convert_all_fields_as_strings
            };

        private:
            void deduce_type(const std::string &buffer, core::stream_handler &writer)
            {
                if (buffer.empty() ||
                        buffer == "~" ||
                        buffer == "null" ||
                        buffer == "Null" ||
                        buffer == "NULL")
                    writer.write(core::null_t());
                else if (buffer == "Y" ||
                         buffer == "y" ||
                         buffer == "yes" ||
                         buffer == "Yes" ||
                         buffer == "YES" ||
                         buffer == "on" ||
                         buffer == "On" ||
                         buffer == "ON" ||
                         buffer == "true" ||
                         buffer == "True" ||
                         buffer == "TRUE")
                    writer.write(true);
                else if (buffer == "N" ||
                         buffer == "n" ||
                         buffer == "no" ||
                         buffer == "No" ||
                         buffer == "NO" ||
                         buffer == "off" ||
                         buffer == "Off" ||
                         buffer == "OFF" ||
                         buffer == "false" ||
                         buffer == "False" ||
                         buffer == "FALSE")
                    writer.write(false);
                else
                {
                    // Attempt to read as an integer
                    {
                        core::istring_wrapper_stream temp_stream(buffer);
                        core::int_t value;
                        temp_stream >> value;
                        if (!temp_stream.fail() && temp_stream.get() == EOF)
                        {
                            writer.write(value);
                            return;
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
                            return;
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
                            return;
                        }
                    }

                    // Revert to string
                    writer.write(buffer);
                }
            }

            core::istream &read_string(core::stream_handler &writer, bool parse_as_strings)
            {
                int chr;
                std::string buffer;

                if (parse_as_strings)
                {
                    writer.begin_string(core::string_t(), core::stream_handler::unknown_size);

                    // buffer is used to temporarily store whitespace for reading strings
                    while (chr = stream().get(), chr != EOF && chr != ',' && chr != '\n')
                    {
                        if (isspace(chr))
                        {
                            buffer.push_back(chr);
                            continue;
                        }
                        else if (buffer.size())
                        {
                            writer.append_to_string(buffer);
                            buffer.clear();
                        }

                        writer.append_to_string(core::string_t(1, chr));
                    }

                    if (chr != EOF)
                        stream().unget();

                    writer.end_string(core::string_t());
                }
                else // Unfortunately, one cannot deduce the type of the incoming data without first loading the field into a buffer
                {
                    while (chr = stream().get(), chr != EOF && chr != ',' && chr != '\n')
                        buffer.push_back(chr);

                    if (chr != EOF)
                        stream().unget();

                    while (!buffer.empty() && isspace(buffer.back() & 0xff))
                        buffer.pop_back();

                    deduce_type(buffer, writer);
                }

                return stream();
            }

            // Expects that the leading quote has already been parsed out of the input_stream
            core::istream &read_quoted_string(core::stream_handler &writer, bool parse_as_strings)
            {
                int chr;
                std::string buffer;

                if (parse_as_strings)
                {
                    writer.begin_string(core::string_t(), core::stream_handler::unknown_size);

                    // buffer is used to temporarily store whitespace for reading strings
                    while (chr = stream().get(), chr != EOF)
                    {
                        if (chr == '"')
                        {
                            if (stream().peek() == '"')
                                chr = stream().get();
                            else
                                break;
                        }

                        if (isspace(chr))
                        {
                            buffer.push_back(chr);
                            continue;
                        }
                        else if (buffer.size())
                        {
                            writer.append_to_string(buffer);
                            buffer.clear();
                        }

                        writer.append_to_string(core::string_t(1, chr));
                    }

                    writer.end_string(core::string_t());
                }
                else // Unfortunately, one cannot deduce the type of the incoming data without first loading the field into a buffer
                {
                    while (chr = stream().get(), chr != EOF)
                    {
                        if (chr == '"')
                        {
                            if (stream().peek() == '"')
                                chr = stream().get();
                            else
                                break;
                        }

                        buffer.push_back(chr);
                    }

                    while (!buffer.empty() && isspace(buffer.back() & 0xff))
                        buffer.pop_back();

                    deduce_type(buffer, writer);
                }

                return stream();
            }

            options opts;
            bool comma_just_parsed = true;
            bool newline_just_parsed = true;

        public:
            parser(core::istream_handle input, options opts = convert_fields_by_deduction)
                : stream_parser(input)
                , opts(opts)
            {
                reset();
            }

            void set_parse_method(options opts) {this->opts = opts;}

            void reset()
            {
                comma_just_parsed = newline_just_parsed = true;
            }

        protected:
            void write_one_()
            {
                bool parse_as_strings = opts == convert_all_fields_as_strings;
                int chr;

                if (!busy())
                    get_output()->begin_array(core::array_t(), core::stream_handler::unknown_size);

                if (chr = stream().get(), chr != EOF)
                {
                    if (newline_just_parsed)
                    {
                        get_output()->begin_array(core::array_t(), core::stream_handler::unknown_size);
                        newline_just_parsed = false;
                    }

                    switch (chr)
                    {
                        case '"':
                            read_quoted_string(*get_output(), parse_as_strings);
                            comma_just_parsed = false;
                            break;
                        case ',':
                            if (comma_just_parsed)
                            {
                                if (parse_as_strings)
                                    get_output()->write(core::string_t());
                                else // parse by deduction of types, assume `,,` means null instead of empty string
                                    get_output()->write(core::null_t());
                            }
                            comma_just_parsed = true;
                            break;
                        case '\n':
                            if (comma_just_parsed)
                            {
                                if (parse_as_strings)
                                    get_output()->write(core::string_t());
                                else // parse by deduction of types, assume `,,` means null instead of empty string
                                    get_output()->write(core::null_t());
                            }
                            comma_just_parsed = newline_just_parsed = true;
                            get_output()->end_array(core::array_t());
                            break;
                        default:
                            if (!isspace(chr))
                            {
                                stream().unget();
                                read_string(*get_output(), parse_as_strings);
                                comma_just_parsed = false;
                            }
                            break;
                    }
                }

                if (!newline_just_parsed)
                {
                    if (comma_just_parsed)
                    {
                        if (parse_as_strings)
                            get_output()->write(core::string_t());
                        else // parse by deduction of types, assume `,,` means null instead of empty string
                            get_output()->write(core::null_t());
                    }

                    get_output()->end_array(core::array_t());
                }

                if (chr == EOF)
                    get_output()->end_array(core::array_t());
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

                        if (c == '"')
                            stream.put('"');

                        stream.put(str[i]);
                    }

                    return stream;
                }
            };
        }

        class row_writer : public impl::stream_writer_base
        {
            char separator;

        public:
            row_writer(core::ostream_handle output, char separator = ',') : stream_writer_base(output), separator(separator) {}

        protected:
            void begin_() {stream().precision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (current_container_size() > 0)
                    stream().put(separator);
            }

            void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true": "false");}
            void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
            void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
            void real_(const core::value &v) {stream() << v.get_real_unchecked();}
            void begin_string_(const core::value &, core::int_t, bool) {stream().put('"');}
            void string_data_(const core::value &v, bool) {write_string(stream(), v.get_string_unchecked());}
            void end_string_(const core::value &, bool) {stream().put('"');}

            void begin_array_(const core::value &, core::int_t, bool) {throw core::error("CSV - 'array' value not allowed in row output");}
            void begin_object_(const core::value &, core::int_t, bool) {throw core::error("CSV - 'object' value not allowed in output");}
        };

        class stream_writer : public impl::stream_writer_base
        {
            char separator;

        public:
            stream_writer(core::ostream_handle output, char separator = ',') : stream_writer_base(output), separator(separator) {}

        protected:
            void begin_() {stream().precision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (current_container_size() > 0)
                {
                    if (nesting_depth() == 1)
                        stream() << "\r\n";
                    else
                        stream().put(separator);
                }
            }

            void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true": "false");}
            void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
            void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
            void real_(const core::value &v) {stream() << v.get_real_unchecked();}
            void begin_string_(const core::value &, core::int_t, bool) {stream().put('"');}
            void string_data_(const core::value &v, bool) {write_string(stream(), v.get_string_unchecked());}
            void end_string_(const core::value &, bool) {stream().put('"');}

            void begin_array_(const core::value &, core::int_t, bool)
            {
                if (nesting_depth() == 2)
                    throw core::error("CSV - 'array' value not allowed in row output");
            }
            void begin_object_(const core::value &, core::int_t, bool) {throw core::error("CSV - 'object' value not allowed in output");}
        };

        inline core::value from_csv_table(core::istream_handle stream, parser::options opts = parser::convert_fields_by_deduction)
        {
            parser reader(stream, opts);
            core::value v;
            reader >> v;
            return v;
        }

        inline std::string to_csv_row(const core::value &v, char separator = ',')
        {
            core::ostringstream stream;
            row_writer writer(stream, separator);
            writer << v;
            return stream.str();
        }

        inline std::string to_csv_table(const core::value &v, char separator = ',')
        {
            core::ostringstream stream;
            stream_writer writer(stream, separator);
            writer << v;
            return stream.str();
        }

        inline core::value from_csv(core::istream_handle stream, parser::options opts = parser::convert_fields_by_deduction) {return from_csv_table(stream, opts);}
        inline std::string to_csv(const core::value &v, char separator = ',') {return to_csv_table(v, separator);}
    }
}

#endif // CPPDATALIB_CSV_H

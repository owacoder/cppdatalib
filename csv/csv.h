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
                        std::istringstream temp_stream(buffer);
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
                        std::istringstream temp_stream(buffer);
                        core::uint_t value;
                        temp_stream >> value;
                        if (!temp_stream.fail() && temp_stream.get() == EOF)
                        {
                            writer.write(value);
                            return;
                        }
                    }

                    // Attempt to read as a real
                    {
                        std::istringstream temp_stream(buffer);
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

            std::istream &read_string(core::stream_handler &writer, bool parse_as_strings)
            {
                int chr;
                std::string buffer;

                if (parse_as_strings)
                {
                    writer.begin_string(core::string_t(), core::stream_handler::unknown_size);

                    // buffer is used to temporarily store whitespace for reading strings
                    while (chr = input_stream.get(), chr != EOF && chr != ',' && chr != '\n')
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
                        input_stream.unget();

                    writer.end_string(core::string_t());
                }
                else // Unfortunately, one cannot deduce the type of the incoming data without first loading the field into a buffer
                {
                    while (chr = input_stream.get(), chr != EOF && chr != ',' && chr != '\n')
                        buffer.push_back(chr);

                    if (chr != EOF)
                        input_stream.unget();

                    while (!buffer.empty() && isspace(buffer.back() & 0xff))
                        buffer.pop_back();

                    deduce_type(buffer, writer);
                }

                return input_stream;
            }

            // Expects that the leading quote has already been parsed out of the input_stream
            std::istream &read_quoted_string(core::stream_handler &writer, bool parse_as_strings)
            {
                int chr;
                std::string buffer;

                if (parse_as_strings)
                {
                    writer.begin_string(core::string_t(), core::stream_handler::unknown_size);

                    // buffer is used to temporarily store whitespace for reading strings
                    while (chr = input_stream.get(), chr != EOF)
                    {
                        if (chr == '"')
                        {
                            if (input_stream.peek() == '"')
                                chr = input_stream.get();
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
                    while (chr = input_stream.get(), chr != EOF)
                    {
                        if (chr == '"')
                        {
                            if (input_stream.peek() == '"')
                                chr = input_stream.get();
                            else
                                break;
                        }

                        buffer.push_back(chr);
                    }

                    while (!buffer.empty() && isspace(buffer.back() & 0xff))
                        buffer.pop_back();

                    deduce_type(buffer, writer);
                }

                return input_stream;
            }

            options opts;

        public:
            parser(std::istream &input, options opts = convert_fields_by_deduction) : stream_parser(input), opts(opts) {}

            core::stream_input &convert(core::stream_handler &writer)
            {
                const bool parse_as_strings = opts == convert_all_fields_as_strings;
                int chr;
                bool comma_just_parsed = true;
                bool newline_just_parsed = true;

                writer.begin_array(core::array_t(), core::stream_handler::unknown_size);

                while (chr = input_stream.get(), chr != EOF)
                {
                    if (newline_just_parsed)
                    {
                        writer.begin_array(core::array_t(), core::stream_handler::unknown_size);
                        newline_just_parsed = false;
                    }

                    switch (chr)
                    {
                        case '"':
                            read_quoted_string(writer, parse_as_strings);
                            comma_just_parsed = false;
                            break;
                        case ',':
                            if (comma_just_parsed)
                            {
                                if (parse_as_strings)
                                    writer.write(core::string_t());
                                else // parse by deduction of types, assume `,,` means null instead of empty string
                                    writer.write(core::null_t());
                            }
                            comma_just_parsed = true;
                            break;
                        case '\n':
                            if (comma_just_parsed)
                            {
                                if (parse_as_strings)
                                    writer.write(core::string_t());
                                else // parse by deduction of types, assume `,,` means null instead of empty string
                                    writer.write(core::null_t());
                            }
                            comma_just_parsed = newline_just_parsed = true;
                            writer.end_array(core::array_t());
                            break;
                        default:
                            if (!isspace(chr))
                            {
                                input_stream.unget();
                                read_string(writer, parse_as_strings);
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
                            writer.write(core::string_t());
                        else // parse by deduction of types, assume `,,` means null instead of empty string
                            writer.write(core::null_t());
                    }

                    writer.end_array(core::array_t());
                }

                writer.end_array(core::array_t());

                return *this;
            }
        };

        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(std::ostream &stream) : core::stream_writer(stream) {}

            protected:
                std::ostream &write_string(std::ostream &stream, const std::string &str)
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
            row_writer(std::ostream &output, char separator = ',') : stream_writer_base(output), separator(separator) {}

        protected:
            void begin_() {output_stream << std::setprecision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (current_container_size() > 0)
                    output_stream.put(separator);
            }

            void bool_(const core::value &v) {output_stream << (v.get_bool()? "true": "false");}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void uinteger_(const core::value &v) {output_stream << v.get_uint();}
            void real_(const core::value &v) {output_stream << v.get_real();}
            void begin_string_(const core::value &, core::int_t, bool) {output_stream.put('"');}
            void string_data_(const core::value &v, bool) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &, bool) {output_stream.put('"');}

            void begin_array_(const core::value &, core::int_t, bool) {throw core::error("CSV - 'array' value not allowed in row output");}
            void begin_object_(const core::value &, core::int_t, bool) {throw core::error("CSV - 'object' value not allowed in output");}
        };

        class stream_writer : public impl::stream_writer_base
        {
            char separator;

        public:
            stream_writer(std::ostream &output, char separator = ',') : stream_writer_base(output), separator(separator) {}

        protected:
            void begin_() {output_stream << std::setprecision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (current_container_size() > 0)
                {
                    if (nesting_depth() == 1)
                        output_stream << "\r\n";
                    else
                        output_stream.put(separator);
                }
            }

            void bool_(const core::value &v) {output_stream << (v.get_bool()? "true": "false");}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void uinteger_(const core::value &v) {output_stream << v.get_uint();}
            void real_(const core::value &v) {output_stream << v.get_real();}
            void begin_string_(const core::value &, core::int_t, bool) {output_stream.put('"');}
            void string_data_(const core::value &v, bool) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &, bool) {output_stream.put('"');}

            void begin_array_(const core::value &, core::int_t, bool)
            {
                if (nesting_depth() == 2)
                    throw core::error("CSV - 'array' value not allowed in row output");
            }
            void begin_object_(const core::value &, core::int_t, bool) {throw core::error("CSV - 'object' value not allowed in output");}
        };

        inline core::value from_csv_table(const std::string &csv, parser::options opts = parser::convert_fields_by_deduction)
        {
            std::istringstream stream(csv);
            parser reader(stream, opts);
            core::value v;
            reader >> v;
            return v;
        }

        inline std::string to_csv_row(const core::value &v, char separator = ',')
        {
            std::ostringstream stream;
            row_writer writer(stream, separator);
            writer << v;
            return stream.str();
        }

        inline std::string to_csv_table(const core::value &v, char separator = ',')
        {
            std::ostringstream stream;
            stream_writer writer(stream, separator);
            writer << v;
            return stream.str();
        }

        inline core::value from_csv(const std::string &csv, parser::options opts = parser::convert_fields_by_deduction) {return from_csv_table(csv, opts);}
        inline std::string to_csv(const core::value &v, char separator = ',') {return to_csv_table(v, separator);}
    }
}

#endif // CPPDATALIB_CSV_H

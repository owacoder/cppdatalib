/*
 * tsv.h
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

#ifndef CPPDATALIB_TSV_H
#define CPPDATALIB_TSV_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace tsv
    {
        class parser : public core::stream_parser
        {
            char separator;

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
                    writer.write(core::value(true));
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
                    writer.write(core::value(false));
                else
                {
                    // Attempt to read as an integer
                    {
                        core::istring_wrapper_stream temp_stream(buffer);
                        core::int_t value;
                        temp_stream >> value;
                        if (!temp_stream.fail() && temp_stream.get() == EOF)
                        {
                            writer.write(core::value(value));
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
                            writer.write(core::value(value));
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
                            writer.write(core::value(value));
                            return;
                        }
                    }

                    // Revert to string
                    writer.write(core::value(buffer));
                }
            }

            core::istream &read_string(core::stream_handler &writer, bool parse_as_strings)
            {
                core::istream::int_type chr;
                core::string_t buffer;

                if (parse_as_strings)
                {
                    writer.begin_string(core::string_t(), core::stream_handler::unknown_size());

                    // buffer is used to temporarily store whitespace for reading strings
                    while (chr = stream().get(), chr != EOF && chr != separator && chr != '\n')
                    {
                        if (isspace(chr))
                        {
                            buffer += core::ucs_to_utf8(chr);
                            continue;
                        }
                        else if (buffer.size())
                        {
                            writer.append_to_string(buffer);
                            buffer.clear();
                        }

                        writer.append_to_string(core::ucs_to_utf8(chr));
                    }

                    if (chr != EOF)
                        stream().unget();

                    writer.end_string(core::string_t());
                }
                else // Unfortunately, one cannot deduce the type of the incoming data without first loading the field into a buffer
                {
                    while (chr = stream().get(), chr != EOF && chr != separator && chr != '\n')
                        buffer += core::ucs_to_utf8(chr);

                    if (chr != EOF)
                        stream().unget();

                    while (!buffer.empty() && isspace(buffer[buffer.size()-1] & 0xff))
                        buffer.erase(buffer.size()-1);

                    deduce_type(buffer, writer);
                }

                return stream();
            }

            options opts;
            bool tab_just_parsed;
            bool newline_just_parsed;

        public:
            parser(core::istream_handle input, char separator = '\t', options opts = convert_fields_by_deduction)
                : stream_parser(input)
                , separator(separator)
                , opts(opts)
                , tab_just_parsed(true)
                , newline_just_parsed(true)
            {
                reset();
            }

            void set_parse_method(options opts) {this->opts = opts;}

        protected:
            void reset_()
            {
                tab_just_parsed = newline_just_parsed = true;
            }

            void write_one_()
            {
                bool parse_as_strings = opts == convert_all_fields_as_strings;
                int chr;

                if (was_just_reset())
                    get_output()->begin_array(core::array_t(), core::stream_handler::unknown_size());

                if (chr = stream().get(), chr != EOF)
                {
                    if (newline_just_parsed)
                    {
                        get_output()->begin_array(core::array_t(), core::stream_handler::unknown_size());
                        newline_just_parsed = false;
                    }

                    if (chr == separator)
                    {
                        if (tab_just_parsed)
                        {
                            if (parse_as_strings)
                                get_output()->write(core::value(core::string_t()));
                            else // parse by deduction of types, assume `<tab><tab>` means null instead of empty string
                                get_output()->write(core::null_t());
                        }
                        tab_just_parsed = true;
                    }
                    else if (chr == '\n')
                    {
                        if (tab_just_parsed)
                        {
                            if (parse_as_strings)
                                get_output()->write(core::value(core::string_t()));
                            else // parse by deduction of types, assume `<tab><tab>` means null instead of empty string
                                get_output()->write(core::null_t());
                        }
                        tab_just_parsed = newline_just_parsed = true;
                        get_output()->end_array(core::array_t());
                    }
                    else if (!isspace(chr))
                    {
                        stream().unget();
                        read_string(*get_output(), parse_as_strings);
                        tab_just_parsed = false;
                    }
                }

                if (chr == EOF)
                {
                    if (!newline_just_parsed)
                    {
                        if (tab_just_parsed)
                        {
                            if (parse_as_strings)
                                get_output()->write(core::value(core::string_t()));
                            else // parse by deduction of types, assume `<tab><tab>` means null instead of empty string
                                get_output()->write(core::null_t());
                        }

                        get_output()->end_array(core::array_t());
                    }

                    get_output()->end_array(core::array_t());
                }
            }
        };

        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            protected:
                char separator;

            public:
                stream_writer_base(core::ostream_handle &stream, int separator = '\t') : core::stream_writer(stream), separator(separator) {}

            protected:
                core::ostream &write_string(core::ostream &stream, core::string_view_t str)
                {
                    for (size_t i = 0; i < str.size(); ++i)
                    {
                        if (str[i] == separator || str[i] == '\n')
                            throw core::error("cppdatalib::tsv::row_writer - 'string' value must not contain separator character or newline");

                        stream.put(str[i]);
                    }

                    return stream;
                }
            };
        }

        class row_writer : public impl::stream_writer_base
        {
        public:
            row_writer(core::ostream_handle output, char separator = '\t') : stream_writer_base(output, separator) {}

            std::string name() const {return "cppdatalib::tsv::row_writer";}

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
            void begin_string_(const core::value &, core::optional_size, bool) {stream().put('"');}
            void string_data_(const core::value &v, bool) {write_string(stream(), v.get_string_unchecked());}
            void end_string_(const core::value &, bool) {stream().put('"');}

            void begin_array_(const core::value &, core::optional_size, bool) {throw core::error("TSV - 'array' value not allowed in row output");}
            void begin_object_(const core::value &, core::optional_size, bool) {throw core::error("TSV - 'object' value not allowed in output");}

            void link_(const core::value &) {throw core::error("TSV - 'link' value not allowed in output");}
        };

        class stream_writer : public impl::stream_writer_base
        {
        public:
            stream_writer(core::ostream_handle output, char separator = '\t') : stream_writer_base(output, separator) {}

            std::string name() const {return "cppdatalib::tsv::stream_writer";}

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
            void string_data_(const core::value &v, bool) {write_string(stream(), v.get_string_unchecked());}

            void begin_array_(const core::value &, core::optional_size, bool)
            {
                if (nesting_depth() == 2)
                    throw core::error("TSV - 'array' value not allowed in row output");
            }
            void begin_object_(const core::value &, core::optional_size, bool) {throw core::error("TSV - 'object' value not allowed in output");}

            void link_(const core::value &) {throw core::error("TSV - 'link' value not allowed in output");}
        };

        inline core::value from_tsv_table(core::istream_handle stream, char separator = '\t', parser::options opts = parser::convert_fields_by_deduction)
        {
            parser reader(stream, separator, opts);
            core::value v;
            core::convert(reader, v);
            return v;
        }

        inline std::string to_tsv_row(const core::value &v, char separator = '\t')
        {
            core::ostringstream stream;
            row_writer writer(stream, separator);
            core::convert(writer, v);
            return stream.str();
        }

        inline std::string to_tsv_table(const core::value &v, char separator = '\t')
        {
            core::ostringstream stream;
            stream_writer writer(stream, separator);
            core::convert(writer, v);
            return stream.str();
        }

        inline core::value from_tsv(core::istream_handle stream, char separator = '\t', parser::options opts = parser::convert_fields_by_deduction) {return from_tsv_table(stream, separator, opts);}
        inline std::string to_tsv(const core::value &v, char separator = '\t') {return to_tsv_table(v, separator);}
    }
}

#endif // CPPDATALIB_TSV_H

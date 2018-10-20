/*
 * line.h
 *
 * Copyright © 2018 Oliver Adams
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

#ifndef CPPDATALIB_LINE_H
#define CPPDATALIB_LINE_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace raw
    {
        class line_parser : public core::stream_parser
        {
            char * const buffer;
            int delimiter;

        public:
            line_parser(core::istream_handle input, int delimiter = '\n')
                : core::stream_parser(input)
                , buffer(new char[core::buffer_size + 1])
                , delimiter(delimiter)
            {
                reset();
            }
            ~line_parser() {delete[] buffer;}

        protected:
            core::istream &read_string(core::istream &stream, core::stream_handler &writer, bool &had_eof)
            {
                core::value str_type((const char *) "", (cppdatalib::core::subtype_t) core::normal, true);
                core::istream::int_type c;
                char *write = buffer;

                writer.begin_string(str_type, core::stream_handler::unknown_size());
                while (c = stream.get(), c != delimiter && c != EOF)
                {
                    *write++ = c;

                    if (write - buffer >= core::buffer_size)
                    {
                        *write = 0;
                        writer.append_to_string(core::value(buffer, write - buffer, core::normal, true));
                        write = buffer;
                    }
                }

                had_eof = (c == EOF);

                if (write != buffer)
                {
                    *write = 0;
                    writer.append_to_string(core::value(buffer, write - buffer, core::normal, true));
                }
                writer.end_string(str_type);
                return stream;
            }

            void reset_() {stream() >> stdx::noskipws;}

            void write_one_()
            {
                bool eof;

                if (was_just_reset())
                    get_output()->begin_array(core::array_t(), core::stream_handler::unknown_size());
                else
                {
                    read_string(stream(), *get_output(), eof);

                    if (eof)
                        get_output()->end_array(core::array_t());
                    else if (!stream())
                        throw core::error("Raw line - could not read line");
                }
            }
        };

        class line_stream_writer : public core::stream_writer, public core::stream_handler
        {
            bool had_item;
            int delimiter;

        public:
            line_stream_writer(core::ostream_handle output, int delimiter = '\n')
                : core::stream_writer(output)
                , had_item(false)
                , delimiter(delimiter)
            {}

            std::string name() const {return "cppdatalib::raw::line_stream_writer";}

        protected:
            void begin_() {had_item = false; stream().precision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &v)
            {
                if (had_item)
                    stream().put(delimiter);
                else if (!v.is_array())
                    had_item = true;
            }

            void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true": "false");}
            void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
            void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
            void real_(const core::value &v) {stream() << v.get_real_unchecked();}
            void string_data_(const core::value &v, bool) {stream() << v.get_string_unchecked();}
            void begin_array_(const core::value &, core::optional_size, bool)
            {
                if (nesting_depth() > 0)
                    throw core::error("Raw line - nested 'array' value not allowed in output");
            }
            void begin_object_(const core::value &, core::optional_size, bool) {throw core::error("Raw line - 'object' value not allowed in output");}

            void link_(const core::value &) {throw core::error("Raw line - 'link' value not allowed in output");}
        };

        inline core::value from_raw_line(core::istream_handle stream)
        {
            line_parser p(stream);
            core::value v;
            core::convert(p, v);
            return v;
        }

#ifdef CPPDATALIB_CPP11
        inline core::value operator "" _raw_line(const char *stream, size_t size)
        {
            core::istringstream wrap(std::string(stream, size));
            return from_raw_line(wrap);
        }
#endif

        inline std::string to_raw_line(const core::value &v)
        {
            core::ostringstream stream;
            line_stream_writer writer(stream);
            core::convert(writer, v);
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_LINE_H


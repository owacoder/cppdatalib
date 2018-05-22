/*
 * dif.h
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

#ifndef CPPDATALIB_DIF_H
#define CPPDATALIB_DIF_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace dif
    {
        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(core::ostream_handle &stream) : core::stream_writer(stream) {}

            protected:
                core::ostream &write_string(core::ostream &stream, core::string_view_t str)
                {
                    for (size_t i = 0; i < str.size(); ++i)
                    {
                        int c = str[i] & 0xff;

                        if (c == '"')
                            stream.put('"');
                        else if (c == '\n' || c == '\r')
                            throw core::error("DIF - newline not allowed in 'string' value");

                        stream.put(str[i]);
                    }

                    return stream;
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
            core::int_t version, columns, rows;
            core::string_t worksheet_name;

        public:
            stream_writer(core::ostream_handle output, const core::string_t &worksheet_name, core::int_t columns, core::int_t rows, core::int_t version = 1)
                : stream_writer_base(output)
                , version(version)
                , columns(columns)
                , rows(rows)
                , worksheet_name(worksheet_name)
            {}

            std::string name() const {return "cppdatalib::dif::stream_writer";}

        protected:
            void begin_()
            {
                stream().precision(CPPDATALIB_REAL_DIG);
                stream().write("TABLE\n0,", 8);
                stream() << version;
                stream().write("\n\"", 2);
                write_string(stream(), worksheet_name);
                stream().write("\"\nVECTORS\n0,", 12);
                stream() << columns;
                stream().write("\n\"\"\nTUPLES\n0,", 13);
                stream() << rows;
                stream().write("\n\"\"\n", 4);
            }
            void end_()
            {
                stream().write("EOD", 3);
            }

            void begin_item_(const core::value &)
            {
                if (nesting_depth() == 1)
                    stream().write("-1,0\nBOT\n", 9);
            }

            void null_(const core::value &) {stream().write("0,0\nNA\n", 7);}
            void bool_(const core::value &v) {stream() << "0," << v.get_bool_unchecked() << (v.get_bool_unchecked()? "\nTRUE\n": "\nFALSE\n");}
            void integer_(const core::value &v) {stream() << "0," << v.get_int_unchecked() << "\nV\n";}
            void uinteger_(const core::value &v) {stream() << "0," << v.get_uint_unchecked() << "\nV\n";}
            void real_(const core::value &v) {stream() << "0," << v.get_real_unchecked() << "\nV\n";}
            void begin_string_(const core::value &, core::int_t, bool) {stream().write("1,0\n\"", 5);}
            void string_data_(const core::value &v, bool) {write_string(stream(), v.get_string_unchecked());}
            void end_string_(const core::value &, bool) {stream().write("\"\n", 2);}

            void begin_array_(const core::value &, core::int_t, bool)
            {
                if (nesting_depth() == 2)
                    throw core::error("DIF - 'array' value not allowed in row output");
            }
            void begin_object_(const core::value &, core::int_t, bool) {throw core::error("DIF - 'object' value not allowed in output");}
        };

        inline std::string to_dif_table(const core::value &v, const core::string_t &worksheet_name, core::int_t columns, core::int_t rows)
        {
            core::ostringstream stream;
            stream_writer writer(stream, worksheet_name, columns, rows);
            writer << v;
            return stream.str();
        }

        inline std::string to_dif(const core::value &v, const core::string_t &worksheet_name, core::int_t columns, core::int_t rows) {return to_dif_table(v, worksheet_name, columns, rows);}
    }
}

#endif // CPPDATALIB_DIF_H

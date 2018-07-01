/*
 * string.h
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

#ifndef CPPDATALIB_STRING_H
#define CPPDATALIB_STRING_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace raw
    {
        class string_parser : public core::stream_parser
        {
        public:
            string_parser(core::istream_handle input)
                : core::stream_parser(input)
            {
                reset();
            }

        protected:
            void reset_() {stream() >> std::noskipws;}

            void write_one_()
            {
                char chr;

                if (was_just_reset())
                    get_output()->begin_string(core::value("", 0, core::blob, true), core::stream_handler::unknown_size());
                else if (stream() >> chr, stream().good())
                    get_output()->append_to_string(std::string(1, chr));
                else
                    get_output()->end_string(core::value("", 0, core::blob, true));
            }
        };

        class string_stream_writer : public core::stream_writer, public core::stream_handler
        {
        public:
            string_stream_writer(core::ostream_handle output) : core::stream_writer(output) {}

            std::string name() const {return "cppdatalib::raw::string_stream_writer";}

        protected:
            void begin_() {stream().precision(CPPDATALIB_REAL_DIG);}

            void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true": "false");}
            void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
            void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
            void real_(const core::value &v) {stream() << v.get_real_unchecked();}
            void string_data_(const core::value &v, bool) {stream() << v.get_string_unchecked();}
        };

        inline core::value from_raw_string(core::istream_handle stream)
        {
            string_parser p(stream);
            core::value v;
            p >> v;
            return v;
        }

        inline core::value operator "" _raw_string(const char *stream, size_t size)
        {
            core::istringstream wrap(std::string(stream, size));
            return from_raw_string(wrap);
        }

        inline std::string to_raw_string(const core::value &v)
        {
            core::ostringstream stream;
            string_stream_writer writer(stream);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_STRING_H

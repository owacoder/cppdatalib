/*
 * lines.h
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

#ifndef CPPDATALIB_LINES_H
#define CPPDATALIB_LINES_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace raw
    {
        class lines_stream_writer : public core::stream_writer, public core::stream_handler
        {
        public:
            lines_stream_writer(core::ostream_handle output) : core::stream_writer(output) {}

            std::string name() const {return "cppdatalib::raw::lines_stream_writer";}

        protected:
            void begin_() {stream().precision(CPPDATALIB_REAL_DIG);}

            void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true\n": "false\n");}
            void integer_(const core::value &v) {stream() << v.get_int_unchecked() << '\n';}
            void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked() << '\n';}
            void real_(const core::value &v) {stream() << v.get_real_unchecked() << '\n';}
            void string_data_(const core::value &v, bool) {stream() << v.get_string_unchecked();}
            void end_string_(const core::value &, bool) {stream() << '\n';}
        };

        inline std::string to_raw_lines(const core::value &v)
        {
            core::ostringstream stream;
            lines_stream_writer writer(stream);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_LINES_H

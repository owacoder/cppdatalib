/*
 * uint8.h
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

#ifndef CPPDATALIB_UINT8_H
#define CPPDATALIB_UINT8_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace raw
    {
        class uint8_parser : public core::stream_parser
        {
        public:
            uint8_parser(core::istream_handle input)
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
                    get_output()->begin_array(core::array_t(), core::stream_handler::unknown_size);
                else if (stream() >> chr, stream().good())
                    get_output()->write(core::value(chr & 0xff));
                else
                    get_output()->end_array(core::array_t());
            }
        };

        class uint8_stream_writer : public core::stream_writer, public core::stream_handler
        {
        public:
            uint8_stream_writer(core::ostream_handle output) : core::stream_writer(output) {}

            std::string name() const {return "cppdatalib::raw::uint8_stream_writer";}

        protected:
            void null_(const core::value &) {stream().put(0x00);}
            void bool_(const core::value &v) {stream().put(v.get_bool_unchecked());}
            void integer_(const core::value &v) {core::write_uint8(stream(), v.get_int_unchecked());}
            void uinteger_(const core::value &v) {core::write_uint8(stream(), v.get_uint_unchecked());}
            void real_(const core::value &) {throw core::error("Raw UINT8 - 'real' value not allowed in output");}
            void begin_string_(const core::value &, core::int_t, bool) {throw core::error("Raw UINT8 - 'string' value not allowed in output");}
            void begin_array_(const core::value &, core::int_t, bool)
            {
                if (nesting_depth() > 0)
                    throw core::error("Raw UINT8 - nested 'array' value not allowed in output");
            }
            void begin_object_(const core::value &, core::int_t, bool) {throw core::error("Raw UINT8 - 'object' value not allowed in output");}
        };

        inline core::value from_raw_uint8(core::istream_handle stream)
        {
            uint8_parser p(stream);
            core::value v;
            p >> v;
            return v;
        }

        inline core::value operator "" _raw_uint8(const char *stream, size_t size)
        {
            core::istring_wrapper_stream wrap(std::string(stream, size));
            return from_raw_uint8(wrap);
        }

        inline std::string to_raw_uint8(const core::value &v)
        {
            core::ostringstream stream;
            uint8_stream_writer writer(stream);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_UINT8_H

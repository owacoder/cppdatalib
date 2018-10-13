/*
 * hex.h
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

#ifndef CPPDATALIB_HEX_H
#define CPPDATALIB_HEX_H

#include "value_builder.h"

namespace cppdatalib
{
    namespace hex
    {
        class encode_accumulator : public core::accumulator_base
        {
        public:
            encode_accumulator() : core::accumulator_base() {}
            encode_accumulator(core::istream_handle handle, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle) {}
            encode_accumulator(core::ostream_handle handle) : core::accumulator_base(handle) {}
            encode_accumulator(accumulator_base *handle, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle) {}
            encode_accumulator(core::stream_handler &handle, bool just_append = false) : core::accumulator_base(handle, just_append) {}

        protected:
            const char *alphabet() {return "0123456789ABCDEF";}

            void accumulate_(core::istream::int_type data)
            {
                char buf[2];

                buf[0] = alphabet()[(data & 0xF0) >> 4];
                buf[1] = alphabet()[data & 0xF];

                flush_out(buf, 2);
            }
        };

        class decode_accumulator : public core::accumulator_base
        {
            uint8_t state;
            int required_input_before_flush;

        public:
            decode_accumulator() : core::accumulator_base() {}
            decode_accumulator(core::istream_handle handle, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle) {}
            decode_accumulator(core::ostream_handle handle) : core::accumulator_base(handle) {}
            decode_accumulator(accumulator_base *handle, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle) {}
            decode_accumulator(core::stream_handler &handle, bool just_append = false) : core::accumulator_base(handle, just_append) {}

        protected:
            static int value(int xdigit)
            {
                const char alpha[] = "abcdef";

                if (isdigit(xdigit))
                    return xdigit - '0';
                else if (!isxdigit(xdigit))
                    return -1;
                else
                    return 10 + int(strchr(alpha, tolower(xdigit)) - alpha);
            }

            void begin_() {state = 0; required_input_before_flush = 2;}
            void end_()
            {
                if (required_input_before_flush == 1)
                {
                    char buf = state << 4;
                    flush_out(&buf, 1);
                }
            }

            void accumulate_(core::istream::int_type data)
            {
                int val = value(data & 0xff);

                if (val < 0)
                    return;

                state = (state << 4) | val;

                if (--required_input_before_flush == 0)
                {
                    char buf = state;
                    flush_out(&buf, 1);
                    begin_();
                }
            }
        };
    }
}

#endif // CPPDATALIB_HEX_H

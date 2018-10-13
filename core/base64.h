/*
 * base64.h
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

#ifndef CPPDATALIB_BASE64_H
#define CPPDATALIB_BASE64_H

#include "value_builder.h"

namespace cppdatalib
{
    namespace base64
    {
        class encode_accumulator : public core::accumulator_base
        {
            uint32_t state;
            int required_input_before_flush;

        public:
            encode_accumulator() : core::accumulator_base() {}
            encode_accumulator(core::istream_handle handle, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle) {}
            encode_accumulator(core::ostream_handle handle) : core::accumulator_base(handle) {}
            encode_accumulator(accumulator_base *handle, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle) {}
            encode_accumulator(core::stream_handler &handle, bool just_append = false) : core::accumulator_base(handle, just_append) {}

        protected:
            const char *alphabet() {return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";}

            void begin_() {state = 0; required_input_before_flush = 3;}
            void end_()
            {
                if (required_input_before_flush && required_input_before_flush < 3)
                {
                    char buf[4];
                    int padding = required_input_before_flush;

                    state <<= 8 * padding;
                    buf[0] = alphabet()[(state >> 18) & 0x3F];
                    buf[1] = alphabet()[(state >> 12) & 0x3F];
                    buf[2] = alphabet()[(state >>  6) & 0x3F];
                    buf[3] = alphabet()[ state        & 0x3F];

                    flush_out(buf, 4 - padding);
                    flush_out("==", padding);
                }
            }

            void accumulate_(core::istream::int_type data)
            {
                state = (state << 8) | (data & 0xff);

                if (--required_input_before_flush == 0)
                {
                    char buf[4];
                    buf[0] = alphabet()[(state >> 18) & 0x3F];
                    buf[1] = alphabet()[(state >> 12) & 0x3F];
                    buf[2] = alphabet()[(state >>  6) & 0x3F];
                    buf[3] = alphabet()[ state        & 0x3F];
                    flush_out(buf, 4);

                    begin_();
                }
            }
        };

        class decode_accumulator : public core::accumulator_base
        {
            uint32_t state;
            int required_input_before_flush;

        public:
            decode_accumulator() : core::accumulator_base() {}
            decode_accumulator(core::istream_handle handle, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle) {}
            decode_accumulator(core::ostream_handle handle) : core::accumulator_base(handle) {}
            decode_accumulator(accumulator_base *handle, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle) {}
            decode_accumulator(core::stream_handler &handle, bool just_append = false) : core::accumulator_base(handle, just_append) {}

        protected:
            const char *alphabet() {return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";}

            void begin_() {state = 0; required_input_before_flush = 4;}
            void end_()
            {
                if (required_input_before_flush && required_input_before_flush < 4)
                {
                    char buf[3];
                    int padding = required_input_before_flush;

                    state <<= 6 * padding;
                    buf[0] = (state >> 16) & 0xFF;
                    buf[1] = (state >>  8) & 0xFF;
                    buf[2] =  state        & 0xFF;

                    flush_out(buf, 3 - padding);
                }
            }

            void accumulate_(core::istream::int_type data)
            {
                const char *ptr = strchr(alphabet(), data & 0xff);

                if (ptr == NULL)
                    return;

                state = (state << 6) | uint32_t(ptr - alphabet());

                if (--required_input_before_flush == 0)
                {
                    char buf[3];
                    buf[0] = (state >> 16) & 0xFF;
                    buf[1] = (state >>  8) & 0xFF;
                    buf[2] =  state        & 0xFF;
                    flush_out(buf, 3);

                    begin_();
                }
            }
        };
    }
}

#endif // CPPDATALIB_BASE64_H

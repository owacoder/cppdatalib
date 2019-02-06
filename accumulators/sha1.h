/*
 * sha1.h
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

#ifndef CPPDATALIB_SHA1_H
#define CPPDATALIB_SHA1_H

#include "../core/value_builder.h"
#include <cmath>

namespace cppdatalib
{
    namespace sha1
    {
        class accumulator : public core::accumulator_base
        {
            alignas(16) uint8_t buffer[64];
            uint32_t state[5];
            size_t buffer_size;
            uint64_t message_len;

        public:
            accumulator() : core::accumulator_base() {}
            accumulator(core::istream_handle handle, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle) {}
            accumulator(core::ostream_handle handle) : core::accumulator_base(handle) {}
            accumulator(accumulator_base *handle, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle) {}
            accumulator(core::stream_handler &handle, bool just_append = false) : core::accumulator_base(handle, just_append) {}

        protected:
            // Adapted from https://en.wikipedia.org/wiki/SHA1
            void begin_()
            {
                state[0] = 0x67452301;
                state[1] = 0xefcdab89;
                state[2] = 0x98badcfe;
                state[3] = 0x10325476;
                state[4] = 0xc3d2e1f0;

                buffer_size = 0;
                message_len = 0;
            }
            void end_()
            {
                if (buffer_size < 56) // Enough room to add 0x80 and add message length
                {
                    buffer[buffer_size++] = 0x80;
                    while (buffer_size < 56)
                        buffer[buffer_size++] = 0;
                }
                else // Add 0x80, add message length in later block
                {
                    buffer[buffer_size++] = 0x80;
                    while (buffer_size < 64)
                        buffer[buffer_size++] = 0;
                    flush_buffer();

                    while (buffer_size < 56)
                        buffer[buffer_size++] = 0;
                }

                for (size_t i = 0; i < 8; ++i)
                    buffer[buffer_size++] = (message_len >> ((7 - i) * 8)) & 0xff;
                flush_buffer();

                char buf[20];
                core::obufferstream bufstream(buf, 20);
                for (size_t i = 0; i < 5; ++i)
                    core::write_uint32_be(bufstream, state[i]);

                flush_out(buf, 20);
            }

            void accumulate_(core::istream::int_type data)
            {
                buffer[buffer_size++] = data;
                message_len += 8;

                if (buffer_size == 64)
                    flush_buffer();
            }

            void flush_buffer()
            {
                uint32_t wbuffer[80] = {0, 0, 0, 0,
                                        0, 0, 0, 0,
                                        0, 0, 0, 0,
                                        0, 0, 0, 0};
                uint32_t mstate[5] = {state[0], state[1], state[2], state[3], state[4]};

                for (size_t i = 0; i < 64; ++i)
                    wbuffer[i / 4] |= uint32_t(buffer[i]) << ((3 - i % 4) * 8);

                for (size_t i = 16; i < 80; ++i)
                    wbuffer[i] = rotate_left(wbuffer[i-3] ^ wbuffer[i-8] ^ wbuffer[i-14] ^ wbuffer[i-16], 1);

                for (size_t i = 0; i < 80; ++i)
                {
                    uint32_t F, k, t;

                    if (i < 20)
                    {
                        F = (mstate[1] & mstate[2]) | (~mstate[1] & mstate[3]);
                        k = 0x5a827999;
                    }
                    else if (i < 40)
                    {
                        F = mstate[1] ^ mstate[2] ^ mstate[3];
                        k = 0x6ed9eba1;
                    }
                    else if (i < 60)
                    {
                        F = (mstate[1] & mstate[2]) | (mstate[1] & mstate[3]) | (mstate[2] & mstate[3]);
                        k = 0x8f1bbcdc;
                    }
                    else
                    {
                        F = mstate[1] ^ mstate[2] ^ mstate[3];
                        k = 0xca62c1d6;
                    }

                    t = rotate_left(mstate[0], 5) + F + mstate[4] + k + wbuffer[i];
                    mstate[4] = mstate[3];
                    mstate[3] = mstate[2];
                    mstate[2] = rotate_left(mstate[1], 30);
                    mstate[1] = mstate[0];
                    mstate[0] = t;
                }

                for (size_t i = 0; i < 5; ++i)
                    state[i] += mstate[i];

                buffer_size = 0;
            }
        };
    }
}

#endif // CPPDATALIB_SHA1_H

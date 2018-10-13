/*
 * md5.h
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

#ifndef CPPDATALIB_MD5_H
#define CPPDATALIB_MD5_H

#include "../core/value_builder.h"
#include <cmath>

namespace cppdatalib
{
    namespace md5
    {
        class accumulator : public core::accumulator_base
        {
#ifndef CPPDATALIB_WATCOM
            const uint32_t table[64] = {
                0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
                0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
                0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
                0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
                0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
                0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
                0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
                0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
                0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
                0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
                0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
                0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
                0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
            };
            const int shift[64] = {
                7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
                5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
                4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
                6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
            };
#else
            uint32_t table[64];
            int shift[64];
#endif
            uint32_t state[4];
            uint8_t buffer[64];
            size_t buffer_size;
            uint64_t message_len;

        public:
            accumulator() : core::accumulator_base() {}
            accumulator(core::istream_handle handle, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle) {}
            accumulator(core::ostream_handle handle) : core::accumulator_base(handle) {}
            accumulator(accumulator_base *handle, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle) {}
            accumulator(core::stream_handler &handle, bool just_append = false) : core::accumulator_base(handle, just_append) {}

        protected:
            // Adapted from https://en.wikipedia.org/wiki/MD5
            void begin_()
            {
                /* Code to compute tables: */
#ifdef CPPDATALIB_WATCOM
                int mshift[16] = {7, 12, 17, 22,
                                  5,  9, 14, 20,
                                  4, 11, 16, 23,
                                  6, 10, 15, 21};

                double power = std::pow(2.0f, 32.0f);

                for (size_t i = 0; i < 64; ++i)
                    table[i] = uint32_t(std::floor(power * std::fabs(std::sin(double(i) + 1))));

                for (size_t i = 0; i < 64; ++i)
                    shift[i] = mshift[(i / 16) * 4 + i%4];
#endif

                state[0] = 0x67452301;
                state[1] = 0xefcdab89;
                state[2] = 0x98badcfe;
                state[3] = 0x10325476;

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
                    buffer[buffer_size++] = (message_len >> (i * 8)) & 0xff;
                flush_buffer();

                char buf[16];
                core::obufferstream bufstream(buf, 16);
                for (size_t i = 0; i < 4; ++i)
                    core::write_uint32_le(bufstream, state[i]);

                flush_out(buf, 16);
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
                uint32_t wbuffer[16] = {0, 0, 0, 0,
                                        0, 0, 0, 0,
                                        0, 0, 0, 0,
                                        0, 0, 0, 0};
                uint32_t mstate[4] = {state[0], state[1], state[2], state[3]};

                for (size_t i = 0; i < 64; ++i)
                    wbuffer[i / 4] |= uint32_t(buffer[i]) << ((i % 4) * 8);

                for (size_t i = 0; i < 64; ++i)
                {
                    uint32_t F;
                    size_t g;

                    if (i < 16)
                    {
                        F = (mstate[1] & mstate[2]) | (~mstate[1] & mstate[3]);
                        g = i;
                    }
                    else if (i < 32)
                    {
                        F = (mstate[3] & mstate[1]) | (~mstate[3] & mstate[2]);
                        g = 5*i + 1;
                    }
                    else if (i < 48)
                    {
                        F = mstate[1] ^ mstate[2] ^ mstate[3];
                        g = 3*i + 5;
                    }
                    else
                    {
                        F = mstate[2] ^ (mstate[1] | ~mstate[3]);
                        g = 7*i;
                    }

                    F += mstate[0] + table[i] + wbuffer[g % 16];
                    mstate[0] = mstate[3];
                    mstate[3] = mstate[2];
                    mstate[2] = mstate[1];
                    mstate[1] += rotate_left(F, shift[i]);
                }

                for (size_t i = 0; i < 4; ++i)
                    state[i] += mstate[i];

                buffer_size = 0;
            }
        };
    }
}

#endif // CPPDATALIB_MD5_H

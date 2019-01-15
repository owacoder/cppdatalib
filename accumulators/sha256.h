/*
 * sha256.h
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

#ifndef CPPDATALIB_SHA256_H
#define CPPDATALIB_SHA256_H

#include "../core/value_builder.h"
#include <cmath>

namespace cppdatalib
{
    namespace sha256
    {
        namespace impl
        {
#ifdef CPPDATALIB_WATCOM
            static const uint32_t table[64] = {
                   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                   0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                   0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                   0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                   0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                   0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                   0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                   0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
            };
#endif
        }

        class accumulator : public core::accumulator_base
        {
            uint32_t state[8];
            uint8_t buffer[64];
            size_t buffer_size;
            uint64_t message_len;

#ifndef CPPDATALIB_WATCOM
            const uint32_t table[64] = {
                   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                   0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                   0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                   0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                   0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                   0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                   0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                   0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
            };
#endif

        public:
            accumulator() : core::accumulator_base() {}
            accumulator(core::istream_handle handle, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle) {}
            accumulator(core::ostream_handle handle) : core::accumulator_base(handle) {}
            accumulator(accumulator_base *handle, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle) {}
            accumulator(core::stream_handler &handle, bool just_append = false) : core::accumulator_base(handle, just_append) {}

        protected:
            // Adapted from https://en.wikipedia.org/wiki/SHA-2
            void begin_()
            {
                state[0] = 0x6a09e667;
                state[1] = 0xbb67ae85;
                state[2] = 0x3c6ef372;
                state[3] = 0xa54ff53a;
                state[4] = 0x510e527f;
                state[5] = 0x9b05688c;
                state[6] = 0x1f83d9ab;
                state[7] = 0x5be0cd19;

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

                char buf[32];
                core::obufferstream bufstream(buf, 32);
                for (size_t i = 0; i < 8; ++i)
                    core::write_uint32_be(bufstream, state[i]);

                flush_out(buf, 32);
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
                using namespace impl;

                uint32_t wbuffer[64] = {0, 0, 0, 0,
                                        0, 0, 0, 0,
                                        0, 0, 0, 0,
                                        0, 0, 0, 0};
                uint32_t mstate[8] = {state[0], state[1], state[2], state[3], state[4], state[5], state[6], state[7]};

                for (size_t i = 0; i < 64; ++i)
                    wbuffer[i / 4] |= uint32_t(buffer[i]) << ((3 - i % 4) * 8);

                for (size_t i = 16; i < 64; ++i)
                {
                    uint32_t temp = rotate_right(wbuffer[i-15], 7) ^ rotate_right(wbuffer[i-15], 18) ^ (wbuffer[i-15] >> 3);
                    uint32_t temp2 = rotate_right(wbuffer[i-2], 17) ^ rotate_right(wbuffer[i-2], 19) ^ (wbuffer[i-2] >> 10);
                    wbuffer[i] = wbuffer[i-16] + temp + wbuffer[i-7] + temp2;
                }

                for (size_t i = 0; i < 64; ++i)
                {
                    uint32_t S1 = rotate_right(mstate[4], 6) ^ rotate_right(mstate[4], 11) ^ rotate_right(mstate[4], 25);
                    uint32_t ch = (mstate[4] & mstate[5]) ^ (~mstate[4] & mstate[6]);
                    uint32_t temp1 = mstate[7] + S1 + ch + table[i] + wbuffer[i];

                    uint32_t S0 = rotate_right(mstate[0], 2) ^ rotate_right(mstate[0], 13) ^ rotate_right(mstate[0], 22);
                    uint32_t maj = (mstate[0] & mstate[1]) ^ (mstate[0] & mstate[2]) ^ (mstate[1] & mstate[2]);
                    uint32_t temp2 = S0 + maj;

                    for (int j = 7; j > 0; --j)
                        mstate[j] = mstate[j-1];

                    mstate[4] += temp1;
                    mstate[0] = temp1 + temp2;
                }

                for (size_t i = 0; i < 8; ++i)
                    state[i] += mstate[i];

                buffer_size = 0;
            }
        };
    }
}

#endif // CPPDATALIB_SHA256_H

/*
 * rand.h
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

#ifndef CPPDATALIB_RAND_H
#define CPPDATALIB_RAND_H

#include "../core/value_builder.h"
#include <cmath>

namespace cppdatalib
{
    namespace rand
    {
        class accumulator : public core::accumulator_base
        {
        public:
            accumulator() : core::accumulator_base() {}
            accumulator(core::istream_handle handle, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle) {}
            accumulator(core::ostream_handle handle) : core::accumulator_base(handle) {}
            accumulator(accumulator_base *handle, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle) {}
            accumulator(core::stream_handler &handle, bool just_append = false) : core::accumulator_base(handle, just_append) {}

            void seed(unsigned int seed)
            {
                std::srand(seed);
            }

        protected:
            void begin_() {}
            void end_() {}

            void accumulate_(core::istream::int_type data)
            {
                const int max_rand = RAND_MAX - RAND_MAX % CHAR_MAX;
                int c;
                char chr;

                (void) data;

                do
                    c = std::rand();
                while (c > max_rand);

                chr = c & 0xff;
                flush_out(&chr, 1);
            }
        };
    }
}

#endif // CPPDATALIB_RAND_H

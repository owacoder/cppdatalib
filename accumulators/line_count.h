/*
 * line_count.h
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

#ifndef CPPDATALIB_LINE_COUNT_H
#define CPPDATALIB_LINE_COUNT_H

#include "../core/value_builder.h"

namespace cppdatalib
{
    namespace line_count
    {
        class accumulator : public core::accumulator_base
        {
            uint64_t count, col; // line count and column count, respectively (0-based)

        public:
            accumulator() : core::accumulator_base(), count(0), col(0) {}
            accumulator(core::istream_handle handle, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle), count(0), col(0) {}
            accumulator(core::ostream_handle handle) : core::accumulator_base(handle), count(0), col(0) {}
            accumulator(accumulator_base *handle, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle), count(0), col(0) {}
            accumulator(core::stream_handler &handle, bool just_append = false) : core::accumulator_base(handle, just_append), count(0), col(0) {}

            uint64_t current_line_count() const {return count;}
            uint64_t current_column() const {return col;}

        protected:
            bool seekc_(streamsize pos)
            {
                if (pos == 0)
                {
                    bool b = core::accumulator_base::seekc_(pos);
                    if (b)
                        count = 0, col = 0;
                    return b;
                }

                return false;
            }

            void accumulate_(core::istream::int_type data)
            {
                char c = data & 0xff;
                if (c == '\n') {
                    ++count;
                    col = 0;
                }
                else
                    ++col;
                flush_out(&c, 1);
            }
        };
    }
}

#endif // CPPDATALIB_LINE_COUNT_H

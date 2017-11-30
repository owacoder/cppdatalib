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

#include "ostream.h"

namespace cppdatalib
{
    namespace hex
    {
        inline core::ostream &debug_write(core::ostream &stream, unsigned char c)
        {
            const char alpha[] = "0123456789ABCDEF";

            if (isprint(c))
                stream.put(c);
            else
            {
                stream.put(alpha[(c & 0xff) >> 4]);
                stream.put(alpha[c & 0xf]);
            }

            return stream.put(' ');
        }

        inline core::ostream &debug_write(core::ostream &stream, const std::string &str)
        {
            for (auto c: str)
                debug_write(stream, c);

            return stream;
        }

        inline core::ostream &write(core::ostream &stream, unsigned char c)
        {
            const char alpha[] = "0123456789ABCDEF";

            stream.put(alpha[(c & 0xff) >> 4]);
            stream.put(alpha[c & 0xf]);

            return stream;
        }

        inline core::ostream &write(core::ostream &stream, const std::string &str)
        {
            for (auto c: str)
                write(stream, c);

            return stream;
        }

        inline std::string encode(const std::string &str)
        {
            core::ostringstream stream;
            write(stream, str);
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_HEX_H

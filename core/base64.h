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

#include "ostream.h"

// TODO: ensure formats using base64::write or base64::decode don't perform dumb concatenation

namespace cppdatalib
{
    namespace base64
    {
        inline core::ostream &write(core::ostream &stream, core::string_view_t str)
        {
            const char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            uint32_t temp;

            for (size_t i = 0; i < str.size();)
            {
                temp = (uint32_t) (str[i++] & 0xFF) << 16;
                if (i + 2 <= str.size())
                {
                    temp |= (uint32_t) (str[i++] & 0xFF) << 8;
                    temp |= (uint32_t) (str[i++] & 0xFF);
                    stream.put(alpha[(temp >> 18) & 0x3F]);
                    stream.put(alpha[(temp >> 12) & 0x3F]);
                    stream.put(alpha[(temp >>  6) & 0x3F]);
                    stream.put(alpha[ temp        & 0x3F]);
                }
                else if (i + 1 == str.size())
                {
                    temp |= (uint32_t) (str[i++] & 0xFF) << 8;
                    stream.put(alpha[(temp >> 18) & 0x3F]);
                    stream.put(alpha[(temp >> 12) & 0x3F]);
                    stream.put(alpha[(temp >>  6) & 0x3F]);
                    stream.put('=');
                }
                else if (i == str.size())
                {
                    stream.put(alpha[(temp >> 18) & 0x3F]);
                    stream.put(alpha[(temp >> 12) & 0x3F]);
                    stream.write("==", 2);
                }
            }

            return stream;
        }

        inline std::string encode(const std::string &str)
        {
            core::ostringstream stream;
            write(stream, str);
            return stream.str();
        }

        inline std::string decode(const std::string &str)
        {
            const std::string alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string result;
            size_t input = 0;
            uint32_t temp = 0;

            for (size_t i = 0; i < str.size(); ++i)
            {
                size_t pos = alpha.find(str[i]);
                if (pos == std::string::npos)
                    continue;
                temp |= (uint32_t) pos << (18 - 6 * input++);
                if (input == 4)
                {
                    result.push_back((temp >> 16) & 0xFF);
                    result.push_back((temp >>  8) & 0xFF);
                    result.push_back( temp        & 0xFF);
                    input = 0;
                    temp = 0;
                }
            }

            if (input > 1) result.push_back((temp >> 16) & 0xFF);
            if (input > 2) result.push_back((temp >>  8) & 0xFF);

            return result;
        }
    }
}

#endif // CPPDATALIB_BASE64_H

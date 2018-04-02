/*
 * utf.h
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

#ifndef UTF_H
#define UTF_H

#include "value.h"
#include "istream.h"

namespace cppdatalib
{
    namespace core
    {
        // Returns empty string if invalid codepoint is encountered
        // Returns UTF-8 string on valid input
        core::string_t ucs_to_utf8(const uint32_t *s, size_t size)
        {
            core::string_t result;

            for (size_t i = 0; i < size; ++i)
            {
                if (s[i] < 0x80)
                    result.push_back(s[i]);
                else if (s[i] < 0x800)
                {
                    result.push_back(0xc0 | (s[i] >> 6));
                    result.push_back(0x80 | (s[i] & 0x3f));
                }
                else if (s[i] < 0x10000)
                {
                    result.push_back(0xe0 | (s[i] >> 12));
                    result.push_back(0x80 | ((s[i] >> 6) & 0x3f));
                    result.push_back(0x80 | (s[i] & 0x3f));
                }
                else if (s[i] < 0x110000)
                {
                    result.push_back(0xf0 | (s[i] >> 24));
                    result.push_back(0x80 | ((s[i] >> 12) & 0x3f));
                    result.push_back(0x80 | ((s[i] >> 6) & 0x3f));
                    result.push_back(0x80 | (s[i] & 0x3f));
                }
                else
                    return core::string_t();
            }

            return result;
        }

        // Returns -1 on conversion failure, invalid codepoint, overlong encoding, or truncated input
        // Returns codepoint on success, with updated `idx` stored in `new_pos`
        uint32_t utf8_to_ucs(const std::string &s, size_t idx, size_t &new_pos)
        {
            uint32_t result = 0;
            int c = s[idx] & 0xff;

            if (c < 0x80) // Single-byte encoding
            {
                new_pos = idx+1;
                return c;
            }
            else if (c < 0xc0) // Invalid, starts on continuation byte
            {
                new_pos = idx+1;
                return -1;
            }
            else if (c < 0xe0) // Two-byte encoding
            {
                result = (c & 0x1f) << 6;

                if (s.size() - idx < 2) // Not enough bytes?
                {
                    new_pos = s.size();
                    return -1;
                }

                c = s[idx+1] & 0xff;
                if ((c >> 6) != 2) // Not a continuation byte?
                {
                    new_pos = idx+1;
                    return -1;
                }

                new_pos = idx+2;
                result |= c & 0x3f;
                if (result < 0x80) // Overlong encoding, could use one byte
                    return -1;

                return result;
            }
            else if (c < 0xf0) // Three-byte encoding
            {
                result = uint32_t(c & 0xf) << 12;

                if (s.size() - idx < 3) // Not enough bytes?
                {
                    new_pos = s.size();
                    return -1;
                }

                c = s[idx+1] & 0xff;
                if ((c >> 6) != 2) // Not a continuation byte?
                {
                    new_pos = idx+1;
                    return -1;
                }

                result |= (c & 0x3f) << 6;
                c = s[idx+2] & 0xff;
                if ((c >> 6) != 2) // Not a continuation byte?
                {
                    new_pos = idx+2;
                    return -1;
                }

                new_pos = idx+3;
                result |= c & 0x3f;
                if (result < 0x800) // Overlong encoding, could use one or two bytes
                    return -1;

                return result;
            }
            else if (c < 0xf8) // Four-byte encoding
            {
                result = uint32_t(c & 0x7) << 24;

                if (s.size() - idx < 4) // Not enough bytes?
                {
                    new_pos = s.size();
                    return -1;
                }

                c = s[idx+1] & 0xff;
                if ((c >> 6) != 2) // Not a continuation byte?
                {
                    new_pos = idx+1;
                    return -1;
                }

                result |= uint32_t(c & 0x3f) << 12;
                c = s[idx+2] & 0xff;
                if ((c >> 6) != 2) // Not a continuation byte?
                {
                    new_pos = idx+2;
                    return -1;
                }

                result |= (c & 0x3f) << 6;
                c = s[idx+3] & 0xff;
                if ((c >> 6) != 2) // Not a continuation byte?
                {
                    new_pos = idx+3;
                    return -1;
                }

                new_pos = idx+4;
                result |= c & 0x3f;
                if (result < 0x10000) // Overlong encoding, could use one, two, or three bytes
                    return -1;

                return result;
            }
            else
            {
                new_pos = idx+1;
                return result;
            }
        }

        // Returns -1 on conversion failure, invalid codepoint, overlong encoding, truncated input, or proper EOF (`eof` is set to true iff proper EOF is met, false otherwise)
        // Returns codepoint on success
        uint32_t utf8_to_ucs(core::istream &stream, bool *eof)
        {
            uint32_t result = 0;
            int32_t c = stream.get();

            if (c == EOF)
            {
                if (eof)
                    *eof = true;
                return -1;
            }
            else if (eof)
                *eof = false;

            c &= 0xff;
            if (c < 0x80) // Single-byte encoding
                return c;
            else if (c < 0xc0) // Invalid, starts on continuation byte
                return -1;
            else if (c < 0xe0) // Two-byte encoding
            {
                result = (c & 0x1f) << 6;

                c = stream.get();
                if (c == EOF || (c & 0xc0) != 0xc0) // EOF, or not a continuation byte?
                    return -1;

                result |= c & 0x3f;
                if (result < 0x80) // Overlong encoding, could use one byte
                    return -1;

                return result;
            }
            else if (c < 0xf0) // Three-byte encoding
            {
                result = uint32_t(c & 0xf) << 12;

                c = stream.get();
                if (c == EOF || (c & 0xc0) != 0xc0) // EOF, or not a continuation byte?
                    return -1;

                result |= (c & 0x3f) << 6;

                c = stream.get();
                if (c == EOF || (c & 0xc0) != 0xc0) // EOF, or not a continuation byte?
                    return -1;

                result |= c & 0x3f;
                if (result < 0x800) // Overlong encoding, could use one or two bytes
                    return -1;

                return result;
            }
            else if (c < 0xf8) // Four-byte encoding
            {
                result = uint32_t(c & 0x7) << 24;

                c = stream.get();
                if (c == EOF || (c & 0xc0) != 0xc0) // EOF, or not a continuation byte?
                    return -1;

                result |= uint32_t(c & 0x3f) << 12;

                c = stream.get();
                if (c == EOF || (c & 0xc0) != 0xc0) // EOF, or not a continuation byte?
                    return -1;

                result |= (c & 0x3f) << 6;

                c = stream.get();
                if (c == EOF || (c & 0xc0) != 0xc0) // EOF, or not a continuation byte?
                    return -1;

                result |= c & 0x3f;
                if (result < 0x10000) // Overlong encoding, could use one, two, or three bytes
                    return -1;

                return result;
            }
            else
                return -1;
        }

        std::vector<uint32_t> utf8_to_ucs(const std::string &s)
        {
            std::vector<uint32_t> result;

            for (size_t i = 0; i < s.size(); )
                result.push_back(utf8_to_ucs(s, i, i));

            return result;
        }

        core::string_t ucs_to_utf8(uint32_t codepoint)
        {
            return ucs_to_utf8(&codepoint, 1);
        }
    }
}

#endif // UTF_H

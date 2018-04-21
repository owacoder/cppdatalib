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
        // Converts ASCII characters in string to lowercase
        void ascii_lowercase(std::string &str)
        {
            for (size_t i = 0; i < str.size(); ++i)
            {
                unsigned char c = str[i];
                if (c < 0x80)
                    str[i] = tolower(c);
            }
        }

        // Returns string with ASCII characters converted to lowercase
        std::string ascii_lowercase_copy(const std::string &str)
        {
            std::string copy(str);
            ascii_lowercase(copy);
            return copy;
        }

        // Converts ASCII characters in string to uppercase
        void ascii_uppercase(std::string &str)
        {
            for (size_t i = 0; i < str.size(); ++i)
            {
                unsigned char c = str[i];
                if (c < 0x80)
                    str[i] = toupper(c);
            }
        }

        // Returns string with ASCII characters converted to uppercase
        std::string ascii_uppercase_copy(const std::string &str)
        {
            std::string copy(str);
            ascii_uppercase(copy);
            return copy;
        }

        // Returns empty string if invalid codepoint is encountered
        // Returns UTF-8 string on valid input
        std::string ucs_to_utf8(const uint32_t *s, size_t size)
        {
            std::string result;

            for (size_t i = 0; i < size; ++i)
            {
                if (s[i] >= 0xd800 && s[i] <= 0xdfff)
                    return std::string();

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
                    return std::string();
            }

            return result;
        }

        // Returns empty string if invalid codepoint is encountered
        // Returns UTF-xx string on valid input
        std::string ucs_to_utf(const uint32_t *s, size_t size, encoding mode)
        {
            if (mode == utf8)
                return ucs_to_utf8(s, size);

            std::string result;

            for (size_t i = 0; i < size; ++i)
            {
                switch (mode)
                {
                    case raw:
                        if (s[i] > 0xff)
                            return std::string();
                        result.push_back(s[i]);
                        break;
                    case raw16:
                        if (s[i] > 0xffff)
                            return std::string();
                        result.push_back(s[i] >> 8);
                        result.push_back(s[i] & 0xff);
                        break;
                    case raw32:
                        result.push_back(s[i] >> 24);
                        result.push_back((s[i] >> 16) & 0xff);
                        result.push_back((s[i] >> 8) & 0xff);
                        result.push_back(s[i] & 0xff);
                        break;
                    case utf16_big_endian:
                        if (s[i] >= 0xd800 && s[i] <= 0xdfff)
                            return std::string();
                        else if (s[i] <= 0xffff)
                        {
                            result.push_back(s[i] >> 8);
                            result.push_back(s[i] & 0xff);
                        }
                        else if (s[i] <= 0x10ffff)
                        {
                            uint32_t temp = s[i] - 0x10000;
                            result.push_back(0xd8 | (temp >> 18));
                            result.push_back((temp >> 10) & 0xff);
                            result.push_back(0xdc | ((temp >> 8) & 0x3));
                            result.push_back(temp & 0xff);
                        }
                        else
                            return std::string();
                        break;
                    case utf16_little_endian:
                        if (s[i] >= 0xd800 && s[i] <= 0xdfff)
                            return std::string();
                        else if (s[i] <= 0xffff)
                        {
                            result.push_back(s[i] & 0xff);
                            result.push_back(s[i] >> 8);
                        }
                        else if (s[i] <= 0x10ffff)
                        {
                            uint32_t temp = s[i] - 0x10000;
                            result.push_back((temp >> 10) & 0xff);
                            result.push_back(0xd8 | (temp >> 18));
                            result.push_back(temp & 0xff);
                            result.push_back(0xdc | ((temp >> 8) & 0x3));
                        }
                        else
                            return std::string();
                        break;
                    case utf32_big_endian:
                        if ((s[i] >= 0xd800 && s[i] <= 0xdfff) || s[i] > 0x10ffff)
                            return std::string();
                        else
                        {
                            result.push_back(s[i] >> 24);
                            result.push_back((s[i] >> 16) & 0xff);
                            result.push_back((s[i] >> 8) & 0xff);
                            result.push_back(s[i] & 0xff);
                        }
                        break;
                    case utf32_little_endian:
                        if ((s[i] >= 0xd800 && s[i] <= 0xdfff) || s[i] > 0x10ffff)
                            return std::string();
                        else
                        {
                            result.push_back(s[i] & 0xff);
                            result.push_back((s[i] >> 8) & 0xff);
                            result.push_back((s[i] >> 16) & 0xff);
                            result.push_back(s[i] >> 24);
                        }
                        break;
                    case utf32_2143_endian:
                        if ((s[i] >= 0xd800 && s[i] <= 0xdfff) || s[i] > 0x10ffff)
                            return std::string();
                        else
                        {
                            result.push_back((s[i] >> 8) & 0xff);
                            result.push_back(s[i] & 0xff);
                            result.push_back(s[i] >> 24);
                            result.push_back((s[i] >> 16) & 0xff);
                        }
                        break;
                    case utf32_3412_endian:
                        if ((s[i] >= 0xd800 && s[i] <= 0xdfff) || s[i] > 0x10ffff)
                            return std::string();
                        else
                        {
                            result.push_back((s[i] >> 16) & 0xff);
                            result.push_back(s[i] >> 24);
                            result.push_back(s[i] & 0xff);
                            result.push_back((s[i] >> 8) & 0xff);
                        }
                        break;
                    default:
                        return std::string();
                }
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
                if (result < 0x10000 || // Overlong encoding, could use one, two, or three bytes
                        result > 0x10ffff) // Codepoint out of range
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
                if (c == EOF || (c & 0xc0) != 0x80) // EOF, or not a continuation byte?
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
                if (c == EOF || (c & 0xc0) != 0x80) // EOF, or not a continuation byte?
                    return -1;

                result |= (c & 0x3f) << 6;

                c = stream.get();
                if (c == EOF || (c & 0xc0) != 0x80) // EOF, or not a continuation byte?
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
                if (c == EOF || (c & 0xc0) != 0x80) // EOF, or not a continuation byte?
                    return -1;

                result |= uint32_t(c & 0x3f) << 12;

                c = stream.get();
                if (c == EOF || (c & 0xc0) != 0x80) // EOF, or not a continuation byte?
                    return -1;

                result |= (c & 0x3f) << 6;

                c = stream.get();
                if (c == EOF || (c & 0xc0) != 0x80) // EOF, or not a continuation byte?
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

        std::string ucs_to_utf8(uint32_t codepoint)
        {
            return ucs_to_utf8(&codepoint, 1);
        }

        // Returns empty string if invalid codepoint is encountered
        // Returns UTF-xx string on valid input
        std::string ucs_to_utf(const uint32_t *s, size_t size, const char *encoding_name)
        {
            return ucs_to_utf(s, size, encoding_from_name(encoding_name));
        }

        std::string ucs_to_utf(uint32_t codepoint, encoding mode)
        {
            return ucs_to_utf(&codepoint, 1, mode);
        }

        std::string ucs_to_utf(uint32_t codepoint, const char *encoding_name)
        {
            return ucs_to_utf(&codepoint, 1, encoding_from_name(encoding_name));
        }

        // Returns -1 on conversion failure, invalid codepoint, overlong encoding, or truncated input
        // Returns codepoint on success, with updated `idx` stored in `new_pos`
        uint32_t utf_to_ucs(const std::string &s, encoding mode, size_t idx, size_t &new_pos)
        {
            uint32_t result = 0;

            switch (mode)
            {
                case raw: new_pos = idx+1; return s[idx] & 0xff;
                case utf8: return utf8_to_ucs(s, idx, new_pos);
                case raw16:
                case utf16_big_endian:
                case utf16_little_endian:
                {
                    if (s.size() - idx < 2)
                    {
                        new_pos = s.size();
                        return -1;
                    }

                    new_pos = idx+2;
                    switch (mode)
                    {
                        default:
                        case raw16:
                        case utf16_big_endian:
                            for (int i = 0; i < 2; ++i)
                                result = (result << 8) | uint8_t(s[idx+i]);
                            if (mode == raw16)
                                return result;

                            if (result >= 0xdc00 && result <= 0xdfff) // Low surrogate without high surrogate
                                return -1;
                            else if (result >= 0xd800 && result <= 0xdbff) // High surrogate
                            {
                                if (s.size() - idx < 4)
                                {
                                    new_pos = s.size();
                                    return -1;
                                }

                                new_pos = idx+4;

                                for (int i = 2; i < 4; ++i)
                                    result = (result << 8) | uint8_t(s[idx+i]);
                                uint32_t low = result & 0xffff;
                                if (low < 0xdc00 || low > 0xdfff) // Not followed by a low surrogate
                                    return -1;

                                result -= 0xd800dc00; // Subtract metadata value of both surrogates
                                result = ((result & 0x3ff0000) >> 6) | (result & 0x3ff);
                                result += 0x10000;
                            }

                            break;
                        case utf16_little_endian:
                            for (int i = 0; i < 2; ++i)
                                result |= uint32_t(uint8_t(s[idx+i])) << i*8;

                            if (result >= 0xdc00 && result <= 0xdfff) // Low surrogate without high surrogate
                                return -1;
                            else if (result >= 0xd800 && result <= 0xdbff) // High surrogate
                            {
                                if (s.size() - idx < 4)
                                {
                                    new_pos = s.size();
                                    return -1;
                                }

                                new_pos = idx+4;

                                result <<= 16;
                                for (int i = 0; i < 2; ++i)
                                    result |= uint32_t(uint8_t(s[idx+i+2])) << i*8;
                                uint32_t low = result & 0xffff;
                                if (low < 0xdc00 || low > 0xdfff) // Not followed by a low surrogate
                                    return -1;

                                result -= 0xd800dc00; // Subtract metadata value of both surrogates
                                result = ((result & 0x3ff0000) >> 6) | (result & 0x3ff);
                                result += 0x10000;
                            }
                            break;
                    }

                    break;
                }
                case raw32:
                case utf32_big_endian:
                case utf32_little_endian:
                case utf32_2143_endian:
                case utf32_3412_endian:
                {
                    if (s.size() - idx < 4)
                    {
                        new_pos = s.size();
                        return -1;
                    }

                    new_pos = idx+4;
                    switch (mode)
                    {
                        default:
                        case raw32:
                        case utf32_big_endian:
                            for (int i = 0; i < 4; ++i)
                                result = (result << 8) | uint8_t(s[idx+i]);
                            if (mode == raw32)
                                return result;
                            break;
                        case utf32_little_endian:
                            for (int i = 0; i < 4; ++i)
                                result |= uint32_t(uint8_t(s[idx+i])) << i*8;
                            break;
                        case utf32_2143_endian:
                            result |= uint32_t(uint8_t(s[idx+0])) << 8;
                            result |= uint32_t(uint8_t(s[idx+1])) << 0;
                            result |= uint32_t(uint8_t(s[idx+2])) << 24;
                            result |= uint32_t(uint8_t(s[idx+3])) << 16;
                            break;
                        case utf32_3412_endian:
                            result |= uint32_t(uint8_t(s[idx+0])) << 16;
                            result |= uint32_t(uint8_t(s[idx+1])) << 24;
                            result |= uint32_t(uint8_t(s[idx+2])) << 0;
                            result |= uint32_t(uint8_t(s[idx+3])) << 8;
                            break;
                    }

                    break;
                }
                default: // Unrecognized encoding
                    return -1;
            }

            return result > 0x10ffff? uint32_t(-1): result;
        }

        // Returns -1 on conversion failure, invalid codepoint, overlong encoding, or truncated input
        // Returns codepoint on success, with updated `idx` stored in `new_pos`
        uint32_t utf_to_ucs(core::istream &stream, encoding mode, bool *eof)
        {
            if (mode == utf8)
                return utf8_to_ucs(stream, eof);

            uint32_t result = 0;
            int c = stream.get();

            if (c == EOF)
            {
                if (eof)
                    *eof = true;
                return -1;
            }
            else if (eof)
                *eof = false;

            result = c;

            switch (mode)
            {
                case raw: return result;
                case raw16:
                case utf16_big_endian:
                case utf16_little_endian:
                {
                    c = stream.get();
                    if (c == EOF)
                        return -1;

                    switch (mode)
                    {
                        default:
                        case raw16:
                        case utf16_big_endian:
                            result = (result << 8) | c;
                            if (mode == raw16)
                                return result;

                            if (result >= 0xdc00 && result <= 0xdfff) // Low surrogate without high surrogate
                                return -1;
                            else if (result >= 0xd800 && result <= 0xdbff) // High surrogate
                            {
                                int32_t low = stream.get();
                                if (low == EOF)
                                    return -1;

                                c = stream.get();
                                if (c == EOF)
                                    return -1;

                                low = (low << 8) | c;
                                if (low < 0xdc00 || low > 0xdfff) // Not followed by a low surrogate
                                    return -1;

                                result = (result << 16) | low;
                                result -= 0xd800dc00; // Subtract metadata value of both surrogates
                                result = ((result & 0x3ff0000) >> 6) | (result & 0x3ff);
                                result += 0x10000;
                            }

                            break;
                        case utf16_little_endian:
                            result |= uint8_t(c) << 8;

                            if (result >= 0xdc00 && result <= 0xdfff) // Low surrogate without high surrogate
                                return -1;
                            else if (result >= 0xd800 && result <= 0xdbff) // High surrogate
                            {
                                int32_t low = stream.get();
                                if (low == EOF)
                                    return -1;

                                c = stream.get();
                                if (c == EOF)
                                    return -1;

                                low |= uint8_t(c) << 8;
                                if (low < 0xdc00 || low > 0xdfff) // Not followed by a low surrogate
                                    return -1;

                                result = (result << 16) | low;
                                result -= 0xd800dc00; // Subtract metadata value of both surrogates
                                result = ((result & 0x3ff0000) >> 6) | (result & 0x3ff);
                                result += 0x10000;
                            }
                            break;
                    }

                    break;
                }
                case raw32:
                case utf32_big_endian:
                case utf32_little_endian:
                case utf32_2143_endian:
                case utf32_3412_endian:
                {
                    char buf[4];

                    buf[0] = c;
                    if (!stream.read(buf+1, 3))
                        return -1;

                    switch (mode)
                    {
                        default:
                        case raw32:
                        case utf32_big_endian:
                            result = (uint32_t(uint8_t(buf[0])) << 24) |
                                    (uint32_t(uint8_t(buf[1])) << 16) |
                                    (uint32_t(uint8_t(buf[2])) << 8) |
                                    (uint32_t(uint8_t(buf[3])));
                            if (mode == raw32)
                                return result;
                            break;
                        case utf32_little_endian:
                            result = (uint32_t(uint8_t(buf[3])) << 24) |
                                    (uint32_t(uint8_t(buf[2])) << 16) |
                                    (uint32_t(uint8_t(buf[1])) << 8) |
                                    (uint32_t(uint8_t(buf[0])));
                            break;
                        case utf32_2143_endian:
                            result = (uint32_t(uint8_t(buf[2])) << 24) |
                                    (uint32_t(uint8_t(buf[3])) << 16) |
                                    (uint32_t(uint8_t(buf[0])) << 8) |
                                    (uint32_t(uint8_t(buf[1])));
                            break;
                        case utf32_3412_endian:
                            result = (uint32_t(uint8_t(buf[1])) << 24) |
                                    (uint32_t(uint8_t(buf[0])) << 16) |
                                    (uint32_t(uint8_t(buf[3])) << 8) |
                                    (uint32_t(uint8_t(buf[2])));
                            break;
                    }

                    break;
                }
                default: // Unrecognized encoding
                    return -1;
            }

            return result > 0x10ffff? uint32_t(-1): result;
        }

        std::vector<uint32_t> utf_to_ucs(const std::string &s, encoding mode)
        {
            std::vector<uint32_t> result;

            for (size_t i = 0; i < s.size(); )
                result.push_back(utf_to_ucs(s, mode, i, i));

            return result;
        }

        encoding encoding_from_name(const char *encoding_name)
        {
            if (!strcmp(encoding_name, "UTF-8"))
                return utf8;
            else if (!strcmp(encoding_name, "UTF-16BE"))
                return utf16_big_endian;
            else if (!strcmp(encoding_name, "UTF-16LE"))
                return utf16_little_endian;
            else if (!strcmp(encoding_name, "UTF-32BE"))
                return utf32_big_endian;
            else if (!strcmp(encoding_name, "UTF-32LE"))
                return utf32_little_endian;
            else if (!strcmp(encoding_name, "UTF-32-2143"))
                return utf32_2143_endian;
            else if (!strcmp(encoding_name, "UTF-32-3412"))
                return utf32_3412_endian;
            else
                return unknown;
        }

        std::vector<uint32_t> utf_to_ucs(const std::string &s, const char *encoding_name)
        {
            return utf_to_ucs(s, encoding_from_name(encoding_name));
        }
    }
}

#endif // UTF_H

/*
 * istream.h
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

#ifndef CPPDATALIB_ISTREAM_H
#define CPPDATALIB_ISTREAM_H

#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <cstring>
#include <memory>

#include "error.h"

namespace cppdatalib
{
    namespace core
    {
#ifdef CPPDATALIB_ENABLE_FAST_IO
        class istream
        {
        public:
            typedef int64_t streamsize;
            typedef unsigned int iostate;

        private:
            friend class sentry;

            struct sentry
            {
                sentry(istream &stream, bool skipws = false)
                {
                    if (skipws && !stream.flags_)
                        skip_spaces(stream);
                    success = stream;
                }

                void skip_spaces(istream &stream)
                {
                    int c;
                    do
                    {
                        c = stream.getc_();
                    } while (c != EOF && isspace(c));
                    last = c;
                }

                operator bool() const {return success;}
                int get_last_char() const {return last;}

            private:
                bool success;
                int last;
            };

        protected:
            static const unsigned int fail_bit = 1;
            static const unsigned int eof_bit = 2;
            static const unsigned int bad_bit = 4;

            unsigned int flags_;
            bool skip_ws;
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
            streamsize last_read_;
#endif

            virtual int getc_() = 0;
            virtual void ungetc_() = 0;
            virtual int peekc_() = 0;

        public:
            istream() : flags_(0), skip_ws(true)
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
              , last_read_(0)
#endif
            {}

            iostate rdstate() const {return flags_;}
            operator bool() const {return rdstate() == 0;}
            bool operator!() const {return rdstate() != 0;}

            bool good() const {return rdstate() == 0;}
            bool eof() const {return rdstate() & eof_bit;}
            bool fail() const {return rdstate() & (fail_bit | bad_bit);}
            bool bad() const {return rdstate() & bad_bit;}

#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
            streamsize gcount() const {return last_read_;}
#endif

            int peek()
            {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = 0;
#endif

                sentry s(*this);
                if (!s)
                    return EOF;

                int c = peekc_();
                if (c == EOF)
                    flags_ = fail_bit | eof_bit;
                return c;
            }

            int get()
            {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = 0;
#endif

                sentry s(*this);
                if (!s)
                {
                    flags_ |= fail_bit;
                    return EOF;
                }

                int c = getc_();
                if (c == EOF)
                    flags_ = fail_bit | eof_bit;
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                else
                    last_read_ = 1;
#endif
                return c;
            }
            istream &get(char &ch)
            {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = 0;
#endif

                sentry s(*this);
                if (!s)
                {
                    flags_ |= fail_bit;
                    return *this;
                }

                int c = getc_();
                if (c == EOF)
                    flags_ = fail_bit | eof_bit;
                else
                {
                    ch = c;
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                    last_read_ = 1;
#endif
                }
                return *this;
            }
            istream &get(char *ch, streamsize n) {return get(ch, n, '\n');}
            istream &get(char *ch, streamsize n, char delim)
            {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = 0;
#endif

                sentry s(*this);
                if (!s || n < 2)
                {
                    flags_ |= fail_bit;
                    return *this;
                }

                while (--n > 0)
                {
                    int c = getc_();
                    if (c == EOF)
                    {
                        flags_ = eof_bit;
                        break;
                    }
                    else if (c == delim)
                    {
                        ungetc_();
                        break;
                    }
                    else
                        *ch++ = c;

#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                    ++last_read_;
#endif
                }
                *ch = 0;

#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                if (last_read_ == 0)
                    flags_ = fail_bit;
#endif

                return *this;
            }

            istream &read(char *ch, streamsize n)
            {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = 0;
#endif

                sentry s(*this);
                if (!s)
                {
                    flags_ |= fail_bit;
                    return *this;
                }

                while (n-- > 0)
                {
                    int c = getc_();
                    if (c == EOF)
                    {
                        flags_ = eof_bit | fail_bit;
                        break;
                    }
                    else
                        *ch++ = c;

#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                    ++last_read_;
#endif
                }

                return *this;
            }

            void unget()
            {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = 0;
#endif
                flags_ &= ~eof_bit;

                sentry s(*this);
                if (s)
                    ungetc_();
            }

            friend istream &operator>>(istream &in, short &val);
            friend istream &operator>>(istream &in, unsigned short &val);
            friend istream &operator>>(istream &in, int &val);
            friend istream &operator>>(istream &in, unsigned int &val);
            friend istream &operator>>(istream &in, long &val);
            friend istream &operator>>(istream &in, unsigned long &val);
            friend istream &operator>>(istream &in, long long &val);
            friend istream &operator>>(istream &in, unsigned long long &val);
            friend istream &operator>>(istream &in, float &val);
            friend istream &operator>>(istream &in, double &val);
            friend istream &operator>>(istream &in, long double &val);

            friend istream &operator>>(istream &in, char &val);
            friend istream &operator>>(istream &in, signed char &val);
            friend istream &operator>>(istream &in, unsigned char &val);

            friend istream &operator>>(istream &in, std::streambuf *buf);

            friend istream &operator>>(istream &in, std::ios_base &(*pf)(std::ios_base &));

        private:
            template<typename T>
            istream &read_formatted_char(T &ch)
            {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = 0;
#endif

                sentry s(*this, skip_ws);
                if (!s)
                {
                    flags_ |= fail_bit;
                    return *this;
                }

                int c = s.get_last_char();
                if (c == EOF)
                    flags_ = fail_bit | eof_bit;
                else
                {
                    ch = c;
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                    last_read_ = 1;
#endif
                }

                return *this;
            }

            template<typename T>
            istream &read_formatted_signed_int(T &i)
            {
                bool negative = false;
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = 0;
#endif

                sentry s(*this, skip_ws);
                if (!s)
                {
                    flags_ |= fail_bit;
                    return *this;
                }

                int c = s.get_last_char();
                if (c == '-')
                    negative = true, c = getc_();

                if (c == EOF)
                    flags_ = fail_bit | eof_bit;
                else
                {
                    bool out_of_range = false;
                    i = 0;
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                    last_read_ += negative;
#else
                    bool read = false;
#endif
                    while (c != EOF && isdigit(c))
                    {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                        ++last_read_;
#else
                        read = true;
#endif
                        if (!out_of_range)
                        {
                            if (negative)
                            {
                                if (std::numeric_limits<T>::min() / 10 + (c - '0') > i)
                                    out_of_range = true, i = std::numeric_limits<T>::min(), flags_ |= fail_bit;
                                else
                                    i = (i * 10) - (c - '0');
                            }
                            else
                            {
                                if (std::numeric_limits<T>::max() / 10 - (c - '0') < i)
                                    out_of_range = true, i = std::numeric_limits<T>::max(), flags_ |= fail_bit;
                                else
                                    i = (i * 10) + (c - '0');
                            }
                        }
                        c = getc_();
                    }

                    if (c == EOF)
                        flags_ |= eof_bit;
                    else
                        ungetc_();

#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                    if (negative == last_read_)
                        flags_ |= fail_bit;
#else
                    if (!read)
                        flags_ |= fail_bit;
#endif
                }

                return *this;
            }

            template<typename T>
            istream &read_formatted_unsigned_int(T &i)
            {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = 0;
#endif

                sentry s(*this, skip_ws);
                if (!s)
                {
                    flags_ |= fail_bit;
                    return *this;
                }

                int c = s.get_last_char();

                if (c == EOF)
                    flags_ = fail_bit | eof_bit;
                else
                {
                    bool out_of_range = false;
                    i = 0;

#ifdef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                    bool read = false;
#endif

                    while (c != EOF && isdigit(c))
                    {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                        ++last_read_;
#else
                        read = true;
#endif
                        if (!out_of_range)
                        {
                            if (std::numeric_limits<T>::max() / 10 - (c - '0') < i)
                                out_of_range = true, i = std::numeric_limits<T>::max(), flags_ |= fail_bit;
                            else
                                i = (i * 10) + (c - '0');
                        }
                        c = getc_();
                    }

                    if (c == EOF)
                        flags_ |= eof_bit;
                    else
                        ungetc_();

#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                    if (last_read_ == 0)
                        flags_ |= fail_bit;
#else
                    if (!read)
                        flags_ |= fail_bit;
#endif
                }

                return *this;
            }

            template<typename T, typename Converter>
            istream &read_formatted_real(T &f, Converter convert)
            {
                std::string str;

#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = 0;
#endif

                sentry s(*this, skip_ws);
                if (!s)
                {
                    flags_ |= fail_bit;
                    return *this;
                }

                int c = s.get_last_char();

                if (c == EOF)
                    flags_ = fail_bit | eof_bit;
                else
                {
                    f = 0;
                    while (c != EOF && (isdigit(c) || strchr(".eE+-", c)))
                    {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                        ++last_read_;
#endif
                        str.push_back(c);
                        c = getc_();
                    }

                    errno = 0;
                    char *end;
                    f = convert(str.c_str(), &end);
                    if (errno == ERANGE || *end != 0)
                        flags_ |= fail_bit;

                    if (c == EOF)
                        flags_ = eof_bit;
                    else
                        ungetc_();
                }

                return *this;
            }
        };

        istream &operator>>(istream &in, char &ch) {return in.read_formatted_char(ch);}
        istream &operator>>(istream &in, signed char &ch) {return in.read_formatted_char(ch);}
        istream &operator>>(istream &in, unsigned char &ch) {return in.read_formatted_char(ch);}

        istream &operator>>(istream &in, signed short &val) {return in.read_formatted_signed_int(val);}
        istream &operator>>(istream &in, signed int &val) {return in.read_formatted_signed_int(val);}
        istream &operator>>(istream &in, signed long &val) {return in.read_formatted_signed_int(val);}
        istream &operator>>(istream &in, signed long long &val) {return in.read_formatted_signed_int(val);}

        istream &operator>>(istream &in, unsigned short &val) {return in.read_formatted_unsigned_int(val);}
        istream &operator>>(istream &in, unsigned int &val) {return in.read_formatted_unsigned_int(val);}
        istream &operator>>(istream &in, unsigned long &val) {return in.read_formatted_unsigned_int(val);}
        istream &operator>>(istream &in, unsigned long long &val) {return in.read_formatted_unsigned_int(val);}

        istream &operator>>(istream &in, float &val) {return in.read_formatted_real(val, strtof);}
        istream &operator>>(istream &in, double &val) {return in.read_formatted_real(val, strtod);}
        istream &operator>>(istream &in, long double &val) {return in.read_formatted_real(val, strtold);}

        istream &operator>>(istream &in, std::streambuf *buf)
        {
            istream::streamsize read = 0;
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
            in.last_read_ = 0;
#endif

            istream::sentry s(in);
            if (!s || !buf)
            {
                in.flags_ |= istream::fail_bit;
                return in;
            }

            int c = in.getc_();
            while (c != EOF)
            {
                ++read;
                buf->sputc(c);
                c = in.getc_();
            }

            in.flags_ |= istream::eof_bit;

#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
            in.last_read_ = read;
#endif

            if (read == 0)
                in.flags_ |= istream::fail_bit;
            return in;
        }

        istream &operator>>(istream &in, std::ios_base &(*pf)(std::ios_base &))
        {
            if (pf == std::skipws)
                in.skip_ws = true;
            else if (pf == std::noskipws)
                in.skip_ws = false;
            else
                throw core::error("cppdatalib::core::istream - stream manipulator applied to input stream is not supported");
            return in;
        }

        class istring_wrapper_stream : public istream
        {
            const std::string &string;
            size_t pos;

        public:
            istring_wrapper_stream(const std::string &string)
                : string(string)
                , pos(0)
            {}

            const std::string &str() const {return string;}

        protected:
            int getc_() {return pos < string.size()? string[pos++] & 0xff: EOF;}
            int peekc_() {return pos < string.size()? string[pos] & 0xff: EOF;}
            void ungetc_()
            {
                if (pos > 0)
                    --pos;
                else
                    flags_ |= bad_bit;
            }
        };

        class istringstream : public istream
        {
            std::string string;
            size_t pos;

        public:
            istringstream()
                : string()
                , pos(0)
            {}
            istringstream(const std::string &string)
                : string(string)
                , pos(0)
            {}

            const std::string &str() const {return string;}
            void str(const std::string &s) {string = s; pos = 0;}

        protected:
            int getc_() {return pos < string.size()? string[pos++]: EOF;}
            int peekc_() {return pos < string.size()? string[pos]: EOF;}
            void ungetc_()
            {
                if (pos > 0)
                    --pos;
                else
                    flags_ |= bad_bit;
            }
        };

        class istd_streambuf_wrapper : public istream
        {
            std::streambuf *stream_;

        public:
            istd_streambuf_wrapper(std::streambuf *stream)
                : stream_(stream)
            {}

        protected:
            int getc_() {return stream_->sbumpc();}
            int peekc_() {return stream_->sgetc();}
            void ungetc_()
            {
                if (stream_->sungetc() == EOF)
                    flags_ |= bad_bit;
            }
        };

        class istream_handle
        {
            std::istream *std_;
            std::shared_ptr<istream> d_;

            istream *predef_;

        public:
            istream_handle(core::istream &stream) : std_(NULL), d_(NULL), predef_(&stream) {}
            istream_handle(std::istream &stream) : std_(&stream), d_(NULL), predef_(NULL)
            {
                d_ = std::make_shared<istd_streambuf_wrapper>(stream.rdbuf());
            }
            istream_handle(const char *string) : std_(NULL), d_(NULL)
            {
                d_ = std::make_shared<istringstream>(string);
            }
            istream_handle(const char *string, size_t len) : std_(NULL), d_(NULL)
            {
                d_ = std::make_shared<istringstream>(std::string(string, len));
            }
            istream_handle(const std::string &string) : std_(NULL), d_(NULL), predef_(NULL)
            {
                d_ = std::make_shared<istring_wrapper_stream>(string);
            }

            operator core::istream &() {return predef_? *predef_: *d_;}

            // std_stream() returns NULL if not created from a standard stream
            std::istream *std_stream() {return std_;}
        };
#else
        typedef std::istream istream;
        typedef std::istringstream istringstream;
        typedef std::istringstream istring_wrapper_stream;

        class istream_handle
        {
            core::istream *std_;
            std::shared_ptr<istream> d_;

        public:
            istream_handle(core::istream &stream) : std_(&stream), d_(NULL) {}
            istream_handle(const char *string) : std_(NULL), d_(NULL)
            {
                d_ = std::make_shared<istringstream>(string);
            }
            istream_handle(const char *string, size_t len) : std_(NULL), d_(NULL)
            {
                d_ = std::make_shared<istringstream>(std::string(string, len));
            }
            istream_handle(const std::string &string) : std_(NULL), d_(NULL)
            {
                d_ = std::make_shared<istringstream>(string);
            }

            operator core::istream &() {return std_? *std_: *d_;}

            // std_stream() returns NULL if not created from a standard stream
            std::istream *std_stream() {return std_;}
        };
#endif

        template<typename T>
        core::istream &read_uint8(core::istream &strm, T &val)
        {
            int chr = strm.get();
            if (chr != EOF)
                val = chr;
            return strm;
        }

        template<typename T>
        core::istream &read_int8(core::istream &strm, T &val)
        {
            uint8_t chr = 0;
            if (read_uint8(strm, chr))
            {
                if (chr < 0x80)
                    val = chr;
                else
                    val = -T((~unsigned(chr) + 1) & 0xff);
            }

            return strm;
        }

        template<typename T>
        core::istream &read_uint16_be(core::istream &strm, T &val)
        {
            char buf[2];
            strm.read(buf, 2);
            if (strm)
                val = (uint16_t(uint8_t(buf[0])) << 8) | uint8_t(buf[1]);
            return strm;
        }

        template<typename T>
        core::istream &read_uint16_le(core::istream &strm, T &val)
        {
            char buf[2];
            strm.read(buf, 2);
            if (strm)
                val = (uint16_t(uint8_t(buf[1])) << 8) | uint8_t(buf[0]);
            return strm;
        }

        template<typename T>
        core::istream &read_int16_be(core::istream &strm, T &val)
        {
            uint16_t temp;
            if (read_uint16_be(strm, temp))
            {
                if (temp < 0x8000)
                    val = temp;
                else if (temp == 0x8000)
                    val = INT16_MIN;
                else
                    val = -T((~unsigned(temp) + 1) & 0xffff);
            }

            return strm;
        }

        template<typename T>
        core::istream &read_int16_le(core::istream &strm, T &val)
        {
            uint16_t temp;
            if (read_uint16_le(strm, temp))
            {
                if (temp < 0x8000)
                    val = temp;
                else if (temp == 0x8000)
                    val = INT16_MIN;
                else
                    val = -T((~unsigned(temp) + 1) & 0xffff);
            }

            return strm;
        }

        template<typename T>
        core::istream &read_uint32_be(core::istream &strm, T &val)
        {
            char buf[4];
            strm.read(buf, 4);
            if (strm)
            {
                val = 0;
                for (int i = 0; i < 4; ++i)
                    val = (val << 8) | uint8_t(buf[i]);
            }
            return strm;
        }

        template<typename T>
        core::istream &read_uint32_le(core::istream &strm, T &val)
        {
            char buf[4];
            strm.read(buf, 4);
            if (strm)
            {
                val = 0;
                for (int i = 0; i < 4; ++i)
                    val |= uint32_t(uint8_t(buf[i])) << 8*i;
            }
            return strm;
        }

        template<typename T>
        core::istream &read_int32_be(core::istream &strm, T &val)
        {
            uint32_t temp;
            if (read_uint32_be(strm, temp))
            {
                if (temp < 0x80000000)
                    val = temp;
                else if (temp == 0x80000000)
                    val = INT32_MIN;
                else
                    val = -T((~(unsigned long) temp + 1) & 0xffffffff);
            }

            return strm;
        }

        template<typename T>
        core::istream &read_int32_le(core::istream &strm, T &val)
        {
            uint32_t temp;
            if (read_uint32_le(strm, temp))
            {
                if (temp < 0x80000000)
                    val = temp;
                else if (temp == 0x80000000)
                    val = INT32_MIN;
                else
                    val = -T((~(unsigned long) temp + 1) & 0xffffffff);
            }

            return strm;
        }

        template<typename T>
        core::istream &read_uint64_be(core::istream &strm, T &val)
        {
            char buf[8];
            strm.read(buf, 8);
            if (strm)
            {
                val = 0;
                for (int i = 0; i < 8; ++i)
                    val = (val << 8) | uint8_t(buf[i]);
            }
            return strm;
        }

        template<typename T>
        core::istream &read_uint64_le(core::istream &strm, T &val)
        {
            char buf[8];
            strm.read(buf, 8);
            if (strm)
            {
                val = 0;
                for (int i = 0; i < 8; ++i)
                    val |= uint64_t(uint8_t(buf[i])) << 8*i;
            }
            return strm;
        }

        template<typename T>
        core::istream &read_int64_be(core::istream &strm, T &val)
        {
            uint64_t temp = 0;
            if (read_uint64_be(strm, temp))
            {
                if (temp < 0x8000000000000000)
                    val = temp;
                else if (temp == 0x8000000000000000)
                    val = INT64_MIN;
                else
                    val = -T((~(unsigned long long) temp + 1) & 0xffffffffffffffff);
            }

            return strm;
        }

        template<typename T>
        core::istream &read_int64_le(core::istream &strm, T &val)
        {
            uint64_t temp;
            if (read_uint64_le(strm, temp))
            {
                if (temp < 0x8000000000000000)
                    val = temp;
                else if (temp == 0x8000000000000000)
                    val = INT64_MIN;
                else
                    val = -T((~(unsigned long long) temp + 1) & 0xffffffffffffffff);
            }

            return strm;
        }
    }
}

#endif // CPPDATALIB_ISTREAM_H

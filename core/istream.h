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
#include "global.h"

#ifdef CPPDATALIB_LINUX
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#elif defined(CPPDATALIB_WINDOWS)
#include <windows.h>
#endif

namespace cppdatalib
{
    namespace core
    {
        enum encoding
        {
            unknown,
            raw, // Read raw bytes
            raw16, // Read raw words
            raw32, // Read raw double-words
            utf8,
            utf16_big_endian,
            utf16_little_endian,
            utf32_big_endian,
            utf32_little_endian,
            utf32_2143_endian,
            utf32_3412_endian
        };

#ifdef CPPDATALIB_ENABLE_FAST_IO
        class istream;
        const char *current_buffer_begin(const istream &stream);

        class istream
        {
        public:
            typedef int64_t streamsize;
            typedef unsigned int iostate;
            typedef int32_t int_type;

        private:
            friend class sentry;

            struct sentry
            {
                sentry(istream &stream, bool skipws = false) : last(0)
                {
                    if (skipws && !stream.flags_)
                        skip_spaces(stream);
                    success = stream;
                }

                void skip_spaces(istream &stream)
                {
                    int_type c;
                    do
                    {
                        c = stream.getc_();
                    } while (c != EOF && c < 0x80 && isspace(c));
                    last = c;
                }

                operator bool() const {return success;}
                int_type get_last_char() const {return last;}

            private:
                bool success;
                int_type last;
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

            // Get one character, bumping position forward, and return it (return EOF if no more characters available)
            virtual int_type getc_() = 0;
            // Unget the last character, bumping position backward. Throw an error if ungetting too far
            virtual void ungetc_() = 0;
            // Peek at the next character, preserving position, and return it (return EOF if no more characters available)
            virtual int_type peekc_() = 0;
            // Read `n` characters, bumping position forward, and place them in `buffer`
            // (return true on success, false on failure, including characters that are too wide for output in a `char`)
            // `eof` should be set to true if and only if an end-of-file was reached during reading
            // `n` should be set to the number of characters actually read
            virtual bool readc_(char *buffer /* out */, size_t &n /* in/out */, bool &eof /* out */) = 0;

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

            virtual const char *current_buffer_begin() const {return nullptr;}
            virtual streamsize used_buffer() const = 0;
            virtual streamsize remaining_buffer() const = 0;

            int_type peek()
            {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = 0;
#endif

                sentry s(*this);
                if (!s)
                    return EOF;

                int_type c = peekc_();
                if (c == EOF)
                    flags_ = fail_bit | eof_bit;
                return c;
            }

            int_type get()
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

                int_type c = getc_();
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

                int_type c = getc_();
                if (c == EOF)
                    flags_ = fail_bit | eof_bit;
                else if (c > 0xff)
                    flags_ = fail_bit;
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
                    int_type c = getc_();
                    if (c == EOF)
                    {
                        flags_ = eof_bit;
                        break;
                    }
                    else if (c > 0xff)
                    {
                        flags_ = fail_bit;
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

                bool eof = false;
                size_t sz = 0;
                if (n < 0 || (uintmax_t(n) > SIZE_MAX) || !readc_(ch, (sz = size_t(n)), eof))
                {
                    if (eof)
                        flags_ = fail_bit | eof_bit;
                    else
                        flags_ = fail_bit;
                }

#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = sz;
#endif

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

                int_type c = s.get_last_char();
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

                int_type c = s.get_last_char();
                if (c == '-')
                {
                    negative = true;
                    c = getc_();
                }

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
                    while (c != EOF && c < 0x80 && isdigit(c))
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
                                if (std::numeric_limits<T>::min() / 10 > i || std::numeric_limits<T>::min() + (c - '0') > i * 10)
                                    out_of_range = true, i = std::numeric_limits<T>::min(), flags_ |= fail_bit;
                                else
                                    i = (i * 10) - (c - '0');
                            }
                            else
                            {
                                if (std::numeric_limits<T>::max() / 10 < i || std::numeric_limits<T>::max() - (c - '0') < i * 10)
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
                    if (streamsize(negative) == last_read_)
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

                int_type c = s.get_last_char();

                if (c == EOF)
                    flags_ = fail_bit | eof_bit;
                else
                {
                    bool out_of_range = false;
                    i = 0;

#ifdef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                    bool read = false;
#endif

                    while (c != EOF && c < 0x80 && isdigit(c))
                    {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                        ++last_read_;
#else
                        read = true;
#endif
                        if (!out_of_range)
                        {
                            if (std::numeric_limits<T>::max() / 10 < i || std::numeric_limits<T>::max() - (c - '0') < i * 10)
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

            template<typename T, typename Converter, typename Alt_Converter>
            istream &read_formatted_real(T &f, Converter convert, Alt_Converter instr_convert)
            {
                std::string str;

#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                last_read_ = 0;
#else
                size_t len = 0;
#endif

                sentry s(*this, skip_ws);
                if (!s)
                {
                    flags_ |= fail_bit;
                    return *this;
                }

                int_type c = s.get_last_char();

                if (c == EOF)
                    flags_ = fail_bit | eof_bit;
                else
                {
                    const char *buffer = current_buffer_begin();
                    if (buffer)
                        --buffer; // The sentry stole one of our buffer characters!

                    while (c != EOF && c < 0x80 && (isdigit(c) || strchr(".eE+-", c)))
                    {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                        ++last_read_;
#else
                        ++len;
#endif
                        if (buffer == nullptr)
                            str.push_back(c);
                        c = getc_();
                    }

                    errno = 0;
                    char *end;
                    if (buffer == nullptr)
                    {
                        f = convert(str.c_str(), &end);
                        if (errno == ERANGE || *end != 0)
                            flags_ |= fail_bit;
                    }
                    else
                    {
#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
                        f = instr_convert(buffer, buffer + last_read_, &end);
#else
                        f = instr_convert(buffer, buffer + len, &end);
#endif
                        if (errno == ERANGE || end == nullptr)
                            flags_ |= fail_bit;
                    }

                    if (c == EOF)
                        flags_ = eof_bit;
                    else
                        ungetc_();
                }

                return *this;
            }
        };

        inline istream &operator>>(istream &in, char &ch) {return in.read_formatted_char(ch);}
        inline istream &operator>>(istream &in, signed char &ch) {return in.read_formatted_char(ch);}
        inline istream &operator>>(istream &in, unsigned char &ch) {return in.read_formatted_char(ch);}

        inline istream &operator>>(istream &in, signed short &val) {return in.read_formatted_signed_int(val);}
        inline istream &operator>>(istream &in, signed int &val) {return in.read_formatted_signed_int(val);}
        inline istream &operator>>(istream &in, signed long &val) {return in.read_formatted_signed_int(val);}
        inline istream &operator>>(istream &in, signed long long &val) {return in.read_formatted_signed_int(val);}

        inline istream &operator>>(istream &in, unsigned short &val) {return in.read_formatted_unsigned_int(val);}
        inline istream &operator>>(istream &in, unsigned int &val) {return in.read_formatted_unsigned_int(val);}
        inline istream &operator>>(istream &in, unsigned long &val) {return in.read_formatted_unsigned_int(val);}
        inline istream &operator>>(istream &in, unsigned long long &val) {return in.read_formatted_unsigned_int(val);}

        inline istream &operator>>(istream &in, float &val) {return in.read_formatted_real(val, &core::fp_from_string<float>, &core::fp_from_in_string<float>);}
        inline istream &operator>>(istream &in, double &val) {return in.read_formatted_real(val, &core::fp_from_string<double>, &core::fp_from_in_string<double>);}
        inline istream &operator>>(istream &in, long double &val) {return in.read_formatted_real(val, &core::fp_from_string<long double>, &core::fp_from_in_string<long double>);}

        inline istream &operator>>(istream &in, std::streambuf *buf)
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

            istream::int_type c = in.getc_();
            while (c != EOF && c <= 0xff)
            {
                ++read;
                buf->sputc(c);
                c = in.getc_();
            }

            in.flags_ |= istream::eof_bit;
            if (c != EOF)
                in.flags_ |= istream::fail_bit;

#ifndef CPPDATALIB_FAST_IO_DISABLE_GCOUNT
            in.last_read_ = read;
#endif

            if (read == 0)
                in.flags_ |= istream::fail_bit;
            return in;
        }

        inline istream &operator>>(istream &in, std::ios_base &(*pf)(std::ios_base &))
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
            std::string my_string;
            size_t pos;

        public:
            istring_wrapper_stream(const std::string &string)
                : string(string)
                , pos(0)
            {}
            istring_wrapper_stream(std::string &&string)
                : string(my_string)
                , my_string(std::move(string))
                , pos(0)
            {}

            const char *current_buffer_begin() const {return string.data() + pos;}
            streamsize used_buffer() const {return pos;}
            streamsize remaining_buffer() const {return string.size() - pos;}

            const std::string &str() const {return string;}

        protected:
            int_type getc_() {return pos < string.size()? string[pos++] & 0xff: EOF;}
            int_type peekc_() {return pos < string.size()? string[pos] & 0xff: EOF;}
            void ungetc_()
            {
                if (pos > 0)
                    --pos;
                else
                    flags_ |= bad_bit;
            }
            bool readc_(char *buffer, size_t &n, bool &eof)
            {
                size_t remaining = string.size() - pos;

                if (remaining < n)
                {
                    // Failure to finish buffer
                    memcpy(buffer, string.data() + pos, remaining);
                    pos = string.size();
                    n = remaining;
                    eof = true;
                    return false;
                }
                else
                {
                    // Success
                    memcpy(buffer, string.data() + pos, n);
                    pos += n;
                    return true;
                }
            }
        };

        class icstring_wrapper_stream : public istream
        {
            const char *string;
            size_t len;
            size_t pos;

        public:
            icstring_wrapper_stream(const char *string)
                : string(string)
                , len(strlen(string))
                , pos(0)
            {}
            icstring_wrapper_stream(const char *string, size_t len)
                : string(string)
                , len(len)
                , pos(0)
            {}

            const char *current_buffer_begin() const {return string + pos;}
            streamsize used_buffer() const {return pos;}
            streamsize remaining_buffer() const {return len - pos;}

            std::string str() const {return std::string(string, len);}

        protected:
            int_type getc_() {return pos < len? string[pos++] & 0xff: EOF;}
            int_type peekc_() {return pos < len? string[pos] & 0xff: EOF;}
            void ungetc_()
            {
                if (pos > 0)
                    --pos;
                else
                    flags_ |= bad_bit;
            }
            bool readc_(char *buffer, size_t &n, bool &eof)
            {
                size_t remaining = len - pos;

                if (remaining < n)
                {
                    // Failure to finish buffer
                    memcpy(buffer, string + pos, remaining);
                    pos = len;
                    n = remaining;
                    eof = true;
                    return false;
                }
                else
                {
                    // Success
                    memcpy(buffer, string + pos, n);
                    pos += n;
                    return true;
                }
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
            istringstream(const char *string)
                : string(string)
                , pos(0)
            {}
            istringstream(const std::string &string)
                : string(string)
                , pos(0)
            {}
            istringstream(std::string &&string)
                : string(std::move(string))
                , pos(0)
            {}

            const char *current_buffer_begin() const {return string.data() + pos;}
            streamsize used_buffer() const {return pos;}
            streamsize remaining_buffer() const {return string.size() - pos;}

            const std::string &str() const {return string;}
            void str(std::string s) {string = std::move(s); pos = 0; flags_ = 0;}

        protected:
            int_type getc_() {return pos < string.size()? string[pos++] & 0xff: EOF;}
            int_type peekc_() {return pos < string.size()? string[pos] & 0xff: EOF;}
            void ungetc_()
            {
                if (pos > 0)
                    --pos;
                else
                    flags_ |= bad_bit;
            }
            bool readc_(char *buffer, size_t &n, bool &eof)
            {
                size_t remaining = string.size() - pos;

                if (remaining < n)
                {
                    // Failure to finish buffer
                    memcpy(buffer, string.data() + pos, remaining);
                    pos = string.size();
                    n = remaining;
                    eof = true;
                    return false;
                }
                else
                {
                    // Success
                    memcpy(buffer, string.data() + pos, n);
                    pos += n;
                    return true;
                }
            }
        };

        class istd_streambuf_wrapper : public istream
        {
            size_t pos_;
            std::streambuf *stream_;

        public:
            istd_streambuf_wrapper(std::streambuf *stream)
                : pos_(0)
                , stream_(stream)
            {}

            streamsize used_buffer() const {return pos_;}
            streamsize remaining_buffer() const {return -1;}

        protected:
            int_type getc_()
            {
                int_type ch = stream_->sbumpc();
                pos_ += ch != EOF;
                return ch;
            }
            int_type peekc_() {return stream_->sgetc();}
            void ungetc_()
            {
                if (stream_->sungetc() == EOF)
                    flags_ |= bad_bit;
                else
                    --pos_;
            }
            bool readc_(char *buffer, size_t &n, bool &eof)
            {
                uint64_t got = stream_->sgetn(buffer, n);

                pos_ += got;

                if (got != n)
                {
                    n = got;
                    eof = true;
                    return false;
                }

                return true;
            }
        };

        class ibufferstream : public istream
        {
            const char *mem_;
            size_t size_, pos_;

        public:
            ibufferstream(const char *buffer, size_t buffer_size)
                : mem_(buffer)
                , size_(buffer_size)
                , pos_(0)
            {}

            const char *current_buffer_begin() const {return mem_ + pos_;}
            streamsize used_buffer() const {return pos_;}
            streamsize remaining_buffer() const {return size_ - pos_;}

            const char *buffer() const {return mem_;}
            size_t buffer_size() const {return size_;}

        protected:
            int_type getc_() {return pos_ == size_? EOF: mem_[pos_++] & 0xff;}
            int_type peekc_() {return pos_ == size_? EOF: mem_[pos_] & 0xff;}
            void ungetc_()
            {
                if (pos_ == 0)
                    flags_ |= bad_bit;
                else
                    --pos_;
            }
            bool readc_(char *buffer, size_t &n, bool &eof)
            {
                size_t remaining = size_ - pos_;

                if (remaining < n)
                {
                    // Failure to finish buffer
                    memcpy(buffer, mem_ + pos_, remaining);
                    pos_ = size_;
                    n = remaining;
                    eof = true;
                    return false;
                }
                else
                {
                    // Success
                    memcpy(buffer, mem_ + pos_, n);
                    pos_ += n;
                    return true;
                }
            }
        };

#ifdef CPPDATALIB_LINUX
        class immapstream : public istream
        {
            ibufferstream stream;

        public:
            immapstream(const char *fname, bool shared_mapping = false)
                : stream(NULL, 0)
            {
                const char *buffer;
                struct stat fdstat;

                int fd = open(fname, O_RDONLY);
                if (fd == -1)
                    goto failure;

                if (fstat(fd, &fdstat) == -1)
                    goto failure;

                buffer = (const char *) mmap(NULL, fdstat.st_size, PROT_READ, shared_mapping? MAP_SHARED: MAP_PRIVATE, fd, 0);
                if (buffer == MAP_FAILED)
                    goto failure;

                {
                    ibufferstream temp(buffer, fdstat.st_size);
                    std::swap(stream, temp);
                }

                return;
            failure:
                throw core::error("cppdatalib::core::immapstream - could not map file");
            }

            ~immapstream()
            {
                munmap((void *) stream.buffer(), stream.buffer_size());
            }

            const char *current_buffer_begin() const {return stream.current_buffer_begin();}
            streamsize used_buffer() const {return stream.used_buffer();}
            streamsize remaining_buffer() const {return stream.remaining_buffer();}

            const char *buffer() const {return stream.buffer();}
            size_t buffer_size() const {return stream.buffer_size();}

        protected:
            int_type getc_() {return stream.get();}
            int_type peekc_() {return stream.peek();}
            void ungetc_() {stream.unget();}
            bool readc_(char *buffer, size_t &n, bool &eof)
            {
                if (!stream.read(buffer, n))
                {
                    eof = stream.eof();
                    return false;
                }

                return true;
            }
        };
#endif
        uint32_t utf_to_ucs(core::istream &stream, encoding mode, bool *eof);
        std::string ucs_to_utf(uint32_t codepoint, encoding mode);
        encoding encoding_from_name(const char *name);

        class iencodingstream : public istream
        {
            core::istream *underlying_stream_;
            int_type last_;
            bool use_buffer_next;
            int_type peek_;
            bool use_peek_next;
            encoding current_encoding_;
            size_t pos_;

        public:
            iencodingstream(core::istream &stream, encoding encoding_type = unknown)
                : underlying_stream_(&stream)
                , last_(0)
                , use_buffer_next(false)
                , peek_(0)
                , use_peek_next(false)
                , current_encoding_(encoding_type)
                , pos_(0)
            {}

            streamsize used_buffer() const {return pos_;}
            streamsize remaining_buffer() const {return -1;}

            encoding get_encoding() const {return current_encoding_;}
            void set_encoding(const char *encoding_name) {current_encoding_ = encoding_from_name(encoding_name);}
            void set_encoding(encoding encoding_type) {current_encoding_ = encoding_type;}

        protected:
            int_type getc_()
            {
                if (use_buffer_next)
                {
                    ++pos_;
                    use_buffer_next = false;
                    return last_;
                }
                else if (use_peek_next)
                {
                    ++pos_;
                    use_peek_next = false;
                    return peek_;
                }

                bool eof;
                uint32_t codepoint = utf_to_ucs(*underlying_stream_, current_encoding_, &eof);

                pos_ += !eof;

                if (eof)
                    return EOF;
                else if (codepoint == uint32_t(-1))
                    return last_ = 0xfffd; // The replacement character

                return last_ = codepoint;
            }

            void ungetc_()
            {
                if (use_buffer_next)
                    flags_ |= fail_bit;
                else
                {
                    use_buffer_next = true;
                    --pos_;
                }
            }

            int_type peekc_()
            {
                if (use_buffer_next)
                    return last_;
                else if (use_peek_next)
                    return peek_;

                bool eof;
                uint32_t codepoint = utf_to_ucs(*underlying_stream_, current_encoding_, &eof);

                if (eof)
                    return EOF;
                else if (codepoint == uint32_t(-1))
                {
                    use_peek_next = true;
                    return peek_ = 0xfffd; // The replacement character
                }

                use_peek_next = true;
                return peek_ = codepoint;
            }

            bool readc_(char *buffer, size_t &n, bool &eof)
            {
                for (size_t i = 0; i < n; ++i)
                {
                    int_type ch = getc_();
                    if (ch == EOF)
                    {
                        n = i;
                        eof = true;
                        return false;
                    }
                    else if (ch > 0xff)
                    {
                        n = i;
                        return false;
                    }

                    buffer[i] = ch;
                }

                return true;
            }
        };

        class istream_handle
        {
            std::istream *std_; // Original standard stream reference
            std::shared_ptr<istream> d_; // Handle to created custom istream

            istream *predef_; // Handle to already-defined custom istream
            istream *p_; // Same as either d_ or predef_, whichever is used

        public:
            istream_handle(core::istream &stream) : std_(NULL), d_(NULL), predef_(&stream), p_(predef_) {}
            istream_handle(std::istream &stream) : std_(&stream), d_(NULL), predef_(NULL)
            {
                d_ = std::make_shared<istd_streambuf_wrapper>(stream.rdbuf());
                p_ = d_.get();
            }
            istream_handle(const char *string) : std_(NULL), d_(NULL)
            {
                d_ = std::make_shared<icstring_wrapper_stream>(string, strlen(string));
                p_ = d_.get();
            }
            istream_handle(const char *string, size_t len) : std_(NULL), d_(NULL)
            {
                d_ = std::make_shared<icstring_wrapper_stream>(string, len);
                p_ = d_.get();
            }
            istream_handle(core::string_view_t string) : std_(NULL), d_(NULL), predef_(NULL)
            {
                d_ = std::make_shared<icstring_wrapper_stream>(string.data(), string.size());
                p_ = d_.get();
            }

            operator core::istream &() {return *p_;}
            core::istream &stream() {return *p_;}

            // std_stream() returns NULL if not created from a standard stream
            std::istream *std_stream() {return std_;}
        };

        const char *current_buffer_begin(const istream &stream) {return stream.current_buffer_begin();}
        istream::streamsize used_buffer(const istream &stream) {return stream.used_buffer();}
        istream::streamsize remaining_buffer(const istream &stream) {return stream.remaining_buffer();}
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
            istream_handle(core::string_view_t string) : std_(NULL), d_(NULL)
            {
                d_ = std::make_shared<istringstream>(static_cast<std::string>(string));
            }

            operator core::istream &() {return std_? *std_: *d_;}
            core::istream &stream() {return std_? *std_: *d_;}

            // std_stream() returns NULL if not created from a standard stream
            std::istream *std_stream() {return std_;}
        };

        const char *current_buffer_begin(const istream &) {return nullptr;}
        std::streamsize used_buffer(istream &stream) {return stream.tellg();}
        std::streamsize remaining_buffer(const istream &) {return -1;}
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
            uint16_t temp = 0;
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
            uint16_t temp = 0;
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
            uint32_t temp = 0;
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
            uint32_t temp = 0;
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
            uint64_t temp = 0;
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

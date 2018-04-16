/*
 * ostream.h
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

#ifndef CPPDATALIB_OSTREAM_H
#define CPPDATALIB_OSTREAM_H

#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <cstring>
#include <memory>

#include "error.h"
#include "global.h"

namespace cppdatalib
{
    namespace core
    {
#ifdef CPPDATALIB_ENABLE_FAST_IO
        // TODO: doesn't support any formatting whatsoever!
        class ostream
        {
        public:
            typedef int64_t streamsize;

            static const unsigned int boolalpha = 1;

            unsigned int fmtflags_;
            streamsize precision_;

        protected:
            virtual void write_(const char *c, size_t n) = 0;
            virtual void putc_(char c) = 0;
            virtual void flush_() {}

        public:
            ostream() : fmtflags_(0), precision_(0) {}

            ostream &put(char c) {putc_(c); return *this;}
            ostream &write(const char *c, streamsize n) {write_(c, n); return *this;}

            friend ostream &operator<<(ostream &out, char ch);
            friend ostream &operator<<(ostream &out, signed char ch);
            friend ostream &operator<<(ostream &out, unsigned char ch);
            friend ostream &operator<<(ostream &out, const char *s);
            friend ostream &operator<<(ostream &out, const std::string &s);

            friend ostream &operator<<(ostream &out, bool val);

            friend ostream &operator<<(ostream &out, signed short val);
            friend ostream &operator<<(ostream &out, signed int val);
            friend ostream &operator<<(ostream &out, signed long val);
            friend ostream &operator<<(ostream &out, signed long long val);

            friend ostream &operator<<(ostream &out, unsigned short val);
            friend ostream &operator<<(ostream &out, unsigned int val);
            friend ostream &operator<<(ostream &out, unsigned long val);
            friend ostream &operator<<(ostream &out, unsigned long long val);

            friend ostream &operator<<(ostream &out, float val);
            friend ostream &operator<<(ostream &out, double val);
            friend ostream &operator<<(ostream &out, long double val);

            friend ostream &operator<<(ostream &out, std::streambuf *buf);

            friend ostream &operator<<(ostream &out, std::ostream &(*pf)(std::ostream &));
            friend ostream &operator<<(ostream &out, std::ios &(*pf)(std::ios &));
            friend ostream &operator<<(ostream &out, std::ios_base &(*pf)(std::ios_base &));

            streamsize precision() const {return precision_;}
            streamsize precision(streamsize prec)
            {
                streamsize temp = precision_;
                precision_ = prec;
                return temp;
            }

        protected:
            ostream &write_formatted_bool(bool b)
            {
                if (fmtflags_ & boolalpha)
                    return write_formatted_string(b? "true": "false");
                else
                    return write_formatted_unsigned_int((unsigned int) b);
            }

            ostream &write_formatted_char(char c) {return put(c);}
            ostream &write_formatted_string(const char *c) {return write(c, strlen(c));}
            ostream &write_formatted_string(const char *c, streamsize n) {return write(c, n);}

            template<typename T>
            ostream &write_formatted_signed_int(T val)
            {
                const int buf_size = std::numeric_limits<T>::digits;
                char buf[buf_size];
                char *p = &buf[buf_size - 1];
                *p = 0;

                if (val < 0)
                {
                    putc_('-');
                    do
                    {
                        *--p = '0' - val % 10;
                        val /= 10;
                    } while (val < 0);
                }
                else
                {
                    do
                    {
                        *--p = '0' + val % 10;
                        val /= 10;
                    } while (val > 0);
                }

                return write_formatted_string(p);
            }

            template<typename T>
            ostream &write_formatted_unsigned_int(T val)
            {
                const int buf_size = std::numeric_limits<T>::digits;
                char buf[buf_size];
                char *p = &buf[buf_size - 1];
                *p = 0;

                do
                {
                    *--p = '0' + val % 10;
                    val /= 10;
                } while (val > 0);

                return write_formatted_string(p);
            }

            template<typename T>
            ostream &write_formatted_real(T val)
            {
                char digits[std::numeric_limits<T>::max_exponent10 + 10];
                char *p = digits;

                if (typeid(T) == typeid(long double))
                    std::snprintf(p, sizeof(digits), "%.*Lg", (int) precision_, (long double) val);
                else
                    std::snprintf(p, sizeof(digits), "%.*g", (int) precision_, (double) val);

                return write_formatted_string(p);
            }
        };

        inline ostream &operator<<(ostream &out, char ch) {return out.write_formatted_char(ch);}
        inline ostream &operator<<(ostream &out, signed char ch) {return out.write_formatted_char(ch);}
        inline ostream &operator<<(ostream &out, unsigned char ch) {return out.write_formatted_char(ch);}

        inline ostream &operator<<(ostream &out, const char *s) {return out.write_formatted_string(s);}
        inline ostream &operator<<(ostream &out, const std::string &s) {return out.write_formatted_string(s.c_str(), s.size());}

        inline ostream &operator<<(ostream &out, bool val) {return out.write_formatted_bool(val);}

        inline ostream &operator<<(ostream &out, signed short val) {return out.write_formatted_signed_int(val);}
        inline ostream &operator<<(ostream &out, signed int val) {return out.write_formatted_signed_int(val);}
        inline ostream &operator<<(ostream &out, signed long val) {return out.write_formatted_signed_int(val);}
        inline ostream &operator<<(ostream &out, signed long long val) {return out.write_formatted_signed_int(val);}

        inline ostream &operator<<(ostream &out, unsigned short val) {return out.write_formatted_unsigned_int(val);}
        inline ostream &operator<<(ostream &out, unsigned int val) {return out.write_formatted_unsigned_int(val);}
        inline ostream &operator<<(ostream &out, unsigned long val) {return out.write_formatted_unsigned_int(val);}
        inline ostream &operator<<(ostream &out, unsigned long long val) {return out.write_formatted_unsigned_int(val);}

        inline ostream &operator<<(ostream &out, float val) {return out.write_formatted_real(val);}
        inline ostream &operator<<(ostream &out, double val) {return out.write_formatted_real(val);}
        inline ostream &operator<<(ostream &out, long double val) {return out.write_formatted_real(val);}

        inline ostream &operator<<(ostream &out, std::streambuf *buf)
        {
            if (!buf)
                return out;

            int c = buf->sbumpc();
            while (c != EOF)
            {
                out.putc_(c);
                c = buf->sbumpc();
            }

            return out;
        }

        inline ostream &operator<<(ostream &out, std::ios &(*pf)(std::ios &))
        {
            (void) pf;
            throw core::error("cppdatalib::core::ostream - stream manipulator applied to output stream is not supported");
            return out;
        }

        inline ostream &operator<<(ostream &out, std::ostream &(*pf)(std::ostream &))
        {
            if (pf == static_cast<std::ostream & (*)(std::ostream &)>(std::endl))
            {
                out.putc_('\n');
                out.flush_();
            }
            else if (pf == static_cast<std::ostream & (*)(std::ostream &)>(std::ends))
                out.putc_(0);
            else if (pf == static_cast<std::ostream & (*)(std::ostream &)>(std::flush))
                out.flush_();
            else
                throw core::error("cppdatalib::core::ostream - stream manipulator applied to output stream is not supported");

            return out;
        }

        inline ostream &operator<<(ostream &out, std::ios_base &(*pf)(std::ios_base &))
        {
            if (pf == std::boolalpha)
                out.fmtflags_ |= ostream::boolalpha;
            else if (pf == std::noboolalpha)
                out.fmtflags_ &= ~ostream::boolalpha;
            else
                throw core::error("cppdatalib::core::ostream - stream manipulator applied to output stream is not supported");

            return out;
        }

        class ostring_wrapper_stream : public ostream
        {
            std::string &string;
            std::unique_ptr<char []> buffer;
            size_t bufpos;

        public:
            ostring_wrapper_stream(std::string &string)
                : string(string)
                , buffer(new char[buffer_size])
                , bufpos(0)
            {}

            const std::string &str() {flush_(); return string;}

        protected:
            void write_(const char *c, size_t n)
            {
                if (n - bufpos >= buffer_size)
                {
                    string.append(buffer.get(), bufpos);
                    bufpos = 0;
                }

                if (n > buffer_size)
                    string.append(c, n);
                else
                {
                    memcpy(buffer.get() + bufpos, c, n);
                    bufpos += n;
                }
            }

            void putc_(char c)
            {
                if (bufpos == buffer_size)
                {
                    string.append(buffer.get(), bufpos);
                    bufpos = 0;
                }
                string.push_back(c);
            }
            void flush_()
            {
                if (bufpos)
                {
                    string.append(buffer.get(), bufpos);
                    bufpos = 0;
                }
            }
        };

        class ostringstream : public ostream
        {
            std::string string;
            std::unique_ptr<char []> buffer;
            size_t bufpos;

        public:
            ostringstream() : buffer(new char[buffer_size]), bufpos(0) {}

            const std::string &str() {flush_(); return string;}

        protected:
            void write_(const char *c, size_t n)
            {
                if (n - bufpos >= buffer_size)
                {
                    string.append(buffer.get(), bufpos);
                    bufpos = 0;
                }

                if (n > buffer_size)
                    string.append(c, n);
                else
                {
                    memcpy(buffer.get() + bufpos, c, n);
                    bufpos += n;
                }
            }

            void putc_(char c)
            {
                if (bufpos == buffer_size)
                {
                    string.append(buffer.get(), bufpos);
                    bufpos = 0;
                }
                buffer[bufpos++] = c;
            }
            void flush_()
            {
                if (bufpos)
                {
                    string.append(buffer.get(), bufpos);
                    bufpos = 0;
                }
            }
        };

        class ostd_streambuf_wrapper : public ostream
        {
            std::streambuf *stream_;

        public:
            ostd_streambuf_wrapper(std::streambuf *stream)
                : stream_(stream)
            {}

        protected:
            void write_(const char *c, size_t n) {stream_->sputn(c, n);}
            void putc_(char c) {stream_->sputc(c);}
            void flush_() {stream_->pubsync();}
        };

        class obufferstream : public ostream
        {
            char *mem_;
            size_t size_;

        public:
            obufferstream(char *buffer, size_t buffer_size)
                : mem_(buffer)
                , size_(buffer_size)
            {}

        protected:
            void write_(const char *c, size_t n)
            {
                if (n > size_)
                    throw core::error("cppdatalib::core::obufferstream - attempt to write past end of buffer");
                memcpy(mem_, c, sizeof(*mem_) * n);
                mem_ += n;
                size_ -= n;
            }
            void putc_(char c)
            {
                if (!size_)
                    throw core::error("cppdatalib::core::obufferstream - attempt to write past end of buffer");
                *mem_++ = c;
                --size_;
            }
            void flush_() {}
        };

        class ostream_handle
        {
            std::ostream *std_;
            std::shared_ptr<ostd_streambuf_wrapper> d_;

            ostream *predef_;

        public:
            ostream_handle(core::ostream &stream) : std_(NULL), d_(NULL), predef_(&stream) {}
            ostream_handle(std::ostream &stream) : std_(&stream), d_(NULL), predef_(NULL)
            {
                d_ = std::make_shared<ostd_streambuf_wrapper>(stream.rdbuf());
            }

            operator core::ostream &() {return predef_? *predef_: *d_;}
            core::ostream &stream() {return predef_? *predef_: *d_;}

            // std_stream() returns NULL if not created from a standard stream
            std::ostream *std_stream() {return std_;}
        };
#else
        typedef std::ostream ostream;
        typedef std::ostringstream ostringstream;

        class ostream_handle
        {
            ostream *d_;

        public:
            ostream_handle(core::ostream &stream) : d_(&stream) {}

            operator core::ostream &() {return *d_;}
            core::ostream &stream() {return *d_;}

            std::ostream *std_stream() {return d_;}
        };
#endif

        template<typename T>
        core::ostream &write_uint8(core::ostream &strm, T val)
        {
            return strm.put(val & 0xff);
        }

        template<typename T>
        core::ostream &write_uint16_be(core::ostream &strm, T val)
        {
            char buf[2];
            buf[0] = uint8_t(val >> 8);
            buf[1] = uint8_t(val);
            return strm.write(buf, 2);
        }

        template<typename T>
        core::ostream &write_uint16_le(core::ostream &strm, T val)
        {
            char buf[2];
            buf[0] = uint8_t(val);
            buf[1] = uint8_t(val >> 8);
            return strm.write(buf, 2);
        }

        template<typename T>
        core::ostream &write_uint32_be(core::ostream &strm, T val)
        {
            char buf[4];
            for (int i = 3; i >= 0; --i, val >>= 8)
                buf[i] = uint8_t(val);
            return strm.write(buf, 4);
        }

        template<typename T>
        core::ostream &write_uint32_le(core::ostream &strm, T val)
        {
            char buf[4];
            for (int i = 0; i < 4; ++i, val >>= 8)
                buf[i] = uint8_t(val);
            return strm.write(buf, 4);
        }

        template<typename T>
        core::ostream &write_uint64_be(core::ostream &strm, T val)
        {
            char buf[8];
            for (int i = 7; i >= 0; --i, val >>= 8)
                buf[i] = uint8_t(val);
            return strm.write(buf, 8);
        }

        template<typename T>
        core::ostream &write_uint64_le(core::ostream &strm, T val)
        {
            char buf[8];
            for (int i = 0; i < 8; ++i, val >>= 8)
                buf[i] = uint8_t(val);
            return strm.write(buf, 8);
        }
    }
}

#endif // CPPDATALIB_OSTREAM_H

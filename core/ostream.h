#ifndef CPPDATALIB_OSTREAM_H
#define CPPDATALIB_OSTREAM_H

#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <cstring>

#include "error.h"
#include "global.h"

namespace cppdatalib
{
    namespace core
    {
#define CPPDATALIB_ENABLE_FAST_IO

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

            // TODO: faster method to write reals than using the STL
            template<typename T>
            ostream &write_formatted_real(T val)
            {
                char digits[std::numeric_limits<T>::max_exponent10 + 10];
                char *p = digits;

                std::snprintf(p, sizeof(digits), "%.*g", (int) precision_, (double) val);

                return write_formatted_string(p);
            }
        };

        template<>
        ostream &ostream::write_formatted_real(long double val)
        {
            char digits[std::numeric_limits<long double>::max_exponent10 + 10];
            char *p = digits;

            std::snprintf(p, sizeof(digits), "%.*Lg", (int) precision_, val);

            return write_formatted_string(p);
        }

        ostream &operator<<(ostream &out, char ch) {return out.write_formatted_char(ch);}
        ostream &operator<<(ostream &out, signed char ch) {return out.write_formatted_char(ch);}
        ostream &operator<<(ostream &out, unsigned char ch) {return out.write_formatted_char(ch);}

        ostream &operator<<(ostream &out, const char *s) {return out.write_formatted_string(s);}
        ostream &operator<<(ostream &out, const std::string &s) {return out.write_formatted_string(s.c_str(), s.size());}

        ostream &operator<<(ostream &out, bool val) {return out.write_formatted_bool(val);}

        ostream &operator<<(ostream &out, signed short val) {return out.write_formatted_signed_int(val);}
        ostream &operator<<(ostream &out, signed int val) {return out.write_formatted_signed_int(val);}
        ostream &operator<<(ostream &out, signed long val) {return out.write_formatted_signed_int(val);}
        ostream &operator<<(ostream &out, signed long long val) {return out.write_formatted_signed_int(val);}

        ostream &operator<<(ostream &out, unsigned short val) {return out.write_formatted_unsigned_int(val);}
        ostream &operator<<(ostream &out, unsigned int val) {return out.write_formatted_unsigned_int(val);}
        ostream &operator<<(ostream &out, unsigned long val) {return out.write_formatted_unsigned_int(val);}
        ostream &operator<<(ostream &out, unsigned long long val) {return out.write_formatted_unsigned_int(val);}

        ostream &operator<<(ostream &out, float val) {return out.write_formatted_real(val);}
        ostream &operator<<(ostream &out, double val) {return out.write_formatted_real(val);}
        ostream &operator<<(ostream &out, long double val) {return out.write_formatted_real(val);}

        ostream &operator<<(ostream &out, std::streambuf *buf)
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

        ostream &operator<<(ostream &out, std::ios &(*pf)(std::ios &))
        {
            (void) pf;
            throw core::error("cppdatalib::core::ostream - stream manipulator applied to output stream is not supported");
            return out;
        }

        ostream &operator<<(ostream &out, std::ostream &(*pf)(std::ostream &))
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

        ostream &operator<<(ostream &out, std::ios_base &(*pf)(std::ios_base &))
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
#else
        typedef core::ostream ostream;
        typedef std::ostringstream ostringstream;
#endif
    }
}

#endif // CPPDATALIB_OSTREAM_H

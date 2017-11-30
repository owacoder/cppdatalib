#ifndef CPPDATALIB_ISTREAM_H
#define CPPDATALIB_ISTREAM_H

#include <iostream>
#include <sstream>
#include <limits>

#include "error.h"

namespace cppdatalib
{
    namespace core
    {
#define CPPDATALIB_ENABLE_FAST_IO

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
                                    out_of_range = true, i = std::numeric_limits<T>::min();
                                else
                                    i = (i * 10) - (c - '0');
                            }
                            else
                            {
                                if (std::numeric_limits<T>::max() / 10 - (c - '0') < i)
                                    out_of_range = true, i = std::numeric_limits<T>::max();
                                else
                                    i = (i * 10) + (c - '0');
                            }
                        }
                        c = getc_();
                    }

                    if (c == EOF)
                        flags_ = eof_bit;
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
                                out_of_range = true, i = std::numeric_limits<T>::max();
                            else
                                i = (i * 10) + (c - '0');
                        }
                        c = getc_();
                    }

                    if (c == EOF)
                        flags_ = eof_bit;
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

#else
        typedef std::istream istream;
        typedef std::istringstream istringstream;
#endif
    }
}

#endif // CPPDATALIB_ISTREAM_H

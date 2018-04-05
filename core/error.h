/*
 * error.h
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

#ifndef CPPDATALIB_ERROR_H
#define CPPDATALIB_ERROR_H

#include <string>

namespace cppdatalib
{
    namespace core
    {
        struct error
        {
        protected:
            error() : what_("") {}

        public:
            error(const char *reason) : what_(reason) {}
            virtual ~error() {}

            virtual const char *what() const {return what_;}

        private:
            const char *what_;
        };

        struct custom_error : public error
        {
            custom_error(const char *reason) : what_(reason) {}
            custom_error(const std::string &reason) : what_(reason) {}
            custom_error(std::string &&reason) : what_(std::move(reason)) {}

            const char *what() const {return what_.c_str();}

        private:
            std::string what_;
        };
    }
}

#endif // CPPDATALIB_ERROR_H

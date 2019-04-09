/*
 * crypto_rand.h
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

#ifndef CPPDATALIB_CRYPTO_RAND_H
#define CPPDATALIB_CRYPTO_RAND_H

#include "../core/value_builder.h"
#include <cmath>
#include <cstdio>

#ifdef CPPDATALIB_WINDOWS
#include <windows.h>
#endif

namespace cppdatalib
{
    namespace crypto_rand
    {
        /* class crypto_rand::blocking_accumulator
         * class crypto_rand::nonblocking_accumulator
         *
         * Obtains cryptographically-secure random 8-bit characters and flushes them to the output,
         *  either blocking until more entropy is available or using a CSPRNG to calculate them
         *
         * This accumulator ignores input
         */
#ifdef CPPDATALIB_LINUX
        class blocking_accumulator : public core::accumulator_base
        {
            FILE *dev_random;

        public:
            blocking_accumulator() : core::accumulator_base() {}
            blocking_accumulator(core::istream_handle handle, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle) {}
            blocking_accumulator(core::ostream_handle handle) : core::accumulator_base(handle) {}
            blocking_accumulator(accumulator_base *handle, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle) {}
            blocking_accumulator(core::stream_handler &handle, bool just_append = false) : core::accumulator_base(handle, just_append) {}

        protected:
            void begin_() {dev_random = fopen("/dev/random", "rb");}
            void end_() {fclose(dev_random);}

            void accumulate_(core::istream::int_type data)
            {
                (void) data;

                if (dev_random)
                {
                    char c;
                    c = fgetc(dev_random);
                    flush_out(&c, 1);
                }
            }
        };

        class nonblocking_accumulator : public core::accumulator_base
        {
            FILE *dev_random;

        public:
            nonblocking_accumulator() : core::accumulator_base() {}
            nonblocking_accumulator(core::istream_handle handle, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle) {}
            nonblocking_accumulator(core::ostream_handle handle) : core::accumulator_base(handle) {}
            nonblocking_accumulator(accumulator_base *handle, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle) {}
            nonblocking_accumulator(core::stream_handler &handle, bool just_append = false) : core::accumulator_base(handle, just_append) {}

        protected:
            void begin_() {dev_random = fopen("/dev/urandom", "rb");}
            void end_() {fclose(dev_random);}

            void accumulate_(core::istream::int_type data)
            {
                (void) data;

                if (dev_random)
                {
                    char c;
                    c = fgetc(dev_random);
                    flush_out(&c, 1);
                }
            }
        };
#elif defined(CPPDATALIB_WINDOWS)
        // See https://docs.microsoft.com/en-us/windows/desktop/api/ntsecapi/nf-ntsecapi-rtlgenrandom
        class blocking_accumulator : public core::accumulator_base
        {
            HMODULE library;
            BOOLEAN (WINAPI *RtlGenRandom)(PVOID buffer, ULONG bufferLength); // MSVC coughs if WINAPI is outside `()`??

        public:
            blocking_accumulator() : core::accumulator_base() {init();}
            blocking_accumulator(core::istream_handle handle, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle) {init();}
            blocking_accumulator(core::ostream_handle handle) : core::accumulator_base(handle) {init();}
            blocking_accumulator(accumulator_base *handle, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle) {init();}
            blocking_accumulator(core::stream_handler &handle, bool just_append = false) : core::accumulator_base(handle, just_append) {init();}
            ~blocking_accumulator() {::FreeLibrary(library);}

        protected:
            void init()
            {
                RtlGenRandom = NULL;

                library = ::LoadLibraryA("advapi32");
                if (!library)
                    return;

                RtlGenRandom = reinterpret_cast<decltype(RtlGenRandom)>(::GetProcAddress(library, "SystemFunction036"));
            }

            void accumulate_(core::istream::int_type data)
            {
                (void) data;
                char c;

                if (RtlGenRandom && RtlGenRandom(&c, 1))
                    flush_out(&c, 1);
            }
        };

        typedef blocking_accumulator nonblocking_accumulator;
#endif
    }
}

#endif // CPPDATALIB_CRYPTO_RAND_H

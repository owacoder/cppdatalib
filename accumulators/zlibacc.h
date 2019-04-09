/*
 * zlib.h
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
 *
 * Disclaimer:
 * Trademarked product names referred to in this file are the property of their
 * respective owners. These trademark owners are not affiliated with the author
 * or copyright holder(s) of this file in any capacity, and do not endorse this
 * software nor the authorship and existence of this file.
 */

#ifndef CPPDATALIB_ZLIB_H
#define CPPDATALIB_ZLIB_H

#ifdef CPPDATALIB_ENABLE_ZLIB

#include "../core/value_builder.h"
#include <zlib.h>

namespace cppdatalib
{
    namespace zlib
    {
        /* class zlib::decode_accumulator
         *
         * Decodes a zlib- or gzip-encoded stream and flushes it to output.
         *
         * Only binary (0x00-0xff) data is accepted
         */
        class decode_accumulator : public core::accumulator_base
        {
            z_stream stream;
            std::vector<char> buf, out_buf;

            static const int default_window_bits = 15;

        public:
            enum accepted_formats
            {
                zlib_only,
                gzip_only,
                raw_deflate,
                zlib_or_gzip
            };

        private:
            accepted_formats format;
            int windowBits;

        public:
            decode_accumulator(accepted_formats format = zlib_or_gzip, int window_bits = default_window_bits) : core::accumulator_base(), format(format), windowBits(window_bits) {}
            decode_accumulator(core::istream_handle handle, accepted_formats format = zlib_or_gzip, int window_bits = default_window_bits, accumulator_base *output_handle = NULL) : core::accumulator_base(handle, output_handle), format(format), windowBits(window_bits) {}
            decode_accumulator(core::ostream_handle handle, accepted_formats format = zlib_or_gzip, int window_bits = default_window_bits) : core::accumulator_base(handle), format(format), windowBits(window_bits) {}
            decode_accumulator(accumulator_base *handle, accepted_formats format = zlib_or_gzip, int window_bits = default_window_bits, bool pull_from_handle = false) : core::accumulator_base(handle, pull_from_handle), format(format), windowBits(window_bits) {}
            decode_accumulator(core::stream_handler &handle, accepted_formats format = zlib_or_gzip, int window_bits = default_window_bits, bool just_append = false) : core::accumulator_base(handle, just_append), format(format), windowBits(window_bits) {}
            ~decode_accumulator() {}

        protected:
            void begin_()
            {
                stream.zalloc = Z_NULL;
                stream.zfree = Z_NULL;
                stream.opaque = Z_NULL;
                stream.avail_in = 0;
                stream.next_in = Z_NULL;

                int windowBitsParam = windowBits & 0xf;

                switch (format)
                {
                    case zlib_only: // No change to windowBits, it's just passed through
                        break;
                    case gzip_only: // Add 16 to windowBits
                        windowBitsParam += 16;
                        break;
                    case raw_deflate: // Negate windowBits
                        windowBitsParam = -windowBitsParam;
                        break;
                    case zlib_or_gzip: // Add 32 to windowBits
                    default:
                        windowBitsParam += 32;
                        break;
                }

                if (inflateInit2(&stream, windowBitsParam) != Z_OK)
                    throw core::error("zlib - cannot initialize inflate stream");
            }
            void end_()
            {
                flush_buffer();
                inflateEnd(&stream);
            }

            void accumulate_(core::istream::int_type data)
            {
                buf.push_back(data);

                if (buf.size() >= core::buffer_size)
                    flush_buffer();
            }

            void flush_buffer()
            {
                out_buf.resize(core::buffer_size);

                try
                {
                    do
                    {
                        stream.avail_in = buf.size();
                        stream.next_in = (unsigned char *) buf.data();

                        stream.avail_out = out_buf.size();
                        stream.next_out = (unsigned char *) out_buf.data();

                        int ret = inflate(&stream, Z_NO_FLUSH);
                        switch (ret)
                        {
                            case Z_NEED_DICT:
                                throw core::error("zlib - cannot decompress with external dictionary");
                            case Z_DATA_ERROR:
                                throw core::error("zlib - data error when decompressing data");
                            case Z_MEM_ERROR:
                                throw core::error("zlib - out of memory when decompressing data");
                        }

                        flush_out(out_buf.data(), out_buf.size() - stream.avail_out);
                    } while (stream.avail_out == 0);
                } catch (...) {
                    inflateEnd(&stream);
                    throw;
                }

                buf.clear();
            }
        };
    }
}

#endif

#endif // CPPDATALIB_ZLIB_H

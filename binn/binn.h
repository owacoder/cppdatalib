/*
 * binn.h
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

#ifndef CPPDATALIB_BINN_H
#define CPPDATALIB_BINN_H

#include "../core/core.h"

// TODO: Refactor into stream_parser API

namespace cppdatalib
{
    namespace binn
    {
        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(std::ostream &output) : core::stream_writer(output) {}

            protected:
                std::ostream &write_type(std::ostream &stream, unsigned int type, unsigned int subtype, int *written = NULL)
                {
                    char c;

                    if (written != NULL)
                        *written = 1 + (subtype > 15);

                    c = (type & 0x7) << 5;
                    if (subtype > 15)
                    {
                        c = (c | 0x10) | ((subtype >> 8) & 0xf);
                        return stream.put(c).put(subtype & 0xff);
                    }

                    c = c | subtype;
                    return stream.put(c);
                }

                std::ostream &write_size(std::ostream &stream, uint64_t size, int *written = NULL)
                {
                    if (written != NULL)
                        *written = 1 + 3 * (size < 128);

                    if (size < 128)
                        return stream.put(size);

                    return stream.put(((size >> 24) & 0xff) | 0x80)
                                 .put((size >> 16) & 0xff)
                                 .put((size >>  8) & 0xff)
                                 .put( size        & 0xff);
                }

                size_t get_size(const core::value &v)
                {
                    switch (v.get_type())
                    {
                        case core::null:
                        case core::boolean: return 1 + (v.get_subtype() >= core::user && v.get_subtype() > 15);
                        case core::integer:
                        {
                            size_t size = 1; // one byte for type specifier

                            if (v.get_subtype() >= core::user && v.get_subtype() > 15)
                                ++size; // type specifier requires another byte

                            if (v.get_int() >= INT8_MIN && v.get_int() <= UINT8_MAX)
                                size += 1;
                            else if (v.get_int() >= INT16_MIN && v.get_int() <= UINT16_MAX)
                                size += 2;
                            else if (v.get_int() >= INT32_MIN && v.get_int() <= UINT32_MAX)
                                size += 4;
                            else
                                size += 8;

                            return size;
                        }
                        case core::uinteger:
                        {
                            size_t size = 1; // one byte for type specifier

                            if (v.get_subtype() >= core::user && v.get_subtype() > 15)
                                ++size; // type specifier requires another byte

                            if (v.get_uint() <= UINT8_MAX)
                                size += 1;
                            else if (v.get_uint() <= UINT16_MAX)
                                size += 2;
                            else if (v.get_uint() <= UINT32_MAX)
                                size += 4;
                            else
                                size += 8;

                            return size;
                        }
                        case core::real:
                        {
                            size_t size = 5; // one byte for type specifier, minimum of four bytes for data

                            // A user-specified subtype is not available for reals
                            // (because when the data is read again, the IEEE-754 representation will be put into an integer instead of a real,
                            // since there is nothing to show that the data should be read as a floating point number)
                            // To prevent the loss of data, the subtype is discarded and the value stays the same

                            if (core::float_from_ieee_754(core::float_to_ieee_754(v.get_real())) != v.get_real() && !std::isnan(v.get_real()))
                                size += 4; // requires more than 32-bit float to losslessly encode

                            return size;
                        }
                        case core::string:
                        {
                            size_t size = 3; // one byte for the type specifier, one for the minimum size specifier of one byte, one for trailing nul

                            if (v.get_subtype() >= core::user && v.get_subtype() > 15)
                                ++size; // type specifier requires another byte

                            if (size + v.size() >= 128)
                                size += 3; // requires a four-byte size specifier

                            return size + v.get_string().size();
                        }
                        case core::array:
                        {
                            size_t size = 3; // one byte for the type specifier,
                                             // one for the minimum size specifier of one byte,
                                             // and one for the minimum count specifier of one byte

                            if (v.get_subtype() >= core::user && v.get_subtype() > 15)
                                ++size; // type specifier requires another byte

                            if (v.size() >= 128)
                                size += 3; // requires a four-byte count specifier

                            for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                                size += get_size(*it);

                            if (size >= 128)
                                size += 3; // requires a four-byte size specifier

                            return size;
                        }
                        case core::object:
                        {
                            size_t size = 3; // one byte for the type specifier,
                                             // one for the minimum size specifier of one byte,
                                             // and one for the minimum count specifier of one byte

                            // A user-specified subtype is not available for objects
                            // (because when the data is read again, there is no way to determine the type of structure the container holds)
                            // To prevent the loss of data, the subtype is discarded and the value stays the same

                            if (v.size() >= 128)
                                size += 3; // requires a four-byte count specifier

                            if (v.get_subtype() == core::map)
                            {
                                for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                                    size += 4 /* map ID */ + get_size(it->second) /* value size */;
                            }
                            else
                            {
                                for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                                    size += 1 /* key size specifier */ + it->first.size() /* key size */ + get_size(it->second) /* value size */;
                            }

                            if (size >= 128)
                                size += 3; // requires a four-byte size specifier

                            return size;
                        }
                    }

                    // Control will never get here
                    return 0;
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
            std::stack<core::subtype_t, std::vector<core::subtype_t>> object_types;

        public:
            stream_writer(std::ostream &output) : impl::stream_writer_base(output) {}

            bool requires_prefix_string_size() const {return true;}
            bool requires_array_buffering() const {return true;}
            bool requires_object_buffering() const {return true;}

        protected:
            enum types
            {
                nobytes,
                byte,
                word,
                dword,
                qword,
                string,
                blob,
                container
            };

            enum subtypes
            {
                null = 0,
                yes, // true
                no, // false

                uint8 = 0,
                int8,

                uint16 = 0,
                int16,

                uint32 = 0,
                int32,
                single_float,

                uint64 = 0,
                int64,
                double_float,

                text = 0,
                datetime,
                date,
                time,
                decimal_str,

                blob_data = 0,

                list = 0,
                map,
                object
            };

            void begin_() {object_types = decltype(object_types)();}

            void begin_key_(const core::value &v)
            {
                if (object_types.top() == core::map)
                {
                    if (!v.is_int() && !v.is_uint())
                        throw core::error("Binn - map key is not an integer");
                    else if ((v.is_int() && (v.get_int() < INT32_MIN || v.get_int() > INT32_MAX)) ||
                             (v.is_uint() && v.get_uint() > INT32_MAX))
                        throw core::error("Binn - map key is out of range");
                }
                else if (!v.is_string())
                    throw core::error("Binn - object key is not a string");
            }

            void begin_scalar_(const core::value &v, bool is_key)
            {
                if (is_key && (v.is_int() || v.is_uint()))
                {
                    /* Bounds of the provided key were already checked in begin_key_() */

                    int64_t key = v.is_int()? v.get_int(): static_cast<int64_t>(v.get_uint());
                    uint64_t out;

                    out = std::abs(key);
                    if (key < 0)
                        out = ~out + 1;
                    out &= INT32_MAX;

                    output_stream.put(out >> 24)
                                 .put((out >> 16) & 0xff)
                                 .put((out >>  8) & 0xff)
                                 .put(out & 0xff);
                    return;
                }
                else if (v.is_int())
                {
                    core::int_t i = v.get_int();
                    uint64_t out;

                    if (i < -std::numeric_limits<core::int_t>::max())
                    {
                        i = uint64_t(INT32_MAX) + 1; // this assignment ensures that the 64-bit write will occur, since the most negative
                                                     // value is (in binary) 1000...0000
                        out = 0; // The initial set bit is implied, and ORed in with `0x80 | xxx` later on
                    }
                    else
                    {
                        i = -i; // Make i positive
                        out = ~uint64_t(i) + 1;
                    }

                    if (v.get_int() >= INT8_MIN && v.get_int() <= UINT8_MAX)
                        write_type(output_stream, byte, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): int8)
                                .put(out);
                    else if (v.get_int() >= INT16_MIN && v.get_int() <= UINT16_MAX)
                        write_type(output_stream, word, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): int16)
                                .put(out >> 8)
                                .put(out & 0xff);
                    else if (v.get_int() >= INT32_MIN && v.get_int() <= UINT32_MAX)
                        write_type(output_stream, dword, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): int32)
                                .put(out >> 24)
                                .put((out >> 16) & 0xff)
                                .put((out >> 8) & 0xff)
                                .put(out & 0xff);
                    else
                        write_type(output_stream, qword, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): int64)
                                .put(out >> 56)
                                .put((out >> 48) & 0xff)
                                .put((out >> 40) & 0xff)
                                .put((out >> 32) & 0xff)
                                .put((out >> 24) & 0xff)
                                .put((out >> 16) & 0xff)
                                .put((out >> 8) & 0xff)
                                .put(out & 0xff);
                }
                else if (v.is_uint())
                {
                    uint64_t out = v.get_uint();

                    if (v.get_uint() <= UINT8_MAX)
                        write_type(output_stream, byte, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): uint8)
                                .put(out);
                    else if (v.get_int() <= UINT16_MAX)
                        write_type(output_stream, word, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): uint16)
                                .put(out >> 8)
                                .put(out & 0xff);
                    else if (v.get_int() <= UINT32_MAX)
                        write_type(output_stream, dword, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): uint32)
                                .put(out >> 24)
                                .put((out >> 16) & 0xff)
                                .put((out >> 8) & 0xff)
                                .put(out & 0xff);
                    else
                        write_type(output_stream, qword, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): uint64)
                                .put(out >> 56)
                                .put((out >> 48) & 0xff)
                                .put((out >> 40) & 0xff)
                                .put((out >> 32) & 0xff)
                                .put((out >> 24) & 0xff)
                                .put((out >> 16) & 0xff)
                                .put((out >> 8) & 0xff)
                                .put(out & 0xff);
                }
            }

            void null_(const core::value &v) {write_type(output_stream, nobytes, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): null);}
            void bool_(const core::value &v) {write_type(output_stream, nobytes, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): v.get_bool()? yes: no);}

            /* Integers and UIntegers are handled in begin_scalar_() */

            void real_(const core::value &v)
            {
                uint64_t out;

                if (core::float_from_ieee_754(core::float_to_ieee_754(v.get_real())) == v.get_real() || std::isnan(v.get_real()))
                {
                    out = core::float_to_ieee_754(v.get_real());
                    write_type(output_stream, dword, single_float)
                            .put(out >> 24)
                            .put((out >> 16) & 0xff)
                            .put((out >> 8) & 0xff)
                            .put(out & 0xff);
                }
                else
                {
                    out = core::double_to_ieee_754(v.get_real());
                    write_type(output_stream, qword, double_float)
                            .put(out >> 56)
                            .put((out >> 48) & 0xff)
                            .put((out >> 40) & 0xff)
                            .put((out >> 32) & 0xff)
                            .put((out >> 24) & 0xff)
                            .put((out >> 16) & 0xff)
                            .put((out >> 8) & 0xff)
                            .put(out & 0xff);
                }
            }

            void begin_string_(const core::value &v, core::int_t size, bool is_key)
            {
                if (size == unknown_size)
                    throw core::error("Binn - 'string' value does not have size specified");

                if (is_key)
                {
                    if (size > 255)
                        throw core::error("Binn - object key is larger than limit of 255 bytes");

                    output_stream.put(size);
                    return;
                }

                switch (v.get_subtype())
                {
                    case core::date: write_type(output_stream, string, date); break;
                    case core::time: write_type(output_stream, string, time); break;
                    case core::datetime: write_type(output_stream, string, datetime); break;
                    case core::bignum: write_type(output_stream, string, decimal_str); break;
                    case core::blob: write_type(output_stream, blob, blob_data); break;
                    default: write_type(output_stream, string, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): text); break;
                }

                write_size(output_stream, size);
            }
            void string_data_(const core::value &v, bool) {output_stream.write(v.get_string().c_str(), v.get_string().size());}

            void begin_array_(const core::value &v, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("Binn - 'array' value does not have size specified");
                else if (v.size() != static_cast<size_t>(size))
                    throw core::error("Binn - entire 'array' value must be buffered before writing");

                write_type(output_stream, container, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): list);
                write_size(output_stream, get_size(v));
                write_size(output_stream, size);
            }

            void begin_object_(const core::value &v, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("Binn - 'object' value does not have size specified");
                else if (v.size() != static_cast<size_t>(size))
                    throw core::error("Binn - entire 'object' value must be buffered before writing");

                write_type(output_stream, container, v.get_subtype() == core::map? map: object);
                write_size(output_stream, get_size(v));
                write_size(output_stream, size);

                object_types.push(v.get_subtype());
            }
            void end_object_(const core::value &, bool) {object_types.pop();}
        };

        /*inline core::value from_binn(const std::string &bencode)
        {
            std::istringstream stream(bencode);
            parser p(stream);
            core::value v;
            p >> v;
            return v;
        }*/

        inline std::string to_binn(const core::value &v)
        {
            std::ostringstream stream;
            stream_writer w(stream);
            w << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_BINN_H

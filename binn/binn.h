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

namespace cppdatalib
{
    namespace binn
    {
        enum type
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

        enum subtype
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

        class parser : public core::stream_parser
        {
            uint32_t read_size(std::istream &input)
            {
                uint32_t size = 0;
                int chr = input.get();
                if (chr == EOF)
                    throw core::error("Binn - expected size specifier");

                size = chr;
                if (chr >> 7) // If topmost bit is set, the size is specified in 4 bytes, not 1. The topmost bit is not included in the size
                {
                    size &= 0x7f;
                    for (int i = 0; i < 3; ++i)
                    {
                        chr = input.get();
                        if (chr == EOF)
                            throw core::error("Binn - expected size specifier");
                        size <<= 8;
                        size |= chr;
                    }
                }

                return size;
            }

        public:
            parser(std::istream &input) : core::stream_parser(input) {}

            bool provides_prefix_string_size() const {return true;}
            bool provides_prefix_object_size() const {return true;}
            bool provides_prefix_array_size() const {return true;}

            core::stream_parser &convert(core::stream_handler &writer)
            {
                struct container_data
                {
                    container_data(subtype container_type, uint32_t remaining_size)
                        : container_type(container_type)
                        , remaining_size(remaining_size)
                    {}

                    subtype container_type;
                    uint32_t remaining_size;
                };

                std::stack<container_data, std::vector<container_data>> containers;

                char buffer[core::buffer_size];
                bool written = false;
                int chr;
                core::uint_t integer;

                writer.begin();

                while (!written || containers.size() > 0)
                {
                    while (containers.size() > 0 && !writer.container_key_was_just_parsed() && containers.top().remaining_size == 0)
                    {
                        if (writer.current_container() == core::array)
                            writer.end_array(core::value(core::array_t(), containers.top().container_type));
                        else if (writer.current_container() == core::object)
                            writer.end_object(core::value(core::object_t(), containers.top().container_type));
                        containers.pop();
                    }

                    if (containers.size() > 0)
                    {
                        if (containers.top().remaining_size > 0)
                            --containers.top().remaining_size;

                        if (writer.current_container() == core::object && !writer.container_key_was_just_parsed())
                        {
                            // Parse keys here
                            if (containers.top().container_type == map) // Integer keys
                            {
                                // Read 4-byte signed integer key
                                integer = 0;
                                for (int i = 0; i < 4; ++i)
                                {
                                    chr = input_stream.get();
                                    if (chr == EOF)
                                        throw core::error("Binn - expected map key");
                                    integer = (integer << 8) | chr;
                                }

                                bool negative = integer >> 31;
                                if (negative)
                                    integer = (~integer + 1) & INT32_MAX;
                                writer.write(negative? -core::int_t(integer): core::int_t(integer));
                            }
                            else // String keys
                            {
                                chr = input_stream.get();
                                if (chr == EOF)
                                    throw core::error("Binn - expected object key");

                                char key_buffer[255]; // 255 is maximum length of key
                                input_stream.read(key_buffer, chr);
                                if (input_stream.fail())
                                    throw core::error("Binn - unexpected end of object key");

                                writer.write(core::string_t(key_buffer, chr));
                            }
                        }
                    }
                    else if (written)
                        break;

                    chr = input_stream.get();
                    if (chr == EOF)
                        throw core::error("Binn - expected type specifier");

                    const type storage_type = static_cast<type>(chr >> 5);
                    int element_subtype = chr & 0xf;

                    if (chr & 0x10)
                    {
                        int chr_2 = input_stream.get();
                        if (chr_2 == EOF)
                            throw core::error("Binn - expected subtype extension");

                        element_subtype <<= 8;
                        element_subtype |= chr_2;
                    }

                    switch (storage_type)
                    {
                        case nobytes: // Null, True, False
                            switch (element_subtype)
                            {
                                case null: writer.write(core::null_t()); break;
                                case yes: writer.write(true); break;
                                case no: writer.write(false); break;
                                default: writer.write(core::value(core::null_t(), element_subtype)); break;
                            }
                            break;
                        case byte: // Int8, UInt8
                            chr = input_stream.get();
                            if (chr == EOF)
                                throw core::error("Binn - expected byte value");

                            switch (element_subtype)
                            {
                                case int8:
                                {
                                    bool negative = chr >> 7;
                                    if (negative)
                                        chr = (~core::uint_t(chr) + 1) & INT8_MAX;
                                    writer.write(negative? -chr: chr);
                                    break;
                                }
                                case uint8: writer.write(core::uint_t(chr)); break;
                                default: writer.write(core::value(core::uint_t(chr), element_subtype)); break;
                            }
                            break;
                        case word: // Int16, UInt16
                            integer = 0;
                            for (int i = 0; i < 2; ++i)
                            {
                                chr = input_stream.get();
                                if (chr == EOF)
                                    throw core::error("Binn - expected word value");
                                integer = (integer << 8) | chr;
                            }

                            switch (element_subtype)
                            {
                                case int16:
                                {
                                    bool negative = integer >> 15;
                                    if (negative)
                                        integer = (~integer + 1) & INT16_MAX;
                                    writer.write(negative? -core::int_t(integer): core::int_t(integer));
                                    break;
                                }
                                case uint16: writer.write(integer); break;
                                default: writer.write(core::value(integer, element_subtype)); break;
                            }
                            break;
                        case dword: // Int32, UInt32
                            integer = 0;
                            for (int i = 0; i < 4; ++i)
                            {
                                chr = input_stream.get();
                                if (chr == EOF)
                                    throw core::error("Binn - expected double-word value");
                                integer = (integer << 8) | chr;
                            }

                            switch (element_subtype)
                            {
                                case int32:
                                {
                                    bool negative = integer >> 31;
                                    if (negative)
                                        integer = (~integer + 1) & INT32_MAX;
                                    writer.write(negative? -core::int_t(integer): core::int_t(integer));
                                    break;
                                }
                                case uint32: writer.write(integer); break;
                                case single_float: writer.write(static_cast<core::real_t>(core::float_from_ieee_754(integer))); break;
                                default: writer.write(core::value(integer, element_subtype)); break;
                            }
                            break;
                        case qword: // Int64, UInt64
                            integer = 0;
                            for (int i = 0; i < 8; ++i)
                            {
                                chr = input_stream.get();
                                if (chr == EOF)
                                    throw core::error("Binn - expected quad-word value");
                                integer = (integer << 8) | chr;
                            }

                            switch (element_subtype)
                            {
                                case int64:
                                {
                                    bool negative = integer >> 63;
                                    if (negative)
                                        integer = ~integer + 1;
                                    writer.write(negative? -core::int_t(integer): core::int_t(integer));
                                    break;
                                }
                                case uint64: writer.write(integer); break;
                                case double_float: writer.write(core::double_from_ieee_754(integer)); break;
                                default: writer.write(core::value(integer, element_subtype)); break;
                            }
                            break;
                        case string:
                        {
                            uint32_t size = read_size(input_stream);
                            core::value string_type = "";

                            switch (element_subtype)
                            {
                                case text: break;
                                case datetime: string_type.set_subtype(core::datetime); break;
                                case date: string_type.set_subtype(core::date); break;
                                case time: string_type.set_subtype(core::time); break;
                                case decimal_str: string_type.set_subtype(core::bignum); break;
                                default: string_type.set_subtype(element_subtype); break;
                            }

                            writer.begin_string(string_type, size);
                            while (size > 0)
                            {
                                core::int_t buffer_size = std::min(uint32_t(core::buffer_size), size);
                                input_stream.read(buffer, buffer_size);
                                if (input_stream.fail())
                                    throw core::error("Binn - unexpected end of string");
                                // Set string in string_type to preserve the subtype
                                string_type.set_string(core::string_t(buffer, buffer_size));
                                writer.append_to_string(string_type);
                                size -= buffer_size;
                            }
                            string_type.set_string("");
                            writer.end_string(string_type);

                            if (input_stream.get() != 0) // Eat trailing NUL
                                throw core::error("Binn - unexpected end of string");

                            break;
                        }
                        case blob:
                        {
                            uint32_t size = read_size(input_stream);
                            core::value string_type = "";

                            switch (element_subtype)
                            {
                                case blob_data: string_type.set_subtype(core::blob); break;
                                default: string_type.set_subtype(element_subtype); break;
                            }

                            writer.begin_string(string_type, size);
                            while (size > 0)
                            {
                                core::int_t buffer_size = std::min(uint32_t(core::buffer_size), size);
                                input_stream.read(buffer, buffer_size);
                                if (input_stream.fail())
                                    throw core::error("Binn - unexpected end of string");
                                // Set string in string_type to preserve the subtype
                                string_type.set_string(core::string_t(buffer, buffer_size));
                                writer.append_to_string(string_type);
                                size -= buffer_size;
                            }
                            string_type.set_string("");
                            writer.end_string(string_type);

                            break;
                        }
                        case container:
                        {
                            read_size(input_stream); // Read size and discard
                            uint32_t count = read_size(input_stream); // Then read element count

                            core::value container;

                            switch (element_subtype)
                            {
                                case map: container.set_object(core::object_t(), core::map); break;
                                case object: container.set_object(core::object_t()); break;
                                case list: container.set_array(core::array_t()); break;
                                default: container.set_array(core::array_t(), element_subtype); break;
                            }

                            if (container.is_object())
                                writer.begin_object(container, count);
                            else
                                writer.begin_array(container, count);
                            containers.push(container_data(static_cast<subtype>(element_subtype), count));
                            break;
                        }
                    }

                    written = true;
                }

                if (!written)
                    throw core::error("Bencode - expected value");

                writer.end();
                return *this;
            }
        };

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

                    if (subtype > 0xfff)
                        throw core::error("Binn - subtype is greater than 4096, cannot write element");

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
                    if (size > INT32_MAX)
                        throw core::error("Binn - size is greater than 2 GB, cannot write element");

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
                    struct traverser
                    {
                    private:
                        std::stack<size_t, std::vector<size_t>> size;

                    public:
                        traverser() {size.push(0);}

                        size_t get_size() const {return size.top();}

                        bool operator()(const core::value *arg, bool prefix)
                        {
                            switch (arg->get_type())
                            {
                                case core::null:
                                case core::boolean:
                                    if (prefix)
                                        size.top() += 1 + (arg->get_subtype() >= core::user && arg->get_subtype() > 15);
                                    break;
                                case core::integer:
                                {
                                    if (prefix)
                                    {
                                        size.top() += 1; // one byte for type specifier

                                        if (arg->get_subtype() >= core::user && arg->get_subtype() > 15)
                                            ++size.top(); // type specifier requires another byte

                                        if (arg->get_int() >= INT8_MIN && arg->get_int() <= UINT8_MAX)
                                            size.top() += 1;
                                        else if (arg->get_int() >= INT16_MIN && arg->get_int() <= UINT16_MAX)
                                            size.top() += 2;
                                        else if (arg->get_int() >= INT32_MIN && arg->get_int() <= UINT32_MAX)
                                            size.top() += 4;
                                        else
                                            size.top() += 8;
                                    }

                                    break;
                                }
                                case core::uinteger:
                                {
                                    if (prefix)
                                    {
                                        size.top() += 1; // one byte for type specifier

                                        if (arg->get_subtype() >= core::user && arg->get_subtype() > 15)
                                            ++size.top(); // type specifier requires another byte

                                        if (arg->get_uint() <= UINT8_MAX)
                                            size.top() += 1;
                                        else if (arg->get_uint() <= UINT16_MAX)
                                            size.top() += 2;
                                        else if (arg->get_uint() <= UINT32_MAX)
                                            size.top() += 4;
                                        else
                                            size.top() += 8;
                                    }

                                    break;
                                }
                                case core::real:
                                {
                                    if (prefix)
                                    {
                                        size.top() += 5; // one byte for type specifier, minimum of four bytes for data

                                        // A user-specified subtype is not available for reals
                                        // (because when the data is read again, the IEEE-754 representation will be put into an integer instead of a real,
                                        // since there is nothing to show that the data should be read as a floating point number)
                                        // To prevent the loss of data, the subtype is discarded and the value stays the same

                                        if (core::float_from_ieee_754(core::float_to_ieee_754(arg->get_real())) != arg->get_real() && !std::isnan(arg->get_real()))
                                            size.top() += 4; // requires more than 32-bit float to losslessly encode
                                    }

                                    break;
                                }
                                case core::string:
                                {
                                    if (prefix)
                                    {
                                        size.top() += 3; // one byte for the type specifier, one for the minimum size specifier of one byte, one for trailing nul

                                        if (arg->get_subtype() >= core::user && arg->get_subtype() > 15)
                                            ++size.top(); // type specifier requires another byte

                                        if (size.top() + arg->size() >= 128)
                                            size.top() += 3; // requires a four-byte size specifier

                                        size.top() += arg->get_string().size();
                                    }
                                    break;
                                }
                                case core::array:
                                {
                                    if (prefix)
                                    {
                                        size.push(3); // one byte for the type specifier,
                                                      // one for the minimum size specifier of one byte,
                                                      // and one for the minimum count specifier of one byte

                                        if (arg->get_subtype() >= core::user && arg->get_subtype() > 15)
                                            ++size.top(); // type specifier requires another byte

                                        if (arg->size() >= 128)
                                            size.top() += 3; // requires a four-byte count specifier
                                    }
                                    else
                                    {
                                        if (size.top() >= 128)
                                            size.top() += 3; // requires a four-byte size specifier

                                        size_t temp = size.top();
                                        size.pop();
                                        size.top() += temp;
                                    }

                                    break;
                                }
                                case core::object:
                                {
                                    if (prefix)
                                    {
                                        size.push(3); // one byte for the type specifier,
                                                      // one for the minimum size specifier of one byte,
                                                      // and one for the minimum count specifier of one byte

                                        // A user-specified subtype is not available for objects
                                        // (because when the data is read again, there is no way to determine the type of structure the container holds)
                                        // To prevent the loss of data, the subtype is discarded and the value stays the same

                                        if (arg->size() >= 128)
                                            size.top() += 3; // requires a four-byte count specifier

                                        // Obtain sizes of keys (ignore values for now, since they'll be added in later invocations)
                                        if (arg->get_subtype() == core::map)
                                            size.top() += 4 * arg->get_object().size();
                                        else
                                        {
                                            for (auto it = arg->get_object().begin(); it != arg->get_object().end(); ++it)
                                                size.top() += 1 /* key size specifier */ + it->first.size() /* key size */;
                                        }
                                    }
                                    else
                                    {
                                        if (size.top() >= 128)
                                            size.top() += 3; // requires a four-byte size specifier

                                        size_t temp = size.top();
                                        size.pop();
                                        size.top() += temp;
                                    }

                                    break;
                                }
                            }

                            return true;
                        }
                    };

                    traverser t;

                    v.value_traverse(t);

                    return t.get_size();
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

                    if (v.get_int() >= INT8_MIN && v.get_int() <= INT8_MAX)
                        write_type(output_stream, byte, v.get_subtype() >= core::user? static_cast<subtype>(v.get_subtype()): int8)
                                .put(out);
                    else if (v.get_int() >= INT16_MIN && v.get_int() <= INT16_MAX)
                        write_type(output_stream, word, v.get_subtype() >= core::user? static_cast<subtype>(v.get_subtype()): int16)
                                .put(out >> 8)
                                .put(out & 0xff);
                    else if (v.get_int() >= INT32_MIN && v.get_int() <= INT32_MAX)
                        write_type(output_stream, dword, v.get_subtype() >= core::user? static_cast<subtype>(v.get_subtype()): int32)
                                .put(out >> 24)
                                .put((out >> 16) & 0xff)
                                .put((out >> 8) & 0xff)
                                .put(out & 0xff);
                    else
                        write_type(output_stream, qword, v.get_subtype() >= core::user? static_cast<subtype>(v.get_subtype()): int64)
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
                        write_type(output_stream, byte, v.get_subtype() >= core::user? static_cast<subtype>(v.get_subtype()): uint8)
                                .put(out);
                    else if (v.get_uint() <= UINT16_MAX)
                        write_type(output_stream, word, v.get_subtype() >= core::user? static_cast<subtype>(v.get_subtype()): uint16)
                                .put(out >> 8)
                                .put(out & 0xff);
                    else if (v.get_uint() <= UINT32_MAX)
                        write_type(output_stream, dword, v.get_subtype() >= core::user? static_cast<subtype>(v.get_subtype()): uint32)
                                .put(out >> 24)
                                .put((out >> 16) & 0xff)
                                .put((out >> 8) & 0xff)
                                .put(out & 0xff);
                    else
                        write_type(output_stream, qword, v.get_subtype() >= core::user? static_cast<subtype>(v.get_subtype()): uint64)
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

            void null_(const core::value &v) {write_type(output_stream, nobytes, v.get_subtype() >= core::user? static_cast<subtype>(v.get_subtype()): null);}
            void bool_(const core::value &v) {write_type(output_stream, nobytes, v.get_subtype() >= core::user? static_cast<subtype>(v.get_subtype()): v.get_bool()? yes: no);}

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
                    case core::blob:
                    case core::clob: write_type(output_stream, blob, blob_data); break;
                    default: write_type(output_stream, string, v.get_subtype() >= core::user? static_cast<subtype>(v.get_subtype()): text); break;
                }

                write_size(output_stream, size);
            }
            void string_data_(const core::value &v, bool) {output_stream.write(v.get_string().c_str(), v.get_string().size());}
            void end_string_(const core::value &v, bool is_key) {if (v.get_subtype() != core::blob && v.get_subtype() != core::clob && !is_key) output_stream.put(0);}

            void begin_array_(const core::value &v, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("Binn - 'array' value does not have size specified");
                else if (v.size() != static_cast<size_t>(size))
                    throw core::error("Binn - entire 'array' value must be buffered before writing");

                write_type(output_stream, container, v.get_subtype() >= core::user? static_cast<subtype>(v.get_subtype()): list);
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

        inline core::value from_binn(const std::string &bencode)
        {
            std::istringstream stream(bencode);
            parser p(stream);
            core::value v;
            p >> v;
            return v;
        }

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

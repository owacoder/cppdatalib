/*
 * message_pack.h
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

#ifndef CPPDATALIB_MESSAGE_PACK_H
#define CPPDATALIB_MESSAGE_PACK_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace message_pack
    {
        class parser : public core::stream_parser
        {
            struct container_data
            {
                container_data(core::subtype_t sub_type, uint32_t remaining_size)
                    : sub_type(sub_type)
                    , remaining_size(remaining_size)
                {}

                core::subtype_t sub_type;
                uint32_t remaining_size;
            };

            std::unique_ptr<char []> buffer;
            std::stack<container_data, std::vector<container_data>> containers;
            bool written;

            uint32_t read_size(core::istream &input)
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
            parser(core::istream_handle input)
                : core::stream_parser(input)
                , buffer(new char [core::buffer_size])
            {
                reset();
            }

            unsigned int features() const {return provides_prefix_array_size |
                                                  provides_prefix_object_size |
                                                  provides_prefix_string_size;}

        protected:
            void reset_()
            {
                containers = decltype(containers)();
                written = false;
            }

            void write_one_()
            {
                int chr;

                while (containers.size() > 0 && !get_output()->container_key_was_just_parsed() && containers.top().remaining_size == 0)
                {
                    if (get_output()->current_container() == core::array)
                        get_output()->end_array(core::value(core::array_t(), containers.top().sub_type));
                    else if (get_output()->current_container() == core::object)
                        get_output()->end_object(core::value(core::object_t(), containers.top().sub_type));
                    containers.pop();
                }

                if (containers.size() > 0)
                {
                    if (containers.top().remaining_size > 0 &&
                            (get_output()->current_container() != core::object || get_output()->container_key_was_just_parsed()))
                        --containers.top().remaining_size;
                }
                else if (written)
                {
                    written = false;
                    return;
                }

                chr = stream().get();
                if (chr == EOF)
                    throw core::error("MessagePack - unexpected end of stream, expected type specifier");

                if (chr < 0x80) // Positive fixint
                    get_output()->write(core::int_t(chr));
                else if (chr < 0x90) // Fixmap
                {
                    chr &= 0xf;
                    get_output()->begin_object(core::object_t(), chr);
                    containers.push(container_data(core::normal, chr));
                }
                else if (chr < 0xa0) // Fixarray
                {
                    chr &= 0xf;
                    get_output()->begin_array(core::array_t(), chr);
                    containers.push(container_data(core::normal, chr));
                }
                else if (chr < 0xc0) // Fixstr
                {
                    char buf[32]; // Maximum of 32-byte string
                    chr &= 0x1f;

                    stream().read(buf, chr);
                    if (stream().fail())
                        throw core::error("MessagePack - unexpected end of string");

                    get_output()->write(core::string_t(buf, chr));
                }
                else if (chr >= 0xe0) // Negative fixint
                    get_output()->write(-core::int_t((~unsigned(chr) + 1) & 0xff));
                else switch (chr)
                {
                    // Null
                    case 0xc0: get_output()->write(core::null_t()); break;
                    // Boolean false
                    case 0xc2: get_output()->write(false); break;
                    // Boolean true
                    case 0xc3: get_output()->write(true); break;
                    // Binary
                    case 0xc4:
                    case 0xc5:
                    case 0xc6:
                    {
                        uint32_t size;
                        core::value string_type = "";

                        if ((chr == 0xc4 && !core::read_uint8(stream(), size)) ||
                            (chr == 0xc5 && !core::read_uint16_be(stream(), size)) ||
                            (chr == 0xc6 && !core::read_uint32_be(stream(), size)))
                            throw core::error("MessagePack - expected 'binary data' length");

                        string_type.set_subtype(core::blob);
                        get_output()->begin_string(string_type, size);
                        while (size > 0)
                        {
                            core::int_t buffer_size = std::min(core::int_t(core::buffer_size), core::int_t(size));
                            stream().read(buffer.get(), buffer_size);
                            if (stream().fail())
                                throw core::error("MessagePack - unexpected end of string");
                            // Set string in string_type to preserve the subtype
                            string_type.set_string(core::string_t(buffer.get(), static_cast<size_t>(buffer_size)));
                            get_output()->append_to_string(string_type);
                            size -= static_cast<size_t>(buffer_size);
                        }
                        string_type.set_string("");
                        get_output()->end_string(string_type);
                        break;
                    }
                    // Extensions
                    case 0xc7:
                    case 0xc8:
                    case 0xc9:
                    {
                        // TODO
                        break;
                    }
                    // Single-precision floating point
                    case 0xca:
                    {
                        uint32_t flt;
                        if (!core::read_uint32_be(stream(), flt))
                            throw core::error("MessagePack - expected 'float' value");
                        get_output()->write(core::float_from_ieee_754(flt));
                        break;
                    }
                    // Double-precision floating point
                    case 0xcb:
                    {
                        uint64_t flt;
                        if (!core::read_uint64_be(stream(), flt))
                            throw core::error("MessagePack - expected 'float' value");
                        get_output()->write(core::double_from_ieee_754(flt));
                        break;
                    }
                    // Unsigned integers
                    case 0xcc:
                    case 0xcd:
                    case 0xce:
                    case 0xcf:
                    {
                        core::uint_t val;
                        if ((chr == 0xcc && !core::read_uint8(stream(), val)) ||
                            (chr == 0xcd && !core::read_uint16_be(stream(), val)) ||
                            (chr == 0xce && !core::read_uint32_be(stream(), val)) ||
                            (chr == 0xcf && !core::read_uint64_be(stream(), val)))
                            throw core::error("MessagePack - expected 'uinteger'");
                        get_output()->write(val);
                        break;
                    }
                    // Signed integers
                    case 0xd0:
                    case 0xd1:
                    case 0xd2:
                    case 0xd3:
                    {
                        core::int_t val = 0;
                        if ((chr == 0xd0 && !core::read_int8(stream(), val)) ||
                            (chr == 0xd1 && !core::read_int16_be(stream(), val)) ||
                            (chr == 0xd2 && !core::read_int32_be(stream(), val)) ||
                            (chr == 0xd3 && !core::read_int64_be(stream(), val)))
                            throw core::error("MessagePack - expected 'integer'");
                        get_output()->write(val);
                        break;
                    }
                    // Fixext
                    case 0xd4:
                    case 0xd5:
                    case 0xd6:
                    case 0xd7:
                    case 0xd8:
                    {
                        // TODO
                        break;
                    }
                    // UTF-8 Strings
                    case 0xd9:
                    case 0xda:
                    case 0xdb:
                    {
                        uint32_t size;
                        core::value string_type = "";

                        if ((chr == 0xd9 && !core::read_uint8(stream(), size)) ||
                            (chr == 0xda && !core::read_uint16_be(stream(), size)) ||
                            (chr == 0xdb && !core::read_uint32_be(stream(), size)))
                            throw core::error("MessagePack - expected 'binary data' length");

                        get_output()->begin_string(string_type, size);
                        while (size > 0)
                        {
                            core::int_t buffer_size = std::min(core::int_t(core::buffer_size), core::int_t(size));
                            stream().read(buffer.get(), buffer_size);
                            if (stream().fail())
                                throw core::error("MessagePack - unexpected end of string");
                            // Set string in string_type to preserve the subtype
                            string_type.set_string(core::string_t(buffer.get(), static_cast<size_t>(buffer_size)));
                            get_output()->append_to_string(string_type);
                            size -= static_cast<size_t>(buffer_size);
                        }
                        string_type.set_string("");
                        get_output()->end_string(string_type);
                        break;
                    }
                    // Arrays
                    case 0xdc:
                    case 0xdd:
                    {
                        uint32_t size;

                        if ((chr == 0xdc && !core::read_uint16_be(stream(), size)) ||
                            (chr == 0xdd && !core::read_uint32_be(stream(), size)))
                            throw core::error("MessagePack - expected 'array' length");

                        get_output()->begin_array(core::array_t(), size);
                        containers.push(container_data(core::normal, size));
                        break;
                    }
                    // Maps
                    case 0xde:
                    case 0xdf:
                    {
                        uint32_t size;

                        if ((chr == 0xde && !core::read_uint16_be(stream(), size)) ||
                            (chr == 0xdf && !core::read_uint32_be(stream(), size)))
                            throw core::error("MessagePack - expected 'object' length");

                        get_output()->begin_object(core::object_t(), size);
                        containers.push(container_data(core::normal, size));
                        break;
                    }
                }

                written = true;
            }
        };

        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(core::ostream_handle &stream) : core::stream_writer(stream) {}

            protected:
                core::ostream &write_int(core::ostream &stream, core::uint_t i)
                {
                    if (i <= UINT8_MAX / 2)
                        return stream.put(static_cast<char>(i));
                    else if (i <= UINT8_MAX)
                        return stream.put(static_cast<unsigned char>(0xcc)).put(static_cast<char>(i));
                    else if (i <= UINT16_MAX)
                        return stream.put(static_cast<unsigned char>(0xcd)).put(static_cast<char>(i >> 8)).put(i & 0xff);
                    else if (i <= UINT32_MAX)
                        return stream.put(static_cast<unsigned char>(0xce))
                                .put(static_cast<char>(i >> 24))
                                .put((i >> 16) & 0xff)
                                .put((i >> 8) & 0xff)
                                .put(i & 0xff);
                    else
                        return stream.put(static_cast<unsigned char>(0xcf))
                                .put((i >> 56) & 0xff)
                                .put((i >> 48) & 0xff)
                                .put((i >> 40) & 0xff)
                                .put((i >> 32) & 0xff)
                                .put((i >> 24) & 0xff)
                                .put((i >> 16) & 0xff)
                                .put((i >> 8) & 0xff)
                                .put(i & 0xff);
                }

                core::ostream &write_int(core::ostream &stream, core::int_t i)
                {
                    if (i >= 0)
                    {
                        if (i <= UINT8_MAX / 2)
                            return stream.put(static_cast<char>(i));
                        else if (i <= UINT8_MAX)
                            return stream.put(static_cast<unsigned char>(0xcc)).put(static_cast<char>(i));
                        else if (i <= UINT16_MAX)
                            return stream.put(static_cast<unsigned char>(0xcd)).put(static_cast<char>(i >> 8)).put(i & 0xff);
                        else if (i <= UINT32_MAX)
                            return stream.put(static_cast<unsigned char>(0xce))
                                    .put(static_cast<char>(i >> 24))
                                    .put((i >> 16) & 0xff)
                                    .put((i >> 8) & 0xff)
                                    .put(i & 0xff);
                        else
                            return stream.put(static_cast<unsigned char>(0xcf))
                                    .put((i >> 56) & 0xff)
                                    .put((i >> 48) & 0xff)
                                    .put((i >> 40) & 0xff)
                                    .put((i >> 32) & 0xff)
                                    .put((i >> 24) & 0xff)
                                    .put((i >> 16) & 0xff)
                                    .put((i >> 8) & 0xff)
                                    .put(i & 0xff);
                    }
                    else
                    {
                        uint64_t temp;
                        if (i < -std::numeric_limits<core::int_t>::max())
                        {
                            i = uint64_t(INT32_MAX) + 1; // this assignment ensures that the 64-bit write will occur, since the most negative
                                                         // value is (in binary) 1000...0000
                            temp = 0; // The initial set bit is implied, and ORed in with `0x80 | xxx` later on
                        }
                        else
                        {
                            i = -i; // Make i positive
                            temp = ~uint64_t(i) + 1;
                        }

                        if (i <= 31)
                            return stream.put(0xe0 + (temp & 0x1f));
                        else if (i <= INT8_MAX)
                            return stream.put(static_cast<unsigned char>(0xd0)).put(0x80 | (temp & 0xff));
                        else if (i <= INT16_MAX)
                            return stream.put(static_cast<unsigned char>(0xd1)).put(0x80 | ((temp >> 8) & 0xff)).put(temp & 0xff);
                        else if (i <= INT32_MAX)
                            return stream.put(static_cast<unsigned char>(0xd2))
                                    .put(0x80 | ((temp >> 24) & 0xff))
                                    .put((temp >> 16) & 0xff)
                                    .put((temp >> 8) & 0xff)
                                    .put(temp & 0xff);
                        else
                            return stream.put(static_cast<unsigned char>(0xd3))
                                    .put(0x80 | (temp >> 56))
                                    .put((temp >> 48) & 0xff)
                                    .put((temp >> 40) & 0xff)
                                    .put((temp >> 32) & 0xff)
                                    .put((temp >> 24) & 0xff)
                                    .put((temp >> 16) & 0xff)
                                    .put((temp >> 8) & 0xff)
                                    .put(temp & 0xff);
                    }
                }

                core::ostream &write_float(core::ostream &stream, core::real_t f)
                {
                    if (core::float_from_ieee_754(core::float_to_ieee_754(static_cast<float>(f))) == f || std::isnan(f))
                    {
                        uint32_t temp = core::float_to_ieee_754(static_cast<float>(f));

                        return stream.put(static_cast<unsigned char>(0xca))
                                .put((temp >> 24) & 0xff)
                                .put((temp >> 16) & 0xff)
                                .put((temp >> 8) & 0xff)
                                .put(temp & 0xff);
                    }
                    else
                    {
                        uint64_t temp = core::double_to_ieee_754(f);

                        return stream.put(static_cast<unsigned char>(0xcb))
                                .put(temp >> 56)
                                .put((temp >> 48) & 0xff)
                                .put((temp >> 40) & 0xff)
                                .put((temp >> 32) & 0xff)
                                .put((temp >> 24) & 0xff)
                                .put((temp >> 16) & 0xff)
                                .put((temp >> 8) & 0xff)
                                .put(temp & 0xff);
                    }
                }

                core::ostream &write_string_size(core::ostream &stream, size_t str_size, core::subtype_t subtype)
                {
                    // Binary string?
                    if (subtype == core::blob || subtype == core::clob)
                    {
                        if (str_size <= UINT8_MAX)
                            return stream.put(static_cast<unsigned char>(0xc4)).put(static_cast<char>(str_size));
                        else if (str_size <= UINT16_MAX)
                            return stream.put(static_cast<unsigned char>(0xc5))
                                    .put(static_cast<char>(str_size >> 8))
                                    .put(str_size & 0xff);
                        else if (str_size <= UINT32_MAX)
                            return stream.put(static_cast<unsigned char>(0xc6))
                                    .put(static_cast<char>(str_size >> 24))
                                    .put((str_size >> 16) & 0xff)
                                    .put((str_size >> 8) & 0xff)
                                    .put(str_size & 0xff);
                        else
                            throw core::error("MessagePack - 'blob' value is too long");
                    }
                    // Normal string?
                    else
                    {
                        if (str_size <= 31)
                            return stream.put(static_cast<char>(0xa0 + str_size));
                        else if (str_size <= UINT8_MAX)
                            return stream.put(static_cast<unsigned char>(0xd9)).put(static_cast<char>(str_size));
                        else if (str_size <= UINT16_MAX)
                            return stream.put(static_cast<unsigned char>(0xda))
                                    .put(static_cast<char>(str_size >> 8))
                                    .put(str_size & 0xff);
                        else if (str_size <= UINT32_MAX)
                            return stream.put(static_cast<unsigned char>(0xdb))
                                    .put(static_cast<char>(str_size >> 24))
                                    .put((str_size >> 16) & 0xff)
                                    .put((str_size >> 8) & 0xff)
                                    .put(str_size & 0xff);
                        else
                            throw core::error("MessagePack - 'string' value is too long");
                    }

                    // TODO: handle user-specified string types
                    return stream;
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
        public:
            stream_writer(core::ostream_handle output) : stream_writer_base(output) {}

            unsigned int required_features() const {return requires_prefix_array_size | requires_prefix_object_size | requires_prefix_string_size;}

            std::string name() const {return "cppdatalib::message_pack::stream_writer";}

        protected:
            void null_(const core::value &) {stream().put(static_cast<unsigned char>(0xc0));}
            void bool_(const core::value &v) {stream().put(0xc2 + v.get_bool_unchecked());}
            void integer_(const core::value &v) {write_int(stream(), v.get_int_unchecked());}
            void uinteger_(const core::value &v) {write_int(stream(), v.get_uint_unchecked());}
            void real_(const core::value &v) {write_float(stream(), v.get_real_unchecked());}
            void begin_string_(const core::value &v, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("MessagePack - 'string' value does not have size specified");
				else if (size > UINT32_MAX)
					throw core::error("MessagePack - 'string' value is too large");

                write_string_size(stream(), static_cast<size_t>(size), v.get_subtype());
            }
            void string_data_(const core::value &v, bool) {stream().write(v.get_string_unchecked().c_str(), v.get_string_unchecked().size());}

            void begin_array_(const core::value &, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("MessagePack - 'array' value does not have size specified");
                else if (size <= 15)
                    stream().put(static_cast<char>(0x90 + size));
                else if (size <= UINT16_MAX)
                    stream().put(static_cast<unsigned char>(0xdc)).put(static_cast<char>(size >> 8)).put(size & 0xff);
                else if (size <= UINT32_MAX)
                    stream().put(static_cast<unsigned char>(0xdd))
                            .put(static_cast<char>(size >> 24))
                            .put((size >> 16) & 0xff)
                            .put((size >> 8) & 0xff)
                            .put(size & 0xff);
                else
                    throw core::error("MessagePack - 'array' value is too long");
            }

            void begin_object_(const core::value &, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("MessagePack - 'object' value does not have size specified");
                else if (size <= 15)
                    stream().put(static_cast<char>(0x80 + size));
                else if (size <= UINT16_MAX)
                    stream().put(static_cast<unsigned char>(0xde)).put(static_cast<char>(size >> 8)).put(size & 0xff);
                else if (size <= UINT32_MAX)
                    stream().put(static_cast<unsigned char>(0xdf))
                            .put(static_cast<char>(size >> 24))
                            .put((size >> 16) & 0xff)
                            .put((size >> 8) & 0xff)
                            .put(size & 0xff);
                else
                    throw core::error("MessagePack - 'object' value is too long");
            }
        };

        inline core::value from_message_pack(core::istream_handle stream)
        {
            parser p(stream);
            core::value v;
            p >> v;
            return v;
        }

        inline core::value operator "" _msgpack(const char *stream, size_t size)
        {
            core::istring_wrapper_stream wrap(std::string(stream, size));
            return from_message_pack(wrap);
        }

        inline std::string to_message_pack(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer writer(stream);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_MESSAGE_PACK_H

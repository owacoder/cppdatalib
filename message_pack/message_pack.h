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
        /* TODO: doesn't really support core::iencodingstream formats other than `raw` */
        class parser : public core::stream_parser
        {
            struct container_data
            {
                container_data(core::subtype_t sub_type = core::normal, uint32_t remaining_size = 0)
                    : sub_type(sub_type)
                    , remaining_size(remaining_size)
                {}

                core::subtype_t sub_type;
                uint32_t remaining_size;
            };

            char * const buffer;
            typedef std::stack< container_data, std::vector<container_data> > containers_t;

            containers_t containers;
            bool written;

        public:
            parser(core::istream_handle input)
                : core::stream_parser(input)
                , buffer(new char [core::buffer_size])
            {
                reset();
            }
            ~parser() {delete[] buffer;}

            unsigned int features() const {return provides_prefix_array_size |
                                                  provides_prefix_object_size |
                                                  provides_prefix_string_size;}

        protected:
            void read_string(core::subtype_t subtype, uint32_t size, const char *failure_message)
            {
                core::value string_type = core::value("", 0, subtype, true);
                get_output()->begin_string(string_type, size);
                while (size > 0)
                {
                    uint32_t buffer_size = std::min(uint32_t(core::buffer_size), size);
                    stream().read(buffer, buffer_size);
                    if (stream().fail())
                        throw core::error(failure_message);
                    // Set string in string_type to preserve the subtype
                    string_type = core::value(buffer, static_cast<size_t>(buffer_size), string_type.get_subtype(), true);
                    get_output()->append_to_string(string_type);
                    size -= buffer_size;
                }
                string_type = core::value("", 0, string_type.get_subtype(), true);
                get_output()->end_string(string_type);
            }

            void reset_()
            {
                containers = containers_t();
                written = false;
            }

#ifdef CPPDATALIB_WATCOM
            template<typename T>
            bool read_int(core::istream &strm, size_t idx, T &result)
            {
                switch (idx)
                {
                    case 0: return core::read_uint8<T>(strm, result);
                    case 1: return core::read_uint16_be<T>(strm, result);
                    case 2: return core::read_uint32_be<T>(strm, result);
                    case 3: return core::read_uint64_be<T>(strm, result);
                    default: return false;
                }
            }
#else
            template<typename T>
            bool read_int(core::istream &strm, size_t idx, T &result)
            {
                // This is just a fancy jump table, basically. It calls the correct read function based on the index used
                core::istream &(*call[])(core::istream &, T &) = {core::read_uint8<T>,
                                                                  core::read_uint16_be<T>,
                                                                  core::read_uint32_be<T>,
                                                                  core::read_uint64_be<T>};

                if (!call[idx](strm, result))
                    return false;

                return true;
            }
#endif

            void write_one_()
            {
                core::istream::int_type chr;

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
                    get_output()->write(core::value(core::uint_t(chr)));
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
                        throw core::error("MessagePack - unexpected end of UTF-8 string");

                    get_output()->write(core::value(buf, chr, core::normal, true));
                }
                else if (chr >= 0xe0) // Negative fixint
                    get_output()->write(core::value(-core::int_t((~unsigned(chr) + 1) & 0xff)));
                else switch (chr)
                {
                    // Null
                    case 0xc0: get_output()->write(core::null_t()); break;
                    // Boolean false
                    case 0xc2: get_output()->write(core::value(false)); break;
                    // Boolean true
                    case 0xc3: get_output()->write(core::value(true)); break;
                    // Binary
                    case 0xc4:
                    case 0xc5:
                    case 0xc6:
                    {
                        uint32_t size = 0;

                        if (!read_int<uint32_t>(stream(), chr - 0xc4, size))
                            throw core::error("MessagePack - expected binary string length");

                        read_string(core::blob, size, "MessagePack - unexpected end of binary string");
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
                        get_output()->write(core::value(core::float_from_ieee_754(flt)));
                        break;
                    }
                    // Double-precision floating point
                    case 0xcb:
                    {
                        uint64_t flt;
                        if (!core::read_uint64_be(stream(), flt))
                            throw core::error("MessagePack - expected 'float' value");
                        get_output()->write(core::value(core::double_from_ieee_754(flt)));
                        break;
                    }
                    // Unsigned integers
                    case 0xcc:
                    case 0xcd:
                    case 0xce:
                    case 0xcf:
                    {
                        core::uint_t val = 0;
                        if (!read_int<core::uint_t>(stream(), chr - 0xcc, val))
                            throw core::error("MessagePack - expected 'uinteger'");
                        get_output()->write(core::value(val));
                        break;
                    }
                    // Signed integers
                    case 0xd0:
                    case 0xd1:
                    case 0xd2:
                    case 0xd3:
                    {
                        core::int_t val = 0;
                        if (!read_int<core::int_t>(stream(), chr - 0xd0, val))
                            throw core::error("MessagePack - expected 'integer'");
                        get_output()->write(core::value(val));
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
                        uint32_t size = 0;

                        if (!read_int<uint32_t>(stream(), chr - 0xd9, size))
                            throw core::error("MessagePack - expected UTF-8 string length");

                        read_string(core::blob, size, "MessagePack - unexpected end of UTF-8 string");
                        break;
                    }
                    // Arrays
                    case 0xdc:
                    case 0xdd:
                    {
                        uint32_t size = 0;

                        if (!read_int<uint32_t>(stream(), chr - 0xdc + 1, size))
                            throw core::error("MessagePack - expected 'array' length");

                        get_output()->begin_array(core::array_t(), size);
                        containers.push(container_data(core::normal, size));
                        break;
                    }
                    // Maps
                    case 0xde:
                    case 0xdf:
                    {
                        uint32_t size = 0;

                        if (!read_int<uint32_t>(stream(), chr - 0xde + 1, size))
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
                    if (i <= std::numeric_limits<uint8_t>::max() / 2)
                        return stream.put(static_cast<char>(i));
                    else if (i <= std::numeric_limits<uint8_t>::max())
                        return stream.put(static_cast<unsigned char>(0xcc)).put(static_cast<char>(i));
                    else if (i <= std::numeric_limits<uint16_t>::max())
                        return stream.put(static_cast<unsigned char>(0xcd)).put(static_cast<char>(i >> 8)).put(i & 0xff);
                    else if (i <= std::numeric_limits<uint32_t>::max())
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
                        if (i <= std::numeric_limits<uint8_t>::max())
                            return stream.put(static_cast<unsigned char>(0xd0)).put(static_cast<char>(i));
                        else if (i <= std::numeric_limits<uint16_t>::max())
                            return stream.put(static_cast<unsigned char>(0xd1)).put(static_cast<char>(i >> 8)).put(i & 0xff);
                        else if (i <= std::numeric_limits<uint32_t>::max())
                            return stream.put(static_cast<unsigned char>(0xd2))
                                    .put(static_cast<char>(i >> 24))
                                    .put((i >> 16) & 0xff)
                                    .put((i >> 8) & 0xff)
                                    .put(i & 0xff);
                        else
                            return stream.put(static_cast<unsigned char>(0xd3))
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
                            i = uint64_t(std::numeric_limits<int32_t>::max()) + 1; // this assignment ensures that the 64-bit write will occur, since the most negative
                                                         // value is (in binary) 1000...0000
                            temp = 0; // The initial set bit is implied, and ORed in with `0x80 | xxx` later on
                        }
                        else
                        {
                            i = -i; // Make i positive
                            temp = ~(uint64_t) (i) + 1;
                        }

                        if (i <= 31)
                            return stream.put(0xe0 + (temp & 0x1f));
                        else if (i <= std::numeric_limits<int8_t>::max())
                            return stream.put(static_cast<unsigned char>(0xd0)).put(0x80 | (temp & 0xff));
                        else if (i <= std::numeric_limits<int16_t>::max())
                            return stream.put(static_cast<unsigned char>(0xd1)).put(0x80 | ((temp >> 8) & 0xff)).put(temp & 0xff);
                        else if (i <= std::numeric_limits<int32_t>::max())
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
                    using namespace std;
                    
                    if (core::float_from_ieee_754(core::float_to_ieee_754(static_cast<float>(f))) == f || isnan(f))
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
                    if (!core::subtype_is_text_string(subtype))
                    {
                        if (str_size <= std::numeric_limits<uint8_t>::max())
                            return stream.put(static_cast<unsigned char>(0xc4)).put(static_cast<char>(str_size));
                        else if (str_size <= std::numeric_limits<uint16_t>::max())
                            return stream.put(static_cast<unsigned char>(0xc5))
                                    .put(static_cast<char>(str_size >> 8))
                                    .put(str_size & 0xff);
                        else if (str_size <= std::numeric_limits<uint32_t>::max())
                            return stream.put(static_cast<unsigned char>(0xc6))
                                    .put(static_cast<char>(str_size >> 24))
                                    .put((str_size >> 16) & 0xff)
                                    .put((str_size >> 8) & 0xff)
                                    .put(str_size & 0xff);
                        else
                            throw core::error("MessagePack - binary 'string' value is too long");
                    }
                    // Normal string?
                    else
                    {
                        if (str_size <= 31)
                            return stream.put(static_cast<char>(0xa0 + str_size));
                        else if (str_size <= std::numeric_limits<uint8_t>::max())
                            return stream.put(static_cast<unsigned char>(0xd9)).put(static_cast<char>(str_size));
                        else if (str_size <= std::numeric_limits<uint16_t>::max())
                            return stream.put(static_cast<unsigned char>(0xda))
                                    .put(static_cast<char>(str_size >> 8))
                                    .put(str_size & 0xff);
                        else if (str_size <= std::numeric_limits<uint32_t>::max())
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
            void begin_string_(const core::value &v, core::optional_size size, bool)
            {
                if (!size.has_value())
                    throw core::error("MessagePack - 'string' value does not have size specified");
                else if (size.value() > std::numeric_limits<uint32_t>::max())
                                        throw core::error("MessagePack - 'string' value is too large");

                write_string_size(stream(), size.value(), v.get_subtype());
            }
            void string_data_(const core::value &v, bool) {stream() << v.get_string_unchecked();}

            void begin_array_(const core::value &, core::optional_size size, bool)
            {
                if (!size.has_value())
                    throw core::error("MessagePack - 'array' value does not have size specified");
                else if (size.value() <= 15)
                    stream().put(static_cast<char>(0x90 + size.value()));
                else if (size.value() <= std::numeric_limits<uint16_t>::max())
                    stream().put(static_cast<unsigned char>(0xdc)).put(static_cast<char>(size.value() >> 8)).put(size.value() & 0xff);
                else if (size.value() <= std::numeric_limits<uint32_t>::max())
                    stream().put(static_cast<unsigned char>(0xdd))
                            .put(static_cast<char>(size.value() >> 24))
                            .put((size.value() >> 16) & 0xff)
                            .put((size.value() >> 8) & 0xff)
                            .put(size.value() & 0xff);
                else
                    throw core::error("MessagePack - 'array' value is too long");
            }

            void begin_object_(const core::value &, core::optional_size size, bool)
            {
                if (!size.has_value())
                    throw core::error("MessagePack - 'object' value does not have size specified");
                else if (size.value() <= 15)
                    stream().put(static_cast<char>(0x80 + size.value()));
                else if (size.value() <= std::numeric_limits<uint16_t>::max())
                    stream().put(static_cast<unsigned char>(0xde)).put(static_cast<char>(size.value() >> 8)).put(size.value() & 0xff);
                else if (size.value() <= std::numeric_limits<uint32_t>::max())
                    stream().put(static_cast<unsigned char>(0xdf))
                            .put(static_cast<char>(size.value() >> 24))
                            .put((size.value() >> 16) & 0xff)
                            .put((size.value() >> 8) & 0xff)
                            .put(size.value() & 0xff);
                else
                    throw core::error("MessagePack - 'object' value is too long");
            }

            void link_(const core::value &) {throw core::error("MessagePack - 'link' value not allowed in output");}
        };

        inline core::value from_message_pack(core::istream_handle stream)
        {
            parser p(stream);
            core::value v;
            core::convert(p, v);
            return v;
        }

#ifdef CPPDATALIB_CPP11
        inline core::value operator "" _msgpack(const char *stream, size_t size)
        {
            core::istringstream wrap(std::string(stream, size));
            return from_message_pack(wrap);
        }
#endif

        inline std::string to_message_pack(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer writer(stream);
            core::convert(writer, v);
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_MESSAGE_PACK_H

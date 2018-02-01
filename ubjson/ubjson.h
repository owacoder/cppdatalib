/*
 * ubjson.h
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

#ifndef CPPDATALIB_UBJSON_H
#define CPPDATALIB_UBJSON_H

#include "../core/core.h"

// TODO: Fix negative integer handling in read and write routines

namespace cppdatalib
{
    namespace ubjson
    {
        class parser : public core::stream_parser
        {
            struct container_data
            {
                container_data(char content_type, core::int_t remaining_size)
                    : content_type(content_type)
                    , remaining_size(remaining_size)
                {}

                char content_type;
                core::int_t remaining_size;
            };

            std::unique_ptr<char []> buffer;
            std::stack<container_data, std::vector<container_data>> containers;

            inline char size_specifier(core::int_t min, core::int_t max)
            {
                if (min >= 0 && max <= UINT8_MAX)
                    return 'U';
                else if (min >= INT8_MIN && max <= INT8_MAX)
                    return 'i';
                else if (min >= INT16_MIN && max <= INT16_MAX)
                    return 'I';
                else if (min >= INT32_MIN && max <= INT32_MAX)
                    return 'l';
                else
                    return 'L';
            }

            inline core::istream &read_int(core::int_t &i, char specifier)
            {
                uint64_t temp;
                bool negative = false;
                int c = stream().get();

                if (c == EOF) throw core::error("UBJSON - expected integer value after type specifier");
                temp = c & 0xff;

                switch (specifier)
                {
                    case 'U': // Unsigned byte
                        break;
                    case 'i': // Signed byte
                        negative = c >> 7;
                        temp |= negative * 0xffffffffffffff00ull;
                        break;
                    case 'I': // Signed word
                        negative = c >> 7;

                        c = stream().get();
                        if (c == EOF) throw core::error("UBJSON - expected integer value after type specifier");

                        temp = (temp << 8) | (c & 0xff);
                        temp |= negative * 0xffffffffffff0000ull;
                        break;
                    case 'l': // Signed double-word
                        negative = c >> 7;

                        for (int i = 0; i < 3; ++i)
                        {
                            c = stream().get();
                            if (c == EOF) throw core::error("UBJSON - expected integer value after type specifier");

                            temp = (temp << 8) | (c & 0xff);
                        }
                        temp |= negative * 0xffffffff00000000ull;
                        break;
                    case 'L': // Signed double-word
                        negative = c >> 7;

                        for (int i = 0; i < 7; ++i)
                        {
                            c = stream().get();
                            if (c == EOF) throw core::error("UBJSON - expected integer value after type specifier");

                            temp = (temp << 8) | (c & 0xff);
                        }
                        break;
                    default:
                        throw core::error("UBJSON - invalid integer specifier found in input");
                }

                if (negative)
                {
                    i = (~temp + 1) & ((1ull << 63) - 1);
                    if (i == 0)
                        i = INT64_MIN;
                    else
                        i = -i;
                }
                else
                    i = temp;

                return stream();
            }

            inline core::istream &read_float(char specifier, core::stream_handler &writer)
            {
                core::real_t r;
                uint64_t temp;
                int c = stream().get();

                if (c == EOF) throw core::error("UBJSON - expected floating-point value after type specifier");
                temp = c & 0xff;

                if (specifier == 'd')
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        c = stream().get();
                        if (c == EOF) throw core::error("UBJSON - expected floating-point value after type specifier");

                        temp = (temp << 8) | (c & 0xff);
                    }
                }
                else if (specifier == 'D')
                {
                    for (int i = 0; i < 7; ++i)
                    {
                        c = stream().get();
                        if (c == EOF) throw core::error("UBJSON - expected floating-point value after type specifier");

                        temp = (temp << 8) | (c & 0xff);
                    }
                }
                else
                    throw core::error("UBJSON - invalid floating-point specifier found in input");

                if (specifier == 'd')
                    r = core::float_from_ieee_754(static_cast<uint32_t>(temp));
                else
                    r = core::double_from_ieee_754(temp);

                writer.write(core::value(r));
                return stream();
            }

            inline core::istream &read_string(char specifier, core::stream_handler &writer)
            {
                int c = stream().get();

                if (c == EOF) throw core::error("UBJSON - expected string value after type specifier");

                if (specifier == 'C')
                {
                    writer.begin_string(core::string_t(), 1);
                    writer.write(core::value(core::string_t(1, c)));
                    writer.end_string(core::string_t());
                }
                else if (specifier == 'H')
                {
                    core::int_t size;

                    read_int(size, c);
                    if (size < 0) throw core::error("UBJSON - invalid negative size specified for high-precision number");

                    writer.begin_string(core::value(core::string_t(), core::bignum), size);
                    while (size > 0)
                    {
                        core::int_t buffer_size = std::min(core::int_t(core::buffer_size), size);
                        stream().read(buffer.get(), buffer_size);
                        if (stream().fail())
                            throw core::error("UBJSON - expected high-precision number value after type specifier");
                        writer.append_to_string(core::string_t(buffer.get(), static_cast<size_t>(buffer_size)));
                        size -= buffer_size;
                    }
                    writer.end_string(core::value(core::string_t(), core::bignum));
                }
                else if (specifier == 'S')
                {
                    core::int_t size;

                    read_int(size, c);
                    if (size < 0) throw core::error("UBJSON - invalid negative size specified for string");

                    writer.begin_string(core::string_t(), size);
                    while (size > 0)
                    {
                        core::int_t buffer_size = std::min(core::int_t(core::buffer_size), size);
                        stream().read(buffer.get(), buffer_size);
                        if (stream().fail())
                            throw core::error("UBJSON - expected string value after type specifier");
                        writer.append_to_string(core::string_t(buffer.get(), static_cast<size_t>(buffer_size)));
                        size -= buffer_size;
                    }
                    writer.end_string(core::string_t());
                }
                else
                    throw core::error("UBJSON - invalid string specifier found in input");

                return stream();
            }

        public:
            parser(core::istream_handle input)
                : core::stream_parser(input)
                , buffer(new char [core::buffer_size])
            {
                reset();
            }

            unsigned int features() const {return provides_prefix_string_size;}

        protected:
            void reset_()
            {
                containers = decltype(containers)();
            }

            void write_one_()
            {
                const char valid_types[] = "ZTFUiIlLdDCHS[{";
                int chr;

                if (containers.size() > 0)
                {
                    if (containers.top().content_type)
                        chr = containers.top().content_type;
                    else
                    {
                        chr = stream().get();
                        if (chr == EOF)
                            throw core::error("UBJSON - unexpected end of stream");
                    }

                    if (containers.top().remaining_size > 0 && !get_output()->container_key_was_just_parsed())
                        --containers.top().remaining_size;
                    if (get_output()->current_container() == core::object && chr != 'N' && chr != '}' && !get_output()->container_key_was_just_parsed())
                    {
                        // Parse key here, remap read character to 'N' (the no-op instruction)
                        if (!containers.top().content_type)
                            stream().unget();
                        read_string('S', *get_output());
                        chr = 'N';
                    }
                }
                else
                {
                    chr = stream().get();
                    if (chr == EOF)
                        throw core::error("UBJSON - unexpected end of stream");
                }

                switch (chr)
                {
                    case 'Z':
                        get_output()->write(core::null_t());
                        break;
                    case 'T':
                        get_output()->write(core::value(true));
                        break;
                    case 'F':
                        get_output()->write(core::value(false));
                        break;
                    case 'U':
                    case 'i':
                    case 'I':
                    case 'l':
                    case 'L':
                    {
                        core::int_t i;
                        read_int(i, chr);
                        get_output()->write(core::value(i));
                        break;
                    }
                    case 'd':
                    case 'D':
                        read_float(chr, *get_output());
                        break;
                    case 'C':
                    case 'H':
                    case 'S':
                        read_string(chr, *get_output());
                        break;
                    case 'N': break;
                    case '[':
                    {
                        int type = 0;
                        core::int_t size = -1;

                        chr = stream().get();
                        if (chr == EOF) throw core::error("UBJSON - expected array value after '['");

                        if (chr == '$') // Type specified
                        {
                            chr = stream().get();
                            if (chr == EOF || !strchr(valid_types, chr)) throw core::error("UBJSON - expected type specifier after '$'");
                            type = chr;
                            chr = stream().get();
                            if (chr == EOF) throw core::error("UBJSON - unexpected end of array");
                        }

                        if (chr == '#') // Count specified
                        {
                            chr = stream().get();
                            if (chr == EOF) throw core::error("UBJSON - expected count specifier after '#'");

                            read_int(size, chr);
                            if (size < 0) throw core::error("UBJSON - invalid negative size specified for array");
                        }

                        // If type != 0, then size must be >= 0
                        if (type != 0 && size < 0)
                            throw core::error("UBJSON - array element type specified but number of elements is not specified");
                        else if (size < 0) // Unless a count was read, one character needs to be put back (from checking chr == '#')
                            stream().unget();

                        get_output()->begin_array(core::array_t(), size >= 0? size: core::int_t(core::stream_handler::unknown_size));
                        containers.push(container_data(type, size));

                        break;
                    }
                    case ']':
                        if (!containers.empty() && containers.top().remaining_size >= 0)
                            throw core::error("UBJSON - attempted to end an array with size specified already");

                        get_output()->end_array(core::array_t());
                        containers.pop();
                        break;
                    case '{':
                    {
                        int type = 0;
                        core::int_t size = -1;

                        chr = stream().get();
                        if (chr == EOF) throw core::error("UBJSON - expected object value after '{'");

                        if (chr == '$') // Type specified
                        {
                            chr = stream().get();
                            if (chr == EOF || !strchr(valid_types, chr)) throw core::error("UBJSON - expected type specifier after '$'");
                            type = chr;
                            chr = stream().get();
                            if (chr == EOF) throw core::error("UBJSON - unexpected end of object");
                        }

                        if (chr == '#') // Count specified
                        {
                            chr = stream().get();
                            if (chr == EOF) throw core::error("UBJSON - expected count specifier after '#'");

                            read_int(size, chr);
                            if (size < 0) throw core::error("UBJSON - invalid negative size specified for object");
                        }

                        // If type != 0, then size must be >= 0
                        if (type != 0 && size < 0)
                            throw core::error("UBJSON - object element type specified but number of elements is not specified");
                        else if (size < 0) // Unless a count was read, one character needs to be put back (from checking chr == '#')
                            stream().unget();

                        get_output()->begin_object(core::object_t(), size >= 0? size: core::int_t(core::stream_handler::unknown_size));
                        containers.push(container_data(type, size));

                        break;
                    }
                    case '}':
                        if (!containers.empty() && containers.top().remaining_size >= 0)
                            throw core::error("UBJSON - attempted to end an object with size specified already");

                        get_output()->end_object(core::object_t());
                        containers.pop();
                        break;
                    default:
                        throw core::error("UBJSON - expected value");
                }

                if (containers.size() > 0 && !get_output()->container_key_was_just_parsed() && containers.top().remaining_size == 0)
                {
                    if (get_output()->current_container() == core::array)
                        get_output()->end_array(core::array_t());
                    else if (get_output()->current_container() == core::object)
                        get_output()->end_object(core::object_t());
                    containers.pop();
                }
            }
        };

        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(core::ostream_handle &stream) : core::stream_writer(stream) {}

            protected:
                core::ostream &write_int(core::ostream &stream, core::int_t i, bool add_specifier, char force_specifier = 0)
                {
                    const std::string specifiers = "UiIlL";
                    size_t force_bits = specifiers.find(force_specifier);

                    if (force_bits == std::string::npos)
                        force_bits = 0;

                    if (force_bits == 0 && (i >= 0 && i <= UINT8_MAX))
                    {
                        stream << (add_specifier? "U": "");
                        stream.put(static_cast<char>(i));
                    }
                    else if (force_bits <= 1 && (i >= INT8_MIN && i < 0))
                    {
                        stream << (add_specifier? "i": "");
                        stream.put(0x80 | ((~std::abs(i) & 0xff) + 1));
                    }
                    else if (force_bits <= 2 && (i >= INT16_MIN && i <= INT16_MAX))
                    {
                        uint16_t t;

                        if (i < 0)
                            t = 0x8000u | ((~std::abs(i) & 0xffffu) + 1);
                        else
                            t = static_cast<uint16_t>(i);

                        stream << (add_specifier? "I": "");
                        stream.put(t >> 8).put(t & 0xff);
                    }
                    else if (force_bits <= 3 && (i >= INT32_MIN && i <= INT32_MAX))
                    {
                        uint32_t t;

                        if (i < 0)
                            t = 0x80000000u | ((~std::abs(i) & 0xffffffffu) + 1);
                        else
                            t = static_cast<uint32_t>(i);

                        stream << (add_specifier? "l": "");
                        stream.put(t >> 24)
                                .put((t >> 16) & 0xff)
                                .put((t >>  8) & 0xff)
                                .put((t      ) & 0xff);
                    }
                    else
                    {
                        uint64_t t;

                        if (i < 0)
                            t = 0x8000000000000000u | ((~std::abs(i) & 0xffffffffffffffffu) + 1);
                        else
                            t = i;

                        stream << (add_specifier? "L": "");
                        stream.put(t >> 56)
                                .put((t >> 48) & 0xff)
                                .put((t >> 40) & 0xff)
                                .put((t >> 32) & 0xff)
                                .put((t >> 24) & 0xff)
                                .put((t >> 16) & 0xff)
                                .put((t >>  8) & 0xff)
                                .put((t      ) & 0xff);
                    }

                    return stream;
                }

                core::ostream &write_float(core::ostream &stream, core::real_t f, bool add_specifier, char force_specifier = 0)
                {
                    const std::string specifiers = "dD";
                    size_t force_bits = specifiers.find(force_specifier);

                    if (force_bits == std::string::npos)
                        force_bits = 0;

                    if (force_bits == 0 && (core::float_from_ieee_754(core::float_to_ieee_754(static_cast<float>(f))) == f || std::isnan(f)))
                    {
                        uint32_t t = core::float_to_ieee_754(static_cast<float>(f));

                        stream << (add_specifier? "d": "");
                        stream.put(t >> 24)
                                .put((t >> 16) & 0xff)
                                .put((t >>  8) & 0xff)
                                .put((t      ) & 0xff);
                    }
                    else
                    {
                        uint64_t t = core::double_to_ieee_754(f);

                        stream << (add_specifier? "D": "");
                        stream.put(t >> 56)
                                .put((t >> 48) & 0xff)
                                .put((t >> 40) & 0xff)
                                .put((t >> 32) & 0xff)
                                .put((t >> 24) & 0xff)
                                .put((t >> 16) & 0xff)
                                .put((t >>  8) & 0xff)
                                .put((t      ) & 0xff);
                    }

                    return stream;
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
        public:
            stream_writer(core::ostream_handle output) : impl::stream_writer_base(output) {}

            unsigned int required_features() const {return requires_prefix_string_size;}

            std::string name() const {return "cppdatalib::ubjson::stream_writer";}

        protected:
            void begin_key_(const core::value &v)
            {
                if (!v.is_string())
                    throw core::error("UBJSON - cannot write non-string key");
            }

            void null_(const core::value &) {stream().put('Z');}
            void bool_(const core::value &v) {stream().put(v.get_bool_unchecked()? 'T': 'F');}
            void integer_(const core::value &v) {write_int(stream(), v.get_int_unchecked(), true);}
            void uinteger_(const core::value &v)
            {
                if (v.get_uint_unchecked() > static_cast<core::uint_t>(std::numeric_limits<core::int_t>::max()))
                    throw core::error("UBJSON - 'integer' value is out of range of output format");
                write_int(stream(), v.get_uint_unchecked(), true);
            }
            void real_(const core::value &v) {write_float(stream(), v.get_real_unchecked(), true);}
            void begin_string_(const core::value &v, core::int_t size, bool is_key)
            {
                if (size == unknown_size)
                    throw core::error("UBJSON - 'string' value does not have size specified");

                if (!is_key)
                    stream().put(v.get_subtype() == core::bignum? 'H': 'S');
                write_int(stream(), size, true);
            }
            void string_data_(const core::value &v, bool) {stream().write(v.get_string_unchecked().data(), v.get_string_unchecked().size());}

            void begin_array_(const core::value &, core::int_t, bool) {stream().put('[');}
            void end_array_(const core::value &, bool) {stream().put(']');}

            void begin_object_(const core::value &, core::int_t, bool) {stream().put('{');}
            void end_object_(const core::value &, bool) {stream().put('}');}
        };

        inline core::value from_ubjson(core::istream_handle stream)
        {
            parser reader(stream);
            core::value v;
            reader >> v;
            return v;
        }

        inline core::value operator "" _ubjson(const char *stream, size_t size)
        {
            core::istring_wrapper_stream wrap(std::string(stream, size));
            return from_ubjson(wrap);
        }

        inline std::string to_ubjson(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer writer(stream);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_UBJSON_H

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
        /* TODO: doesn't really support core::iencodingstream formats other than `raw` */
        class parser : public core::stream_parser
        {
            struct container_data
            {
                container_data(char content_type = 0, core::int_t remaining_size = 0)
                    : content_type(content_type)
                    , remaining_size(remaining_size)
                {}

                char content_type;
                core::int_t remaining_size;
            };

            char * const buffer;
            typedef std::stack< container_data, std::vector<container_data> > containers_t;

            containers_t containers;

#ifdef CPPDATALIB_WATCOM
            template<typename T>
            bool read_int_internal(core::istream &strm, size_t idx, T &result)
            {
                switch (idx)
                {
                    case 0: return core::read_uint8<T>(strm, result);
                    case 1: return core::read_int8<T>(strm, result);
                    case 2: return core::read_int16_be<T>(strm, result);
                    case 3: return core::read_int32_be<T>(strm, result);
                    case 4: return core::read_int64_be<T>(strm, result);
                    default: return false;
                }
            }
#else
            template<typename T>
            bool read_int_internal(core::istream &strm, size_t idx, T &result)
            {
                // This is just a fancy jump table, basically. It calls the correct read function based on the index used
                core::istream &(*call[])(core::istream &, T &) = {core::read_uint8<T>,
                                                                  core::read_int8<T>,
                                                                  core::read_int16_be<T>,
                                                                  core::read_int32_be<T>,
                                                                  core::read_int64_be<T>};

                if (!call[idx](strm, result))
                    return false;

                return true;
            }
#endif

            inline core::istream &read_int(core::int_t &i, char specifier)
            {
                const char specifiers[] = "UiIlL";

                const char *p = strchr(specifiers, specifier);
                if (!p)
                    throw core::error("UBJSON - invalid integer specifier found in input");

                if (!read_int_internal<core::int_t>(stream(), p - specifiers, i))
                    throw core::error("UBJSON - expected integer value after type specifier");

                return stream();
            }

            inline core::istream &read_float(char specifier, core::stream_handler &writer)
            {
                core::real_t r;
                uint64_t temp;

                if (specifier == 'd')
                {
                    if (!core::read_uint32_be(stream(), temp))
                        throw core::error("UBJSON - expected floating-point value after type specifier");
                }
                else if (specifier == 'D')
                {
                    if (!core::read_uint64_be(stream(), temp))
                        throw core::error("UBJSON - expected floating-point value after type specifier");
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
                core::istream::int_type c = stream().get();

                if (c == EOF) throw core::error("UBJSON - expected string value after type specifier");

                if (specifier == 'C')
                {
                    writer.begin_string(core::string_t(), 1);
                    writer.write(core::value(core::ucs_to_utf8(c)));
                    writer.end_string(core::string_t());
                }
                else if (specifier == 'H')
                {
                    core::int_t size;
                    core::value string_type("", 0, core::bignum, true);

                    read_int(size, c);
                    if (size < 0) throw core::error("UBJSON - invalid negative size specified for high-precision number");

                    writer.begin_string(string_type, size);
                    while (size > 0)
                    {
                        core::int_t buffer_size = std::min(core::int_t(core::buffer_size), size);
                        stream().read(buffer, buffer_size);
                        if (stream().fail())
                            throw core::error("UBJSON - expected high-precision number value after type specifier");
                        string_type = core::value(buffer, static_cast<size_t>(buffer_size), string_type.get_subtype(), true);
                        writer.append_to_string(string_type);
                        size -= buffer_size;
                    }
                    writer.end_string(string_type);
                }
                else if (specifier == 'S')
                {
                    core::int_t size;
                    core::value string_type("", 0, core::normal, true);

                    read_int(size, c);
                    if (size < 0) throw core::error("UBJSON - invalid negative size specified for string");

                    writer.begin_string(string_type, size);
                    while (size > 0)
                    {
                        core::int_t buffer_size = std::min(core::int_t(core::buffer_size), size);
                        stream().read(buffer, buffer_size);
                        if (stream().fail())
                            throw core::error("UBJSON - expected string value after type specifier");
                        string_type = core::value(buffer, static_cast<size_t>(buffer_size), string_type.get_subtype(), true);
                        writer.append_to_string(string_type);
                        size -= buffer_size;
                    }
                    writer.end_string(string_type);
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
            ~parser() {delete[] buffer;}

            unsigned int features() const {return provides_prefix_string_size;}

        protected:
            void reset_()
            {
                containers = containers_t();
            }

            void write_one_()
            {
                const char valid_types[] = "ZTFUiIlLdDCHS[{";
                core::istream::int_type chr;

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
                        // Parse key here, remap the character that was read to 'N' (the no-op instruction)
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

                        get_output()->begin_array(core::array_t(), size >= 0? core::optional_size(size): core::stream_handler::unknown_size());
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
                        core::istream::int_type type = 0;
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

                        get_output()->begin_object(core::object_t(), size >= 0? core::optional_size(size): core::stream_handler::unknown_size());
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

                    if (force_bits == 0 && (i >= 0 && i <= std::numeric_limits<uint8_t>::max()))
                    {
                        if (add_specifier)
                            stream.put('U');
                        stream.put(static_cast<char>(i));
                    }
                    else if (force_bits <= 1 && (i >= std::numeric_limits<int8_t>::min() && i < 0))
                    {
                        if (add_specifier)
                            stream.put('i');
                        stream.put(0x80 | ((~stdx::abs(i) & 0xff) + 1));
                    }
                    else if (force_bits <= 2 && (i >= std::numeric_limits<int16_t>::min() && i <= std::numeric_limits<int16_t>::max()))
                    {
                        uint16_t t = static_cast<uint16_t>(i);

                        if (add_specifier)
                            stream.put('I');
                        stream.put(t >> 8).put(t & 0xff);
                    }
                    else if (force_bits <= 3 && (i >= std::numeric_limits<int32_t>::min() && i <= std::numeric_limits<int32_t>::max()))
                    {
                        uint32_t t = static_cast<uint32_t>(i);

                        if (add_specifier)
                            stream.put('l');
                        stream.put(t >> 24)
                                .put((t >> 16) & 0xff)
                                .put((t >>  8) & 0xff)
                                .put((t      ) & 0xff);
                    }
                    else
                    {
                        uint64_t t = static_cast<uint64_t>(i);

                        if (add_specifier)
                            stream.put('L');
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

                    using namespace std;

                    if (force_bits == std::string::npos)
                        force_bits = 0;

                    if (force_bits == 0 && (core::float_from_ieee_754(core::float_to_ieee_754(static_cast<float>(f))) == f || isnan(f)))
                    {
                        uint32_t t = core::float_to_ieee_754(static_cast<float>(f));

                        if (add_specifier)
                            stream.put('d');
                        stream.put(t >> 24)
                                .put((t >> 16) & 0xff)
                                .put((t >>  8) & 0xff)
                                .put((t      ) & 0xff);
                    }
                    else
                    {
                        uint64_t t = core::double_to_ieee_754(f);

                        if (add_specifier)
                            stream.put('D');
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
                {
                    std::string str = stdx::to_string(v.get_uint_unchecked());
                    stream().put('H');
                    write_int(stream(), str.size(), true);
                    stream() << str;
                }
                else
                    write_int(stream(), v.get_uint_unchecked(), true);
            }
            void real_(const core::value &v) {write_float(stream(), v.get_real_unchecked(), true);}
            void begin_string_(const core::value &v, core::optional_size size, bool is_key)
            {
                if (!size.has_value())
                    throw core::error("UBJSON - 'string' value does not have size specified");
                else if (!core::subtype_is_text_string(v.get_subtype()))
                    throw core::error("UBJSON - 'string' value must be in UTF-8 format");
                else if (size.value() > uintmax_t(std::numeric_limits<core::int_t>::max()))
                    throw core::error("UBJSON - 'string' value is too long");

                if (!is_key)
                    stream().put(v.get_subtype() == core::bignum? 'H': 'S');
                write_int(stream(), size.value(), true);
            }
            void string_data_(const core::value &v, bool) {stream().write(v.get_string_unchecked().data(), v.get_string_unchecked().size());}

            void begin_array_(const core::value &, core::optional_size, bool) {stream().put('[');}
            void end_array_(const core::value &, bool) {stream().put(']');}

            void begin_object_(const core::value &, core::optional_size, bool) {stream().put('{');}
            void end_object_(const core::value &, bool) {stream().put('}');}

            void link_(const core::value &) {throw core::error("UBJSON - 'link' value not allowed in output");}
        };

        inline core::value from_ubjson(core::istream_handle stream)
        {
            parser reader(stream);
            core::value v;
            core::convert(reader, v);
            return v;
        }

#ifdef CPPDATALIB_CPP11
        inline core::value operator "" _ubjson(const char *stream, size_t size)
        {
            core::istringstream wrap(std::string(stream, size));
            return from_ubjson(wrap);
        }
#endif

        inline std::string to_ubjson(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer writer(stream);
            core::convert(writer, v);
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_UBJSON_H

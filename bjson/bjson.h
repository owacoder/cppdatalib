/*
 * bjson.h
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

#ifndef CPPDATALIB_BJSON_H
#define CPPDATALIB_BJSON_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace bjson
    {
        /* TODO: doesn't really support core::iencodingstream formats other than `raw` */
        class parser : public core::stream_parser
        {
            struct container_data
            {
                container_data(core::subtype_t sub_type, uint64_t remaining_size)
                    : sub_type(sub_type)
                    , remaining_size(remaining_size)
                {}

                core::subtype_t sub_type;
                uint64_t remaining_size;
            };

            std::unique_ptr<char []> buffer;
            std::stack<container_data, std::vector<container_data>> containers;
            bool written;

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
            void read_string(core::subtype_t subtype, uint64_t size, const char *failure_message)
            {
                core::value string_type = core::value("", 0, subtype, true);
                get_output()->begin_string(string_type, size);
                while (size > 0)
                {
                    uint64_t buffer_size = std::min(uint64_t(core::buffer_size), size);
                    stream().read(buffer.get(), buffer_size);
                    if (stream().fail())
                        throw core::error(failure_message);
                    // Set string in string_type to preserve the subtype
                    string_type = core::value(buffer.get(), static_cast<size_t>(buffer_size), string_type.get_subtype(), true);
                    get_output()->append_to_string(string_type);
                    size -= buffer_size;
                }
                string_type = core::value("", 0, string_type.get_subtype(), true);
                get_output()->end_string(string_type);
            }

            void reset_()
            {
                containers = decltype(containers)();
                written = false;
            }

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
                    throw core::error("BJSON - unexpected end of stream, expected type specifier");

                switch (chr)
                {
                    // Null
                    case 0: get_output()->write(core::value()); break;
                    // Boolean false
                    case 1:
                    case 24:
                        get_output()->write(core::value(false)); break;
                    // Empty UTF-8 string
                    case 2: get_output()->write(core::value("", 0, core::normal, true)); break;
                    // Boolean true
                    case 3:
                    case 25:
                        get_output()->write(core::value(true)); break;
                    // Positive numbers
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    {
                        // This is just a fancy jump table, basically. It calls the correct read function based on the index used
                        core::istream &(*call[])(core::istream &, core::uint_t &) = {core::read_uint8<core::uint_t>,
                                                                                     core::read_uint16_le<core::uint_t>,
                                                                                     core::read_uint32_le<core::uint_t>,
                                                                                     core::read_uint64_le<core::uint_t>};

                        core::uint_t val = 0;
                        if (!call[chr - 4](stream(), val))
                            throw core::error("BJSON - expected 'uinteger'");
                        get_output()->write(core::value(val));
                        break;
                    }
                    // Negative numbers
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    {
                        // This is just a fancy jump table, basically. It calls the correct read function based on the index used
                        core::istream &(*call[])(core::istream &, core::uint_t &) = {core::read_uint8<core::uint_t>,
                                                                                     core::read_uint16_le<core::uint_t>,
                                                                                     core::read_uint32_le<core::uint_t>,
                                                                                     core::read_uint64_le<core::uint_t>};

                        core::uint_t val = 0;
                        if (!call[chr - 8](stream(), val))
                            throw core::error("BJSON - expected 'uinteger'");

                        if (val > core::uint_t(std::numeric_limits<core::int_t>::max()))
                            get_output()->write(core::value("-" + std::to_string(val), core::bignum));
                        else
                            get_output()->write(core::value(-core::int_t(val)));

                        break;
                    }
                    // Single-precision floating-point
                    case 12:
                    case 14:
                    {
                        uint32_t flt;
                        if (!core::read_uint32_le(stream(), flt))
                            throw core::error("BJSON - expected 'float' value");
                        get_output()->write(core::value(core::float_from_ieee_754(flt)));
                        break;
                    }
                    // Double-precision floating-point
                    case 13:
                    case 15:
                    {
                        uint64_t flt;
                        if (!core::read_uint64_le(stream(), flt))
                            throw core::error("BJSON - expected 'float' value");
                        get_output()->write(core::value(core::double_from_ieee_754(flt)));
                        break;
                    }
                    // UTF-8 strings
                    case 16:
                    case 17:
                    case 18:
                    case 19:
                    {
                        // This is just a fancy jump table, basically. It calls the correct read function based on the index used
                        core::istream &(*call[])(core::istream &, uint64_t &) = {core::read_uint8<uint64_t>,
                                                                                 core::read_uint16_le<uint64_t>,
                                                                                 core::read_uint32_le<uint64_t>,
                                                                                 core::read_uint64_le<uint64_t>};

                        uint64_t size = 0;

                        if (!call[chr - 16](stream(), size))
                            throw core::error("BJSON - expected UTF-8 string length");

                        read_string(core::normal, size, "BJSON - unexpected end of UTF-8 string");
                        break;
                    }
                    // Binary strings
                    case 20:
                    case 21:
                    case 22:
                    case 23:
                    {
                        // This is just a fancy jump table, basically. It calls the correct read function based on the index used
                        core::istream &(*call[])(core::istream &, uint64_t &) = {core::read_uint8<uint64_t>,
                                                                                 core::read_uint16_le<uint64_t>,
                                                                                 core::read_uint32_le<uint64_t>,
                                                                                 core::read_uint64_le<uint64_t>};

                        uint64_t size = 0;

                        if (!call[chr - 20](stream(), size))
                            throw core::error("BJSON - expected binary string length");

                        read_string(core::blob, size, "BJSON - unexpected end of binary string");
                        break;
                    }
                    // Strict small integers
                    case 26: get_output()->write(core::uint_t(0)); break;
                    case 27: get_output()->write(core::uint_t(1)); break;
                    // Arrays
                    case 32:
                    case 33:
                    case 34:
                    case 35:
                    {
                        // This is just a fancy jump table, basically. It calls the correct read function based on the index used
                        core::istream &(*call[])(core::istream &, uint64_t &) = {core::read_uint8<uint64_t>,
                                                                                 core::read_uint16_le<uint64_t>,
                                                                                 core::read_uint32_le<uint64_t>,
                                                                                 core::read_uint64_le<uint64_t>};

                        uint64_t size = 0;

                        if (!call[chr - 32](stream(), size))
                            throw core::error("BJSON - expected 'array' length");

                        get_output()->begin_array(core::array_t(), size);
                        containers.push(container_data(core::normal, size));
                        break;
                    }
                    // Maps
                    case 36:
                    case 37:
                    case 38:
                    case 39:
                    {
                        // This is just a fancy jump table, basically. It calls the correct read function based on the index used
                        core::istream &(*call[])(core::istream &, uint64_t &) = {core::read_uint8<uint64_t>,
                                                                                 core::read_uint16_le<uint64_t>,
                                                                                 core::read_uint32_le<uint64_t>,
                                                                                 core::read_uint64_le<uint64_t>};

                        uint64_t size = 0;

                        if (!call[chr - 36](stream(), size))
                            throw core::error("BJSON - expected 'object' length");

                        get_output()->begin_object(core::object_t(), size);
                        containers.push(container_data(core::normal, size));
                        break;
                    }
                    default:
                        throw core::error("BJSON - unknown type specifier encountered");
                }

                written = true;
            }
        };

        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(core::ostream_handle output) : core::stream_writer(output) {}

            protected:
                core::ostream &write_size(core::ostream &stream, int initial_type, uint64_t size)
                {
                    if (size >= UINT32_MAX)
                    {
                        stream.put(initial_type + 3);
                        return core::write_uint64_le(stream, size);
                    }
                    else if (size >= UINT16_MAX)
                    {
                        stream.put(initial_type + 2);
                        return core::write_uint32_le(stream, size);
                    }
                    else if (size >= UINT8_MAX)
                    {
                        stream.put(initial_type + 1);
                        return core::write_uint16_le(stream, size);
                    }
                    else
                    {
                        stream.put(initial_type);
                        return core::write_uint8(stream, size);
                    }
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
        public:
            stream_writer(core::ostream_handle output) : impl::stream_writer_base(output) {}

            unsigned int required_features() const {return requires_prefix_array_size | requires_prefix_object_size | requires_prefix_string_size;}

            std::string name() const {return "cppdatalib::bjson::stream_writer";}

        protected:
            void null_(const core::value &) {stream().put(0);}
            void bool_(const core::value &v) {stream().put(24 + v.get_bool_unchecked());}

            void integer_(const core::value &v)
            {
                if (v.get_int_unchecked() < 0)
                {
                    write_size(stream(), 8, -v.get_int_unchecked());
                }
                else if (v.get_int_unchecked() <= 1)
                {
                    stream().put(static_cast<char>(26 + v.get_uint_unchecked())); // Shortcut 0 and 1
                    return;
                }
                else /* positive integer */
                {
                    write_size(stream(), 4, v.get_int_unchecked());
                }
            }

            void uinteger_(const core::value &v)
            {
                if (v.get_uint_unchecked() <= 1)
                {
                    stream().put(static_cast<char>(26 + v.get_uint_unchecked())); // Shortcut 0 and 1
                    return;
                }

                write_size(stream(), 4, v.get_uint_unchecked());
            }

            void real_(const core::value &v)
            {
                uint64_t out;

                if (core::float_from_ieee_754(core::float_to_ieee_754(static_cast<float>(v.get_real_unchecked()))) == v.get_real_unchecked() || std::isnan(v.get_real_unchecked()))
                {
                    out = core::float_to_ieee_754(static_cast<float>(v.get_real_unchecked()));
                    stream().put(14);
                    core::write_uint32_le(stream(), out);
                }
                else
                {
                    out = core::double_to_ieee_754(v.get_real_unchecked());
                    stream().put(15);
                    core::write_uint64_le(stream(), out);
                }
            }

            void begin_string_(const core::value &v, core::optional_size size, bool)
            {
                int initial_type = 16;

                if (size.empty())
                    throw core::error("BJSON - 'string' value does not have size specified");
                else if (size.value() == 0 && core::subtype_is_text_string(v.get_subtype()))
                {
                    stream().put(2); // Empty string type
                    return;
                }

                if (!core::subtype_is_text_string(v.get_subtype()))
                    initial_type += 4;

                write_size(stream(), initial_type, size.value());
            }
            void string_data_(const core::value &v, bool) {stream() << v.get_string_unchecked();}

            void begin_array_(const core::value &, core::optional_size size, bool)
            {
                if (size.empty())
                    throw core::error("BJSON - 'array' value does not have size specified");

                write_size(stream(), 32, size.value());
            }

            void begin_object_(const core::value &, core::optional_size size, bool)
            {
                if (size.empty())
                    throw core::error("BJSON - 'object' value does not have size specified");

                write_size(stream(), 36, size.value());
            }
        };

        inline std::string to_bjson(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer w(stream);
            w << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_BJSON_H

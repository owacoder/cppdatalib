/*
 * cbor.h
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

#ifndef CPPDATALIB_CBOR_H
#define CPPDATALIB_CBOR_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace cbor
    {
        // TODO: WIP. Doesn't work entirely yet. No semantic support for tags has been implemented.
        /* TODO: doesn't really support core::iencodingstream formats other than `raw` */
        class parser : public core::stream_parser
        {
            struct container_data
            {
                container_data(core::subtype_t sub_type = core::normal, core::optional_size remaining_size = core::optional_size())
                    : sub_type(sub_type)
                    , remaining_size(remaining_size)
                {}

                core::subtype_t sub_type;
                core::optional_size remaining_size;
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
            void read_string(core::subtype_t subtype, uint64_t size, const char *failure_message)
            {
                bool string_already_existed = get_output()->current_container() == core::string;

                core::value string_type = core::value("", 0, subtype, true);

                if (!string_already_existed)
                    get_output()->begin_string(string_type, size);

                while (size > 0)
                {
                    uint64_t buffer_size = std::min(uint64_t(core::buffer_size), size);
                    stream().read(buffer, buffer_size);
                    if (stream().fail())
                        throw core::error(failure_message);
                    // Set string in string_type to preserve the subtype
                    string_type = core::value(buffer, static_cast<size_t>(buffer_size), string_type.get_subtype(), true);
                    get_output()->append_to_string(string_type);
                    size -= buffer_size;
                }
                string_type = core::value("", 0, string_type.get_subtype(), true);

                if (!string_already_existed)
                    get_output()->end_string(string_type);
            }

            void reset_()
            {
                containers = containers_t();
                written = false;
            }

#ifdef CPPDATALIB_WATCOM
            bool read_int64(core::istream &strm, size_t idx, uint64_t &result)
            {
                switch (idx)
                {
                    case 0: return core::read_uint8<uint64_t>(strm, result);
                    case 1: return core::read_uint16_be<uint64_t>(strm, result);
                    case 2: return core::read_uint32_be<uint64_t>(strm, result);
                    case 3: return core::read_uint64_be<uint64_t>(strm, result);
                    default: return false;
                }
            }
#else
            bool read_int64(core::istream &strm, size_t idx, uint64_t &result)
            {
                // This is just a fancy jump table, basically. It calls the correct read function based on the index used
                core::istream &(*call[])(core::istream &, uint64_t &) = {core::read_uint8<uint64_t>,
                                                                         core::read_uint16_be<uint64_t>,
                                                                         core::read_uint32_be<uint64_t>,
                                                                         core::read_uint64_be<uint64_t>};

                if (!call[idx](strm, result))
                    return false;

                return true;
            }
#endif

            void write_one_()
            {
                core::istream::int_type chr;

                while (containers.size() > 0 && !get_output()->container_key_was_just_parsed() && containers.top().remaining_size == core::optional_size(0))
                {
                    if (get_output()->current_container() == core::array)
                        get_output()->end_array(core::value(core::array_t(), containers.top().sub_type));
                    else if (get_output()->current_container() == core::object)
                        get_output()->end_object(core::value(core::object_t(), containers.top().sub_type));
                    containers.pop();
                }

                if (containers.size() > 0)
                {
                    if (containers.top().remaining_size.has_value() &&
                            containers.top().remaining_size.value() > 0 &&
                            (get_output()->current_container() != core::object || get_output()->container_key_was_just_parsed()))
                        containers.top().remaining_size = core::optional_size(containers.top().remaining_size.value() - 1);
                }
                else if (written)
                {
                    written = false;
                    return;
                }

                chr = stream().get();
                if (chr == EOF)
                    throw core::error("CBOR - unexpected end of stream, expected type specifier");
                else if (chr > 0xff)
                    throw core::error("CBOR - invalid input stream");

                unsigned int major_type = (chr & 0xff) >> 5;
                unsigned int sub_type = chr & 0x1f;
                uint64_t payload = 0;

                switch (sub_type)
                {
                    case 24: // Single byte of additional data
                    case 25:
                    case 26:
                    case 27:
                    {
                        if (!read_int64(stream(), sub_type - 24, payload))
                            throw core::error("CBOR - expected type payload");
                        break;
                    }
                    case 28:
                    case 29:
                    case 30:
                    case 31:
                        break; // Do nothing
                    default: // < 24
                        payload = sub_type;
                        break;
                }

                switch (major_type)
                {
                    // Positive integer
                    case 0: get_output()->write(core::value(payload)); break;
                    // Negative integer
                    case 1:
                    {
                        if (payload >= uint64_t(std::numeric_limits<core::int_t>::min()))
                            get_output()->write(core::value("-" + stdx::to_string(payload), core::bignum));
                        else
                            get_output()->write(core::value(-1 - core::int_t(payload)));
                        break;
                    }
                    // Binary string
                    case 2:
                        if (get_output()->current_container() == core::string && core::subtype_is_text_string(get_output()->current_container_subtype()))
                            throw core::error("CBOR - indefinite-length binary string must have chunks of the same type as the master string");

                        if (sub_type != 31)
                            read_string(core::blob, payload, "CBOR - unexpected end of binary string");
                        else
                        {
                            get_output()->begin_string(core::value("", 0, core::blob, true), core::optional_size());
                            containers.push(container_data(core::blob, core::optional_size()));
                        }
                        break;
                    // UTF-8 string
                    case 3:
                        if (get_output()->current_container() == core::string && !core::subtype_is_text_string(get_output()->current_container_subtype()))
                            throw core::error("CBOR - indefinite-length UTF-8 string must have chunks of the same type as the master string");

                        if (sub_type != 31)
                            read_string(core::normal, payload, "CBOR - unexpected end of UTF-8 string");
                        else
                        {
                            get_output()->begin_string(core::value("", 0, core::normal, true), core::optional_size());
                            containers.push(container_data(core::normal, core::optional_size()));
                        }
                        break;
                    // Array
                    case 4:
                    {
                        core::optional_size size = sub_type == 31? core::optional_size(): core::optional_size(payload);
                        get_output()->begin_array(core::array_t(), size);
                        containers.push(container_data(core::normal, size));
                        break;
                    }
                    // Object
                    case 5:
                    {
                        core::optional_size size = sub_type == 31? core::optional_size(): core::optional_size(payload);
                        get_output()->begin_object(core::object_t(), size);
                        containers.push(container_data(core::normal, size));
                        break;
                    }
                    // Tag
                    case 6:
                        break;
                    // Simple values
                    case 7:
                        switch (sub_type)
                        {
                            case 20: get_output()->write(core::value(false)); break;
                            case 21: get_output()->write(core::value(true)); break;
                            case 22: get_output()->write(core::value()); break;
                            case 23: get_output()->write(core::value(core::null_t(), core::undefined)); break;
                            case 24: break; // Not used
                            case 25: get_output()->write(core::value(core::float_from_ieee_754_half(uint16_t(payload)))); break;
                            case 26: get_output()->write(core::value(core::float_from_ieee_754(uint32_t(payload)))); break;
                            case 27: get_output()->write(core::value(core::double_from_ieee_754(payload))); break;
                            case 31:
                                if (get_output()->current_container() == core::array)
                                    get_output()->end_array(core::array_t());
                                else if (get_output()->current_container() == core::object)
                                    get_output()->end_object(core::object_t());
                                else if (get_output()->current_container() == core::string)
                                    get_output()->end_string(core::value("", 0, get_output()->current_container_subtype(), true));
                                break;
                            default: // < 20
                                break; // Not used
                        }
                        break;
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
                core::ostream &write_int(core::ostream &stream, unsigned int major_type, uint64_t integer)
                {
                    if (integer > std::numeric_limits<uint32_t>::max())
                    {
                        stream.put((major_type << 5) | 27);
                        return core::write_uint64_be(stream, integer);
                    }
                    else if (integer > std::numeric_limits<uint16_t>::max())
                    {
                        stream.put((major_type << 5) | 26);
                        return core::write_uint32_be(stream, integer);
                    }
                    else if (integer > std::numeric_limits<uint8_t>::max())
                    {
                        stream.put((major_type << 5) | 25);
                        return core::write_uint16_be(stream, integer);
                    }
                    else if (integer >= 24)
                    {
                        stream.put((major_type << 5) | 24);
                        return core::write_uint8(stream, integer);
                    }
                    else
                        return stream.put((major_type << 5) | uint8_t(integer));
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
            unsigned int major_type;
            bool require_prefix_sizes;

        public:
            stream_writer(core::ostream_handle output, bool require_prefix_sizes = true) : impl::stream_writer_base(output), major_type(0), require_prefix_sizes(require_prefix_sizes) {}

            unsigned int required_features() const {return require_prefix_sizes? requires_prefix_array_size | requires_prefix_object_size | requires_prefix_string_size: requires_none;}

            std::string name() const {return "cppdatalib::cbor::stream_writer";}

        protected:
            void null_(const core::value &v) {stream().put(0xf6 + v.get_subtype() == core::undefined);}
            void bool_(const core::value &v) {stream().put(0xf4 + v.get_bool_unchecked());}

            void integer_(const core::value &v)
            {
                if (v.get_int_unchecked() < 0)
                    write_int(stream(), 1, -(v.get_int_unchecked() + 1));
                else
                    write_int(stream(), 0, v.get_int_unchecked());
            }

            void uinteger_(const core::value &v)
            {
                write_int(stream(), 0, v.get_uint_unchecked());
            }

            void real_(const core::value &v)
            {
                uint64_t out;

                using namespace std;

                if (core::float_from_ieee_754_half(core::float_to_ieee_754_half(static_cast<float>(v.get_real_unchecked()))) == v.get_real_unchecked() || isnan(v.get_real_unchecked()))
                {
                    out = core::float_to_ieee_754_half(static_cast<float>(v.get_real_unchecked()));
                    stream().put(static_cast<unsigned char>(0xf9));
                    core::write_uint16_be(stream(), out);
                }
                else if (core::float_from_ieee_754(core::float_to_ieee_754(static_cast<float>(v.get_real_unchecked()))) == v.get_real_unchecked())
                {
                    out = core::float_to_ieee_754(static_cast<float>(v.get_real_unchecked()));
                    stream().put(static_cast<unsigned char>(0xfa));
                    core::write_uint32_be(stream(), out);
                }
                else
                {
                    out = core::double_to_ieee_754(v.get_real_unchecked());
                    stream().put(static_cast<unsigned char>(0xfb));
                    core::write_uint64_be(stream(), out);
                }
            }

            void begin_string_(const core::value &v, core::optional_size size, bool)
            {
                major_type = core::subtype_is_text_string(v.get_subtype())? 3: 2;

                if (size.has_value()) // Size specified in advance
                    write_int(stream(), major_type, size.value());
                else
                    stream().put((major_type << 5) | 31);
            }
            void string_data_(const core::value &v, bool)
            {
                // TODO: this implementation might split up UTF-8 characters, which is not recommended by the specification
                // Getting around this, however, might impose a hefty performance penalty
                if (!current_container_reported_size().has_value()) // No size specified for string, so output definite-sized chunks
                    write_int(stream(), major_type, v.size());

                stream() << v.get_string_unchecked();
            }
            void end_string_(const core::value &, bool)
            {
                if (!current_container_reported_size().has_value()) // No size specified for string, output `break` code
                    stream().put(static_cast<unsigned char>(0xff));
            }

            void begin_array_(const core::value &, core::optional_size size, bool)
            {
                if (size.has_value()) // Size specified in advance
                    write_int(stream(), 4, size.value());
                else
                    stream().put(static_cast<unsigned char>(0x9f));
            }
            void end_array_(const core::value &, bool)
            {
                if (!current_container_reported_size().has_value()) // No size specified for array, output `break` code
                    stream().put(static_cast<unsigned char>(0xff));
            }

            void begin_object_(const core::value &, core::optional_size size, bool)
            {
                if (size.has_value()) // Size specified in advance
                    write_int(stream(), 5, size.value());
                else
                    stream().put(static_cast<unsigned char>(0xbf));
            }
            void end_object_(const core::value &, bool)
            {
                if (!current_container_reported_size().has_value()) // No size specified for object, output `break` code
                    stream().put(static_cast<unsigned char>(0xff));
            }

            void link_(const core::value &) {throw core::error("CBOR - 'link' value not allowed in output");}
        };

        inline std::string to_cbor(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer w(stream);
            core::convert(w, v);
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_CBOR_H

/*
 * pson.h
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

#ifndef CPPDATALIB_PSON_H
#define CPPDATALIB_PSON_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace pson
    {
        /* TODO: doesn't really support core::iencodingstream formats other than `raw` */
        class parser : public core::stream_parser
        {
            struct container_data
            {
                container_data(core::subtype_t sub_type = core::normal, uint64_t remaining_size = 0)
                    : sub_type(sub_type)
                    , remaining_size(remaining_size)
                {}

                core::subtype_t sub_type;
                uint64_t remaining_size;
            };

            char * const buffer;
            typedef std::stack< container_data, std::vector<container_data> > containers_t;
            typedef std::map<uint32_t, core::string_t> dict_t;

            containers_t containers;
            dict_t string_dict, original_string_dict;
            bool written;

        public:
            parser(core::istream_handle input, const dict_t &string_dictionary = dict_t())
                : core::stream_parser(input)
                , buffer(new char [core::buffer_size])
                , string_dict(string_dictionary)
                , original_string_dict(string_dictionary)
            {
                reset();
            }
            ~parser() {delete[] buffer;}

            unsigned int features() const {return provides_prefix_array_size |
                                                  provides_prefix_object_size |
                                                  provides_prefix_string_size;}

        protected:
            void read_string(core::subtype_t subtype, uint64_t size, bool add_to_dict, const char *failure_message)
            {
                core::string_t *dict_entry = add_to_dict? stdx::addressof(string_dict.insert(dict_t::value_type(uint32_t(string_dict.size()), core::string_t())).first->second): nullptr;

                core::value string_type = core::value("", 0, subtype, true);
                get_output()->begin_string(string_type, size);
                while (size > 0)
                {
                    uint64_t buffer_size = std::min(uint64_t(core::buffer_size), size);
                    stream().read(buffer, buffer_size);
                    if (stream().fail())
                        throw core::error(failure_message);
                    // Set string in string_type to preserve the subtype
                    string_type = core::value(buffer, static_cast<size_t>(buffer_size), string_type.get_subtype(), true);
                    if (add_to_dict)
                        dict_entry->append(buffer, static_cast<size_t>(buffer_size));
                    get_output()->append_to_string(string_type);
                    size -= buffer_size;
                }
                string_type = core::value("", 0, string_type.get_subtype(), true);
                get_output()->end_string(string_type);
            }

            bool read_int(uint32_t &value)
            {
                unsigned int shift = 0;
                core::istream::int_type chr;
                value = 0;

                chr = stream().get();
                while (chr != EOF && chr >= 0x80)
                {
                    if (chr > 0xff || (shift == 28 && chr > 0xf))
                        return false;

                    value |= uint32_t(chr & 0x7f) << shift;
                    shift += 7;
                    chr = stream().get();
                }

                if (chr == EOF || (shift == 28 && chr > 0xf))
                    return false;

                value |= uint32_t(chr & 0x7f) << shift;

                return true;
            }

            bool read_int(int32_t &value)
            {
                uint32_t uvalue;

                if (!read_int(uvalue))
                    return false;

                value = (uvalue >> 1) ^ (~(uvalue & 1) + 1);

                return true;
            }

            bool read_int(int64_t &value)
            {
                unsigned int shift = 0;
                core::istream::int_type chr;
                uint64_t uvalue = 0;

                chr = stream().get();
                while (chr != EOF && chr >= 0x80)
                {
                    if (chr > 0xff || (shift == 63 && chr > 0x1))
                        return false;

                    uvalue |= uint64_t(chr & 0x7f) << shift;
                    shift += 7;
                    chr = stream().get();
                }

                if (chr == EOF || (shift == 63 && chr > 0x1))
                    return false;

                uvalue |= uint64_t(chr & 0x7f) << shift;
                value = (uvalue >> 1) ^ (~(uvalue & 1) + 1);

                return true;
            }

            void reset_()
            {
                containers = containers_t();
                written = false;

                string_dict = original_string_dict;
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
                    throw core::error("PSON - unexpected end of stream, expected type specifier");

                switch (chr)
                {
                    // Null
                    case 0xf0: get_output()->write(core::value()); break;
                    // Boolean
                    case 0xf1:
                    case 0xf2:
                        get_output()->write(core::value(core::bool_t(chr & 1))); break;
                    // Empty object
                    case 0xf3: get_output()->write(core::value(core::object_t())); break;
                    // Empty array
                    case 0xf4: get_output()->write(core::value(core::array_t())); break;
                    // Empty UTF-8 string
                    case 0xf5: get_output()->write(core::value("", 0, core::normal, true)); break;
                    // Maps
                    case 0xf6:
                    {
                        uint32_t size = 0;

                        if (!read_int(size))
                            throw core::error("PSON - expected 'object' length");

                        get_output()->begin_object(core::object_t(), size);
                        containers.push(container_data(core::normal, size));
                        break;
                    }
                    // Arrays
                    case 0xf7:
                    {
                        uint32_t size = 0;

                        if (!read_int(size))
                            throw core::error("PSON - expected 'array' length");

                        get_output()->begin_array(core::array_t(), size);
                        containers.push(container_data(core::normal, size));
                        break;
                    }
                    // 32-bit integers
                    case 0xf8:
                    {
                        int32_t size = 0;

                        if (!read_int(size))
                            throw core::error("PSON - expected integer");

                        get_output()->write(core::value(size));
                        break;
                    }
                    // 64-bit integers
                    case 0xf9:
                    {
                        int64_t size = 0;

                        if (!read_int(size))
                            throw core::error("PSON - expected integer");

                        get_output()->write(core::value(size));
                        break;
                    }
                    // 32-bit floating-point
                    case 0xfa:
                    {
                        uint32_t flt;

                        if (!core::read_uint32_le(stream(), flt))
                            throw core::error("PSON - expected 'float' value");

                        get_output()->write(core::value(core::float_from_ieee_754(flt)));
                        break;
                    }
                    // 64-bit floating-point
                    case 0xfb:
                    {
                        uint64_t flt;

                        if (!core::read_uint64_le(stream(), flt))
                            throw core::error("PSON - expected 'float' value");

                        get_output()->write(core::value(core::double_from_ieee_754(flt)));
                        break;
                    }
                    // UTF-8 strings
                    case 0xfc:
                    case 0xfd:
                    {
                        uint32_t size = 0;

                        if (!read_int(size))
                            throw core::error("PSON - expected UTF-8 string length");

                        read_string(core::normal, size, chr == 0xfd, "PSON - unexpected end of UTF-8 string");
                        break;
                    }
                    case 0xfe:
                    {
                        uint32_t spec = 0;

                        if (!read_int(spec))
                            throw core::error("PSON - expected UTF-8 string dictionary specifier");

                        dict_t::const_iterator it = string_dict.find(spec);
                        if (it == string_dict.end())
                            throw core::custom_error("PSON - " + stdx::to_string(spec) + " is not a valid string dictionary specifier");

                        get_output()->write(core::value(it->second.data(), it->second.size(), core::normal, true));
                        break;
                    }
                    // Binary strings
                    case 0xff:
                    {
                        uint32_t size = 0;

                        if (!read_int(size))
                            throw core::error("PSON - expected binary string length");

                        read_string(core::normal, size, chr == 0xfd, "PSON - unexpected end of binary string");
                        break;
                    }
                    default:
                    {
                        if (chr > 0xff)
                            throw core::error("PSON - unknown type specifier");

                        bool negative = chr & 1;
                        get_output()->write(core::value(core::int_t(negative? -(chr >> 1): (chr >> 1))));
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
                stream_writer_base(core::ostream_handle output) : core::stream_writer(output) {}

            protected:
                core::ostream &write_int(core::ostream &stream, int32_t size)
                {
                    uint32_t zigzag = size;

                    return write_int(stream, ((zigzag >> 31) * std::numeric_limits<uint32_t>::max()) ^ (zigzag << 1));
                }

                core::ostream &write_int(core::ostream &stream, int64_t size)
                {
                    uint64_t zigzag = size;

                    zigzag = ((zigzag >> 63) * std::numeric_limits<uint64_t>::max()) ^ (zigzag << 1);

                    while (zigzag > 0x7f)
                    {
                        stream.put(0x80 | (zigzag & 0x7f));
                        zigzag >>= 7;
                    }

                    stream.put(zigzag);
                    return stream;
                }

                core::ostream &write_int(core::ostream &stream, uint32_t zigzag)
                {
                    while (zigzag > 0x7f)
                    {
                        stream.put(0x80 | (zigzag & 0x7f));
                        zigzag >>= 7;
                    }

                    stream.put(zigzag);
                    return stream;
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
        public:
            stream_writer(core::ostream_handle output) : impl::stream_writer_base(output) {}

            unsigned int required_features() const {return requires_prefix_array_size | requires_prefix_object_size | requires_prefix_string_size;}

            std::string name() const {return "cppdatalib::pson::stream_writer";}

        protected:
            void null_(const core::value &) {stream().put(static_cast<unsigned char>(0xf0));}
            void bool_(const core::value &v) {stream().put(0xf2 - v.get_bool_unchecked());}

            void integer_(const core::value &v)
            {
                if (v.get_int_unchecked() < 0)
                {
                    if (v.get_int_unchecked() >= -120)
                        stream().put((-(v.get_int_unchecked() + 1) << 1) | 1);
                    else if (v.get_int_unchecked() >= std::numeric_limits<int32_t>::min())
                    {
                        stream().put(static_cast<unsigned char>(0xf8));
                        write_int(stream(), int32_t(v.get_int_unchecked()));
                    }
                    else if (v.get_int_unchecked() >= std::numeric_limits<int64_t>::min())
                    {
                        stream().put(static_cast<unsigned char>(0xf9));
                        write_int(stream(), int64_t(v.get_int_unchecked()));
                    }
                    else
                        throw core::error("PSON - 'integer' value is out-of-range");
                }
                else
                {
                    if (v.get_int_unchecked() < 120)
                        stream().put(v.get_int_unchecked() << 1);
                    else if (v.get_int_unchecked() <= std::numeric_limits<int32_t>::max())
                    {
                        stream().put(static_cast<unsigned char>(0xf8));
                        write_int(stream(), int32_t(v.get_int_unchecked()));
                    }
                    else if (v.get_int_unchecked() <= std::numeric_limits<int64_t>::max())
                    {
                        stream().put(static_cast<unsigned char>(0xf9));
                        write_int(stream(), int64_t(v.get_int_unchecked()));
                    }
                    else
                        throw core::error("PSON - 'integer' value is out-of-range");
                }
            }

            void uinteger_(const core::value &v)
            {
                if (v.get_uint_unchecked() < 120)
                    stream().put(v.get_uint_unchecked() << 1);
                else if (v.get_uint_unchecked() <= std::numeric_limits<int32_t>::max())
                {
                    stream().put(static_cast<unsigned char>(0xf8));
                    write_int(stream(), int32_t(v.get_uint_unchecked()));
                }
                else if (v.get_uint_unchecked() <= uintmax_t(std::numeric_limits<int64_t>::max()))
                {
                    stream().put(static_cast<unsigned char>(0xf9));
                    write_int(stream(), int64_t(v.get_uint_unchecked()));
                }
                else
                    throw core::error("PSON - 'integer' value is out-of-range");
            }

            void real_(const core::value &v)
            {
                uint64_t out;

                using namespace std;

                if (core::float_from_ieee_754(core::float_to_ieee_754(static_cast<float>(v.get_real_unchecked()))) == v.get_real_unchecked() || isnan(v.get_real_unchecked()))
                {
                    out = core::float_to_ieee_754(static_cast<float>(v.get_real_unchecked()));
                    stream().put(static_cast<unsigned char>(0xfa));
                    core::write_uint32_le(stream(), out);
                }
                else
                {
                    out = core::double_to_ieee_754(v.get_real_unchecked());
                    stream().put(static_cast<unsigned char>(0xfb));
                    core::write_uint64_le(stream(), out);
                }
            }

            void begin_string_(const core::value &v, core::optional_size size, bool)
            {
                if (!size.has_value())
                    throw core::error("PSON - 'string' value does not have size specified");
                else if (size.value() == 0 && core::subtype_is_text_string(v.get_subtype()))
                {
                    stream().put(static_cast<unsigned char>(0xf5)); // Empty string type
                    return;
                }

                if (!core::subtype_is_text_string(v.get_subtype()))
                    stream().put(static_cast<unsigned char>(0xff));
                else
                    stream().put(static_cast<unsigned char>(0xfc)); // TODO: no support for dictionary writing currently

                write_int(stream(), uint32_t(size.value()));
            }
            void string_data_(const core::value &v, bool) {stream() << v.get_string_unchecked();}

            void begin_array_(const core::value &, core::optional_size size, bool)
            {
                if (!size.has_value())
                    throw core::error("PSON - 'array' value does not have size specified");
                else if (size.value() > std::numeric_limits<uint32_t>::max())
                    throw core::error("PSON - 'array' size is too large");

                if (size.value() == 0)
                    stream().put(static_cast<unsigned char>(0xf4));
                else
                {
                    stream().put(static_cast<unsigned char>(0xf7));
                    write_int(stream(), uint32_t(size.value()));
                }
            }

            void begin_object_(const core::value &, core::optional_size size, bool)
            {
                if (!size.has_value())
                    throw core::error("PSON - 'object' value does not have size specified");
                else if (size.value() > std::numeric_limits<uint32_t>::max())
                    throw core::error("PSON - 'object' size is too large");

                if (size.value() == 0)
                    stream().put(static_cast<unsigned char>(0xf3));
                else
                {
                    stream().put(static_cast<unsigned char>(0xf6));
                    write_int(stream(), uint32_t(size.value()));
                }
            }

            void link_(const core::value &) {throw core::error("PSON - 'link' value not allowed in output");}
        };

        inline std::string to_pson(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer w(stream);
            core::convert(w, v);
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_PSON_H

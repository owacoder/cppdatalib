#ifndef CPPDATALIB_MESSAGE_PACK_H
#define CPPDATALIB_MESSAGE_PACK_H

#include "../core/core.h"

namespace cppdatalib
{
    // TODO: per stream, change single character writes to put() calls, and string writes to write() calls

    namespace message_pack
    {
        inline std::istream &read_int(std::istream &stream, core::int_t &i, char specifier)
        {
            uint64_t temp;
            bool negative = false;
            int c = stream.get();

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

                    c = stream.get();
                    if (c == EOF) throw core::error("UBJSON - expected integer value after type specifier");

                    temp = (temp << 8) | (c & 0xff);
                    temp |= negative * 0xffffffffffff0000ull;
                    break;
                case 'l': // Signed double-word
                    negative = c >> 7;

                    for (int i = 0; i < 3; ++i)
                    {
                        c = stream.get();
                        if (c == EOF) throw core::error("UBJSON - expected integer value after type specifier");

                        temp = (temp << 8) | (c & 0xff);
                    }
                    temp |= negative * 0xffffffff00000000ull;
                    break;
                case 'L': // Signed double-word
                    negative = c >> 7;

                    for (int i = 0; i < 7; ++i)
                    {
                        c = stream.get();
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

            return stream;
        }

        inline std::istream &read_float(std::istream &stream, char specifier, core::stream_handler &writer)
        {
            core::real_t r;
            uint64_t temp;
            int c = stream.get();

            if (c == EOF) throw core::error("UBJSON - expected floating-point value after type specifier");
            temp = c & 0xff;

            if (specifier == 'd')
            {
                for (int i = 0; i < 3; ++i)
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("UBJSON - expected floating-point value after type specifier");

                    temp = (temp << 8) | (c & 0xff);
                }
            }
            else if (specifier == 'D')
            {
                for (int i = 0; i < 7; ++i)
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("UBJSON - expected floating-point value after type specifier");

                    temp = (temp << 8) | (c & 0xff);
                }
            }
            else
                throw core::error("UBJSON - invalid floating-point specifier found in input");

            if (specifier == 'd')
                r = core::float_from_ieee_754(temp);
            else
                r = core::double_from_ieee_754(temp);

            writer.write(r);
            return stream;
        }

        inline std::istream &read_string(std::istream &stream, char specifier, core::stream_handler &writer)
        {
            char buffer[core::buffer_size];
            int c = stream.get();

            if (c == EOF) throw core::error("UBJSON - expected string value after type specifier");

            if (specifier == 'C')
            {
                writer.begin_string(core::string_t(), 1);
                writer.write(std::string(1, c));
                writer.end_string(core::string_t());
            }
            else if (specifier == 'H')
            {
                core::int_t size;

                read_int(stream, size, c);
                if (size < 0) throw core::error("UBJSON - invalid negative size specified for high-precision number");

                writer.begin_string(core::value(core::string_t(), core::bignum), size);
                while (size > 0)
                {
                    core::int_t buffer_size = std::min(core::int_t(core::buffer_size), size);
                    stream.read(buffer, buffer_size);
                    if (stream.fail())
                        throw core::error("UBJSON - expected high-precision number value after type specifier");
                    writer.append_to_string(core::string_t(buffer, buffer_size));
                    size -= buffer_size;
                }
                writer.end_string(core::value(core::string_t(), core::bignum));
            }
            else if (specifier == 'S')
            {
                core::int_t size;

                read_int(stream, size, c);
                if (size < 0) throw core::error("UBJSON - invalid negative size specified for string");

                writer.begin_string(core::string_t(), size);
                while (size > 0)
                {
                    core::int_t buffer_size = std::min(core::int_t(core::buffer_size), size);
                    stream.read(buffer, buffer_size);
                    if (stream.fail())
                        throw core::error("UBJSON - expected string value after type specifier");
                    writer.append_to_string(core::string_t(buffer, buffer_size));
                    size -= buffer_size;
                }
                writer.end_string(core::string_t());
            }
            else
                throw core::error("UBJSON - invalid string specifier found in input");

            return stream;
        }

        inline std::ostream &write_int(std::ostream &stream, core::uint_t i)
        {
            if (i <= UINT8_MAX / 2)
                return stream.put(i);
            else if (i <= UINT8_MAX)
                return stream.put(0xcc).put(i);
            else if (i <= UINT16_MAX)
                return stream.put(0xcd).put(i >> 8).put(i & 0xff);
            else if (i <= UINT32_MAX)
                return stream.put(0xce)
                        .put(i >> 24)
                        .put((i >> 16) & 0xff)
                        .put((i >> 8) & 0xff)
                        .put(i & 0xff);
            else
                return stream.put(0xcf)
                        .put((i >> 56) & 0xff)
                        .put((i >> 48) & 0xff)
                        .put((i >> 40) & 0xff)
                        .put((i >> 32) & 0xff)
                        .put((i >> 24) & 0xff)
                        .put((i >> 16) & 0xff)
                        .put((i >> 8) & 0xff)
                        .put(i & 0xff);
        }

        inline std::ostream &write_int(std::ostream &stream, core::int_t i)
        {
            if (i >= 0)
            {
                if (i <= UINT8_MAX / 2)
                    return stream.put(i);
                else if (i <= UINT8_MAX)
                    return stream.put(0xcc).put(i);
                else if (i <= UINT16_MAX)
                    return stream.put(0xcd).put(i >> 8).put(i & 0xff);
                else if (i <= UINT32_MAX)
                    return stream.put(0xce)
                            .put(i >> 24)
                            .put((i >> 16) & 0xff)
                            .put((i >> 8) & 0xff)
                            .put(i & 0xff);
                else
                    return stream.put(0xcf)
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
                    return stream.put(0xd0).put(0x80 | (temp & 0xff));
                else if (i <= INT16_MAX)
                    return stream.put(0xd1).put(0x80 | ((temp >> 8) & 0xff)).put(temp & 0xff);
                else if (i <= INT32_MAX)
                    return stream.put(0xd2)
                            .put(0x80 | ((temp >> 24) & 0xff))
                            .put((temp >> 16) & 0xff)
                            .put((temp >> 8) & 0xff)
                            .put(temp & 0xff);
                else
                    return stream.put(0xd3)
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

        inline std::ostream &write_float(std::ostream &stream, core::real_t f)
        {
            if (core::float_from_ieee_754(core::float_to_ieee_754(f)) == f || std::isnan(f))
            {
                uint32_t temp = core::float_to_ieee_754(f);

                return stream.put(0xca)
                        .put((temp >> 24) & 0xff)
                        .put((temp >> 16) & 0xff)
                        .put((temp >> 8) & 0xff)
                        .put(temp & 0xff);
            }
            else
            {
                uint64_t temp = core::double_to_ieee_754(f);

                return stream.put(0xcb)
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

        inline std::ostream &write_string_size(std::ostream &stream, size_t str_size, core::subtype_t subtype)
        {
            // Binary string?
            if (subtype == core::blob)
            {
                if (str_size <= UINT8_MAX)
                    return stream.put(0xc4).put(str_size);
                else if (str_size <= UINT16_MAX)
                    return stream.put(0xc5)
                            .put(str_size >> 8)
                            .put(str_size & 0xff);
                else if (str_size <= UINT32_MAX)
                    return stream.put(0xc6)
                            .put(str_size >> 24)
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
                    return stream.put(0xa0 + str_size);
                else if (str_size <= UINT8_MAX)
                    return stream.put(0xd9).put(str_size);
                else if (str_size <= UINT16_MAX)
                    return stream.put(0xda)
                            .put(str_size >> 8)
                            .put(str_size & 0xff);
                else if (str_size <= UINT32_MAX)
                    return stream.put(0xdb)
                            .put(str_size >> 24)
                            .put((str_size >> 16) & 0xff)
                            .put((str_size >> 8) & 0xff)
                            .put(str_size & 0xff);
                else
                    throw core::error("MessagePack - 'string' value is too long");
            }

            // TODO: handle user-specified string types
            return stream;
        }

        inline std::istream &convert(std::istream &stream, core::stream_handler &writer)
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

            const char valid_types[] = "ZTFUiIlLdDCHS[{";
            std::stack<container_data, std::vector<container_data>> containers;
            bool written = false;
            int chr;

            writer.begin();

            while (!written || writer.nesting_depth() > 0)
            {
                if (containers.size() > 0)
                {
                    if (containers.top().content_type)
                        chr = containers.top().content_type;
                    else
                    {
                        chr = stream.get();
                        if (chr == EOF) break;
                    }

                    if (containers.top().remaining_size > 0 && !writer.container_key_was_just_parsed())
                        --containers.top().remaining_size;
                    if (writer.current_container() == core::object && chr != 'N' && chr != '}' && !writer.container_key_was_just_parsed())
                    {
                        // Parse key here, remap read character to 'N' (the no-op instruction)
                        if (!containers.top().content_type)
                            stream.unget();
                        read_string(stream, 'S', writer);
                        chr = 'N';
                    }
                }
                else
                {
                    chr = stream.get();
                    if (chr == EOF) break;
                }

                written |= chr != 'N';

                switch (chr)
                {
                    case 'Z':
                        writer.write(core::null_t());
                        break;
                    case 'T':
                        writer.write(true);
                        break;
                    case 'F':
                        writer.write(false);
                        break;
                    case 'U':
                    case 'i':
                    case 'I':
                    case 'l':
                    case 'L':
                    {
                        core::int_t i;
                        read_int(stream, i, chr);
                        writer.write(i);
                        break;
                    }
                    case 'd':
                    case 'D':
                        read_float(stream, chr, writer);
                        break;
                    case 'C':
                    case 'H':
                    case 'S':
                        read_string(stream, chr, writer);
                        break;
                    case 'N': break;
                    case '[':
                    {
                        int type = 0;
                        core::int_t size = -1;

                        chr = stream.get();
                        if (chr == EOF) throw core::error("UBJSON - expected array value after '['");

                        if (chr == '$') // Type specified
                        {
                            chr = stream.get();
                            if (chr == EOF || !strchr(valid_types, chr)) throw core::error("UBJSON - expected type specifier after '$'");
                            type = chr;
                            chr = stream.get();
                            if (chr == EOF) throw core::error("UBJSON - unexpected end of array");
                        }

                        if (chr == '#') // Count specified
                        {
                            chr = stream.get();
                            if (chr == EOF) throw core::error("UBJSON - expected count specifier after '#'");

                            read_int(stream, size, chr);
                            if (size < 0) throw core::error("UBJSON - invalid negative size specified for array");
                        }

                        // If type != 0, then size must be >= 0
                        if (type != 0 && size < 0)
                            throw core::error("UBJSON - array element type specified but number of elements is not specified");
                        else if (size < 0) // Unless a count was read, one character needs to be put back (from checking chr == '#')
                            stream.unget();

                        writer.begin_array(core::array_t(), size >= 0? size: core::int_t(core::stream_handler::unknown_size));
                        containers.push(container_data(type, size));

                        break;
                    }
                    case ']':
                        if (!containers.empty() && containers.top().remaining_size >= 0)
                            throw core::error("UBJSON - attempted to end an array with size specified already");

                        writer.end_array(core::array_t());
                        containers.pop();
                        break;
                    case '{':
                    {
                        int type = 0;
                        core::int_t size = -1;

                        chr = stream.get();
                        if (chr == EOF) throw core::error("UBJSON - expected object value after '{'");

                        if (chr == '$') // Type specified
                        {
                            chr = stream.get();
                            if (chr == EOF || !strchr(valid_types, chr)) throw core::error("UBJSON - expected type specifier after '$'");
                            type = chr;
                            chr = stream.get();
                            if (chr == EOF) throw core::error("UBJSON - unexpected end of object");
                        }

                        if (chr == '#') // Count specified
                        {
                            chr = stream.get();
                            if (chr == EOF) throw core::error("UBJSON - expected count specifier after '#'");

                            read_int(stream, size, chr);
                            if (size < 0) throw core::error("UBJSON - invalid negative size specified for object");
                        }

                        // If type != 0, then size must be >= 0
                        if (type != 0 && size < 0)
                            throw core::error("UBJSON - object element type specified but number of elements is not specified");
                        else if (size < 0) // Unless a count was read, one character needs to be put back (from checking chr == '#')
                            stream.unget();

                        writer.begin_object(core::object_t(), size >= 0? size: core::int_t(core::stream_handler::unknown_size));
                        containers.push(container_data(type, size));

                        break;
                    }
                    case '}':
                        if (!containers.empty() && containers.top().remaining_size >= 0)
                            throw core::error("UBJSON - attempted to end an object with size specified already");

                        writer.end_object(core::object_t());
                        containers.pop();
                        break;
                    default:
                        throw core::error("UBJSON - expected value");
                }

                if (containers.size() > 0 && !writer.container_key_was_just_parsed() && containers.top().remaining_size == 0)
                {
                    if (writer.current_container() == core::array)
                        writer.end_array(core::array_t());
                    else if (writer.current_container() == core::object)
                        writer.end_object(core::object_t());
                    containers.pop();
                }
            }

            if (!written)
                throw core::error("UBJSON - expected value");
            else if (containers.size() > 0)
                throw core::error("UBJSON - unexpected end of data");

            writer.end();
            return stream;
        }

        class stream_writer : public core::stream_handler, public core::stream_writer
        {
        public:
            stream_writer(std::ostream &output) : core::stream_writer(output) {}

        protected:
            void null_(const core::value &) {output_stream.put(0xc0);}
            void bool_(const core::value &v) {output_stream.put(0xc2 + v.get_bool());}
            void integer_(const core::value &v) {write_int(output_stream, v.get_int());}
            void uinteger_(const core::value &v) {write_int(output_stream, v.get_uint());}
            void real_(const core::value &v) {write_float(output_stream, v.get_real());}
            void begin_string_(const core::value &v, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("MessagePack - 'string' value does not have size specified");

                write_string_size(output_stream, size, v.get_subtype());
            }
            void string_data_(const core::value &v) {output_stream.write(v.get_string().c_str(), v.get_string().size());}

            void begin_array_(const core::value &, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("MessagePack - 'array' value does not have size specified");
                else if (size <= 15)
                    output_stream.put(0x90 + size);
                else if (size <= UINT16_MAX)
                    output_stream.put(0xdc).put(size >> 8).put(size & 0xff);
                else if (size <= UINT32_MAX)
                    output_stream.put(0xdd)
                            .put(size >> 24)
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
                    output_stream.put(0x80 + size);
                else if (size <= UINT16_MAX)
                    output_stream.put(0xde).put(size >> 8).put(size & 0xff);
                else if (size <= UINT32_MAX)
                    output_stream.put(0xdf)
                            .put(size >> 24)
                            .put((size >> 16) & 0xff)
                            .put((size >> 8) & 0xff)
                            .put(size & 0xff);
                else
                    throw core::error("MessagePack - 'object' value is too long");
            }
        };

        inline std::ostream &print(std::ostream &stream, const core::value &v)
        {
            stream_writer writer(stream);
            core::convert(v, writer);
            return stream;
        }

        inline std::istream &input(std::istream &stream, core::value &v)
        {
            core::value_builder builder(v);
            convert(stream, builder);
            return stream;
        }

        inline std::istream &operator>>(std::istream &stream, core::value &v) {return input(stream, v);}
        inline std::ostream &operator<<(std::ostream &stream, const core::value &v) {return print(stream, v);}

        inline core::value from_message_pack(const std::string &msgpack)
        {
            std::istringstream stream(msgpack);
            core::value v;
            stream >> v;
            return v;
        }

        inline std::string to_message_pack(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_MESSAGE_PACK_H

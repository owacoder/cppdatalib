#ifndef CPPDATALIB_UBJSON_H
#define CPPDATALIB_UBJSON_H

#include "../core/value_builder.h"

namespace cppdatalib
{
    namespace ubjson
    {
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
                while (size-- > 0)
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("UBJSON - expected high-precision number value after type specifier");

                    writer.append_to_string(std::string(1, c));
                }
                writer.end_string(core::value(core::string_t(), core::bignum));
            }
            else if (specifier == 'S')
            {
                core::int_t size;

                read_int(stream, size, c);
                if (size < 0) throw core::error("UBJSON - invalid negative size specified for string");

                writer.begin_string(core::string_t(), size);
                while (size-- > 0)
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("UBJSON - expected string value after type specifier");

                    writer.append_to_string(std::string(1, c));
                }
                writer.end_string(core::string_t());
            }
            else
                throw core::error("UBJSON - invalid string specifier found in input");

            return stream;
        }

        inline std::ostream &write_int(std::ostream &stream, core::int_t i, bool add_specifier, char force_specifier = 0)
        {
            const std::string specifiers = "UiIlL";
            size_t force_bits = specifiers.find(force_specifier);

            if (force_bits == std::string::npos)
                force_bits = 0;

            if (force_bits == 0 && (i >= 0 && i <= UINT8_MAX))
                return stream << (add_specifier? "U": "") << static_cast<unsigned char>(i);
            else if (force_bits <= 1 && (i >= INT8_MIN && i < 0))
                return stream << (add_specifier? "i": "") << static_cast<unsigned char>(0x80 | ((~std::abs(i) & 0xff) + 1));
            else if (force_bits <= 2 && (i >= INT16_MIN && i <= INT16_MAX))
            {
                uint16_t t;

                if (i < 0)
                    t = 0x8000u | ((~std::abs(i) & 0xffffu) + 1);
                else
                    t = i;

                return stream << (add_specifier? "I": "") << static_cast<unsigned char>(t >> 8) <<
                                                             static_cast<unsigned char>(t & 0xff);
            }
            else if (force_bits <= 3 && (i >= INT32_MIN && i <= INT32_MAX))
            {
                uint32_t t;

                if (i < 0)
                    t = 0x80000000u | ((~std::abs(i) & 0xffffffffu) + 1);
                else
                    t = i;

                return stream << (add_specifier? "l": "") << static_cast<unsigned char>(t >> 24) <<
                                                             static_cast<unsigned char>((t >> 16) & 0xff) <<
                                                             static_cast<unsigned char>((t >>  8) & 0xff) <<
                                                             static_cast<unsigned char>((t      ) & 0xff);
            }
            else
            {
                uint64_t t;

                if (i < 0)
                    t = 0x8000000000000000u | ((~std::abs(i) & 0xffffffffffffffffu) + 1);
                else
                    t = i;

                return stream << (add_specifier? "L": "") << static_cast<unsigned char>(t >> 56) <<
                                                             static_cast<unsigned char>((t >> 48) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 40) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 32) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 24) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 16) & 0xff) <<
                                                             static_cast<unsigned char>((t >>  8) & 0xff) <<
                                                             static_cast<unsigned char>((t      ) & 0xff);
            }
        }

        inline std::ostream &write_float(std::ostream &stream, core::real_t f, bool add_specifier, char force_specifier = 0)
        {
            const std::string specifiers = "dD";
            size_t force_bits = specifiers.find(force_specifier);

            if (force_bits == std::string::npos)
                force_bits = 0;

            if (force_bits == 0 && (core::float_from_ieee_754(core::float_to_ieee_754(f)) == f || std::isnan(f)))
            {
                uint32_t t = core::float_to_ieee_754(f);

                return stream << (add_specifier? "d": "") << static_cast<unsigned char>(t >> 24) <<
                                                             static_cast<unsigned char>((t >> 16) & 0xff) <<
                                                             static_cast<unsigned char>((t >>  8) & 0xff) <<
                                                             static_cast<unsigned char>((t      ) & 0xff);
            }
            else
            {
                uint64_t t = core::double_to_ieee_754(f);

                return stream << (add_specifier? "L": "") << static_cast<unsigned char>(t >> 56) <<
                                                             static_cast<unsigned char>((t >> 48) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 40) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 32) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 24) & 0xff) <<
                                                             static_cast<unsigned char>((t >> 16) & 0xff) <<
                                                             static_cast<unsigned char>((t >>  8) & 0xff) <<
                                                             static_cast<unsigned char>((t      ) & 0xff);
            }
        }

        inline std::ostream &write_string(std::ostream &stream, const std::string &str, bool add_specifier, core::subtype_t subtype)
        {
            if (subtype != core::bignum && str.size() == 1 && static_cast<unsigned char>(str[0]) < 128)
                return stream << (add_specifier? "C": "") << str[0];

            if (add_specifier)
                stream << (subtype == core::bignum? 'H': 'S');

            write_int(stream, str.size(), true);

            return stream << str;
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
            void begin_key_(const core::value &v)
            {
                if (!v.is_string())
                    throw core::error("UBJSON - cannot write non-string key");
            }

            void null_(const core::value &) {output_stream << 'Z';}
            void bool_(const core::value &v) {output_stream << (v.get_bool()? 'T': 'F');}
            void integer_(const core::value &v) {write_int(output_stream, v.get_int(), true);}
            void real_(const core::value &v) {write_float(output_stream, v.get_real(), true);}
            void begin_string_(const core::value &v, core::int_t size, bool is_key)
            {
                if (size == unknown_size)
                    throw core::error("UBJSON - 'string' value does not have size specified");

                if (!is_key)
                    output_stream << (v.get_subtype() == core::bignum? 'H': 'S');
                write_int(output_stream, size, true);
            }
            void string_data_(const core::value &v) {output_stream << v.get_string();}

            void begin_array_(const core::value &, core::int_t, bool) {output_stream << '[';}
            void end_array_(const core::value &, bool) {output_stream << ']';}

            void begin_object_(const core::value &, core::int_t, bool) {output_stream << '{';}
            void end_object_(const core::value &, bool) {output_stream << '}';}
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

        inline core::value from_ubjson(const std::string &ubjson)
        {
            std::istringstream stream(ubjson);
            core::value v;
            stream >> v;
            return v;
        }

        inline std::string to_ubjson(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_UBJSON_H

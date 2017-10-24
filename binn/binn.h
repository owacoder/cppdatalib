#ifndef CPPDATALIB_BINN_H
#define CPPDATALIB_BINN_H

#include "../core/core.h"

// TODO: Refactor into stream_parser API and impl::stream_writer_base API

namespace cppdatalib
{
    // TODO: per stream, change single character writes to put() calls, and string writes to write() calls

    namespace binn
    {
        inline std::ostream &write_type(std::ostream &stream, unsigned int type, unsigned int subtype, int *written = NULL)
        {
            char c;

            if (written != NULL)
                *written = 1 + (subtype > 15);

            c = (type & 0x7) << 5;
            if (subtype > 15)
            {
                c = (c | 0x10) | ((subtype >> 8) & 0xf);
                return stream << c << static_cast<char>(subtype & 0xff);
            }

            c = c | subtype;
            return stream << c;
        }

        inline std::ostream &write_size(std::ostream &stream, uint64_t size, int *written = NULL)
        {
            if (written != NULL)
                *written = 1 + 3 * (size < 128);

            if (size < 128)
                return stream << static_cast<char>(size);

            return stream << static_cast<char>(((size >> 24) & 0xff) | 0x80)
                          << static_cast<char>((size >> 16) & 0xff)
                          << static_cast<char>((size >>  8) & 0xff)
                          << static_cast<char>( size        & 0xff);
        }

        inline size_t get_size(const core::value &v)
        {
            switch (v.get_type())
            {
                case core::null:
                case core::boolean: return 1 + (v.get_subtype() >= core::user && v.get_subtype() > 15);
                case core::integer:
                {
                    size_t size = 1; // one byte for type specifier

                    if (v.get_subtype() >= core::user && v.get_subtype() > 15)
                        ++size; // type specifier requires another byte

                    if (v.get_int() >= INT8_MIN && v.get_int() <= UINT8_MAX)
                        size += 1;
                    else if (v.get_int() >= INT16_MIN && v.get_int() <= UINT16_MAX)
                        size += 2;
                    else if (v.get_int() >= INT32_MIN && v.get_int() <= UINT32_MAX)
                        size += 4;
                    else
                        size += 8;

                    return size;
                }
                case core::uinteger:
                {
                    size_t size = 1; // one byte for type specifier

                    if (v.get_subtype() >= core::user && v.get_subtype() > 15)
                        ++size; // type specifier requires another byte

                    if (v.get_uint() <= UINT8_MAX)
                        size += 1;
                    else if (v.get_uint() <= UINT16_MAX)
                        size += 2;
                    else if (v.get_uint() <= UINT32_MAX)
                        size += 4;
                    else
                        size += 8;

                    return size;
                }
                case core::real:
                {
                    size_t size = 5; // one byte for type specifier, minimum of four bytes for data

                    // A user-specified subtype is not available for reals
                    // (because when the data is read again, the IEEE-754 representation will be put into an integer instead of a real,
                    // since there is nothing to show that the data should be read as a floating point number)
                    // To prevent the loss of data, the subtype is discarded and the value stays the same

                    if (core::float_from_ieee_754(core::float_to_ieee_754(v.get_real())) != v.get_real() && !std::isnan(v.get_real()))
                        size += 4; // requires more than 32-bit float to losslessly encode

                    return size;
                }
                case core::string:
                {
                    size_t size = 3; // one byte for the type specifier, one for the minimum size specifier of one byte, one for trailing nul

                    if (v.get_subtype() >= core::user && v.get_subtype() > 15)
                        ++size; // type specifier requires another byte

                    if (size + v.size() >= 128)
                        size += 3; // requires a four-byte size specifier

                    return size + v.get_string().size();
                }
                case core::array:
                {
                    size_t size = 3; // one byte for the type specifier,
                                     // one for the minimum size specifier of one byte,
                                     // and one for the minimum count specifier of one byte

                    if (v.get_subtype() >= core::user && v.get_subtype() > 15)
                        ++size; // type specifier requires another byte

                    if (v.size() >= 128)
                        size += 3; // requires a four-byte count specifier

                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        size += get_size(*it);

                    if (size >= 128)
                        size += 3; // requires a four-byte size specifier

                    return size;
                }
                case core::object:
                {
                    size_t size = 3; // one byte for the type specifier,
                                     // one for the minimum size specifier of one byte,
                                     // and one for the minimum count specifier of one byte

                    // A user-specified subtype is not available for objects
                    // (because when the data is read again, there is no way to determine the type of structure the container holds)
                    // To prevent the loss of data, the subtype is discarded and the value stays the same

                    if (v.size() >= 128)
                        size += 3; // requires a four-byte count specifier

                    if (v.get_subtype() == core::map)
                    {
                        for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                            size += 4 /* map ID */ + get_size(it->second) /* value size */;
                    }
                    else
                    {
                        for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                            size += 1 /* key size specifier */ + it->first.size() /* key size */ + get_size(it->second) /* value size */;
                    }

                    if (size >= 128)
                        size += 3; // requires a four-byte size specifier

                    return size;
                }
            }

            // Control will never get here
            return 0;
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            enum types
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

            enum subtypes
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

            switch (v.get_type())
            {
                case core::null: return write_type(stream, nobytes, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): null);
                case core::boolean: return write_type(stream, nobytes, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): v.get_bool()? yes: no);
                case core::integer:
                {
                    uint64_t out = std::abs(v.get_int());
                    if (v.get_int() < 0)
                        out = ~out + 1;

                    if (v.get_int() >= INT8_MIN && v.get_int() <= UINT8_MAX)
                        return write_type(stream, byte, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): v.get_int() < 0? int8: uint8) << static_cast<char>(out);
                    else if (v.get_int() >= INT16_MIN && v.get_int() <= UINT16_MAX)
                        return write_type(stream, word, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): v.get_int() < 0? int16: uint16)
                                << static_cast<char>(out >> 8)
                                << static_cast<char>(out & 0xff);
                    else if (v.get_int() >= INT32_MIN && v.get_int() <= UINT32_MAX)
                        return write_type(stream, dword, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): v.get_int() < 0? int32: uint32)
                                << static_cast<char>(out >> 24)
                                << static_cast<char>((out >> 16) & 0xff)
                                << static_cast<char>((out >> 8) & 0xff)
                                << static_cast<char>(out & 0xff);
                    else
                        return write_type(stream, qword, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): v.get_int() < 0? int64: uint64)
                                << static_cast<char>(out >> 56)
                                << static_cast<char>((out >> 48) & 0xff)
                                << static_cast<char>((out >> 40) & 0xff)
                                << static_cast<char>((out >> 32) & 0xff)
                                << static_cast<char>((out >> 24) & 0xff)
                                << static_cast<char>((out >> 16) & 0xff)
                                << static_cast<char>((out >> 8) & 0xff)
                                << static_cast<char>(out & 0xff);
                }
                case core::real:
                {
                    uint64_t out;

                    if (core::float_from_ieee_754(core::float_to_ieee_754(v.get_real())) == v.get_real() || std::isnan(v.get_real()))
                    {
                        out = core::float_to_ieee_754(v.get_real());
                        return write_type(stream, dword, single_float)
                                << static_cast<char>(out >> 24)
                                << static_cast<char>((out >> 16) & 0xff)
                                << static_cast<char>((out >> 8) & 0xff)
                                << static_cast<char>(out & 0xff);
                    }
                    else
                    {
                        out = core::double_to_ieee_754(v.get_real());
                        return write_type(stream, qword, double_float)
                                << static_cast<char>(out >> 56)
                                << static_cast<char>((out >> 48) & 0xff)
                                << static_cast<char>((out >> 40) & 0xff)
                                << static_cast<char>((out >> 32) & 0xff)
                                << static_cast<char>((out >> 24) & 0xff)
                                << static_cast<char>((out >> 16) & 0xff)
                                << static_cast<char>((out >> 8) & 0xff)
                                << static_cast<char>(out & 0xff);
                    }
                }
                case core::string:
                {
                    switch (v.get_subtype())
                    {
                        case core::date: write_type(stream, string, date); break;
                        case core::time: write_type(stream, string, time); break;
                        case core::datetime: write_type(stream, string, datetime); break;
                        case core::bignum: write_type(stream, string, decimal_str); break;
                        case core::blob: write_type(stream, blob, blob_data); break;
                        default: write_type(stream, string, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): text); break;
                    }

                    return write_size(stream, v.get_string().size()) << v.get_string() << static_cast<char>(0);
                }
                case core::array:
                {
                    write_type(stream, container, v.get_subtype() >= core::user? static_cast<subtypes>(v.get_subtype()): list);
                    write_size(stream, get_size(v));
                    write_size(stream, v.size());

                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        stream << *it;

                    return stream;
                }
                case core::object:
                {
                    write_type(stream, container, v.get_subtype() == core::map? map: object);
                    write_size(stream, get_size(v));
                    write_size(stream, v.size());

                    if (v.get_subtype() == core::map)
                    {
                        for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        {
                            int64_t key = it->first.get_int();
                            uint32_t out;

                            if (!it->first.is_int())
                                throw core::error("Binn - map key is not an integer");
                            else if (key < INT32_MIN || key > INT32_MAX)
                                throw core::error("Binn - map key is out of range");

                            out = std::abs(key);
                            if (key < 0)
                                out = ~out + 1;

                            stream << static_cast<char>(out >> 24) <<
                                      static_cast<char>((out >> 16) & 0xff) <<
                                      static_cast<char>((out >>  8) & 0xff) <<
                                      static_cast<char>(out & 0xff) <<
                                      it->second;
                        }
                    }
                    else
                    {
                        for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        {
                            if (!it->first.is_string())
                                throw core::error("Binn - object key is not a string");

                            if (it->first.size() > 255)
                                throw core::error("Binn - object key is larger than limit of 255 bytes");

                            stream << static_cast<char>(it->first.size()) << it->first.get_string() << it->second;
                        }
                    }

                    return stream;
                }
                default: break;
            }

            // Control will never get here
            return stream;
        }

        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline std::string to_binn(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_BINN_H

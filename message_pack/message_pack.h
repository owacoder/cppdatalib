#ifndef CPPDATALIB_MESSAGE_PACK_H
#define CPPDATALIB_MESSAGE_PACK_H

#include "../core/core.h"

namespace cppdatalib
{
    // TODO: per stream, change single character writes to put() calls, and string writes to write() calls

    namespace message_pack
    {
        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(std::ostream &stream) : core::stream_writer(stream) {}

            protected:
                std::ostream &write_int(std::ostream &stream, core::uint_t i)
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

                std::ostream &write_int(std::ostream &stream, core::int_t i)
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

                std::ostream &write_float(std::ostream &stream, core::real_t f)
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

                std::ostream &write_string_size(std::ostream &stream, size_t str_size, core::subtype_t subtype)
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
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
        public:
            stream_writer(std::ostream &output) : stream_writer_base(output) {}

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
            void string_data_(const core::value &v, bool) {output_stream.write(v.get_string().c_str(), v.get_string().size());}

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

        inline std::string to_message_pack(const core::value &v)
        {
            std::ostringstream stream;
            stream_writer writer(stream);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_MESSAGE_PACK_H

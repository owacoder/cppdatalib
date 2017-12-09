#ifndef CPPDATALIB_BJSON_H
#define CPPDATALIB_BJSON_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace bjson
    {
        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(core::ostream &output) : core::stream_writer(output) {}

            protected:
                core::ostream &write_size(core::ostream &stream, int initial_type, uint64_t size)
                {
                    char buffer[9]; // 8 + 1 byte type specifier
                    size_t buffer_size = 0;

                    if (size >= UINT32_MAX)
                    {
                        buffer[0] = initial_type + 3;
                        for (size_t i = 0; i < 8; ++i)
                            buffer[i+1] = (size >> (8*i)) & 0xff;
                        buffer_size = 9;
                    }
                    else if (size >= UINT16_MAX)
                    {
                        buffer[0] = initial_type + 2;
                        for (size_t i = 0; i < 4; ++i)
                            buffer[i+1] = (size >> (8*i)) & 0xff;
                        buffer_size = 5;
                    }
                    else if (size >= UINT8_MAX)
                    {
                        buffer[0] = initial_type + 1;
                        buffer[1] = size >> 8;
                        buffer[2] = size & 0xff;
                        buffer_size = 3;
                    }
                    else
                    {
                        buffer[0] = initial_type;
                        buffer[1] = size;
                        buffer_size = 2;
                    }

                    return stream.write(buffer, buffer_size);
                }

                size_t get_size(const core::value &v)
                {
                    struct traverser
                    {
                    private:
                        std::stack<size_t, std::vector<size_t>> size;

                    public:
                        traverser() {size.push(0);}

                        size_t get_size() const {return size.top();}

                        bool operator()(const core::value *arg, core::value::traversal_ancestry_finder, bool prefix)
                        {
                            switch (arg->get_type())
                            {
                                case core::null:
                                case core::boolean:
                                    if (prefix)
                                        size.top() += 1;
                                    break;
                                case core::integer:
                                {
                                    if (prefix)
                                    {
                                        size.top() += 1; // one byte for type specifier

                                        if (arg->get_int_unchecked() == 0 || arg->get_int_unchecked() == 1)
                                            ; // do nothing
                                        else if (arg->get_int_unchecked() >= -core::int_t(UINT8_MAX) && arg->get_int_unchecked() <= UINT8_MAX)
                                            size.top() += 1;
                                        else if (arg->get_int_unchecked() >= -core::int_t(UINT16_MAX) && arg->get_int_unchecked() <= UINT16_MAX)
                                            size.top() += 2;
                                        else if (arg->get_int_unchecked() >= -core::int_t(UINT32_MAX) && arg->get_int_unchecked() <= UINT32_MAX)
                                            size.top() += 4;
                                        else
                                            size.top() += 8;
                                    }

                                    break;
                                }
                                case core::uinteger:
                                {
                                    if (prefix)
                                    {
                                        size.top() += 1; // one byte for type specifier

                                        if (arg->get_uint_unchecked() == 0 || arg->get_uint_unchecked() == 1)
                                            ; // do nothing
                                        else if (arg->get_uint_unchecked() <= UINT8_MAX)
                                            size.top() += 1;
                                        else if (arg->get_uint_unchecked() <= UINT16_MAX)
                                            size.top() += 2;
                                        else if (arg->get_uint_unchecked() <= UINT32_MAX)
                                            size.top() += 4;
                                        else
                                            size.top() += 8;
                                    }

                                    break;
                                }
                                case core::real:
                                {
                                    if (prefix)
                                    {
                                        size.top() += 5; // one byte for type specifier, minimum of four bytes for data

                                        // A user-specified subtype is not available for reals
                                        // (because when the data is read again, the IEEE-754 representation will be put into an integer instead of a real,
                                        // since there is nothing to show that the data should be read as a floating point number)
                                        // To prevent the loss of data, the subtype is discarded and the value stays the same

                                        if (core::float_from_ieee_754(core::float_to_ieee_754(arg->get_real_unchecked())) != arg->get_real_unchecked() && !std::isnan(arg->get_real_unchecked()))
                                            size.top() += 4; // requires more than 32-bit float to losslessly encode
                                    }

                                    break;
                                }
                                case core::string:
                                {
                                    if (prefix)
                                    {
                                        size.top() += 1; // one byte for type specifier

                                        if (arg->size() >= UINT32_MAX)
                                            size.top() += 8; // requires an eight-byte size specifier
                                        else if (arg->size() >= UINT16_MAX)
                                            size.top() += 4; // requires a four-byte size specifier
                                        else if (arg->size() >= UINT8_MAX)
                                            size.top() += 2; // requires a two-byte size specifier
                                        else if (arg->size() > 0 && arg->get_subtype() != core::blob && arg->get_subtype() != core::clob)
                                            size.top() += 1; // requires a one-byte size specifier

                                        size.top() += arg->string_size();
                                    }
                                    break;
                                }
                                case core::array:
                                {
                                    if (prefix)
                                    {
                                        size.push(size.size() > 2); // one byte for type specifier
                                    }
                                    else
                                    {
                                        if (size.size() > 2)
                                        {
                                            if (size.top() >= UINT32_MAX)
                                                size.top() += 8; // requires an eight-byte size specifier
                                            else if (size.top() >= UINT16_MAX)
                                                size.top() += 4; // requires a four-byte size specifier
                                            else if (size.top() >= UINT8_MAX)
                                                size.top() += 2; // requires a two-byte size specifier
                                            else
                                                size.top() += 1; // requires a one-byte size specifier
                                        }

                                        size_t temp = size.top();
                                        size.pop();
                                        size.top() += temp;
                                    }

                                    break;
                                }
                                case core::object:
                                {
                                    if (prefix)
                                    {
                                        size.push(size.size() > 2); // one byte for type specifier
                                    }
                                    else
                                    {
                                        if (size.size() > 2)
                                        {
                                            if (size.top() >= UINT32_MAX)
                                                size.top() += 8; // requires an eight-byte size specifier
                                            else if (size.top() >= UINT16_MAX)
                                                size.top() += 4; // requires a four-byte size specifier
                                            else if (size.top() >= UINT8_MAX)
                                                size.top() += 2; // requires a two-byte size specifier
                                            else
                                                size.top() += 1; // requires a one-byte size specifier
                                        }

                                        size_t temp = size.top();
                                        size.pop();
                                        size.top() += temp;
                                    }

                                    break;
                                }
                            }

                            return true;
                        }
                    };

                    traverser t;

                    v.traverse(t);

                    return t.get_size();
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
        public:
            stream_writer(core::ostream &output) : impl::stream_writer_base(output) {}

            bool requires_prefix_string_size() const {return true;}
            bool requires_array_buffering() const {return true;}
            bool requires_object_buffering() const {return true;}

        protected:
            void null_(const core::value &) {output_stream.put(0);}
            void bool_(const core::value &v) {output_stream.put(24 + v.get_bool_unchecked());}

            void integer_(const core::value &v)
            {
                if (v.get_int_unchecked() < 0)
                {
                    write_size(output_stream, 8, -v.get_int_unchecked());
                }
                else if (v.get_int_unchecked() <= 1)
                {
                    output_stream.put(26 + v.get_uint_unchecked()); // Shortcut 0 and 1
                    return;
                }
                else /* positive integer */
                {
                    write_size(output_stream, 4, v.get_int_unchecked());
                }
            }

            void uinteger_(const core::value &v)
            {
                if (v.get_uint_unchecked() <= 1)
                {
                    output_stream.put(26 + v.get_uint_unchecked()); // Shortcut 0 and 1
                    return;
                }

                write_size(output_stream, 4, v.get_uint_unchecked());
            }

            void real_(const core::value &v)
            {
                uint64_t out;

                if (core::float_from_ieee_754(core::float_to_ieee_754(v.get_real_unchecked())) == v.get_real_unchecked() || std::isnan(v.get_real_unchecked()))
                {
                    out = core::float_to_ieee_754(v.get_real_unchecked());
                    output_stream.put(14)
                            .put(out & 0xff)
                            .put((out >> 8) & 0xff)
                            .put((out >> 16) & 0xff)
                            .put(out >> 24);
                }
                else
                {
                    out = core::double_to_ieee_754(v.get_real_unchecked());
                    output_stream.put(15)
                            .put(out & 0xff)
                            .put((out >> 8) & 0xff)
                            .put((out >> 16) & 0xff)
                            .put((out >> 24) & 0xff)
                            .put((out >> 32) & 0xff)
                            .put((out >> 40) & 0xff)
                            .put((out >> 48) & 0xff)
                            .put(out >> 56);
                }
            }

            void begin_string_(const core::value &v, core::int_t size, bool)
            {
                int initial_type = 16;

                if (size == unknown_size)
                    throw core::error("BJSON - 'string' value does not have size specified");
                else if (size == 0 && v.get_subtype() != core::blob && v.get_subtype() != core::clob)
                {
                    output_stream.put(2); // Empty string type
                    return;
                }

                switch (v.get_subtype())
                {
                    case core::blob:
                    case core::clob:
                        initial_type += 4;
                        break;
                    default: break;
                }

                write_size(output_stream, initial_type, size);
            }
            void string_data_(const core::value &v, bool) {output_stream.write(v.get_string_unchecked().c_str(), v.get_string_unchecked().size());}

            void begin_array_(const core::value &v, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("BJSON - 'array' value does not have size specified");
                else if (v.size() != static_cast<size_t>(size))
                    throw core::error("BJSON - entire 'array' value must be buffered before writing");

                write_size(output_stream, 32, get_size(v));
            }

            void begin_object_(const core::value &v, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("BJSON - 'object' value does not have size specified");
                else if (v.size() != static_cast<size_t>(size))
                    throw core::error("BJSON - entire 'object' value must be buffered before writing");

                write_size(output_stream, 36, get_size(v));
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

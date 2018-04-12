/*
 * bson.h
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

#ifndef BSON_H
#define BSON_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace bson
    {
        enum type
        {
            end_of_document,
            floating_point,
            utf8_string,
            object,
            array,
            binary,
            undefined,
            object_id,
            boolean,
            utc_datetime,
            null,
            regex,
            db_pointer,
            javascript,
            symbol,
            javascript_code_w_scope,
            int32,
            timestamp,
            int64,
            decimal128,
            min_key = 0xff,
            max_key = 0x7f
        };

        enum subtype
        {
            generic_binary,
            function,
            binary_deprecated,
            uuid_deprecated,
            uuid,
            md5,
            user = 0x80
        };

        class parser : public core::stream_parser
        {
            struct container
            {
                container() : size_(0), type_(end_of_document) {}
                container(int32_t size_, type type_) : size_(size_), type_(type_) {}

                int32_t size_;
                type type_;
            };

            core::cache_vector_n<container, core::cache_size> containers;
            std::unique_ptr<char []> buffer;

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
                if (containers.empty())
                {
                    int32_t size;
                    if (!core::read_int32_le(stream(), size))
                        throw core::error("BSON - expected object size");

                    if (size < 5) // Requires one for terminating NUL, four for size specifier
                        throw core::error("BSON - invalid document size specified");

                    containers.push_back(container(size - 4, object));
                    get_output()->begin_object(core::value(core::object_t()), core::stream_handler::unknown_size);
                }

                core::istream::int_type element_type = stream().get();

                if (element_type > 0xff) // Use of non-standard encoding stream?
                    throw core::error("BSON - invalid input encoding, binary is required");

                switch (static_cast<type>(element_type))
                {
                    case end_of_document:
                        decrement_counter(1, true);
                        break;
                    case floating_point:
                    {
                        uint64_t fp;

                        read_name();

                        if (!core::read_uint64_le(stream(), fp))
                            throw core::error("BSON - expected floating point value");

                        get_output()->write(core::value(core::double_from_ieee_754(fp)));
                        decrement_counter(9);

                        break;
                    }
                    case utf8_string:
                    {
                        int32_t size;
                        core::value string_type{core::string_t()};

                        read_name();

                        if (!core::read_uint32_le(stream(), size))
                            throw core::error("BSON - expected string size");

                        if (size <= 0)
                            throw core::error("BSON - string size must not be negative or zero");

                        size -= 1; // Remove trailing NUL

                        get_output()->begin_string(string_type, size);
                        while (size > 0)
                        {
                            int32_t buffer_size = std::min(int32_t(core::buffer_size), size);
                            if (stream().read(buffer.get(), buffer_size))
                                throw core::error("BSON - unexpected end of string");
                            // Set string in string_type to preserve the subtype
                            string_type.set_string(core::string_t(buffer.get(), static_cast<size_t>(buffer_size)));
                            get_output()->append_to_string(string_type);
                            size -= buffer_size;
                        }
                        string_type.set_string("");
                        get_output()->end_string(string_type);

                        if (stream().get() != 0)
                            throw core::error("BSON - invalid string terminator");

                        decrement_counter(size + 5);
                        break;
                    }
                    case object:
                    case array:
                    {
                        int32_t size;

                        read_name();

                        if (!core::read_int32_le(stream(), size))
                            throw core::error("BSON - expected document size");

                        if (size < 5)
                            throw core::error("BSON - invalid document size specified");

                        if (containers.back().size_ < size)
                            throw core::error("BSON - invalid document size specified");
                        containers.back().size_ -= size;
                        containers.push_back(container(size, static_cast<type>(element_type)));

                        if (static_cast<type>(element_type) == object)
                            get_output()->begin_object(core::value(core::object_t()), core::stream_handler::unknown_size);
                        else
                            get_output()->begin_array(core::value(core::array_t()), core::stream_handler::unknown_size);

                        break;
                    }
                    case binary:
                    {
                        int32_t size;
                        core::value string_type{core::string_t()};

                        read_name();

                        if (!core::read_uint32_le(stream(), size))
                            throw core::error("BSON - expected string size");

                        if (size < 0)
                            throw core::error("BSON - binary element size must not be negative");

                        core::istream::int_type subtype = stream().get();

                        switch (subtype)
                        {
                            case generic_binary: string_type.set_subtype(core::blob); break;
                            case function: string_type.set_subtype(core::function); break;
                            case uuid: string_type.set_subtype(core::uuid); break;
                            default:
                                if (subtype > 0xff)
                                    throw core::error("BSON - invalid input encoding");
                                else if (subtype > 0x7f)
                                    string_type.set_subtype(core::user + subtype - 0x80);
                                else
                                    string_type.set_subtype(core::reserved + subtype);
                                break;
                        }

                        get_output()->begin_string(string_type, size);
                        while (size > 0)
                        {
                            int32_t buffer_size = std::min(int32_t(core::buffer_size), size);
                            if (stream().read(buffer.get(), buffer_size))
                                throw core::error("BSON - unexpected end of string");
                            // Set string in string_type to preserve the subtype
                            string_type.set_string(core::string_t(buffer.get(), static_cast<size_t>(buffer_size)));
                            get_output()->append_to_string(string_type);
                            size -= buffer_size;
                        }
                        string_type.set_string("");
                        get_output()->end_string(string_type);

                        decrement_counter(size + 5);
                        break;
                    }
                    case undefined:
                    {
                        read_name();

                        get_output()->write(core::value());
                        break;
                    }
                    case object_id:
                    {
                        read_name();

                        char buf[12];
                        if (!stream().read(buf, 12))
                            throw core::error("BSON - expected ObjectID");

                        decrement_counter(12);
                        break;
                    }
                    case boolean:
                    {
                        read_name();

                        core::istream::int_type c = stream().get();

                        if (c == 0)
                            get_output()->write(core::value(false));
                        else if (c == 1)
                            get_output()->write(core::value(true));
                        else
                            throw core::error("BSON - expected boolean value");

                        decrement_counter(1);
                        break;
                    }
                    default:
                        break;
                }
            }

        private:
            void read_name()
            {
                core::istream::int_type c;
                core::value str(core::string_t(), core::clob);
                core::string_t &ref = str.get_string_ref();

                while (c = stream().get(), c != EOF && c != 0 && c <= 0xff)
                    ref.push_back(c);

                if (c > 0xff)
                    throw core::error("BSON - invalid character found in document element name");
                else if (c != 0)
                    throw core::error("BSON - unexpected end of string while parsing document element name");

                if (containers.back().type_ != array)
                    get_output()->write(str);
                decrement_counter(str.size() + 1);
            }

            void decrement_counter(int32_t amount, bool allow_popping = false)
            {
                containers.back().size_ -= amount;
                if (containers.back().size_ < 0)
                    throw core::error("BSON - invalid size prefix specified on document");
                else if (containers.back().size_ == 0)
                {
                    if (allow_popping)
                    {
                        if (containers.back().type_ == array)
                            get_output()->end_array(core::value(core::array_t()));
                        else
                            get_output()->end_object(core::value(core::object_t()));
                        containers.pop_back();
                    }
                    else
                        throw core::error("BSON - invalid size prefix specified on document");
                }
            }
        };
    }
}

#endif // BSON_H

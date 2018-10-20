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

        // TODO: dbpointers, javascript code with scope, decimal128, and min and max keys are not supported for writing
        class parser : public core::stream_parser
        {
            struct container
            {
                container() : size_(0), type_(end_of_document) {}
                container(int32_t size_, type type_) : size_(size_), type_(type_) {}

                int32_t size_;
                type type_;
            };

            typedef core::cache_vector_n<container, core::cache_size> containers_t;

            containers_t containers;
            char * const buffer;

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
                if (containers.empty())
                {
                    int32_t size;
                    if (!core::read_int32_le(stream(), size))
                        throw core::error("BSON - expected object size");

                    if (size < 5) // Requires one for terminating NUL, four for size specifier
                        throw core::error("BSON - invalid document size specified");

                    containers.push_back(container(size - 4, object));
                    get_output()->begin_object(core::value(core::object_t()), core::stream_handler::unknown_size());
                }

                core::istream::int_type element_type = stream().get();

                if (element_type > 0xff) // Use of non-standard encoding stream?
                    throw core::error("BSON - invalid input encoding, binary is required");
                else if (element_type == EOF)
                    throw core::error("BSON - unexpected end of input");

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
                    case javascript:
                    case symbol:
                    {
                        int32_t size;
                        core::value string_type("", 0, element_type == javascript? core::javascript:
                                                       element_type == symbol? core::symbol: core::normal, true);

                        read_name();

                        if (!core::read_uint32_le(stream(), size))
                            throw core::error("BSON - expected string size");

                        if (size <= 0)
                            throw core::error("BSON - string size must not be negative or zero");

                        decrement_counter(size + 5);
                        size -= 1; // Remove trailing NUL

                        get_output()->begin_string(string_type, size);
                        while (size > 0)
                        {
                            int32_t buffer_size = std::min(int32_t(core::buffer_size), size);
                            if (!stream().read(buffer, buffer_size))
                                throw core::error("BSON - unexpected end of string");
                            // Set string in string_type to preserve the subtype
                            string_type = core::value(buffer, static_cast<size_t>(buffer_size), string_type.get_subtype(), true);
                            get_output()->append_to_string(string_type);
                            size -= buffer_size;
                        }
                        string_type = core::value("", 0, string_type.get_subtype(), true);
                        get_output()->end_string(string_type);

                        if (stream().get() != 0)
                            throw core::error("BSON - invalid string terminator");

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

                        if (containers.back().size_ - 1 < size)
                            throw core::error("BSON - invalid document size specified");
                        containers.back().size_ -= size + 1;
                        containers.push_back(container(size - 4, static_cast<type>(element_type)));

                        if (static_cast<type>(element_type) == object)
                            get_output()->begin_object(core::value(core::object_t()), core::stream_handler::unknown_size());
                        else
                            get_output()->begin_array(core::value(core::array_t()), core::stream_handler::unknown_size());

                        break;
                    }
                    case binary:
                    {
                        int32_t size;
                        core::value string_type("", 0, core::normal, true);

                        read_name();

                        if (!core::read_uint32_le(stream(), size))
                            throw core::error("BSON - expected string size");

                        if (size < 0)
                            throw core::error("BSON - binary element size must not be negative");

                        core::istream::int_type subtype = stream().get();

                        switch (subtype)
                        {
                            case generic_binary: string_type.set_subtype(core::blob); break;
                            case function: string_type.set_subtype(core::binary_function); break;
                            case uuid: string_type.set_subtype(core::binary_uuid); break;
                            default:
                                if (subtype > 0xff)
                                    throw core::error("BSON - invalid input encoding");
                                else if (subtype > 0x7f)
                                    string_type.set_subtype(core::user + subtype - 0x80);
                                else
                                    string_type.set_subtype(core::reserved + subtype);
                                break;
                        }

                        decrement_counter(size + 5);

                        get_output()->begin_string(string_type, size);
                        while (size > 0)
                        {
                            int32_t buffer_size = std::min(int32_t(core::buffer_size), size);
                            if (!stream().read(buffer, buffer_size))
                                throw core::error("BSON - unexpected end of string");
                            // Set string in string_type to preserve the subtype
                            string_type = core::value(buffer, static_cast<size_t>(buffer_size), string_type.get_subtype(), true);
                            get_output()->append_to_string(string_type);
                            size -= buffer_size;
                        }
                        string_type = core::value("", 0, string_type.get_subtype(), true);
                        get_output()->end_string(string_type);

                        break;
                    }
                    case undefined:
                    case null:
                    {
                        read_name();

                        get_output()->write(core::value());
                        decrement_counter(1);
                        break;
                    }
                    case object_id:
                    {
                        read_name();

                        char buf[12];
                        if (!stream().read(buf, 12))
                            throw core::error("BSON - expected ObjectID");

                        get_output()->write(core::value(core::string_t(buf, 12), core::binary_object_id));
                        decrement_counter(13);
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

                        decrement_counter(2);
                        break;
                    }
                    case utc_datetime:
                    {
                        int64_t time;

                        read_name();

                        if (!core::read_int64_le(stream(), time))
                            throw core::error("BSON - expected UTC timestamp");

                        decrement_counter(9);
                        get_output()->write(core::value(time, core::utc_timestamp_ms));
                        break;
                    }
                    case regex:
                    {
                        core::value str = core::value(core::string_t(), core::regexp);

                        decrement_counter(1);

                        read_name();

                        if (!read_cstring(str.get_owned_string_ref()))
                            throw core::error("BSON - expected regular expression");

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                        if (!read_cstring(str.attribute("options").get_owned_string_ref()))
                            throw core::error("BSON - expected regular expression options");
#else
                        core::string_t temp;
                        if (!read_cstring(temp))
                            throw core::error("BSON - expected regular expression options");
#endif
                        get_output()->write(str);

                        break;
                    }
                    case db_pointer:
                        throw core::error("BSON - DBPointer not supported");
                    case javascript_code_w_scope:
                        throw core::error("BSON - JavaScript code with scope not supported");
                    case int32:
                    {
                        int32_t i;

                        read_name();

                        if (!core::read_int32_le(stream(), i))
                            throw core::error("BSON - expected 32-bit integer");

                        decrement_counter(5);
                        get_output()->write(core::value(i));
                        break;
                    }
                    case timestamp:
                    {
                        uint64_t i;

                        read_name();

                        if (!core::read_uint64_le(stream(), i))
                            throw core::error("BSON - expected timestamp");

                        decrement_counter(9);
                        get_output()->write(core::value(i, core::mongodb_timestamp));
                        break;
                    }
                    case int64:
                    {
                        int64_t i;

                        read_name();

                        if (!core::read_int64_le(stream(), i))
                            throw core::error("BSON - expected 64-bit integer");

                        decrement_counter(9);
                        get_output()->write(core::value(i));
                        break;
                    }
                    case decimal128:
                        throw core::error("BSON - 128-bit decimal floating-point values are not supported");
                    case min_key:
                        throw core::error("BSON - minimum keys are not supported");
                    case max_key:
                        throw core::error("BSON - maximum keys are not supported");
                    default:
                        throw core::error("BSON - unknown datatype or corrupt stream encountered");
                }
            }

        private:
            bool read_cstring(core::string_t &ref)
            {
                core::istream::int_type c;
                ref.clear();
                while (c = stream().get(), c != EOF && c != 0 && c <= 0xff)
                    ref.push_back(c);

                decrement_counter(ref.size() + 1);

                if (c != 0)
                    return false;

                return true;
            }

            void read_name()
            {
                core::istream::int_type c;
                core::value str(core::string_t(), core::clob);
                core::string_t &ref = str.get_owned_string_ref();

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

            void decrement_counter(size_t amount, bool allow_popping = false)
            {
                                if (amount > std::numeric_limits<int32_t>::max())
                                        throw core::error("BSON - invalid size prefix specified on document");
                containers.back().size_ -= int32_t(amount);
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

        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(core::ostream_handle &output) : core::stream_writer(output) {}

            protected:
                size_t get_size(const core::value &v)
                {
                    struct traverser
                    {
                    private:
                        typedef std::stack< size_t, core::cache_vector_n<size_t, core::cache_size> > sizes_t;

                        sizes_t size;

                    public:
                        traverser() {size.push(0);}

                        size_t get_size() const {return size.top();}

                        bool operator()(const core::value *arg, core::value::traversal_ancestry_finder finder, bool prefix)
                        {
                            switch (arg->get_type())
                            {
                                case core::link:
                                    throw core::error("BSON - links are not supported by this format");
                                case core::null: // No data added for a null value
                                    break;
                                case core::boolean: // Add one for actual payload
                                    if (prefix)
                                        size.top() += 1;
                                    break;
                                case core::integer: // Add 4 or 8 for actual payload
                                {
                                    if (prefix)
                                    {
                                        if (arg->get_subtype() != core::mongodb_timestamp &&
                                                arg->get_subtype() != core::utc_timestamp &&
                                                arg->get_subtype() != core::utc_timestamp_ms &&
                                                arg->get_int_unchecked() >= std::numeric_limits<int32_t>::min() && arg->get_int_unchecked() <= std::numeric_limits<int32_t>::max())
                                            size.top() += 4;
                                        else
                                            size.top() += 8;
                                    }
                                    break;
                                }
                                case core::uinteger: // Add 4 or 8 for actual payload
                                {
                                    if (prefix)
                                    {
                                        // TODO: handle values above std::numeric_limits<int64_t>::max()
                                        if (arg->get_subtype() != core::mongodb_timestamp &&
                                                arg->get_subtype() != core::utc_timestamp &&
                                                arg->get_subtype() != core::utc_timestamp_ms &&
                                                arg->get_uint_unchecked() <= std::numeric_limits<int32_t>::max())
                                            size.top() += 4;
                                        else
                                            size.top() += 8;
                                    }
                                    break;
                                }
                                case core::real: // Add 8 for actual payload
                                {
                                    if (prefix)
                                        size.top() += 8;
                                    break;
                                }
                                case core::string:
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                                case core::temporary_string:
#endif
                                {
                                    if (prefix)
                                    {
                                        core::value::traversal_ancestry_finder::ancestry_t ancestry = finder.get_ancestry();

                                        if (!ancestry.empty() && ancestry[0].is_object_key())
                                            size.top() += 1 + arg->string_size(); // If key, add actual payload size plus nul-terminator
                                        else if (arg->get_subtype() == core::regexp)
                                        {
                                            size.top() += 2 + arg->string_size(); // If regexp, add actual payload size plus two nul-terminators
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                                            size.top() += arg->attribute(core::value((const char *) "options", (cppdatalib::core::subtype_t) core::domain_comparable, true)).as_string().size();
#endif
                                        }
                                        else if (arg->get_subtype() == core::binary_object_id)
                                            size.top() += 12; // If ObjectID, add 12 for payload
                                        else
                                            size.top() += 5 + arg->string_size(); // If regular string, add 4 for size specifier, payload size, and 1 for nul-terminator or subtype specifier (binary)
                                    }
                                    break;
                                }
                                case core::array:
                                {
                                    if (prefix)
                                    {
                                        size.top() += 5 + arg->array_size() * 2; // Add 4 for size specifier, 1 for nul-terminator, and length of all keys and element types
                                        size_t limit = arg->array_size();

                                        // Add in the length of string representations of array indexes
                                        if (limit > 1000000000)
                                            size.top() += (limit - 1000000000) * 10 + 8888888890;
                                        else if (limit > 100000000)
                                            size.top() += (limit - 100000000) * 9 + 788888890;
                                        else if (limit > 10000000)
                                            size.top() += (limit - 10000000) * 8 + 68888890;
                                        else if (limit > 1000000)
                                            size.top() += (limit - 1000000) * 7 + 5888890;
                                        else if (limit > 100000)
                                            size.top() += (limit - 100000) * 6 + 488890;
                                        else if (limit > 10000)
                                            size.top() += (limit - 10000) * 5 + 38890;
                                        else if (limit > 1000)
                                            size.top() += (limit - 1000) * 4 + 2890;
                                        else if (limit > 100)
                                            size.top() += (limit - 100) * 3 + 190;
                                        else if (limit > 10)
                                            size.top() += (limit - 10) * 2 + 10;
                                        else
                                            size.top() += limit;
                                    }

                                    break;
                                }
                                case core::object:
                                {
                                    if (prefix)
                                        size.top() += 5 + arg->object_size(); // Add 4 for size specifier, 1 for nul-terminator, and length of all element types

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

        // TODO: dbpointers, javascript code with scope, decimal128, and min and max keys are not supported for writing
        class stream_writer : public impl::stream_writer_base
        {
            std::string key_name;

        public:
            stream_writer(core::ostream_handle output) : impl::stream_writer_base(output) {}

            unsigned int required_features() const {return requires_single_write;}

            std::string name() const {return "cppdatalib::bson::stream_writer";}

        protected:
            void write_key_name()
            {
                if (key_name.find('\0') != key_name.npos)
                    throw core::error("BSON - key names must not contain NUL");
                stream() << key_name << '\0';
            }

            void begin_key_(const core::value &v)
            {
                key_name.clear();
                if (!v.is_string())
                    throw core::error("BSON - object keys must be strings");
            }

            void begin_item_(const core::value &)
            {
                if (current_container() == core::array)
                    key_name = stdx::to_string(current_container_size());
            }

            void null_(const core::value &v)
            {
                if (current_container() == core::null)
                    throw core::error("BSON - 'null' value must be part of an object or array");

                if (v.get_subtype() == core::undefined)
                    stream().put(0x06);
                else
                    stream().put(0x0a);
                write_key_name();
            }

            void bool_(const core::value &v)
            {
                if (current_container() == core::null)
                    throw core::error("BSON - 'boolean' value must be part of an object or array");

                stream().put(0x08);
                write_key_name();
                stream().put(v.get_bool_unchecked());
            }

            void link_(const core::value &) {throw core::error("BSON - 'link' value not allowed in output");}

            void integer_(const core::value &v)
            {
                if (current_container() == core::null)
                    throw core::error("BSON - 'integer' value must be part of an object or array");

                if (v.get_subtype() == core::mongodb_timestamp)
                {
                    stream().put(0x11);
                    write_key_name();
                    core::write_uint64_le(stream(), uint64_t(v.get_int_unchecked()));
                }
                else if (v.get_subtype() == core::utc_timestamp ||
                         v.get_subtype() == core::utc_timestamp_ms)
                {
                    stream().put(0x09);
                    write_key_name();
                    core::write_uint64_le(stream(), uint64_t(v.get_int_unchecked() * (v.get_subtype() == core::utc_timestamp? 1000: 1)));
                }
                else if (v.get_int_unchecked() >= std::numeric_limits<int32_t>::min() && v.get_int_unchecked() <= std::numeric_limits<int32_t>::max())
                {
                    stream().put(0x10);
                    write_key_name();
                    core::write_uint32_le(stream(), uint32_t(v.get_int_unchecked()));
                }
                else
                {
                    stream().put(0x12);
                    write_key_name();
                    core::write_uint64_le(stream(), uint64_t(v.get_int_unchecked()));
                }
            }

            void uinteger_(const core::value &v)
            {
                if (current_container() == core::null)
                    throw core::error("BSON - 'uinteger' value must be part of an object or array");

                if (v.get_subtype() == core::mongodb_timestamp)
                {
                    stream().put(0x11);
                    write_key_name();
                    core::write_uint64_le(stream(), v.get_uint_unchecked());
                }
                else if (v.get_subtype() == core::utc_timestamp ||
                         v.get_subtype() == core::utc_timestamp_ms)
                {
                    stream().put(0x09);
                    write_key_name();
                    core::write_uint64_le(stream(), v.get_uint_unchecked() * (v.get_subtype() == core::utc_timestamp? 1000: 1));
                }
                else if (v.get_uint_unchecked() <= std::numeric_limits<int32_t>::max())
                {
                    stream().put(0x10);
                    write_key_name();
                    core::write_uint32_le(stream(), v.get_uint_unchecked());
                }
                else // TODO: handle values above std::numeric_limits<int64_t>::max()
                {
                    stream().put(0x12);
                    write_key_name();
                    core::write_uint64_le(stream(), v.get_uint_unchecked());
                }
            }

            void real_(const core::value &v)
            {
                if (current_container() == core::null)
                    throw core::error("BSON - 'real' value must be part of an object or array");

                stream().put(0x01);
                write_key_name();
                core::write_uint64_le(stream(), core::double_to_ieee_754(v.get_real_unchecked()));
            }

            void begin_string_(const core::value &v, core::optional_size size, bool is_key)
            {
                if (is_key)
                    return;

                if (current_container() == core::null)
                    throw core::error("BSON - 'string' value must be part of an object or array");
                else if (!size.has_value())
                    throw core::error("BSON - 'string' value does not have size specified");
                else if (v.size() != size.value())
                    throw core::error("BSON - entire 'string' value must be buffered before writing");

                if (v.get_subtype() == core::binary_object_id)
                {
                    if (size.value() != 12)
                        throw core::error("BSON - ObjectID is not 12 bytes");

                    stream().put(0x07);
                    write_key_name();
                }
                else if (!core::subtype_is_text_string(v.get_subtype()))
                {
                    stream().put(0x05);
                    write_key_name();
                    core::write_uint32_le(stream(), size.value()+1);
                    switch (v.get_subtype())
                    {
                        case core::binary_uuid: stream().put(0x04); break;
                        case core::binary_function: stream().put(0x01); break;
                        default:
                            if (v.get_subtype() >= core::reserved && v.get_subtype() <= core::reserved_max)
                                stream().put(v.get_subtype() - core::reserved);
                            else if (v.get_subtype() >= core::user && v.get_subtype() <= core::user_max)
                                stream().put(v.get_subtype() - core::user + 0x80);
                            else
                                stream().put(0x00);
                            break;
                    }
                }
                else // text string
                {
                    switch (v.get_subtype())
                    {
                        case core::javascript: stream().put(0x0d); break;
                        case core::symbol: stream().put(0x0e); break;
                        case core::regexp:
                            stream().put(0x0b);
                            break;
                        default:
                            stream().put(0x02);
                            break;
                    }
                    write_key_name();
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    // Reuse key_name to contain regexp options if available
                    if (v.get_subtype() == core::regexp)
                        key_name = v.attribute(core::value((const char *) "options", (cppdatalib::core::subtype_t) core::domain_comparable, true)).as_string();
                    else
#else
                    // No options available for regexp
                    if (v.get_subtype() == core::regexp)
                        key_name.clear();
                    else
#endif
                        core::write_uint32_le(stream(), size.value()+1);
                }
            }
            void string_data_(const core::value &v, bool is_key)
            {
                if (is_key)
                    key_name += v.get_string_unchecked();
                else
                    stream() << v.get_string_unchecked();
            }
            void end_string_(const core::value &, bool is_key)
            {
                if (is_key)
                    return;

                if (core::subtype_is_text_string(current_container_subtype()))
                    stream().put(0);

                if (current_container_subtype() == core::regexp)
                    write_key_name(); // Write regexp options
            }

            void begin_array_(const core::value &v, core::optional_size size, bool)
            {
                if (current_container() == core::null)
                    throw core::error("BSON - 'array' value must be part of an object or array");
                else if (!size.has_value())
                    throw core::error("BSON - 'array' value does not have size specified");
                else if (v.size() != size.value())
                    throw core::error("BSON - entire 'array' value must be buffered before writing");

                stream().put(0x04);
                write_key_name();
                core::write_uint32_le(stream(), get_size(v));
            }
            void end_array_(const core::value &, bool) {stream().put(0);}

            void begin_object_(const core::value &v, core::optional_size size, bool)
            {
                if (!size.has_value())
                    throw core::error("BSON - 'object' value does not have size specified");
                else if (v.size() != size.value())
                    throw core::error("BSON - entire 'object' value must be buffered before writing");

                if (current_container() != core::null)
                {
                    stream().put(0x03);
                    write_key_name();
                }

                core::write_uint32_le(stream(), get_size(v));
            }
            void end_object_(const core::value &, bool) {stream().put(0);}
        };

        inline core::value from_bson(core::istream_handle stream)
        {
            parser p(stream);
            core::value v;
            core::convert(p, v);
            return v;
        }

#ifdef CPPDATALIB_CPP11
        inline core::value operator "" _bson(const char *stream, size_t size)
        {
            core::istringstream wrap(std::string(stream, size));
            return from_bson(wrap);
        }
#endif

        inline std::string to_bson(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer w(stream);
            core::convert(w, v);
            return stream.str();
        }
    }
}

#endif // BSON_H

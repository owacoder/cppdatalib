/*
 * stream_filters.h
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

#ifndef CPPDATALIB_STREAM_FILTERS_H
#define CPPDATALIB_STREAM_FILTERS_H

#include "value_builder.h"
#include <set> // For duplicate_key_check_filter

#include <cassert>

namespace cppdatalib
{
    namespace core
    {
        class stream_filter_base : public stream_handler
        {
        protected:
            core::stream_handler &output;

        public:
            stream_filter_base(core::stream_handler &output)
                : output(output)
            {}
            stream_filter_base(const stream_filter_base &f)
                : output(f.output)
            {
                assert(("cppdatalib::core::stream_filter_base(const stream_filter_base &) - attempted to copy a stream_filter_base while active" && !f.active()));
            }
            stream_filter_base(stream_filter_base &&f)
                : output(f.output)
            {
                assert(("cppdatalib::core::stream_filter_base(stream_filter_base &&) - attempted to move a stream_filter_base while active" && !f.active()));
            }

            bool requires_prefix_string_size() const {return output.requires_prefix_string_size();}
            bool requires_prefix_array_size() const {return output.requires_prefix_array_size();}
            bool requires_prefix_object_size() const {return output.requires_prefix_object_size();}
            bool requires_string_buffering() const {return output.requires_string_buffering();}
            bool requires_array_buffering() const {return output.requires_array_buffering();}
            bool requires_object_buffering() const {return output.requires_object_buffering();}
        };

        namespace impl
        {
            template<core::type from, core::type to>
            struct stream_filter_converter
            {
                void operator()(core::value &value)
                {
                    if (value.get_type() != from || from == to)
                        return;

                    switch (from)
                    {
                        case null:
                            switch (to)
                            {
                                case boolean: value.set_bool(false); break;
                                case integer: value.set_int(0); break;
                                case uinteger: value.set_uint(0); break;
                                case real: value.set_real(0.0); break;
                                case string: value.set_string(""); break;
                                case array: value.set_array(array_t()); break;
                                case object: value.set_object(object_t()); break;
                                default: value.set_null(); break;
                            }
                            break;
                        case boolean:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case integer: value.set_int(value.get_bool()); break;
                                case uinteger: value.set_uint(value.get_bool()); break;
                                case real: value.set_real(value.get_bool()); break;
                                case string: value.set_string(value.get_bool()? "true": "false"); break;
                                case array: value.set_array(array_t()); break;
                                case object: value.set_object(object_t()); break;
                                default: value.set_null(); break;
                            }
                            break;
                        case integer:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case boolean: value.set_bool(value.get_int() != 0); break;
                                case uinteger: value.convert_to_uint(); break;
                                case real: value.set_real(value.get_int()); break;
                                case string: value.convert_to_string(); break;
                                case array: value.set_array(array_t()); break;
                                case object: value.set_object(object_t()); break;
                                default: value.set_null(); break;
                            }
                            break;
                        case uinteger:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case boolean: value.set_bool(value.get_uint() != 0); break;
                                case integer: value.convert_to_int(); break;
                                case real: value.set_real(value.get_uint()); break;
                                case string: value.convert_to_string(); break;
                                case array: value.set_array(array_t()); break;
                                case object: value.set_object(object_t()); break;
                                default: value.set_null(); break;
                            }
                            break;
                        case real:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case boolean: value.set_bool(value.get_real() != 0.0); break;
                                case integer: value.convert_to_int(); break;
                                case uinteger: value.convert_to_uint(); break;
                                case string: value.convert_to_string(); break;
                                case array: value.set_array(array_t()); break;
                                case object: value.set_object(object_t()); break;
                                default: value.set_null(); break;
                            }
                            break;
                        case string:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case boolean: value.set_bool(value.get_string() == "true" || value.as_int()); break;
                                case integer: value.convert_to_int(); break;
                                case uinteger: value.convert_to_uint(); break;
                                case real: value.convert_to_real(); break;
                                case array: value.set_array(array_t()); break;
                                case object: value.set_object(object_t()); break;
                                default: value.set_null(); break;
                            }
                            break;
                        case array:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case boolean: value.set_bool(false); break;
                                case integer: value.set_int(0); break;
                                case uinteger: value.set_uint(0); break;
                                case real: value.set_real(0.0); break;
                                case string: value.set_string(""); break;
                                case object:
                                {
                                    object_t obj;
                                    if (value.size() % 2 != 0)
                                        throw core::error("cppdatalib::core::stream_filter_converter - cannot convert 'array' to 'object' with odd number of elements");

                                    for (size_t i = 0; i < value.get_array().size(); i += 2)
                                        obj[value[i]] = value[i+1];

                                    value.set_object(obj);
                                    break;
                                }
                                default: value.set_null(); break;
                            }
                            break;
                        case object:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case boolean: value.set_bool(false); break;
                                case integer: value.set_int(0); break;
                                case uinteger: value.set_uint(0); break;
                                case real: value.set_real(0.0); break;
                                case string: value.set_string(""); break;
                                case array:
                                {
                                    array_t arr;

                                    for (auto it: value.get_object())
                                    {
                                        arr.push_back(it.first);
                                        arr.push_back(it.second);
                                    }

                                    value.set_array(arr);
                                    break;
                                }
                                default: value.set_null(); break;
                            }
                            break;
                        default: break;
                    }
                }
            };
        }

        enum buffer_filter_flags
        {
            buffer_strings = 0x01,
            buffer_arrays = 0x02,
            buffer_objects = 0x04
        };

        class buffer_filter : public core::stream_filter_base
        {
        protected:
            core::value cache_val;
            core::value_builder cache;

            buffer_filter_flags flags;

        public:
            buffer_filter(core::stream_handler &output, buffer_filter_flags flags)
                : stream_filter_base(output)
                , cache(cache_val)
                , flags(flags)
            {}
            buffer_filter(const buffer_filter &f)
                : stream_filter_base(f.output)
                , cache(cache_val)
                , flags(f.flags)
            {}
            buffer_filter(buffer_filter &&f)
                : stream_filter_base(f.output)
                , cache(cache_val)
                , flags(f.flags)
            {}

            bool requires_prefix_string_size() const {return flags & buffer_strings? false: stream_filter_base::requires_prefix_string_size();}
            bool requires_prefix_array_size() const {return flags & buffer_arrays? false: stream_filter_base::requires_prefix_array_size();}
            bool requires_prefix_object_size() const {return flags & buffer_objects? false: stream_filter_base::requires_prefix_object_size();}
            bool requires_string_buffering() const {return flags & buffer_strings? false: stream_filter_base::requires_string_buffering();}
            bool requires_array_buffering() const {return flags & buffer_arrays? false: stream_filter_base::requires_array_buffering();}
            bool requires_object_buffering() const {return flags & buffer_objects? false: stream_filter_base::requires_object_buffering();}

        protected:
            // Called when this filter is done buffering. This function should write the value to the output stream.
            // The default implementation is just a pass-through to the output, as you can see.
            virtual void write_buffered_value_(const value &v, bool is_key) {(void) is_key; output.write(v);}

            void begin_() {output.begin();}
            void end_()
            {
                if (cache.active())
                    cache.end();
                output.end();
            }

            bool write_(const value &v, bool is_key)
            {
                (void) is_key;
                if (cache.active())
                    return cache.write(v);
                else
                    return output.write(v);
            }

            void begin_array_(const value &v, int_t size, bool)
            {
                if (cache.active() || (flags & buffer_arrays && size == unknown_size))
                {
                    if (!cache.active())
                        cache.begin();

                    cache.begin_array(v, size);
                }
                else
                    output.begin_array(v, size);
            }
            void end_array_(const value &v, bool is_key)
            {
                if (cache.active())
                {
                    cache.end_array(v);

                    if (cache.nesting_depth() == 0) // This was the last container
                    {
                        cache.end();
                        write_buffered_value_(cache_val, is_key);
                        cache_val.set_null();
                    }
                }
                else
                    output.end_array(v);
            }

            void begin_object_(const value &v, int_t size, bool)
            {
                if (cache.active() || (flags & buffer_objects && size == unknown_size))
                {
                    if (!cache.active())
                        cache.begin();

                    cache.begin_object(v, size);
                }
                else
                    output.begin_object(v, size);
            }
            void end_object_(const value &v, bool is_key)
            {
                if (cache.active())
                {
                    cache.end_object(v);

                    if (cache.nesting_depth() == 0) // This was the last container
                    {
                        cache.end();
                        write_buffered_value_(cache_val, is_key);
                        cache_val.set_null();
                    }
                }
                else
                    output.end_object(v);
            }

            void begin_string_(const value &v, int_t size, bool)
            {
                if (cache.active() || (flags & buffer_strings && size == unknown_size))
                {
                    if (!cache.active())
                        cache.begin();

                    cache.begin_string(v, size);
                }
                else
                    output.begin_string(v, size);
            }
            void string_data_(const value &v, bool)
            {
                if (cache.active())
                    cache.append_to_string(v);
                else
                    output.append_to_string(v);
            }
            void end_string_(const value &v, bool is_key)
            {
                if (cache.active())
                {
                    cache.end_string(v);

                    if (cache.nesting_depth() == 0) // This was the last container
                    {
                        cache.end();
                        write_buffered_value_(cache_val, is_key);
                        cache_val.set_null();
                    }
                }
                else
                    output.end_string(v);
            }
        };

        class automatic_buffer_filter : public core::buffer_filter
        {
        public:
            automatic_buffer_filter(core::stream_handler &output)
                : buffer_filter(output, static_cast<buffer_filter_flags>(
                                    (output.requires_prefix_array_size() || output.requires_array_buffering()? buffer_arrays: 0) |
                                    (output.requires_prefix_object_size() || output.requires_object_buffering()? buffer_objects: 0) |
                                    (output.requires_prefix_string_size() || output.requires_string_buffering()? buffer_strings: 0)))
            {}
        };

        class tee_filter : public core::stream_filter_base
        {
        protected:
            core::stream_handler &output2;

        public:
            tee_filter(core::stream_handler &output,
                       core::stream_handler &output2)
                : stream_filter_base(output)
                , output2(output2)
            {}

            bool requires_prefix_string_size() const {return stream_filter_base::requires_prefix_string_size() || output2.requires_prefix_string_size();}
            bool requires_prefix_array_size() const {return stream_filter_base::requires_prefix_array_size() || output2.requires_prefix_array_size();}
            bool requires_prefix_object_size() const {return stream_filter_base::requires_prefix_object_size() || output2.requires_prefix_object_size();}
            bool requires_string_buffering() const {return stream_filter_base::requires_string_buffering() || output2.requires_string_buffering();}
            bool requires_array_buffering() const {return stream_filter_base::requires_array_buffering() || output2.requires_array_buffering();}
            bool requires_object_buffering() const {return stream_filter_base::requires_object_buffering() || output2.requires_object_buffering();}

        protected:
            void begin_() {output.begin(); output2.begin();}
            void end_() {output.end(); output2.end();}

            bool write_(const value &v, bool is_key)
            {
                (void) is_key;
                output.write(v);
                output2.write(v);
                return true;
            }

            void begin_array_(const value &v, int_t size, bool)
            {
                output.begin_array(v, size);
                output2.begin_array(v, size);
            }
            void end_array_(const value &v, bool)
            {
                output.end_array(v);
                output2.end_array(v);
            }

            void begin_object_(const value &v, int_t size, bool)
            {
                output.begin_object(v, size);
                output2.begin_object(v, size);
            }
            void end_object_(const value &v, bool)
            {
                output.end_object(v);
                output2.end_object(v);
            }

            void begin_string_(const value &v, int_t size, bool)
            {
                output.begin_string(v, size);
                output2.begin_string(v, size);
            }
            void string_data_(const value &v, bool)
            {
                output.append_to_string(v);
                output2.append_to_string(v);
            }
            void end_string_(const value &v, bool)
            {
                output.end_string(v);
                output2.end_string(v);
            }
        };

        // TODO: duplicate_key_check_filter doesn't do much good as a separate filter unless core::value supports duplicate-key maps
        class duplicate_key_check_filter : public core::stream_filter_base
        {
            class layer
            {
                void operator=(const layer &) {}

            public:
                layer() : key_builder(key) {}
                // WARNING: layers should NOT be copied while key_builder.active() is true!!!
                // This will corrupt the internal state, since value_builders cannot be copied!
                // The following constructors are SOLELY for interoperability with the STL, when the layer is inactive
                layer(const layer &other)
                    : key(other.key)
                    , key_builder(key)
                    , keys(other.keys)
                {}
                layer(layer &&other)
                    : key(std::move(other.key))
                    , key_builder(key)
                    , keys(std::move(other.keys))
                {}

                void begin() {key_builder.begin();}
                void end() {key_builder.end();}

                value key;
                value_builder key_builder;
                std::set<core::value> keys;
            };

            // WARNING: Underlying container type of `layer`s MUST not copy stored layers when adding to it,
            // so the value_builders in the layer don't get corrupted (i.e. DON'T USE A VECTOR)
            std::list<layer> layers;

        public:
            duplicate_key_check_filter(core::stream_handler &output) : stream_filter_base(output) {}

        protected:
            void begin_() {output.begin(); layers.clear();}
            void end_() {output.end();}

            void begin_key_(const value &) {layers.back().begin();}
            void end_key_(const value &)
            {
                layers.back().end();

                // Check against already parsed keys for current object, if it exists, throw an error
                if (layers.back().keys.find(layers.back().key) != layers.back().keys.end())
                    throw core::error("cppdatalib::core::duplicate_key_check_filter - invalid duplicate object key found");
                else
                    layers.back().keys.insert(layers.back().key);
            }

            void begin_scalar_(const value &v, bool)
            {
                output.write(v);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.write(v);
            }

            void begin_array_(const value &v, int_t size, bool)
            {
                output.begin_array(v, size);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.begin_array(v, size);
            }
            void end_array_(const value &v, bool)
            {
                output.end_array(v);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.end_array(v);
            }

            void begin_object_(const value &v, int_t size, bool)
            {
                output.begin_object(v, size);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.begin_object(v, size);
                layers.push_back(layer());
            }
            void end_object_(const value &v, bool)
            {
                output.end_object(v);
                layers.pop_back();
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.end_object(v);
            }

            void begin_string_(const value &v, int_t size, bool)
            {
                output.begin_string(v, size);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.begin_string(v, size);
            }
            void string_data_(const value &v, bool)
            {
                output.append_to_string(v);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.append_to_string(v);
            }
            void end_string_(const value &v, bool)
            {
                output.end_string(v);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.end_string(v);
            }
        };

        template<core::type from, core::type to>
        class converter_filter : public core::buffer_filter
        {
            static_assert(from != to, "cppdatalib::core::converter_filter - filter is being used as a pass-through filter");

            impl::stream_filter_converter<from, to> convert;

        public:
            converter_filter(core::stream_handler &output)
                : buffer_filter(output, static_cast<buffer_filter_flags>((from == core::string? buffer_strings: 0) |
                                                                         (from == core::array? buffer_arrays: 0) |
                                                                         (from == core::object? buffer_objects: 0)))
            {}

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                if (v.get_type() == from && from != to)
                {
                    value copy(v);
                    convert(copy);
                    return buffer_filter::write_buffered_value_(copy, is_key);
                }
                return buffer_filter::write_buffered_value_(v, is_key);
            }

            bool write_(const value &v, bool is_key)
            {
                if (v.get_type() == from && from != to)
                {
                    value copy(v);
                    convert(copy);
                    return buffer_filter::write_(copy, is_key);
                }
                return buffer_filter::write_(v, is_key);
            }
        };

        template<core::type from, typename Converter>
        class custom_converter_filter : public core::buffer_filter
        {
            Converter convert;

        public:
            custom_converter_filter(core::stream_handler &output, Converter c = Converter())
                : buffer_filter(output, static_cast<buffer_filter_flags>((from == core::string? buffer_strings: 0) |
                                                                         (from == core::array? buffer_arrays: 0) |
                                                                         (from == core::object? buffer_objects: 0)))
                , convert(c)
            {}

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                if (v.get_type() == from)
                {
                    value copy(v);
                    convert(copy);
                    return buffer_filter::write_buffered_value_(copy, is_key);
                }
                return buffer_filter::write_buffered_value_(v, is_key);
            }

            bool write_(const value &v, bool is_key)
            {
                if (v.get_type() == from)
                {
                    value copy(v);
                    convert(copy);
                    return buffer_filter::write_(copy, is_key);
                }
                return buffer_filter::write_(v, is_key);
            }
        };

        // TODO: Currently, conversions from arrays and objects are not supported.
        //       This is because conversion of arrays and objects would require a cache of the entire stream,
        //       and conversion of scalar values would then not operate properly.
        // Conversions to all types are supported.
        //
        // This class supports multiple scalar conversions in one filter.
        template<typename Converter>
        class generic_converter_filter : public buffer_filter
        {
            Converter convert;

        public:
            generic_converter_filter(core::stream_handler &output, Converter c = Converter())
                : buffer_filter(output, buffer_strings)
                , convert(c)
            {}

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                value copy(v);
                convert(copy);
                return buffer_filter::write_buffered_value_(copy, is_key);
            }

            bool write_(const value &v, bool is_key)
            {
                value copy(v);
                convert(copy);
                return buffer_filter::write_(copy, is_key);
            }
        };
    }
}

#endif // CPPDATALIB_STREAM_FILTERS_H

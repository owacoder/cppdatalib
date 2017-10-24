#ifndef CPPDATALIB_STREAM_FILTERS_H
#define CPPDATALIB_STREAM_FILTERS_H

#include "value_builder.h"
#include <set> // For duplicate_key_check_filter

namespace cppdatalib
{
    namespace core
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
                                    throw core::error("core::stream_filter_converter - cannot convert 'array' to 'object' with odd number of elements");

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

        class tee_filter : public core::stream_handler
        {
            core::stream_handler &output1, &output2;

        public:
            tee_filter(core::stream_handler &output1,
                          core::stream_handler &output2)
                : output1(output1)
                , output2(output2)
            {}

        protected:
            void begin_() {output1.begin(); output2.begin();}
            void end_() {output1.end(); output2.end();}

            bool write_(const value &v, bool is_key)
            {
                (void) is_key;
                output1.write(v);
                output2.write(v);
                return true;
            }

            void begin_array_(const value &v, int_t size, bool)
            {
                output1.begin_array(v, size);
                output2.begin_array(v, size);
            }
            void end_array_(const value &v, bool)
            {
                output1.end_array(v);
                output2.end_array(v);
            }

            void begin_object_(const value &v, int_t size, bool)
            {
                output1.begin_object(v, size);
                output2.begin_object(v, size);
            }
            void end_object_(const value &v, bool)
            {
                output1.end_object(v);
                output2.end_object(v);
            }

            void begin_string_(const value &v, int_t size, bool)
            {
                output1.begin_string(v, size);
                output2.begin_string(v, size);
            }
            void string_data_(const value &v, bool)
            {
                output1.append_to_string(v);
                output2.append_to_string(v);
            }
            void end_string_(const value &v, bool)
            {
                output1.end_string(v);
                output2.end_string(v);
            }
        };

        // TODO: duplicate_key_check_filter doesn't do much good as a separate filter unless core::value supports duplicate-key maps
        class duplicate_key_check_filter : public core::stream_handler
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

            core::stream_handler &output;

            // WARNING: Underlying container type of `layer`s MUST not copy stored layers when adding to it,
            // so the value_builders in the layer don't get corrupted (i.e. DON'T USE A VECTOR)
            std::list<layer> layers;

        public:
            duplicate_key_check_filter(core::stream_handler &output) : output(output) {}

        protected:
            void begin_() {output.begin();}
            void end_() {output.end();}

            void begin_key_(const value &) {layers.back().begin();}
            void end_key_(const value &)
            {
                layers.back().end();

                // Check against already parsed keys for current object, if it exists, throw an error
                if (layers.back().keys.find(layers.back().key) != layers.back().keys.end())
                    throw core::error("cppdatalib::duplicate_key_check_filter - Invalid duplicate object key found");
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

        // TODO: currently, conversions from arrays and objects are not supported.
        // Conversions to all types are supported
        template<core::type from, core::type to>
        class stream_filter : public core::stream_handler
        {
            core::stream_handler &output;
            core::value str;
            stream_filter_converter<from, to> convert;

        public:
            stream_filter(core::stream_handler &output) : output(output) {}

        protected:
            void begin_() {output.begin();}
            void end_() {output.end();}

            bool write_(const value &v, bool is_key)
            {
                (void) is_key;
                if (v.get_type() == from && from != to)
                {
                    value copy(v);
                    convert(copy);
                    return output.write(copy);
                }
                return output.write(v);
            }

            void begin_array_(const value &v, int_t size, bool) {output.begin_array(v, size);}
            void end_array_(const value &v, bool) {output.end_array(v);}

            void begin_object_(const value &v, int_t size, bool) {output.begin_object(v, size);}
            void end_object_(const value &v, bool) {output.end_object(v);}

            void begin_string_(const value &v, int_t size, bool)
            {
                if (from == core::string && from != to)
                    str.set_string("");
                else
                    output.begin_string(v, size);
            }
            void string_data_(const value &v, bool)
            {
                if (from == core::string && from != to)
                    str.get_string() += v.get_string();
                else
                    output.append_to_string(v);
            }
            void end_string_(const value &v, bool)
            {
                if (from == core::string && from != to)
                {
                    convert(str);
                    output.write(str);
                }
                else
                    output.end_string(v);
            }
        };

        // TODO: currently, conversions from arrays and objects are not supported.
        // Conversions to all types are supported
        template<core::type from, typename Converter>
        class custom_stream_filter : public core::stream_handler
        {
            core::stream_handler &output;
            core::value str;
            Converter convert;

        public:
            custom_stream_filter(core::stream_handler &output, Converter c = Converter()) : output(output), convert(c) {}

        protected:
            void begin_() {output.begin();}
            void end_() {output.end();}

            bool write_(const value &v, bool is_key)
            {
                (void) is_key;
                if (v.get_type() == from)
                {
                    value copy(v);
                    convert(copy);
                    return output.write(copy);
                }
                return output.write(v);
            }

            void begin_array_(const value &v, int_t size, bool) {output.begin_array(v, size);}
            void end_array_(const value &v, bool) {output.end_array(v);}

            void begin_object_(const value &v, int_t size, bool) {output.begin_object(v, size);}
            void end_object_(const value &v, bool) {output.end_object(v);}

            void begin_string_(const value &v, int_t size, bool)
            {
                if (from == core::string)
                    str.set_string("");
                else
                    output.begin_string(v, size);
            }
            void string_data_(const value &v, bool)
            {
                if (from == core::string)
                    str.get_string() += v.get_string();
                else
                    output.append_to_string(v);
            }
            void end_string_(const value &v, bool)
            {
                if (from == core::string)
                {
                    convert(str);
                    output.write(str);
                }
                else
                    output.end_string(v);
            }
        };

        // TODO: currently, conversions from arrays and objects are not supported.
        // Conversions to all types are supported
        template<typename Converter>
        class generic_stream_filter : public core::stream_handler
        {
            core::stream_handler &output;
            core::value str;
            Converter convert;

        public:
            generic_stream_filter(core::stream_handler &output, Converter c = Converter()) : output(output), convert(c) {}

        protected:
            void begin_() {output.begin();}
            void end_() {output.end();}

            bool write_(const value &v, bool is_key)
            {
                (void) is_key;
                value copy(v);
                convert(copy);
                return output.write(copy);
            }

            void begin_array_(const value &v, int_t size, bool) {output.begin_array(v, size);}
            void end_array_(const value &v, bool) {output.end_array(v);}

            void begin_object_(const value &v, int_t size, bool) {output.begin_object(v, size);}
            void end_object_(const value &v, bool) {output.end_object(v);}

            void begin_string_(const value &, int_t, bool) {str.set_string("");}
            void string_data_(const value &v, bool) {str.get_string() += v.get_string();}
            void end_string_(const value &, bool)
            {
                convert(str);
                output.write(str);
            }
        };
    }
}

#endif // CPPDATALIB_STREAM_FILTERS_H

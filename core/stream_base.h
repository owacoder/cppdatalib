/*
 * stream_base.h
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

#ifndef CPPDATALIB_STREAM_BASE_H
#define CPPDATALIB_STREAM_BASE_H

#include "value.h"
#include "istream.h"
#include "ostream.h"

namespace cppdatalib
{
    namespace core
    {
        inline bool stream_starts_with(core::istream &stream, const char *str)
        {
            int c;
            while (*str)
            {
                c = stream.get();
                if ((*str++ & 0xff) != c) return false;
            }
            return true;
        }

        class stream_writer
        {
        protected:
            core::ostream &output_stream;

        public:
            stream_writer(core::ostream &stream) : output_stream(stream) {}
            virtual ~stream_writer() {}

            core::ostream &stream() {return output_stream;}
        };

        class stream_handler
        {
        protected:
            struct scope_data
            {
                scope_data(type t, bool parsed_key = false)
                    : type_(t)
                    , parsed_key_(parsed_key)
                    , items_(0)
                {}

                type get_type() const {return type_;}
                size_t items_parsed() const {return items_;}
                bool key_was_parsed() const {return parsed_key_;}

                type type_; // The type of container that is being parsed
                bool parsed_key_; // false if the object key needs to be or is being parsed, true if it has already been parsed but the value associated with it has not
                size_t items_; // The number of items parsed into this container
            };

            bool active_;
            bool is_key_;

        public:
            stream_handler() : active_(false), is_key_(false) {}
            virtual ~stream_handler() {}

            enum {
                unknown_size = -1 // No other negative value besides unknown_size should be used for the `size` parameters of handlers
            };

            bool active() const {return active_;}

            void begin()
            {
                assert("cppdatalib::core::stream_handler - begin() called on active handler" && !active());

                active_ = true;
                nested_scopes = decltype(nested_scopes)();
                nested_scopes.push_back(scope_data(null));
                is_key_ = false;
                begin_();
            }
            void end()
            {
                assert("cppdatalib::core::stream_handler - end() called on inactive handler" && active());
                assert("cppdatalib::core::stream_handler - unexpected end of stream" && nested_scopes.size() == 1);

                if (nested_scopes.size() != 1)
                    throw error("cppdatalib::core::stream_handler - unexpected end of stream");

                end_();
                active_ = false;
            }

        protected:
            virtual void begin_() {}
            virtual void end_() {}

        public:
            // The following functions must be reimplemented to return true if the output format requires
            // an element's size to be specified before the actual object.
            // For example, the Netstring (https://en.wikipedia.org/wiki/Netstring)
            // `3:abc` requires the size to be written before the string itself.
            //
            // When these functions are reimplemented, the parser MAY have to cache the value before providing
            // it to the output handler
            virtual bool requires_prefix_string_size() const {return false;}
            virtual bool requires_prefix_array_size() const {return false;}
            virtual bool requires_prefix_object_size() const {return false;}

            // The following functions must be reimplemented to return true if the output format requires
            // an element to be entirely specified before writing.
            // For example, converting filters require that the entire element be processed before it can be converted.
            //
            // When these functions are reimplemented, the parser WILL have to cache the value before providing
            // it to the output handler.
            virtual bool requires_string_buffering() const {return false;}
            virtual bool requires_array_buffering() const {return false;}
            virtual bool requires_object_buffering() const {return false;}

            // Nesting depth is 0 before a container is created, and updated afterward
            // (increments after the begin_xxx() handler is called)
            // Nesting depth is > 0 before a container is ended, and updated afterward
            // (decrements after the end_xxx() handler is called)
            size_t nesting_depth() const {return nested_scopes.size() - 1;}

            type current_container() const
            {
                return nested_scopes.back().get_type();
            }

            // Container size is updated after the item was handled
            // (begins at 0 with no elements, the first element is handled, then it increments to 1)
            size_t current_container_size() const
            {
                return nested_scopes.back().items_parsed();
            }

            bool container_key_was_just_parsed() const
            {
                return nested_scopes.back().key_was_parsed();
            }

            // An API must call this when a scalar value is encountered,
            // although it should operate correctly for any value.
            // Returns true if value was handled, false otherwise
            bool write(const value &v)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

                const bool is_key =
                        nested_scopes.back().type_ == object &&
                        !nested_scopes.back().key_was_parsed();

                if (!write_(v, is_key))
                {
                    if ((v.is_array() || v.is_object()) && v.size() > 0)
                        *this << v;
                    else
                    {
                        if (is_key)
                            begin_key_(v);
                        else
                            begin_item_(v);

                        switch (v.get_type())
                        {
                            case string:
                                begin_string_(v, v.size(), is_key);
                                string_data_(v, is_key);
                                end_string_(v, is_key);
                                break;
                            case array:
                                begin_array_(v, 0, is_key);
                                end_array_(v, is_key);
                                break;
                            case object:
                                begin_object_(v, 0, is_key);
                                end_object_(v, is_key);
                                break;
                            default:
                                begin_scalar_(v, is_key);

                                switch (v.get_type())
                                {
                                    case null: null_(v); break;
                                    case boolean: bool_(v); break;
                                    case integer: integer_(v); break;
                                    case uinteger: uinteger_(v); break;
                                    case real: real_(v); break;
                                    default: return false;
                                }

                                end_scalar_(v, is_key);
                                break;
                        }

                        if (is_key)
                            end_key_(v);
                        else
                            end_item_(v);
                    }
                }

                if (nested_scopes.back().get_type() == object)
                {
                    nested_scopes.back().items_ += !is_key;
                    nested_scopes.back().parsed_key_ = !nested_scopes.back().key_was_parsed();
                }
                else if (nested_scopes.size() > 1)
                    ++nested_scopes.back().items_;

                return true;
            }

        protected:
            // Called when write() is written
            // Return value from external routine:
            //     true: item was written, cancel write routine
            //     false: item was not written, continue write routine
            virtual bool write_(const core::value &v, bool is_key) {(void) v; (void) is_key; return false;}

            // Called when any non-key item is parsed
            virtual void begin_item_(const core::value &v) {(void) v;}
            virtual void end_item_(const core::value &v) {(void) v;}

            // Called when any non-array, non-object, non-string item is parsed
            virtual void begin_scalar_(const core::value &v, bool is_key) {(void) v; (void) is_key;}
            virtual void end_scalar_(const core::value &v, bool is_key) {(void) v; (void) is_key;}

            // Called when object keys are parsed. Keys may be complex, and have other calls within these events.
            virtual void begin_key_(const core::value &v) {(void) v;}
            virtual void end_key_(const core::value &v) {(void) v;}

            // Called when a scalar null is parsed
            virtual void null_(const core::value &v) {(void) v;}

            // Called when a scalar boolean is parsed
            virtual void bool_(const core::value &v) {(void) v;}

            // Called when a scalar integer is parsed
            virtual void integer_(const core::value &v) {(void) v;}

            // Called when a scalar unsigned integer is parsed
            virtual void uinteger_(const core::value &v) {(void) v;}

            // Called when a scalar real is parsed
            virtual void real_(const core::value &v) {(void) v;}

            // Called when a scalar string is parsed
            // If the length of v is equal to size, the entire string is provided
            virtual void begin_string_(const core::value &v, core::int_t size, bool is_key) {(void) v; (void) size; (void) is_key;}
            virtual void string_data_(const core::value &v, bool is_key) {(void) v; (void) is_key;}
            virtual void end_string_(const core::value &v, bool is_key) {(void) v; (void) is_key;}

        public:
            // An API must call these when a long string is parsed. The number of bytes is passed in size, if possible
            // size < 0 means unknown size
            // If the length of v is equal to size, the entire string is provided
            void begin_string(const core::value &v, core::int_t size)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

                if (nested_scopes.back().type_ == object &&
                    !nested_scopes.back().key_was_parsed())
                {
                    begin_key_(v);
                    begin_string_(v, size, is_key_ = true);
                }
                else
                {
                    begin_item_(v);
                    begin_string_(v, size, is_key_ = false);
                }

                nested_scopes.push_back(string);
            }
            void append_to_string(const core::value &v)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

#ifndef CPPDATALIB_DISABLE_WRITE_CHECKS
                if (nested_scopes.back().get_type() != string)
                    throw error("cppdatalib::core::stream_handler - attempted to append to string that was never begun");
#endif

                string_data_(v, is_key_);
                nested_scopes.back().items_ += v.string_size();
            }
            void end_string(const core::value &v)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

#ifndef CPPDATALIB_DISABLE_WRITE_CHECKS
                if (nested_scopes.back().get_type() != string)
                    throw error("cppdatalib::core::stream_handler - attempted to end string that was never begun");
#endif

                if (is_key_)
                {
                    end_string_(v, true);
                    end_key_(v);
                }
                else
                {
                    end_string_(v, false);
                    end_item_(v);
                }
                nested_scopes.pop_back();

                if (nested_scopes.back().get_type() == object)
                {
                    nested_scopes.back().items_ += nested_scopes.back().key_was_parsed();
                    nested_scopes.back().parsed_key_ = !nested_scopes.back().key_was_parsed();
                }
                else if (nested_scopes.size() > 1)
                    ++nested_scopes.back().items_;
            }

            // An API must call these when an array is parsed. The number of elements is passed in size, if possible
            // size < 0 means unknown size
            // If the number of elements of v is equal to size, the entire array is provided
            void begin_array(const core::value &v, core::int_t size)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

                if (nested_scopes.back().type_ == object &&
                    !nested_scopes.back().key_was_parsed())
                {
                    begin_key_(v);
                    begin_array_(v, size, true);
                }
                else
                {
                    begin_item_(v);
                    begin_array_(v, size, false);
                }

                nested_scopes.push_back(array);
            }
            void end_array(const core::value &v)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

#ifndef CPPDATALIB_DISABLE_WRITE_CHECKS
                if (nested_scopes.back().get_type() != array)
                    throw error("cppdatalib::core::stream_handler - attempted to end array that was never begun");
#endif

                if (nested_scopes[nested_scopes.size() - 2].get_type() == object &&
                    !nested_scopes[nested_scopes.size() - 2].key_was_parsed())
                {
                    end_array_(v, true);
                    end_key_(v);
                }
                else
                {
                    end_array_(v, false);
                    end_item_(v);
                }
                nested_scopes.pop_back();

                if (nested_scopes.back().get_type() == object)
                {
                    nested_scopes.back().items_ += nested_scopes.back().key_was_parsed();
                    nested_scopes.back().parsed_key_ = !nested_scopes.back().key_was_parsed();
                }
                else if (nested_scopes.size() > 1)
                    ++nested_scopes.back().items_;
            }

            // An API must call these when an object is parsed. The number of key/value pairs is passed in size, if possible
            // size < 0 means unknown size
            // If the number of elements of v is equal to size, the entire object is provided
            void begin_object(const core::value &v, core::int_t size)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

                if (nested_scopes.back().type_ == object &&
                    !nested_scopes.back().key_was_parsed())
                {
                    begin_key_(v);
                    begin_object_(v, size, true);
                }
                else
                {
                    begin_item_(v);
                    begin_object_(v, size, false);
                }

                nested_scopes.push_back(object);
            }
            void end_object(const core::value &v)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

#ifndef CPPDATALIB_DISABLE_WRITE_CHECKS
                if (nested_scopes.back().get_type() != object)
                    throw error("cppdatalib::core::stream_handler - attempted to end object that was never begun");
                else if (nested_scopes.back().key_was_parsed())
                    throw error("cppdatalib::core::stream_handler - attempted to end object before final value was written");
#endif

                if (nested_scopes[nested_scopes.size() - 2].get_type() == object &&
                    !nested_scopes[nested_scopes.size() - 2].key_was_parsed())
                {
                    end_object_(v, true);
                    end_key_(v);
                }
                else
                {
                    end_object_(v, false);
                    end_item_(v);
                }
                nested_scopes.pop_back();

                if (nested_scopes.back().get_type() == object)
                {
                    nested_scopes.back().items_ += nested_scopes.back().key_was_parsed();
                    nested_scopes.back().parsed_key_ = !nested_scopes.back().key_was_parsed();
                }
                else if (nested_scopes.size() > 1)
                    ++nested_scopes.back().items_;
            }

        protected:
            // Overloads to detect beginnings and ends of arrays
            virtual void begin_array_(const core::value &v, core::int_t size, bool is_key) {(void) v; (void) size; (void) is_key;}
            virtual void end_array_(const core::value &v, bool is_key) {(void) v; (void) is_key;}

            // Overloads to detect beginnings and ends of objects
            virtual void begin_object_(const core::value &v, core::int_t size, bool is_key) {(void) v; (void) size; (void) is_key;}
            virtual void end_object_(const core::value &v, bool is_key) {(void) v; (void) is_key;}

            std::vector<scope_data> nested_scopes; // Used as a stack, but not a stack so we can peek below the top
        };

        class stream_input
        {
        public:
            virtual ~stream_input() {}

            // The following functions should be reimplemented to return true if the parser ALWAYS provides the element
            // size (for each respective type) in the begin_xxx() functions.
            virtual bool provides_prefix_string_size() const {return false;}
            virtual bool provides_prefix_array_size() const {return false;}
            virtual bool provides_prefix_object_size() const {return false;}

            // The following functions should be reimplemented to return true if the parser ALWAYS provides the entire
            // (for each respective type) in the begin_xxx() and end_xxx() functions. (or uses the write() function exclusively,
            // which requires the entire value anyway)
            virtual bool provides_buffered_strings() const {return false;}
            virtual bool provides_buffered_arrays() const {return false;}
            virtual bool provides_buffered_objects() const {return false;}

            virtual stream_input &convert(core::stream_handler &output) = 0;
        };

        class stream_parser : public stream_input
        {
        protected:
            core::istream &input_stream;

        public:
            stream_parser(core::istream &input) : input_stream(input) {}
            virtual ~stream_parser() {}

            core::istream &stream() {return input_stream;}
        };

        // Convert directly from parser to serializer
        stream_handler &operator<<(stream_handler &output, stream_input &input)
        {
            if (output.requires_string_buffering() > input.provides_buffered_strings() ||
                output.requires_array_buffering() > input.provides_buffered_arrays() ||
                output.requires_object_buffering() > input.provides_buffered_objects() ||
                output.requires_prefix_string_size() > input.provides_prefix_string_size() ||
                output.requires_prefix_array_size() > input.provides_prefix_array_size() ||
                output.requires_prefix_object_size() > input.provides_prefix_object_size())
                throw core::error("cppdatalib::stream_handler::operator<<() - output requires buffering capabilities the input doesn't provide. Using cppdatalib::core::automatic_buffer_filter on the output stream will fix this problem.");

            const bool stream_ready = output.active();

            if (!stream_ready)
                output.begin();
            input.convert(output);
            if (!stream_ready)
                output.end();

            return output;
        }

        // Convert directly from parser to serializer
        stream_input &operator>>(stream_input &input, stream_handler &output)
        {
            if (output.requires_string_buffering() > input.provides_buffered_strings() ||
                output.requires_array_buffering() > input.provides_buffered_arrays() ||
                output.requires_object_buffering() > input.provides_buffered_objects() ||
                output.requires_prefix_string_size() > input.provides_prefix_string_size() ||
                output.requires_prefix_array_size() > input.provides_prefix_array_size() ||
                output.requires_prefix_object_size() > input.provides_prefix_object_size())
                throw core::error("cppdatalib::stream_input::operator>>() - output requires buffering capabilities the input doesn't provide. Using cppdatalib::core::automatic_buffer_filter on the output stream will fix this problem.");

            const bool stream_ready = output.active();

            if (!stream_ready)
                output.begin();
            input.convert(output);
            if (!stream_ready)
                output.end();

            return input;
        }

        struct value::traverse_node_prefix_serialize
        {
            traverse_node_prefix_serialize(stream_handler &handler) : stream(handler) {}

            bool operator()(const value *arg, value::traversal_ancestry_finder)
            {
                if (arg->is_array())
                    stream.begin_array(*arg, arg->size());
                else if (arg->is_object())
                    stream.begin_object(*arg, arg->size());
                return true;
            }

        private:
            stream_handler &stream;
        };

        struct value::traverse_node_postfix_serialize
        {
            traverse_node_postfix_serialize(stream_handler &handler) : stream(handler) {}

            bool operator()(const value *arg, value::traversal_ancestry_finder)
            {
                if (arg->is_array())
                    stream.end_array(*arg);
                else if (arg->is_object())
                    stream.end_object(*arg);
                else
                    stream.write(*arg);
                return true;
            }

        private:
            stream_handler &stream;
        };

        struct value::traverse_less_than_compare_prefix
        {
        private:
            int compare;

        public:
            traverse_less_than_compare_prefix() : compare(0) {}

            int comparison() const {return compare;}

            void run(const value *arg, const value *arg2)
            {
                if (!compare)
                {
                    if (arg == NULL && arg2 != NULL)
                        compare = -1;
                    else if (arg != NULL && arg2 == NULL)
                        compare = 1;
                    else if (arg != NULL && arg2 != NULL)
                    {
                        if (arg->get_type() < arg2->get_type())
                            compare = -1;
                        else if (arg->get_type() > arg2->get_type())
                            compare = 1;
                        else
                        {
                            if (arg->get_subtype() < arg2->get_subtype())
                                compare = -1;
                            else if (arg->get_subtype() > arg2->get_subtype())
                                compare = 1;
                            else
                            {
                                switch (arg->get_type())
                                {
                                    case boolean:
                                        compare = -(arg->get_bool_unchecked() < arg2->get_bool_unchecked());
                                        break;
                                    case integer:
                                        compare = -(arg->get_int_unchecked() < arg2->get_int_unchecked());
                                        break;
                                    case uinteger:
                                        compare = -(arg->get_uint_unchecked() < arg2->get_uint_unchecked());
                                        break;
                                    case real:
                                        compare = -(arg->get_real_unchecked() < arg2->get_real_unchecked());
                                        break;
                                    case string:
                                        compare = -(arg->get_string_unchecked() < arg2->get_string_unchecked());
                                        break;
                                    case array:
                                    case object:
                                    case null:
                                    default:
                                        break;
                                }
                            }
                        }
                    }
                }
            }

            bool operator()(const value *arg, const value *arg2, value::traversal_ancestry_finder, value::traversal_ancestry_finder)
            {
                run(arg, arg2);
                return compare == 0;
            }
        };

        struct value::traverse_compare_prefix
        {
        private:
            int compare;

        public:
            traverse_compare_prefix() : compare(0) {}

            int comparison() const {return compare;}

            void run(const value *arg, const value *arg2)
            {
                if (!compare)
                {
                    if (arg == NULL && arg2 != NULL)
                        compare = -1;
                    else if (arg != NULL && arg2 == NULL)
                        compare = 1;
                    else if (arg != NULL && arg2 != NULL)
                    {
                        if (arg->get_type() < arg2->get_type())
                            compare = -1;
                        else if (arg->get_type() > arg2->get_type())
                            compare = 1;
                        else
                        {
                            if (arg->get_subtype() < arg2->get_subtype())
                                compare = -1;
                            else if (arg->get_subtype() > arg2->get_subtype())
                                compare = 1;
                            else
                            {
                                switch (arg->get_type())
                                {
                                    case boolean:
                                        compare = (arg->get_bool_unchecked() > arg2->get_bool_unchecked()) - (arg->get_bool_unchecked() < arg2->get_bool_unchecked());
                                        break;
                                    case integer:
                                        compare = (arg->get_int_unchecked() > arg2->get_int_unchecked()) - (arg->get_int_unchecked() < arg2->get_int_unchecked());
                                        break;
                                    case uinteger:
                                        compare = (arg->get_uint_unchecked() > arg2->get_uint_unchecked()) - (arg->get_uint_unchecked() < arg2->get_uint_unchecked());
                                        break;
                                    case real:
                                        compare = (arg->get_real_unchecked() > arg2->get_real_unchecked()) - (arg->get_real_unchecked() < arg2->get_real_unchecked());
                                        break;
                                    case string:
                                        compare = (arg->get_string_unchecked() > arg2->get_string_unchecked()) - (arg->get_string_unchecked() < arg2->get_string_unchecked());
                                        break;
                                    case array:
                                    case object:
                                    case null:
                                    default:
                                        break;
                                }
                            }
                        }
                    }
                }
            }

            bool operator()(const value *arg, const value *arg2, value::traversal_ancestry_finder, value::traversal_ancestry_finder)
            {
                run(arg, arg2);
                return compare == 0;
            }
        };

        struct value::traverse_equality_compare_prefix
        {
        private:
            bool equal;

        public:
            traverse_equality_compare_prefix() : equal(true) {}

            bool comparison_equal() const {return equal;}

            void run(const value *arg, const value *arg2)
            {
                if (equal)
                {
                    if (arg == NULL && arg2 != NULL)
                        equal = false;
                    else if (arg != NULL && arg2 == NULL)
                        equal = false;
                    else if (arg != NULL && arg2 != NULL)
                    {
                        if (arg->get_type() != arg2->get_type() ||
                                arg->get_subtype() != arg2->get_subtype())
                            equal = false;
                        else
                        {
                            switch (arg->get_type())
                            {
                                case boolean:
                                    equal = (arg->get_bool_unchecked() == arg2->get_bool_unchecked());
                                    break;
                                case integer:
                                    equal = (arg->get_int_unchecked() == arg2->get_int_unchecked());
                                    break;
                                case uinteger:
                                    equal = (arg->get_uint_unchecked() == arg2->get_uint_unchecked());
                                    break;
                                case real:
                                    equal = (arg->get_real_unchecked() == arg2->get_real_unchecked());
                                    break;
                                case string:
                                    equal = (arg->get_string_unchecked() == arg2->get_string_unchecked());
                                    break;
                                case array:
                                case object:
                                    equal = (arg->size() == arg2->size());
                                    break;
                                case null:
                                default:
                                    break;
                            }
                        }
                    }
                }
            }

            bool operator()(const value *arg, const value *arg2, value::traversal_ancestry_finder, value::traversal_ancestry_finder)
            {
                run(arg, arg2);
                return equal;
            }
        };

        struct value::traverse_compare_postfix
        {
        public:
            bool operator()(const value *,
                            const value *,
                            value::traversal_ancestry_finder,
                            value::traversal_ancestry_finder)
            {
                return true;
            }
        };
    }
}

#endif // CPPDATALIB_STREAM_BASE_H

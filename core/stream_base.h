/*
 * stream_base.h
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

#ifndef CPPDATALIB_STREAM_BASE_H
#define CPPDATALIB_STREAM_BASE_H

#include "utf.h"
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

        /* The base class of all output formats that output to an output stream.
         * All output formats should inherit this class if they write to a stream.
         */
        class stream_writer
        {
            core::ostream_handle handle;

        public:
            stream_writer(core::ostream_handle &output) : handle(output) {}
            virtual ~stream_writer() {}

            core::ostream &stream() {return handle.stream();}
            // May return NULL! See core::ostream_handle::std_stream() for details.
            std::ostream *std_stream() {return handle.std_stream();}
        };

        /* The base class of all output formats. All output formats must inherit from this class.
         */
        class stream_handler
        {
        protected:
            struct scope_data
            {
                scope_data(type t, subtype_t s, core::int_t reported_size, bool parsed_key = false)
                    : type_(t)
                    , subtype_(s)
                    , parsed_key_(parsed_key)
                    , items_(0)
                    , reported_size_(reported_size)
                {}

                type get_type() const {return type_;}
                subtype_t get_subtype() const {return subtype_;}
                uintmax_t items_parsed() const {return items_;}
                bool key_was_parsed() const {return parsed_key_;}
                int_t reported_size() const {return reported_size_;}

                type type_; // The type of container that is being parsed
                subtype_t subtype_; // The subtype of container that is being parsed
                bool parsed_key_; // false if the object key needs to be or is being parsed, true if it has already been parsed but the value associated with it has not
                uintmax_t items_; // The number of items parsed into this container
                int_t reported_size_; // The number of items reported to be in this container
            };

            bool active_;
            bool is_key_;

        public:
            static const unsigned int requires_none = 0x00;

            // The following values must be set (in `required_features()`) if the output format requires
            // an element's size to be specified before the actual object.
            // For example, the Netstring (https://en.wikipedia.org/wiki/Netstring)
            // `3:abc` requires the size to be written before the string itself.
            //
            // When these values are requested, the parser MAY have to cache the value before providing
            // it to the output handler
            static const unsigned int requires_prefix_array_size = 0x01;
            static const unsigned int requires_prefix_object_size = 0x02;
            static const unsigned int requires_prefix_string_size = 0x04;

            // The following values must be set (in `required_features()`) if the output format requires
            // an element to be entirely specified before writing.
            // For example, converting filters require that the entire element be processed before it can be converted.
            //
            // When these values are requested, the parser WILL have to cache the value before providing
            // it to the output handler.
            static const unsigned int requires_buffered_arrays = 0x08;
            static const unsigned int requires_buffered_objects = 0x10;
            static const unsigned int requires_buffered_strings = 0x20;

            // The following value must be set (in `required_features()`) if the output format requires
            // an element to be written in a single write() call
            static const unsigned int requires_single_write = 0x7f;

            enum {
                unknown_size = -1 // No other negative value besides unknown_size should be used for the `size` parameters of handlers
            };

            stream_handler() : active_(false), is_key_(false)
            {
                nested_scopes.push_back(scope_data(null, normal, unknown_size));
            }
            virtual ~stream_handler() {}

            // Returns all the features required by this handler
            virtual unsigned int required_features() const {return requires_none;}

            // Returns the user-friendly name of this output format
            virtual std::string pretty_name() const {return name();}

            // Returns the name of this output format
            virtual std::string name() const {return "cppdatalib::core::stream_handler";}

            // Returns true if this handler is active (i.e. `begin()` has been called but `end()` has not)
            bool active() const {return active_;}

            // Begins output on this handler
            void begin()
            {
                assert("cppdatalib::core::stream_handler - begin() called on active handler" && !active());

                active_ = true;
                nested_scopes = decltype(nested_scopes)();
                nested_scopes.push_back(scope_data(null, normal, unknown_size));
                out_of_order_buffer.clear();
                is_key_ = false;
                begin_();
            }

            // Ends output on this handler
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
            // Nesting depth is 0 before a container is created, and updated afterward
            // (increments after the begin_xxx() handler is called)
            // Nesting depth is > 0 before a container is ended, and updated afterward
            // (decrements after the end_xxx() handler is called)
            size_t nesting_depth() const {return nested_scopes.size() - 1;}

            // Current container is array, object, string, or null if no container is present
            type current_container() const {return nested_scopes.back().get_type();}

            // Current subtype of current container, or normal if no container is present
            subtype_t current_container_subtype() const {return nested_scopes.back().get_subtype();}

            // Parent container is array, object, or null if no parent is present
            type parent_container() const {return nested_scopes.size() > 1? nested_scopes[nested_scopes.size() - 2].get_type(): null;}

            // Container size is updated after the item was handled
            // (begins at 0 with no elements, the first element is handled, then it increments to 1)
            uintmax_t current_container_size() const {return nested_scopes.back().items_parsed();}

            // Reported container size (if known, the exact reported size is returned, otherwise `unknown_size`)
            int_t current_container_reported_size() const {return nested_scopes.back().reported_size();}

            // Returns true if this is an object and a value of a key/value pair is expected
            bool container_key_was_just_parsed() const {return nested_scopes.back().key_was_parsed();}

            // An API must call this when a scalar value is encountered,
            // although it should operate correctly for any value.
            //
            // Comments and program instructions will be written as such,
            // or discarded if the output format does not support them.
            // They do not affect nesting, validity, or other constraints.
            //
            // Returns true if value was handled, false otherwise
            bool write(const value &v)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

                const bool is_key =
                        nested_scopes.back().type_ == object &&
                        !nested_scopes.back().key_was_parsed();

                if (v.get_subtype() == comment && v.is_string())
                {
                    comment_(v);
                    return true;
                }
                else if (v.get_subtype() == program_directive && v.is_string())
                {
                    directive_(v);
                    return true;
                }

                if (!write_(v, is_key))
                {
                    if (v.is_nonempty_array() || v.is_nonempty_object())
                    {
                        *this << v;
                        return true; // Skip post-processing, since the '<<' operator works all that out anyway
                    }
                    else
                    {
                        bool remove_scope = false;

                        if (is_key)
                            begin_key_(v);
                        else
                            begin_item_(v);

                        switch (v.get_type())
                        {
                            case string:
                                begin_string_(v, v.size(), is_key);
                                nested_scopes.push_back({string, v.get_subtype(), core::int_t(v.size())});
                                remove_scope = true;
                                string_data_(v, is_key);
                                end_string_(v, is_key);
                                break;
                            case array:
                                begin_array_(v, 0, is_key);
                                nested_scopes.push_back({array, v.get_subtype(), 0});
                                remove_scope = true;
                                end_array_(v, is_key);
                                break;
                            case object:
                                begin_object_(v, 0, is_key);
                                nested_scopes.push_back({object, v.get_subtype(), 0});
                                remove_scope = true;
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

                        if (remove_scope)
                            nested_scopes.pop_back();
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

            // An API must call this when a comment is requested to be written.
            // Comments are not guaranteed to be supported by the output format.
            // The default implementation does nothing
            void write_comment(const core::value &v)
            {
                comment_(v);
            }

            // An API must call this when a program directive is requested to be written.
            // Program directives are not guaranteed to be supported by the output format.
            // The default implementation does nothing
            void write_program_directive(const core::value &v)
            {
                directive_(v);
            }

            // An API must call this when an out-of-order write is requested
            // for an array write. This works for scalars or complex objects.
            //
            // Out-of-order writes can only be requested when writing arrays,
            // and an exception will be thrown otherwise.
            //
            // By default, this function checks that each index is used at most once, and
            // throws an exception otherwise. If write_out_of_order_() is overloaded, this guarantee
            // does not hold, and use of an index more than once results in format-specific behavior.
            //
            // It also detects writes beyond the reported array bounds immediately,
            // and throws an exception otherwise. This behavior is always present.
            //
            // When the array is done being written, empty (unspecified) indexes have a normal
            // null value written to them. This includes indexes past written values, up to the end of the array.
            // The resulting array size is dependent on the reported array size:
            //
            //     If the reported size is unknown - The resulting array size is one larger than the highest written index
            //     If the reported size is known - The resulting array size is equal to the reported size
            //
            // If `write_out_of_order_()` is thread-safe and returns true (note that this is not the default behavior),
            // this function is also thread-safe, as long as no other begin_xxx(), end_xxx(), or write function is called on this instance
            // in the meantime.
            //
            // Returns true if value was handled, false otherwise
            bool write_out_of_order(uint64_t idx, const value &v)
            {
                bool result = true;

                if (current_container() != array)
                    throw error("cppdatalib::core::stream_handler - An out-of-order write can only be performed while writing an array");
                else if (current_container_reported_size() != unknown_size &&
                         uintmax_t(current_container_reported_size()) <= idx)
                    throw error("cppdatalib::core::stream_handler - An out-of-order write was attempted beyond reported array bounds");

                if (write_out_of_order_(idx, v, false))
                    return true;

                if (current_container_size() == idx)
                    result = write(v);
                else if (current_container_size() > idx || !out_of_order_buffer.insert({idx, v}).second)
                    throw custom_error("cppdatalib::core::stream_handler - An out-of-order write on index " + std::to_string(idx) + " was performed more than once");

                while (out_of_order_buffer.size() && out_of_order_buffer.begin()->first == current_container_size())
                {
                    if (!write(out_of_order_buffer.begin()->second))
                        return false;
                    out_of_order_buffer.erase(out_of_order_buffer.begin());
                }

                return result;
            }

        protected:
            // Called when write() is written
            // Return value from external routine:
            //     true: item was written, cancel write routine
            //     false: item was not written, continue write routine
            virtual bool write_(const core::value &v, bool is_key) {(void) v; (void) is_key; return false;}

            // Called when write_out_of_order() is written
            // Return value from external routine:
            //     true: item was written, cancel write routine
            //     false: item was not written, continue write routine
            // is_key is always false
            virtual bool write_out_of_order_(uint64_t idx, const core::value &v, bool is_key) {(void) idx; (void) v; (void) is_key; return false;}

            // Called when write_comment() is written
            virtual void comment_(const core::value &v) {(void) v;}

            // Called when write_program_directive() is written
            virtual void directive_(const core::value &v) {(void) v;}

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
            // size == -1 means unknown size
            //
            // Comments and processing instructions will be passed as such to the resulting handler, but no special action is taken.
            // If special action should be taken for a comment or processing instruction, the entire value should be passed as a string
            // (with the proper subtype) to `write()`, or passed to `write_comment()` or `write_directive()`, respectively, regardless of
            // whether it is a string or not. The latter options don't require values to be strings.
            //
            // If the length of v is equal to size, the entire string is provided
            void begin_string(const core::string_t &v, core::int_t size) {begin_string(value(v), size);}
            void begin_string(const core::value &v, core::int_t size)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

#ifndef CPPDATALIB_DISABLE_WRITE_CHECKS
                if (!v.is_string())
                    throw error("cppdatalib::core::stream_handler - attempted to begin string with non-string value");
#endif

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

                nested_scopes.push_back({string, v.get_subtype(), size});
            }
            void append_to_string(const core::string_t &v) {append_to_string(value(v));}
            void append_to_string(const core::value &v)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

#ifndef CPPDATALIB_DISABLE_WRITE_CHECKS
                if (nested_scopes.back().get_type() != string)
                    throw error("cppdatalib::core::stream_handler - attempted to append to string that was never begun");
                else if (!v.is_string())
                    throw error("cppdatalib::core::stream_handler - attempted to append non-string value to string");
#endif

                string_data_(v, is_key_);
                nested_scopes.back().items_ += v.string_size();
            }
            void end_string(const core::string_t &v) {end_string(value(v));}
            void end_string(const core::value &v)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

#ifndef CPPDATALIB_DISABLE_WRITE_CHECKS
                if (nested_scopes.back().get_type() != string)
                    throw error("cppdatalib::core::stream_handler - attempted to end string that was never begun");
                else if (!v.is_string())
                    throw error("cppdatalib::core::stream_handler - attempted to end string with non-string value");
#endif

                if (current_container_reported_size() != unknown_size &&
                        uintmax_t(current_container_reported_size()) != current_container_size())
                    throw error("cppdatalib::core::stream_handler - reported string size and actual string size do not match");

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
            // size == -1 means unknown size
            // If the number of elements of v is equal to size, the entire array is provided
            void begin_array(const core::array_t &v, core::int_t size) {begin_array(value(v), size);}
            void begin_array(const core::value &v, core::int_t size)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

#ifndef CPPDATALIB_DISABLE_WRITE_CHECKS
                if (!v.is_array())
                    throw error("cppdatalib::core::stream_handler - attempted to begin array with non-array value");
#endif

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

                nested_scopes.push_back({array, v.get_subtype(), size});
            }
            void end_array(const core::array_t &v) {end_array(value(v));}
            void end_array(const core::value &v)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

#ifndef CPPDATALIB_DISABLE_WRITE_CHECKS
                if (nested_scopes.back().get_type() != array)
                    throw error("cppdatalib::core::stream_handler - attempted to end array that was never begun");
                else if (!v.is_array())
                    throw error("cppdatalib::core::stream_handler - attempted to end array with non-array value");
#endif

                if (out_of_order_buffer.size())
                {
                    while (out_of_order_buffer.size())
                    {
                        if (current_container_size() < out_of_order_buffer.begin()->first)
                            write(core::value());
                        else // current_container_size() == out_of_order_buffer.begin()->first,
                             // since elements in out_of_order_buffer are always cleared out by write_out_of_order() when possible
                        {
                            write(out_of_order_buffer.begin()->second);
                            out_of_order_buffer.erase(out_of_order_buffer.begin());
                        }
                    }

                    // If unknown size, assume the highest index written to is the end of the array
                    // Otherwise, fill to the reported size
                    if (current_container_reported_size() != unknown_size)
                        while (current_container_size() < uintmax_t(current_container_reported_size()))
                            write(core::value());
                }

                if (current_container_reported_size() != unknown_size &&
                        uintmax_t(current_container_reported_size()) != current_container_size())
                    throw error("cppdatalib::core::stream_handler - reported array size and actual array size do not match");

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
            // size == -1 means unknown size
            // If the number of elements of v is equal to size, the entire object is provided
            void begin_object(const core::object_t &v, core::int_t size) {begin_object(value(v), size);}
            void begin_object(const core::value &v, core::int_t size)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

#ifndef CPPDATALIB_DISABLE_WRITE_CHECKS
                if (!v.is_object())
                    throw error("cppdatalib::core::stream_handler - attempted to begin object with non-object value");
#endif

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

                nested_scopes.push_back({object, v.get_subtype(), size});
            }
            void end_object(const core::object_t &v) {end_object(value(v));}
            void end_object(const core::value &v)
            {
                assert("cppdatalib::core::stream_handler - begin() must be called before handler can be used" && active());

#ifndef CPPDATALIB_DISABLE_WRITE_CHECKS
                if (nested_scopes.back().get_type() != object)
                    throw error("cppdatalib::core::stream_handler - attempted to end object that was never begun");
                else if (nested_scopes.back().key_was_parsed())
                    throw error("cppdatalib::core::stream_handler - attempted to end object before final value was written");
                else if (!v.is_object())
                    throw error("cppdatalib::core::stream_handler - attempted to end object with non-object value");
#endif

                if (current_container_reported_size() != unknown_size &&
                        uintmax_t(current_container_reported_size()) != current_container_size())
                    throw error("cppdatalib::core::stream_handler - reported object size and actual object size do not match");

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
            std::map<uint64_t, core::value> out_of_order_buffer; // Used for out-of-order write buffering if it is not supported by the handler
        };

        /* The base class of all input formats. All input formats must inherit from this class */
        class stream_input
        {
        private:
            core::stream_handler *output;
            bool mReset; // Whether the input has just been reset()

        protected:
            size_t initial_nesting_level;

        public:
            static const unsigned int provides_none = 0x00;

            // The following values should return true if the parser ALWAYS provides the element
            // size (for each respective type) in the begin_xxx() functions.
            static const unsigned int provides_prefix_array_size = 0x01;
            static const unsigned int provides_prefix_object_size = 0x02;
            static const unsigned int provides_prefix_string_size = 0x04;

            // The following values should return true if the parser ALWAYS provides the entire value
            // (for each respective type) in the begin_xxx() and end_xxx() functions. (or uses the write() function exclusively,
            // which requires the entire value anyway)
            static const unsigned int provides_buffered_arrays = 0x08;
            static const unsigned int provides_buffered_objects = 0x10;
            static const unsigned int provides_buffered_strings = 0x20;

            // The following value should return true if the parser ALWAYS provides the entire value as a single write() call
            static const unsigned int provides_single_write = 0x7f;

            stream_input() : output(NULL), mReset(false), initial_nesting_level(0) {}
            stream_input(core::stream_handler &output) : output(&output), mReset(false), initial_nesting_level(output.nesting_depth()) {}
            virtual ~stream_input() {}

            // Resets the input class to start parsing a new stream (NOTE: should NOT attempt to seek to the beginning of the stream!)
            void reset()
            {
                reset_();
                mReset = true;
            }

            // Returns whether the input is in its initial state (just reset)
            bool was_just_reset() const {return mReset;}

            // Returns all the features provided by this class
            virtual unsigned int features() const {return provides_none;}

            // Returns false if nothing is being parsed, true if in the middle of parsing
            virtual bool busy() const {return output && output->nesting_depth() > initial_nesting_level;}

            // Sets the current output handler (does nothing if set while `busy()`)
            void set_output(core::stream_handler &output)
            {
                if (!busy())
                {
                    const bool changed = &output != this->output;
                    this->output = &output;
                    initial_nesting_level = output.nesting_depth();
                    if (changed)
                        output_changed_();
                }
            }

            // Returns the current output handler, or NULL if not set.
            const core::stream_handler *get_output() const {return output;}
            core::stream_handler *get_output() {return output;}

            // Returns true if an output is bound to this stream_input
            bool has_output() const {return output;}

            // Returns true if the bound output is active (inside a `begin()`...`end()` block)
            // Returns false if no output is bound to this parser
            bool output_is_active() const {return output && output->active();}

            // Begins parse sequence. Must be called after `set_output()`, with inactive output
            void begin()
            {
                if (output)
                {
                    output->begin();
                    begin_();
                }
                reset();
            }

            // Begins parse sequence and sets output stream
            void begin(core::stream_handler &output) {set_output(output); begin();}

            // Ends parse sequence. Must be called with active output
            void end() {if (output) {end_(); output->end();}}

            // Performs one minimal parse step with specified output and then returns.
            stream_input &write_one()
            {
                if (output)
                {
                    // If just reset, fix nesting depth (it may have changed since this parser was created)
                    if (mReset)
                        initial_nesting_level = output->nesting_depth();
                    write_one_();
                    mReset = false;
                }
                else
                    throw core::error("cppdatalib::core::stream_input - attempted to parse without output specified");
                return *this;
            }

            // Performs (or finishes, if `busy()`) one value conversion and then returns.
            stream_input &convert()
            {
                if (output && !busy())
                {
                    const bool active = output_is_active();

                    if (!active)
                    {
                        output->begin();
                        begin_();
                    }

                    reset();

                    // fix nesting depth (it may have changed since this parser was created)
                    initial_nesting_level = output->nesting_depth();
                    write_one_();
                    mReset = false;

#ifdef CPPDATALIB_EVENT_LOOP_CALLBACK
                    while (busy())
                    {
                        CPPDATALIB_EVENT_LOOP_CALLBACK;
                        write_one_();
                    }
#else
                    while (busy()) write_one_();
#endif

                    if (!active)
                    {
                        end_();
                        output->end();
                    }
                }
                else
                    throw core::error("cppdatalib::core::stream_input - attempted to parse without output specified or while busy");
                return *this;
            }

            // Performs one value conversion and then returns.
            // The current handler is set to the new output.
            // NOTE: this function does nothing if `busy()` returns true.
            stream_input &convert(core::stream_handler &output)
            {
                set_output(output);
                return convert();
            }

        protected:
            // Provides the nesting level of this parser (since parsers can be initialized on an existing stream,
            // checking the nesting level of the output may not be accurate)
            size_t nesting_depth() const {return output? output->nesting_depth() - initial_nesting_level: 0;}

            // Called when output changes, allows parser to reset any external output references needed
            // Note that get_output() always returns non-NULL when this function is executing!
            virtual void output_changed_() {}

            // Performs one minimal parse step with specified output and then returns.
            // Note that this "minimal parse step" is rather vague, but that's okay.
            // It can be any combination of the following, from preferred to not-so-preferred:
            //
            //    1. (preferred)
            //      A single write to the output, whether it be a begin_xxx(), end_xxx(), write(), or anything else that produces output.
            //      Strings should be sent chunked (preferably with maximum length of `core::buffer_size`),
            //      or by character, with `output.append_to_string()`.
            //    2.
            //      A single scalar write to the output, strings included as scalars. Containers should have begin_xxx(), end_xxx(), and write()
            //      called at different invocations of `read_one()`
            //    3. (not-so-preferred)
            //      Write the entire value at once, even if it is a container
            //
            // Basically, as long as repeated calls (while busy() returns true) to this function write the entire output to `get_output()`,
            // the implementation is acceptable.
            // Read-states should be stored as class members, not in this function, and should be resettable with `reset()`.
            //
            // Note that get_output() always returns non-NULL when this function is executing!
            virtual void write_one_() = 0;

            // Called when beginning a new parse sequence (this is not necessarily called every time a value is read)
            // Parse sequences may include multiple values, or call begin() and end() every time a value is parsed.
            // Note that get_output() always returns non-NULL when this function is executing!
            virtual void begin_() {}

            // Called when ending a new parse sequence (this is not necessarily called every time a value is read)
            // Parse sequences may include multiple values, or call begin() and end() every time a value is parsed.
            // Note that get_output() always returns non-NULL when this function is executing!
            virtual void end_() {}

            // Resets the input class to start parsing a new stream (NOTE: if input reads from a stream, reset_() should NOT attempt to seek to the beginning of the stream!)
            // Note that get_output() may return NULL when this function is executing!
            virtual void reset_() = 0;
        };

        /* The base class of all input formats that read from an input stream. All input formats that read from an input stream must inherit from this class */
        class stream_parser : public stream_input
        {
            core::istream_handle handle;

        public:
            stream_parser(core::istream_handle &input) : handle(input) {}
            virtual ~stream_parser() {}

            core::istream &stream() {return handle;}
            // May return NULL! See core::istream_handle::std_stream() for details.
            std::istream *std_stream() {return handle.std_stream();}
        };

        // Convert directly from parser to serializer
        inline stream_handler &operator<<(stream_handler &output, stream_input &input)
        {
            if (output.required_features() & ~input.features())
                throw core::error("cppdatalib::stream_handler::operator<<() - output requires features the input doesn't provide. Using cppdatalib::core::automatic_buffer_filter on the output stream may fix this problem.");

            input.convert(output);

            return output;
        }

        // Convert directly from parser to serializer
        inline stream_handler &operator<<(stream_handler &output, stream_input &&input)
        {
            if (output.required_features() & ~input.features())
                throw core::error("cppdatalib::stream_handler::operator<<() - output requires features the input doesn't provide. Using cppdatalib::core::automatic_buffer_filter on the output stream may fix this problem.");

            input.convert(output);

            return output;
        }

        // Convert directly from parser to serializer
        inline void operator<<(stream_handler &&output, stream_input &input)
        {
            if (output.required_features() & ~input.features())
                throw core::error("cppdatalib::stream_handler::operator<<() - output requires features the input doesn't provide. Using cppdatalib::core::automatic_buffer_filter on the output stream may fix this problem.");

            input.convert(output);
        }

        // Convert directly from parser to rvalue serializer
        inline void operator<<(stream_handler &&output, stream_input &&input)
        {
            if (output.required_features() & ~input.features())
                throw core::error("cppdatalib::stream_handler::operator<<() - output requires features the input doesn't provide. Using cppdatalib::core::automatic_buffer_filter on the output stream may fix this problem.");

            input.convert(output);
        }

        // Convert directly from parser to serializer
        inline stream_input &operator>>(stream_input &input, stream_handler &output)
        {
            output << input;
            return input;
        }

        // Convert directly from parser to serializer
        inline stream_input &operator>>(stream_input &input, stream_handler &&output)
        {
            output << input;
            return input;
        }

        // Convert directly from parser to serializer
        inline void operator>>(stream_input &&input, stream_handler &output)
        {
            output << input;
        }

        // Convert directly from parser to rvalue serializer
        inline void operator>>(stream_input &&input, stream_handler &&output)
        {
            output << input;
        }

        struct value::traverse_node_prefix_serialize
        {
            traverse_node_prefix_serialize(stream_handler *handler) : stream(handler) {}

            bool operator()(const value *arg, value::traversal_ancestry_finder)
            {
                if (arg->is_array())
                    stream->begin_array(*arg, arg->array_size());
                else if (arg->is_object())
                    stream->begin_object(*arg, arg->object_size());
                return true;
            }

        private:
            stream_handler *stream;
        };

        struct value::traverse_node_postfix_serialize
        {
            traverse_node_postfix_serialize(stream_handler *handler) : stream(handler) {}

            bool operator()(const value *arg, value::traversal_ancestry_finder)
            {
                if (arg->is_array())
                    stream->end_array(*arg);
                else if (arg->is_object())
                    stream->end_object(*arg);
                else
                    stream->write(*arg);
                return true;
            }

        private:
            stream_handler *stream;
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
                                        compare = (arg->get_bool_unchecked() < arg2->get_bool_unchecked());
                                        break;
                                    case integer:
                                        compare = (arg->get_int_unchecked() < arg2->get_int_unchecked());
                                        break;
                                    case uinteger:
                                        compare = (arg->get_uint_unchecked() < arg2->get_uint_unchecked());
                                        break;
                                    case real:
                                        compare = (arg->get_real_unchecked() < arg2->get_real_unchecked());
                                        break;
                                    case string:
                                        compare = (arg->get_string_unchecked() < arg2->get_string_unchecked());
                                        break;
                                    case array:
                                    case object:
                                    case null:
                                    default:
                                        break;
                                }

								// Invert true cases above to -1 instead
								compare = -compare;
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

        namespace xml_impl
        {
            class xml_base
            {
            protected:
                static bool is_valid_char(uint32_t codepoint)
                {
                    return (codepoint >= 0x20 && codepoint <= 0xd7ff) ||
                            codepoint == 0x9 ||
                            codepoint == 0xa ||
                            codepoint == 0xd ||
                           (codepoint >= 0xe000 && codepoint <= 0xfffd) ||
                           (codepoint >= 0x10000 && codepoint <= 0x10ffff);
                }

                static bool is_name_start_char(unsigned long code)
                {
                    if (code < 128)
                        return isalpha(code) || code == ':' || code == '_';
                    else if ((code >= 0xC0 && code <= 0xD6) ||
                             (code >= 0xD8 && code <= 0xF6) ||
                             (code >= 0xF8 && code <= 0x2FF) ||
                             (code >= 0x370 && code <= 0x37D) ||
                             (code >= 0x37F && code <= 0x1FFF) ||
                             (code >= 0x200C && code <= 0x200D) ||
                             (code >= 0x2070 && code <= 0x218F) ||
                             (code >= 0x2C00 && code <= 0x2FEF) ||
                             (code >= 0x3001 && code <= 0xD7FF) ||
                             (code >= 0xF900 && code <= 0xFDCF) ||
                             (code >= 0xFDF0 && code <= 0xFFFD) ||
                             (code >= 0x10000 && code <= 0xEFFFF))
                        return true;
                    return false;
                }

                static bool is_name_char(unsigned long code)
                {
                    if (code < 128 && (isdigit(code) || code == '-' || code == '.'))
                        return true;
                    return is_name_start_char(code) || code == 0xB7 || (code >= 0x300 && code <= 0x36F) || (code >= 0x203F && code <= 0x2040);
                }
            };

            class stream_parser : public core::stream_parser, public xml_base
            {
                // First: entity value
                // Second: true if parameter entity, false if general entity
                typedef std::pair<core::string_t, bool> entity;

                std::string last_error_;

                core::cache_vector_n<core::string_t, core::cache_size> element_stack;
                std::map<core::string_t, entity> entities;
                std::map<core::string_t, entity> parameter_entities;

                // Format of doctypedecl:
                // {
                //     "root": <expected root element name>
                //     "internal_subset": <internal subset>
                // }
                core::value doctypedecl;
                bool has_root_element;

                std::vector<std::pair<core::string_t, core::istringstream>> entity_buffers;

            public:
                stream_parser(core::istream_handle input) : core::stream_parser(input) {}

                const core::cache_vector_n<core::string_t, core::cache_size> &current_element_stack() const {return element_stack;}
                const std::map<core::string_t, entity> registered_entities() const {return entities;}
                const std::map<core::string_t, entity> registered_parameter_entities() const {return parameter_entities;}

                const std::string &last_error() const {return last_error_;}

            protected:
                enum entity_deref_mode
                {
                    deref_no_entities,
                    deref_all_but_general_entities,
                    deref_all_entities
                };

                enum what_was_read
                {
                    eof_was_reached,
                    nothing_was_read,
                    comment_was_read,
                    processing_instruction_was_read,
                    content_was_read,
                    entity_declaration_was_read,
                    element_declaration_was_read,
                    attlist_declaration_was_read,
                    notation_declaration_was_read,
                    entity_value_was_read, // That is, an entity that could resolve to markup was read
                    start_tag_was_read,
                    complete_tag_was_read,
                    end_tag_was_read
                };

                void reset_()
                {
                    element_stack.clear();
                    parameter_entities.clear();
                    entities.clear();
                    entity_buffers.clear();
                    doctypedecl.set_null();
                    has_root_element = false;
                }

                istream::int_type peek() {
                    if (entity_buffers.size())
                    {
                        istream::int_type c;
                        while (entity_buffers.size() && (c = entity_buffers.back().second.peek(), c == EOF))
                            entity_buffers.pop_back();

                        if (c != EOF)
                            return c;
                    }

                    return stream().peek();
                }

                istream::int_type get() {
                    if (entity_buffers.size())
                    {
                        istream::int_type c;
                        while (entity_buffers.size() && (c = entity_buffers.back().second.get(), c == EOF))
                            entity_buffers.pop_back();

                        if (c != EOF)
                            return c;
                    }

                    return stream().get();
                }

                bool match(istream::int_type c)
                {
                    if (get() != c)
                    {
                        last_error_ = "expected '" + std::string(1, c) + '\'';
                        return false;
                    }

                    return true;
                }

                istream::int_type peek_from_current_level_only()
                {
                    if (entity_buffers.size())
                        return entity_buffers.back().second.peek();
                    else
                        return stream().peek();
                }

                istream::int_type get_from_current_level_only()
                {
                    if (entity_buffers.size())
                        return entity_buffers.back().second.get();
                    else
                        return stream().get();
                }

                bool match_from_current_level_only(istream::int_type c)
                {
                    if (get_from_current_level_only() != c)
                    {
                        last_error_ = "expected '" + std::string(1, c) + '\'';
                        return false;
                    }

                    return true;
                }

                void unget() {
                    if (entity_buffers.size())
                        entity_buffers.back().second.unget();
                    else
                        stream().unget();
                }

                bool read(char *buf, size_t size)
                {
                    while (size--)
                    {
                        istream::int_type c = get();
                        if (c == EOF || c > 0xff)
                            return false;
                        *buf++ = c;
                    }
                    return true;
                }

                bool read_from_current_level_only(char *buf, size_t size)
                {
                    while (size--)
                    {
                        istream::int_type c = get_from_current_level_only();
                        if (c == EOF || c > 0xff)
                            return false;
                        *buf++ = c;
                    }
                    return true;
                }

                void register_entity(const core::string_t &name, const core::string_t &value, bool parameter_entity)
                {
                    if (parameter_entity)
                        parameter_entities.insert({name, entity(value, parameter_entity)});
                    else
                        entities.insert({name, entity(value, parameter_entity)});
                }

                bool add_entity_to_be_parsed(const core::string_t &entity_name, const core::string_t &entity_value)
                {
                    for (size_t i = 0; i < entity_buffers.size(); ++i)
                        if (entity_buffers[i].first == entity_name)
                        {
                            last_error_ = "invalid recursive entity reference";
                            return false;
                        }

                    if (entity_buffers.empty() || entity_buffers.back().second.peek() != EOF)
                        entity_buffers.push_back({entity_name, core::istringstream(entity_value)});
                    else // At end of buffer, just replace it
                    {
                        std::pair<core::string_t, core::istringstream> temp(entity_name, core::istringstream(entity_value));
                        std::swap(entity_buffers.back(), temp);
                    }

                    return true;
                }

                // read_entity() assumes that the initial '&' or '%' has already been read and discarded
                bool read_entity(bool parse_param_entity, entity_deref_mode mode, core::string_t &entity_name, core::string_t &value, bool &entity_should_be_parsed)
                {
                    if (mode == deref_no_entities)
                    {
                        value.clear();
                        value.push_back(parse_param_entity? '%': '&');
                        return true;
                    }

                    istream::int_type c = get_from_current_level_only();

                    if (c == '#') // Character reference
                    {
                        entity_name.clear();
                        entity_should_be_parsed = false;
                        c = get_from_current_level_only();

                        if (c == 'x') // Hexadecimal reference
                        {
                            const char hexchars[] = "0123456789abcdef";
                            uint32_t code = 0;

                            for (; code <= 0x10ffff; )
                            {
                                c = get_from_current_level_only();
                                if (isxdigit(c))
                                    code = (code << 4) + (strchr(hexchars, tolower(c)) - hexchars);
                                else
                                    break;
                            }

                            if (!is_valid_char(code) || // Invalid code point
                                c != ';') // Invalid closing character
                            {
                                last_error_ = "invalid hexadecimal character reference";
                                return false;
                            }

                            value = core::ucs_to_utf8(code);
                        }
                        else if (isdigit(c)) // Decimal reference
                        {
                            uint32_t code = 0;

                            for (; code <= 0x10ffff; )
                            {
                                if (isdigit(c))
                                    code = (code * 10) + (c - '0');
                                else
                                    break;
                                c = get_from_current_level_only();
                            }

                            if (!is_valid_char(code) || // Invalid code point
                                c != ';') // Invalid closing character
                            {
                                last_error_ = "invalid decimal character reference";
                                return false;
                            }

                            value = core::ucs_to_utf8(code);
                        }
                        else
                        {
                            last_error_ = "invalid character reference";
                            return false;
                        }
                    }
                    else if (!parse_param_entity && mode == deref_all_but_general_entities)
                    {
                        entity_name.clear();
                        entity_should_be_parsed = false;
                        value.clear();
                        value.push_back('&');
                        value += ucs_to_utf8(c);
                        return true;
                    }
                    else
                    {
                        unget();

                        if (!read_name(value) ||
                            !match_from_current_level_only(';'))
                        {
                            last_error_ = "invalid entity reference syntax";
                            return false;
                        }

                        entity_name = value;
                        if (parse_param_entity)
                        {
                            entity_should_be_parsed = false;

                            auto it = parameter_entities.find(value);
                            if (it == parameter_entities.end()) // Entity not found
                            {
                                last_error_ = "parameter entity '" + value + "' is not declared";
                                return false;
                            }
                            else
                                value = it->second.first;
                        }
                        else
                        {
                            entity_should_be_parsed = true;

                            auto it = entities.find(value);
                            if (it == entities.end()) // Entity not found
                            {
                                if (value == "amp")
                                    value = "&", entity_should_be_parsed = false;
                                else if (value == "lt")
                                    value = "<", entity_should_be_parsed = false;
                                else if (value == "gt")
                                    value = ">", entity_should_be_parsed = false;
                                else if (value == "quot")
                                    value = "\"", entity_should_be_parsed = false;
                                else if (value == "apos")
                                    value = "'", entity_should_be_parsed = false;
                                else
                                {
                                    last_error_ = "general entity '" + value + "' is not declared";
                                    return false;
                                }
                            }
                            else
                                value = it->second.first;
                        }
                    }

                    return true;
                }

                // read_spaces() reads any number of space characters from the stream and discards them,
                // failing (returning false) if the number of spaces is less than `minimum`
                bool read_spaces(unsigned int minimum)
                {
                    istream::int_type c;
                    while (c = get_from_current_level_only(), c == ' ' || c == '\t' || c == '\n' || c == '\r')
                        if (minimum > 0)
                            --minimum;

                    if (c != EOF)
                        unget();

                    if (minimum != 0)
                        last_error_ = "expected space separator";

                    return minimum == 0;
                }

                // read_name() reads a tag or entity name from the stream, leaving the next non-name (or EOF) character in the stream, ready to be read
                bool read_name(core::string_t &name)
                {
                    uint32_t codepoint;
                    bool eof = true;

                    name.clear();
                    if (entity_buffers.size())
                    {
                        while (codepoint = utf8_to_ucs(entity_buffers.back().second, &eof), codepoint <= 0x10ffff)
                        {
                            if (name.empty())
                            {
                                if (is_name_start_char(codepoint))
                                    name += ucs_to_utf8(codepoint);
                                else
                                    break;
                            }
                            else if (is_name_char(codepoint))
                                name += ucs_to_utf8(codepoint);
                            else
                                break;
                        }
                    }
                    else
                    {
                        while (codepoint = utf8_to_ucs(stream(), &eof), codepoint <= 0x10ffff)
                        {
                            if (name.empty())
                            {
                                if (is_name_start_char(codepoint))
                                    name += ucs_to_utf8(codepoint);
                                else
                                    break;
                            }
                            else if (is_name_char(codepoint))
                                name += ucs_to_utf8(codepoint);
                            else
                                break;
                        }
                    }

                    if (eof)
                        return true;
                    else if (codepoint > 0x80) // Invalid UTF-8, or not a valid character following a name (names are followed by ASCII characters)
                    {
                        last_error_ = "invalid tag, attribute, or other name";
                        return false;
                    }

                    unget();
                    return true;
                }

                // read_attribute_value() assumes that the '=' between the name and the attribute value has already been read and discarded
                bool read_attribute_value(entity_deref_mode allow_references, core::string_t &value, bool allow_less_than = false)
                {
                    core::string_t entity_name, entity;

                    istream::int_type c = get();
                    if (c == '"')
                    {
                        size_t starting_nesting_level = entity_buffers.size();
                        value.clear();
                        while (c = get(), c != EOF && (starting_nesting_level < entity_buffers.size() || c != '"'))
                        {
                            // Normalize end-of-line
                            if (c == '\r')
                            {
                                if (peek() == '\n')
                                    get();
                                c = '\n';
                            }
                            else if (!allow_less_than && c == '<') // There should be no raw (unescaped) '<' in attribute values
                            {
                                last_error_ = "unescaped '<' is not allowed in attribute values";
                                return false;
                            }

                            if (allow_references == deref_no_entities)
                                value += ucs_to_utf8(c);
                            else if (c == '&' ||
                                     c == '%')
                            {
                                bool add_entity;

                                if (!read_entity(c == '%', allow_references, entity_name, entity, add_entity))
                                {
                                    last_error_ = "invalid attribute value; " + last_error_;
                                    return false;
                                }

                                if (add_entity)
                                {
                                    if (!add_entity_to_be_parsed(entity_name, entity))
                                    {
                                        last_error_ = "invalid attribute value; " + last_error_;
                                        return false;
                                    }
                                }
                                else
                                    value += entity;
                            }
                            else
                                value += ucs_to_utf8(c);
                        }

                        if (c != '"')
                            return false;
                    }
                    else if (c == '\'')
                    {
                        size_t starting_nesting_level = entity_buffers.size();
                        value.clear();
                        while (c = get(), c != EOF && (starting_nesting_level < entity_buffers.size() || c != '\''))
                        {
                            // Normalize end-of-line
                            if (c == '\r')
                            {
                                if (peek() == '\n')
                                    get();
                                c = '\n';
                            }
                            else if (!allow_less_than && c == '<') // There should be no raw (unescaped) '<' in attribute values
                            {
                                last_error_ = "unescaped '<' is not allowed in attribute values";
                                return false;
                            }

                            if (allow_references == deref_no_entities)
                                value += ucs_to_utf8(c);
                            else if (c == '&' ||
                                     c == '%')
                            {
                                bool add_entity;

                                if (!read_entity(c == '%', allow_references, entity_name, entity, add_entity))
                                {
                                    last_error_ = "invalid attribute value; " + last_error_;
                                    return false;
                                }

                                if (add_entity)
                                {
                                    if (!add_entity_to_be_parsed(entity_name, entity))
                                    {
                                        last_error_ = "invalid attribute value; " + last_error_;
                                        return false;
                                    }
                                }
                                else
                                    value += entity;
                            }
                            else
                                value += ucs_to_utf8(c);
                        }

                        if (c != '\'')
                            return false;
                    }
                    else
                        return false;

                    return true;
                }

                // read_prolog() assumes nothing has been read in the current document
                bool read_prolog(core::value &attributes)
                {
                    istream::int_type c;
                    char buf[10];

                    if ((!read(buf, 5) || memcmp(buf, "<?xml", 5)) ||
                        (!read_spaces(1)))
                    {
                        last_error_ = "invalid prolog, expected '<?xml '";
                        return false;
                    }

                    c = get();

                    // Is there a version specified
                    if (c == 'v')
                    {
                        buf[0] = c;
                        if ((!read(buf+1, 6) || memcmp(buf, "version", 7)) ||
                            (!read_spaces(0)) ||
                            (!match('=')) ||
                            (!read_spaces(0)) ||
                            (!read_attribute_value(deref_no_entities, attributes["version"].get_string_ref())) ||
                            (!read_spaces(0)))
                        {
                            last_error_ = "invalid prolog; " + last_error_;
                            return false;
                        }
                        c = get();
                    }

                    // Is there an encoding specified?
                    if (c == 'e')
                    {
                        buf[0] = c;
                        if ((!read(buf+1, 7) || memcmp(buf, "encoding", 8)) ||
                            (!read_spaces(0)) ||
                            (!match('=')) ||
                            (!read_spaces(0)) ||
                            (!read_attribute_value(deref_no_entities, attributes["encoding"].get_string_ref())))
                        {
                            last_error_ = "invalid prolog; " + last_error_;
                            return false;
                        }
                        c = get();
                    }

                    // Is standalone specified?
                    if (c == 's')
                    {
                        buf[0] = c;
                        if ((!read(buf+1, 9) || memcmp(buf, "standalone", 10)) ||
                            (!read_spaces(0)) ||
                            (!match('=')) ||
                            (!read_spaces(0)) ||
                            (!read_attribute_value(deref_no_entities, attributes["standalone"].get_string_ref())))
                        {
                            last_error_ = "invalid prolog; " + last_error_;
                            return false;
                        }
                        c = get();
                    }

                    if (c != '?')
                    {
                        unget();
                        if (!read_spaces(0) ||
                            !match('?'))
                        {
                            last_error_ = "invalid prolog; " + last_error_;
                            return false;
                        }
                    }

                    if (!match('>'))
                    {
                        last_error_ = "invalid prolog; " + last_error_;
                        return false;
                    }

                    return true;
                }

                // read_next() reads the next XML element from the stream, whether it be content, or a start, complete, or end tag, or a comment, or a processing instruction, or nothing (garbage)
                // `value_with_attributes` will only be assigned if `read` contains `start_tag_was_read` or `complete_tag_was_read`.
                bool read_next(bool parsing_inside_element, what_was_read &read, core::string_t &value, core::value &value_with_attributes)
                {
                    core::string_t entity_name;
                    (void) parsing_inside_element;

                    istream::int_type c = peek();

                    if (c < 0x80 && isspace(c) && !parsing_inside_element)
                    {
                        read = nothing_was_read;
                        return read_spaces(1);
                    }
                    else if (c == '<')
                    {
                        get_from_current_level_only(); // Skip '<'
                        c = get_from_current_level_only();
                        if (c == '?') // Processing instruction, starting with "<?"
                        {
                            read = processing_instruction_was_read;
                            value.clear();
                            while (c = get_from_current_level_only(), c != '?' && c != EOF)
                                value += ucs_to_utf8(c);
                            bool b = c == '?' && get_from_current_level_only() == '>';
                            if (!b)
                                last_error_ = "processing instruction is not closed properly";
                            return b;
                        }
                        else if (c == '!') // ENTITY, DOCTYPE, CDATA, or comment
                        {
                            char buf[10];

                            c = get_from_current_level_only();
                            if (c == '-') // Comment
                            {
                                read = comment_was_read;
                                if (!match_from_current_level_only('-'))
                                {
                                    last_error_ = "invalid comment prefix; " + last_error_;
                                    return false;
                                }

                                value.clear();
                                while (c = get_from_current_level_only(), c != EOF)
                                {
                                    if (c == '-' && peek() == '-')
                                        break;

                                    // Normalize end-of-line
                                    if (c == '\r')
                                    {
                                        if (peek_from_current_level_only() == '\n')
                                            get_from_current_level_only();
                                        c = '\n';
                                    }

                                    value += ucs_to_utf8(c);
                                }

                                if (!match_from_current_level_only('-') ||
                                    !match_from_current_level_only('>'))
                                {
                                    last_error_ = "comment is not closed properly; " + last_error_;
                                    return false;
                                }
                            }
                            else if (c == 'E') // ENTITY
                            {
                                bool parameter_entity = false;
                                buf[0] = c;
                                core::string_t attribute_value;

                                read = entity_declaration_was_read;

                                if (!this->read_from_current_level_only(buf+1, 5) || memcmp(buf, "ENTITY", 6) ||
                                    !read_spaces(1))
                                {
                                    last_error_ = "expected ENTITY declaration";
                                    return false;
                                }

                                if (get_from_current_level_only() == '%')
                                {
                                    parameter_entity = true;
                                    if (!read_spaces(1))
                                        return false;
                                }
                                else
                                    unget();

                                if (!read_name(value) ||
                                    !read_spaces(1) ||
                                    !read_attribute_value(deref_all_but_general_entities, attribute_value, true) ||
                                    !read_spaces(0) ||
                                    !match_from_current_level_only('>'))
                                {
                                    last_error_ = "invalid ENTITY declaration; " + last_error_;
                                    return false;
                                }

                                register_entity(value, attribute_value, parameter_entity);
                            }
                            else if (c == '[') // CDATA
                            {
                                if (!this->read_from_current_level_only(buf, 6) || memcmp(buf, "CDATA[", 6))
                                {
                                    last_error_ = "expected CDATA";
                                    return false;
                                }

                                read = content_was_read;
                                value.clear();
                                while (c = get_from_current_level_only(), c != EOF)
                                {
                                    // Normalize end-of-line
                                    if (c == '\r')
                                    {
                                        if (peek_from_current_level_only() == '\n')
                                            get_from_current_level_only();
                                        c = '\n';
                                    }
                                    else if (c == '>' &&
                                             value.size() >= 2 &&
                                             value[value.size() - 2] == ']' &&
                                             value[value.size() - 1] == ']')
                                        break;

                                    value += ucs_to_utf8(c);
                                }

                                if (c == EOF)
                                {
                                    last_error_ = "CDATA is not closed properly";
                                    return false;
                                }

                                // Pop the two trailing ']' characters off
                                value.pop_back();
                                value.pop_back();
                            }
                            else if (c == 'D') // DOCTYPE
                            {
                                buf[0] = c;

                                if (!this->read_from_current_level_only(buf+1, 6) || memcmp(buf, "DOCTYPE", 7) ||
                                    !read_spaces(1) ||
                                    !read_name(value) ||
                                    !read_spaces(0) ||
                                    !match_from_current_level_only('>'))
                                {
                                    last_error_ = "invalid DOCTYPE; " + last_error_;
                                    return false;
                                }

                                doctypedecl["root"] = core::value(value);
                            }
                            else
                                return false;
                        }
                        else if (c == '/') // End tag, starts with "</"
                        {
                            value.clear();
                            read = end_tag_was_read;
                            if (!read_name(value) ||
                                !read_spaces(0) ||
                                !match_from_current_level_only('>'))
                            {
                                last_error_ = "invalid end tag; " + last_error_;
                                return false;
                            }

                            if (element_stack.empty() || value != element_stack.back())
                            {
                                last_error_ = "mismatched start and end tags";
                                return false;
                            }
                            element_stack.pop_back();
                        }
                        else // Element tag? Starts with '<', not followed by '!', '?', or '/'
                        {
                            core::string_t attribute_name, attribute_value;
                            value.clear();
                            read = start_tag_was_read;

                            unget();
                            if (!read_name(value) ||
                                !read_spaces(0))
                            {
                                last_error_ = "invalid start tag; " + last_error_;
                                return false;
                            }

                            if (!parsing_inside_element && doctypedecl.is_member("root"))
                            {
                                if (doctypedecl["root"].get_string() != value)
                                {
                                    last_error_ = "invalid document; root element does not match DTD";
                                    return false;
                                }
                            }

                            value_with_attributes = core::value(value);

                            while (true)
                            {
                                c = get_from_current_level_only();
                                if (c == '/') // Closed tag
                                {
                                    read = complete_tag_was_read;
                                    c = get_from_current_level_only();
                                }

                                if (c == '>')
                                    break;
                                else if (read == complete_tag_was_read)
                                {
                                    last_error_ = "invalid closed tag";
                                    return false; // closing slash not followed by '>'
                                }

                                // If at this point, assume an attribute name follows
                                unget();
                                if (!read_name(attribute_name) ||
                                    !read_spaces(0) ||
                                    !match_from_current_level_only('=') ||
                                    !read_spaces(0) ||
                                    !read_attribute_value(deref_all_entities, attribute_value) ||
                                    !read_spaces(0))
                                {
                                    last_error_ = "invalid start tag; " + last_error_;
                                    return false;
                                }

                                if (value_with_attributes.is_attribute(attribute_name)) // Cannot have duplicate attribute keys
                                {
                                    last_error_ = "invalid start tag; duplicate attribute names found";
                                    return false;
                                }

                                value_with_attributes.add_attribute(core::value(attribute_name), core::value(attribute_value));
                            }

                            if (read == start_tag_was_read)
                                element_stack.push_back(value);
                        }
                    }
                    else if (c == EOF)
                        read = eof_was_reached;
                    else // Must be content! Any entities found here are parsable as content, and should be
                    {
                        read = content_was_read;
                        value.clear();
                        get_from_current_level_only(); // Eat first character
                        while (c != EOF && c != '&' && c != '%' && c != '<' && value.size() < core::buffer_size)
                        {
                            // Normalize end-of-line
                            if (c == '\r')
                            {
                                if (peek_from_current_level_only() == '\n')
                                    get_from_current_level_only();
                                c = '\n';
                            }

                            value += ucs_to_utf8(c);
                            c = get_from_current_level_only();
                        }

                        if (value.empty())
                        {
                            if (c == '&' || c == '%')
                            {
                                bool should_be_added_to_entity_list;
                                bool b = read_entity(c == '%', deref_all_entities, entity_name, value, should_be_added_to_entity_list);

                                if (should_be_added_to_entity_list)
                                {
                                    read = entity_value_was_read;
                                    if (!add_entity_to_be_parsed(entity_name, value))
                                        return false;
                                }
                                else
                                    read = content_was_read;

                                return b;
                            }
                        }
                        else if (c != EOF)
                            unget();
                    }

                    return true;
                }
            };

            class stream_writer_base : public core::stream_handler, public core::stream_writer, public xml_base
            {
            public:
                stream_writer_base(core::ostream_handle &stream) : core::stream_writer(stream) {}

            protected:
                void write_attributes(const core::value &v)
                {
                    for (auto attr: v.get_attributes())
                    {
                        if (!attr.first.is_string())
                            throw core::error("XML - cannot write attribute with non-string key");
                        stream().put(' ');
                        write_name(stream(), attr.first.get_string_unchecked());

                        stream().write("=\"", 2);
                        switch (attr.second.get_type())
                        {
                            case core::null: break;
                            case core::boolean: bool_(attr.second); break;
                            case core::integer: integer_(attr.second); break;
                            case core::uinteger: uinteger_(attr.second); break;
                            case core::real: real_(attr.second); break;
                            case core::string: write_attribute_content(stream(), attr.second.get_string_unchecked()); break;
                            case core::array:
                            case core::object: throw core::error("XML - cannot write attribute with 'array' or 'object' value");
                        }
                        stream().put('"');
                    }
                }

                // Assumes name is UTF-8 encoded, not binary
                core::ostream &write_name(core::ostream &stream, const std::string &str)
                {
                    if (str.empty())
                        throw core::error("XML - tag or attribute name must not be empty string");

                    std::vector<uint32_t> ucs = utf8_to_ucs(str);

                    if (!is_name_start_char(ucs[0]))
                        throw core::custom_error("XML - invalid tag or attribute name \"" + str + "\"");

                    for (size_t i = 1; i < ucs.size(); ++i)
                        if (!is_name_char(ucs[i]))
                            throw core::custom_error("XML - invalid tag or attribute name \"" + str + "\"");

                    return stream << str;
                }

                core::ostream &write_attribute_content(core::ostream &stream, const std::string &str)
                {
                    for (size_t i = 0; i < str.size(); ++i)
                    {
                        int c = str[i] & 0xff;

                        switch (c)
                        {
                            case '"': stream.write("&quot;", 6); break;
                            case '&': stream.write("&amp;", 5); break;
                            case '\'': stream.write("&apos;", 6); break;
                            case '<': stream.write("&lt;", 4); break;
                            case '>': stream.write("&gt;", 4); break;
                            default:
                                if (iscntrl(c))
                                    (stream.write("&#", 2) << c).put(';');
                                else
                                    stream.put(c);
                                break;
                        }
                    }

                    return stream;
                }

                core::ostream &write_element_content(core::ostream &stream, const std::string &str)
                {
                    for (size_t i = 0; i < str.size();)
                    {
                        uint32_t c = utf8_to_ucs(str, i, i);

                        if (c == uint32_t(-1))
                            throw core::error("XML - invalid UTF-8 string");

                        switch (c)
                        {
                            case '"': stream.write("&quot;", 6); break;
                            case '&': stream.write("&amp;", 5); break;
                            case '\'': stream.write("&apos;", 6); break;
                            case '<': stream.write("&lt;", 4); break;
                            case '>': stream.write("&gt;", 4); break;
                            case '\n':
                            case '\r':
                            case '\t': stream.put(c); break;
                            default:
                                if (c > 0x80 || iscntrl(c))
                                    (stream.write("&#", 2) << c).put(';');
                                else
                                    stream.put(c);
                                break;
                        }
                    }

                    return stream;
                }

                core::ostream &write_binary_element_content(core::ostream &stream, const std::string &str)
                {
                    for (size_t i = 0; i < str.size(); ++i)
                    {
                        int c = str[i] & 0xff;

                        switch (c)
                        {
                            case '"': stream.write("&quot;", 6); break;
                            case '&': stream.write("&amp;", 5); break;
                            case '\'': stream.write("&apos;", 6); break;
                            case '<': stream.write("&lt;", 4); break;
                            case '>': stream.write("&gt;", 4); break;
                            case '\n':
                            case '\r':
                            case '\t': stream.put(c); break;
                            default:
                                if (iscntrl(c))
                                    (stream.write("&#", 2) << c).put(';');
                                else
                                    stream.put(c);
                                break;
                        }
                    }

                    return stream;
                }
            };
        }
    }
}

#endif // CPPDATALIB_STREAM_BASE_H

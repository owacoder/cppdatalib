#ifndef CPPDATALIB_STREAM_BASE_H
#define CPPDATALIB_STREAM_BASE_H

#include "value.h"

namespace cppdatalib
{
    namespace core
    {
        inline bool stream_starts_with(std::istream &stream, const char *str)
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
            std::ostream &output_stream;

        public:
            stream_writer(std::ostream &stream) : output_stream(stream) {}

            std::ostream &stream() {return output_stream;}
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

        public:
            enum {
                unknown_size = -1
            };

            void begin()
            {
                while (!nested_scopes.empty())
                    nested_scopes.pop_back();
                begin_();
            }
            void end()
            {
                if (!nested_scopes.empty())
                    throw error("cppdatalib::stream_handler - unexpected end of stream");
                end_();
            }

        protected:
            virtual void begin_() {}
            virtual void end_() {}

        public:
            size_t nesting_depth() const {return nested_scopes.size();}

            type current_container() const
            {
                if (nested_scopes.empty())
                    return null;
                return nested_scopes.back().get_type();
            }

            size_t current_container_size() const
            {
                if (nested_scopes.empty())
                    return 0;
                return nested_scopes.back().items_parsed();
            }

            bool container_key_was_just_parsed() const
            {
                if (nested_scopes.empty())
                    return false;
                return nested_scopes.back().key_was_parsed();
            }

            // An API must call this when a scalar value is encountered.
            // Returns true if value was handled, false otherwise
            bool write(const value &v)
            {
                const bool is_key =
                       !nested_scopes.empty() &&
                        nested_scopes.back().type_ == object &&
                       !nested_scopes.back().key_was_parsed();

                if (!write_(v, is_key))
                {
                    if (is_key)
                        begin_key_(v);
                    else
                        begin_item_(v);

                    switch (v.get_type())
                    {
                        case string:
                            begin_string_(v, v.size(), is_key);
                            string_data_(v);
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

                if (!nested_scopes.empty())
                {
                    if (nested_scopes.back().get_type() == object)
                    {
                        nested_scopes.back().items_ += !is_key;
                        nested_scopes.back().parsed_key_ = !nested_scopes.back().key_was_parsed();
                    }
                    else
                        ++nested_scopes.back().items_;
                }

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

            // Called when a scalar real is parsed
            virtual void real_(const core::value &v) {(void) v;}

            // Called when a scalar string is parsed
            virtual void begin_string_(const core::value &v, core::int_t size, bool is_key) {(void) v; (void) size; (void) is_key;}
            virtual void string_data_(const core::value &v) {(void) v;}
            virtual void end_string_(const core::value &v, bool is_key) {(void) v; (void) is_key;}

        public:
            // An API must call these when a long string is parsed. The number of bytes is passed in size, if possible
            // size < 0 means unknown size
            void begin_string(const core::value &v, core::int_t size)
            {
                if (!nested_scopes.empty() &&
                     nested_scopes.back().type_ == object &&
                    !nested_scopes.back().key_was_parsed())
                {
                    begin_key_(v);
                    begin_string_(v, size, true);
                }
                else
                {
                    begin_item_(v);
                    begin_string_(v, size, false);
                }

                nested_scopes.push_back(string);
            }
            // An API must call these when a long string is parsed. The number of bytes of this chunk is passed in size, if possible
            void append_to_string(const core::value &v)
            {
                if (nested_scopes.empty() || nested_scopes.back().get_type() != string)
                    throw error("cppdatalib::stream_handler - attempted to append to string that was never begun");

                string_data_(v);
                nested_scopes.back().items_ += v.get_string().size();
            }
            void end_string(const core::value &v)
            {
                if (nested_scopes.empty() || nested_scopes.back().get_type() != string)
                    throw error("cppdatalib::stream_handler - attempted to end string that was never begun");

                if ( nested_scopes.size() > 1 &&
                     nested_scopes[nested_scopes.size() - 2].get_type() == object &&
                    !nested_scopes[nested_scopes.size() - 2].key_was_parsed())
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

                if (!nested_scopes.empty())
                {
                    if (nested_scopes.back().get_type() == object)
                    {
                        nested_scopes.back().items_ += nested_scopes.back().key_was_parsed();
                        nested_scopes.back().parsed_key_ = !nested_scopes.back().key_was_parsed();
                    }
                    else
                        ++nested_scopes.back().items_;
                }
            }

            // An API must call these when an array is parsed. The number of elements is passed in size, if possible
            // size < 0 means unknown size
            void begin_array(const core::value &v, core::int_t size)
            {
                if (!nested_scopes.empty() &&
                     nested_scopes.back().type_ == object &&
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
                if (nested_scopes.empty() || nested_scopes.back().get_type() != array)
                    throw error("cppdatalib::stream_handler - attempted to end array that was never begun");

                if ( nested_scopes.size() > 1 &&
                     nested_scopes[nested_scopes.size() - 2].get_type() == object &&
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

                if (!nested_scopes.empty())
                {
                    if (nested_scopes.back().get_type() == object)
                    {
                        nested_scopes.back().items_ += nested_scopes.back().key_was_parsed();
                        nested_scopes.back().parsed_key_ = !nested_scopes.back().key_was_parsed();
                    }
                    else
                        ++nested_scopes.back().items_;
                }
            }

            // An API must call these when an object is parsed. The number of key/value pairs is passed in size, if possible
            // size < 0 means unknown size
            void begin_object(const core::value &v, core::int_t size)
            {
                if (!nested_scopes.empty() &&
                     nested_scopes.back().type_ == object &&
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
                if (nested_scopes.empty() || nested_scopes.back().get_type() != object)
                    throw error("cppdatalib::stream_handler - attempted to end object that was never begun");
                if (nested_scopes.back().key_was_parsed())
                    throw error("cppdatalib::stream_handler - attempted to end object before final value was written");

                if ( nested_scopes.size() > 1 &&
                     nested_scopes[nested_scopes.size() - 2].get_type() == object &&
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

                if (!nested_scopes.empty())
                {
                    if (nested_scopes.back().get_type() == object)
                    {
                        nested_scopes.back().items_ += nested_scopes.back().key_was_parsed();
                        nested_scopes.back().parsed_key_ = !nested_scopes.back().key_was_parsed();
                    }
                    else
                        ++nested_scopes.back().items_;
                }
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

        struct value::traverse_node_prefix_serialize
        {
            traverse_node_prefix_serialize(stream_handler &handler) : stream(handler) {}

            void operator()(const value *arg)
            {
                if (arg->is_array())
                    stream.begin_array(*arg, stream_handler::unknown_size);
                else if (arg->is_object())
                    stream.begin_object(*arg, stream_handler::unknown_size);
            }

        private:
            stream_handler &stream;
        };

        struct value::traverse_node_postfix_serialize
        {
            traverse_node_postfix_serialize(stream_handler &handler) : stream(handler) {}

            void operator()(const value *arg)
            {
                if (arg->is_array())
                    stream.end_array(*arg);
                else if (arg->is_object())
                    stream.end_object(*arg);
                else
                    stream.write(*arg);
            }

        private:
            stream_handler &stream;
        };

        inline stream_handler &convert(const value &v, stream_handler &handler)
        {
            value::traverse_node_prefix_serialize prefix(handler);
            value::traverse_node_postfix_serialize postfix(handler);

            handler.begin();
            v.traverse(prefix, postfix);
            handler.end();

            return handler;
        }
    }
}

#endif // CPPDATALIB_STREAM_BASE_H

/*
 * xml.h
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

#ifndef CPPDATALIB_XML_XML_H
#define CPPDATALIB_XML_XML_H

#ifdef CPPDATALIB_ENABLE_XML
#include "../core/core.h"

#ifndef CPPDATALIB_ENABLE_ATTRIBUTES
#error The XML library requires CPPDATALIB_ENABLE_ATTRIBUTES to be defined
#endif

namespace cppdatalib
{
    namespace xml
    {
        class stream_writer : public core::xml_impl::stream_writer_base
        {
            core::cache_vector_n<core::string_t, core::cache_size> current_keys;

        public:
            stream_writer(core::ostream_handle output) : core::xml_impl::stream_writer_base(output) {}

            std::string name() const {return "cppdatalib::xml::stream_writer";}

        protected:
            void begin_() {stream().precision(CPPDATALIB_REAL_DIG); current_keys.clear();}

            void begin_item_(const core::value &v)
            {
                if (container_key_was_just_parsed())
                {
                    write_attributes(v);
                    stream().put('>');
                }
                else if (current_container() == core::array) // Must be using an array to counteract the lack of ordering when using objects
                {
                    // Not an object? Then we can't serialize. An object is required so that the key may be used for the tag name!
                    // Otherwise, all the array elements would be concatenated together as raw content, and that doesn't really make sense for most cases
                    if (!v.is_object())
                        throw core::error("XML - array elements must be objects");
                }
            }

            void begin_key_(const core::value &v)
            {
                if (!v.is_string())
                    throw core::error("XML - cannot write non-string key");

                if (current_container_size() > 0)
                {
                    write_name(stream().write("</", 2), current_keys.back()).put('>');
                    current_keys.pop_back();
                }

                stream().put('<');
                current_keys.push_back("");
            }
            void end_key_(const core::value &v)
            {
                write_attributes(v);
            }

            // Implementation of null_() does nothing
            // void null_(const core::value &) {}
            void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true": "false");}
            void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
            void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
            void real_(const core::value &v)
            {
                if (!std::isfinite(v.get_real_unchecked()))
                    throw core::error("XML - cannot write 'NaN' or 'Infinity' values");
                stream() << v.get_real_unchecked();
            }
            void string_data_(const core::value &v, bool is_key)
            {
                if (is_key)
                    current_keys.back() += v.get_string_unchecked();
                else
                    write_element_content(stream(), v.get_string_unchecked());
            }
            void end_string_(const core::value &, bool is_key)
            {
                if (is_key)
                    write_name(stream(), current_keys.back());
            }

            void end_object_(const core::value &, bool)
            {
                write_name(stream().write("</", 2), current_keys.back()).put('>');
                current_keys.pop_back();
            }
        };

        class pretty_stream_writer : public core::xml_impl::stream_writer_base
        {
            std::unique_ptr<char []> buffer;
            size_t indent_width;
            size_t current_indent;
            core::cache_vector_n<core::string_t, core::cache_size> current_keys;

            void output_padding(size_t padding)
            {
                while (padding > 0)
                {
                    size_t size = std::min(size_t(core::buffer_size-1), padding);

                    memset(buffer.get(), ' ', size);

                    stream().write(buffer.get(), size);
                    padding -= size;
                }
            }

        public:
            pretty_stream_writer(core::ostream_handle output, size_t indent_width)
                : core::xml_impl::stream_writer_base(output)
                , buffer(new char [core::buffer_size])
                , indent_width(indent_width)
                , current_indent(0)
            {}

            size_t indent() {return indent_width;}

            std::string name() const {return "cppdatalib::xml::pretty_stream_writer";}

        protected:
            void begin_() {current_indent = 0; stream().precision(CPPDATALIB_REAL_DIG); current_keys.clear();}

            void begin_item_(const core::value &v)
            {
                if (container_key_was_just_parsed())
                {
                    write_attributes(v);
                    stream().put('>');
                    stream().put('\n'), output_padding(current_indent);
                }
                else if (current_container() == core::array) // Must be using an array to counteract the lack of ordering when using objects
                {
                    // Not an object? Then we can't serialize. An object is required so that the key may be used for the tag name!
                    // Otherwise, all the array elements would be concatenated together, and that doesn't really make sense for most cases
                    if (!v.is_object())
                        throw core::error("XML - array elements must be objects");

                    if (current_container_size() > 0)
                        stream().put('\n'), output_padding(current_indent);
                }
            }

            void begin_key_(const core::value &v)
            {
                if (!v.is_string())
                    throw core::error("XML - cannot write non-string key");

                if (current_container_size() > 0)
                {
                    stream().put('\n'), output_padding(current_indent);
                    write_name(stream().write("</", 2), current_keys.back()).put('>');
                    current_keys.pop_back();
                }

                stream().put('<');
                current_keys.push_back("");
            }
            void end_key_(const core::value &v)
            {
                write_attributes(v);
            }

            // Implementation of null_() does nothing
            // void null_(const core::value &) {}
            void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true": "false");}
            void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
            void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
            void real_(const core::value &v)
            {
                if (!std::isfinite(v.get_real_unchecked()))
                    throw core::error("XML - cannot write 'NaN' or 'Infinity' values");
                stream() << v.get_real_unchecked();
            }
            void string_data_(const core::value &v, bool is_key)
            {
                if (is_key)
                    current_keys.back() += v.get_string_unchecked();
                else
                    write_element_content(stream(), v.get_string_unchecked());
            }
            void end_string_(const core::value &, bool is_key)
            {
                if (is_key)
                    write_name(stream(), current_keys.back());
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                current_indent += indent_width;
            }
            void end_object_(const core::value &, bool)
            {
                current_indent -= indent_width;

                stream().put('\n'), output_padding(current_indent);
                write_name(stream().write("</", 2), current_keys.back()).put('>');
                current_keys.pop_back();
            }
        };

        inline std::string to_xml(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer writer(stream);
            writer << v;
            return stream.str();
        }

        inline std::string to_pretty_xml(const core::value &v, size_t indent_width)
        {
            core::ostringstream stream;
            pretty_stream_writer writer(stream, indent_width);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_ENABLE_XML
#endif // CPPDATALIB_XML_XML_H

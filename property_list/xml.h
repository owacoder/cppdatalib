/*
 * xml.h
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

#ifndef CPPDATALIB_XML_PROPERTY_LIST_H
#define CPPDATALIB_XML_PROPERTY_LIST_H

#include "../core/core.h"

#ifdef CPPDATALIB_ENABLE_XML

namespace cppdatalib
{
    namespace xml_property_list
    {
        class stream_writer : public core::xml_impl::stream_writer_base
        {
            base64::encode_accumulator base64;

        public:
            stream_writer(core::ostream_handle output) : core::xml_impl::stream_writer_base(output) {}

            std::string name() const {return "cppdatalib::xml_property_list::stream_writer";}

        protected:
            void begin_() {stream().precision(CPPDATALIB_REAL_DIG);}

            void begin_key_(const core::value &v)
            {
                if (!v.is_string())
                    throw core::error("XML Property List - cannot write non-string key");
            }

            void null_(const core::value &) {throw core::error("XML Property List - 'null' value not allowed in output");}
            void bool_(const core::value &v) {stream().put('<') << (v.get_bool_unchecked()? "true": "false") << "/>";}
            void integer_(const core::value &v) {stream() << "<integer>" << v.get_int_unchecked() << "</integer>";}
            void uinteger_(const core::value &v) {stream() << "<integer>" << v.get_uint_unchecked() << "</integer>";}
            void real_(const core::value &v) {stream() << "<real>" << v.get_real_unchecked() << "</real>";}
            void begin_string_(const core::value &v, core::optional_size, bool is_key)
            {
                if (is_key)
                    stream() << "<key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: stream() << "<date>"; break;
                        default:
                            if (!core::subtype_is_text_string(v.get_subtype()))
                            {
                                stream() << "<data>";
                                base64 = base64::encode_accumulator(stream());
                            }
                            else
                                stream() << "<string>";
                            break;
                    }
            }
            void string_data_(const core::value &v, bool)
            {
                if (!core::subtype_is_text_string(current_container_subtype()))
                    base64.accumulate(v.get_string_unchecked());
                else
                    write_element_content(stream(), v.get_string_unchecked());
            }
            void end_string_(const core::value &v, bool is_key)
            {
                if (is_key)
                    stream() << "</key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: stream() << "</date>"; break;
                        default:
                            if (!core::subtype_is_text_string(v.get_subtype()))
                            {
                                base64.end();
                                stream() << "</data>";
                            }
                            else
                                stream() << "</string>";
                            break;
                    }
            }

            void begin_array_(const core::value &, core::optional_size, bool) {stream() << "<array>";}
            void end_array_(const core::value &, bool) {stream() << "</array>";}

            void begin_object_(const core::value &, core::optional_size, bool) {stream() << "<dict>";}
            void end_object_(const core::value &, bool) {stream() << "</dict>";}

            void link_(const core::value &) {throw core::error("XML Property List - 'link' value not allowed in output");}
        };

        class pretty_stream_writer : public core::xml_impl::stream_writer_base
        {
            base64::encode_accumulator base64;

            char * const buffer;
            size_t indent_width;
            size_t current_indent;

            void output_padding(size_t padding)
            {
                while (padding > 0)
                {
                    size_t size = std::min(size_t(core::buffer_size-1), padding);

                    memset(buffer, ' ', size);

                    stream().write(buffer, size);
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
            ~pretty_stream_writer() {delete[] buffer;}

            size_t indent() {return indent_width;}

            std::string name() const {return "cppdatalib::xml_property_list::pretty_stream_writer";}

        protected:
            void begin_() {current_indent = 0; stream().precision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (current_container() != core::null)
                    stream() << '\n', output_padding(current_indent);
            }
            void begin_key_(const core::value &v)
            {
                stream() << '\n', output_padding(current_indent);

                if (!v.is_string())
                    throw core::error("XML Property List - cannot write non-string key");
            }

            void bool_(const core::value &v) {stream() << '<' << (v.get_bool_unchecked()? "true": "false") << "/>";}
            void integer_(const core::value &v)
            {
                stream() << "<integer>\n", output_padding(current_indent + indent_width);
                stream() << v.get_int_unchecked() << '\n'; output_padding(current_indent);
                stream() << "</integer>";
            }
            void uinteger_(const core::value &v)
            {
                stream() << "<integer>\n", output_padding(current_indent + indent_width);
                stream() << v.get_uint_unchecked() << '\n'; output_padding(current_indent);
                stream() << "</integer>";
            }
            void real_(const core::value &v)
            {
                stream() << "<real>\n", output_padding(current_indent + indent_width);
                stream() << v.get_real_unchecked() << '\n'; output_padding(current_indent);
                stream() << "</real>";
            }
            void begin_string_(const core::value &v, core::optional_size, bool is_key)
            {
                if (is_key)
                    stream() << "<key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: stream() << "<date>"; break;
                        default:
                            if (!core::subtype_is_text_string(v.get_subtype()))
                            {
                                stream() << "<data>";
                                base64 = base64::encode_accumulator(stream());
                            }
                            else
                                stream() << "<string>";
                            break;
                    }
            }
            void string_data_(const core::value &v, bool)
            {
                if (current_container_size() == 0)
                    stream() << '\n', output_padding(current_indent + indent_width);

                // TODO: dumb Base64 concatenation is dangerous and wrong. Needs to be fixed
                if (!core::subtype_is_text_string(current_container_subtype()))
                    base64.accumulate(v.get_string_unchecked());
                else
                    write_element_content(stream(), v.get_string_unchecked());
            }
            void end_string_(const core::value &v, bool is_key)
            {
                if (current_container_size() > 0)
                    stream() << '\n', output_padding(current_indent);

                if (is_key)
                    stream() << "</key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: stream() << "</date>"; break;
                        default:
                            if (!core::subtype_is_text_string(v.get_subtype()))
                            {
                                base64.end();
                                stream() << "</data>";
                            }
                            else
                                stream() << "</string>";
                            break;
                    }
            }

            void begin_array_(const core::value &, core::optional_size, bool)
            {
                stream() << "<array>";
                current_indent += indent_width;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    stream() << '\n', output_padding(current_indent);

                stream() << "</array>";
            }

            void begin_object_(const core::value &, core::optional_size, bool)
            {
                stream() << "<dict>";
                current_indent += indent_width;
            }
            void end_object_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    stream() << '\n', output_padding(current_indent);

                stream() << "</dict>";
            }

            void link_(const core::value &) {throw core::error("XML Property List - 'link' value not allowed in output");}
        };

        inline std::string to_xml_property_list(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer writer(stream);
            core::convert(writer, v);
            return stream.str();
        }

        inline std::string to_pretty_xml_property_list(const core::value &v, size_t indent_width)
        {
            core::ostringstream stream;
            pretty_stream_writer writer(stream, indent_width);
            core::convert(writer, v);
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_ENABLE_XML

#endif // CPPDATALIB_XML_PROPERTY_LIST_H

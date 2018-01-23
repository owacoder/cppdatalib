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

namespace cppdatalib
{
    // TODO: per stream, change single character writes to put() calls, and string writes to write() calls

    namespace xml_property_list
    {
        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(core::ostream_handle &stream) : core::stream_writer(stream) {}

            protected:
                core::ostream &write_string(core::ostream &stream, const std::string &str)
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
                                    stream.write("&#", 2).put(c).put(';');
                                else
                                    stream.put(str[i]);
                                break;
                        }
                    }

                    return stream;
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
        public:
            stream_writer(core::ostream_handle output) : impl::stream_writer_base(output) {}

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
            void begin_string_(const core::value &v, core::int_t, bool is_key)
            {
                if (is_key)
                    stream() << "<key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: stream() << "<date>"; break;
                        case core::blob:
                        case core::clob: stream() << "<data>"; break;
                        default: stream() << "<string>"; break;
                    }
            }
            void string_data_(const core::value &v, bool)
            {
                if (v.get_subtype() == core::blob || v.get_subtype() == core::clob)
                    base64::write(stream(), v.get_string_unchecked());
                else
                    write_string(stream(), v.get_string_unchecked());
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
                        case core::blob:
                        case core::clob: stream() << "</data>"; break;
                        default: stream() << "</string>"; break;
                    }
            }

            void begin_array_(const core::value &, core::int_t, bool) {stream() << "<array>";}
            void end_array_(const core::value &, bool) {stream() << "</array>";}

            void begin_object_(const core::value &, core::int_t, bool) {stream() << "<dict>";}
            void end_object_(const core::value &, bool) {stream() << "</dict>";}
        };

        class pretty_stream_writer : public impl::stream_writer_base
        {
            std::unique_ptr<char []> buffer;
            size_t indent_width;
            size_t current_indent;

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
                : impl::stream_writer_base(output)
                , buffer(new char [core::buffer_size])
                , indent_width(indent_width)
                , current_indent(0)
            {}

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
            void begin_string_(const core::value &v, core::int_t, bool is_key)
            {
                if (is_key)
                    stream() << "<key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: stream() << "<date>"; break;
                        case core::blob:
                        case core::clob: stream() << "<data>"; break;
                        default: stream() << "<string>"; break;
                    }
            }
            void string_data_(const core::value &v, bool)
            {
                if (current_container_size() == 0)
                    stream() << '\n', output_padding(current_indent + indent_width);

                if (v.get_subtype() == core::blob || v.get_subtype() == core::clob)
                    base64::write(stream(), v.get_string_unchecked());
                else
                    write_string(stream(), v.get_string_unchecked());
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
                        case core::blob:
                        case core::clob: stream() << "</data>"; break;
                        default: stream() << "</string>"; break;
                    }
            }

            void begin_array_(const core::value &, core::int_t, bool)
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

            void begin_object_(const core::value &, core::int_t, bool)
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
        };

        inline std::string to_xml_property_list(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer writer(stream);
            writer << v;
            return stream.str();
        }

        inline std::string to_pretty_xml_property_list(const core::value &v, size_t indent_width)
        {
            core::ostringstream stream;
            pretty_stream_writer writer(stream, indent_width);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_XML_PROPERTY_LIST_H

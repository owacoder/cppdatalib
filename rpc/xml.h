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

#ifndef CPPDATALIB_XML_RPC_H
#define CPPDATALIB_XML_RPC_H

#include "../core/value_builder.h"

namespace cppdatalib
{
    // TODO: per stream, change single character writes to put() calls, and string writes to write() calls

    namespace xml_rpc
    {
        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(std::ostream &stream) : core::stream_writer(stream) {}

            protected:
                std::ostream &write_string(std::ostream &stream, const std::string &str)
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
            stream_writer(std::ostream &output) : impl::stream_writer_base(output) {}

        protected:
            void begin_() {output_stream << std::setprecision(CPPDATALIB_REAL_DIG);}

            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    output_stream << "</member>";

                if (!v.is_string())
                    throw core::error("XML RPC - cannot write non-string key");

                output_stream << "<member>";
            }

            void null_(const core::value &) {throw core::error("XML RPC - 'null' value not allowed in output");}
            void bool_(const core::value &v) {output_stream << "<value><boolean>" << v.as_int() << "</boolean></value>";}
            void integer_(const core::value &v) {output_stream << "<value><int>" << v.get_int() << "</int></value>";}
            void uinteger_(const core::value &v) {output_stream << "<value><int>" << v.get_uint() << "</int></value>";}
            void real_(const core::value &v) {output_stream << "<value><double>" << v.get_real() << "</double></value>";}
            void begin_string_(const core::value &, core::int_t, bool is_key)
            {
                if (is_key)
                    output_stream << "<name>";
                else
                    output_stream << "<value><string>";
            }
            void string_data_(const core::value &v, bool) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &, bool is_key)
            {
                if (is_key)
                    output_stream << "</name>";
                else
                    output_stream << "</string></value>";
            }

            void begin_array_(const core::value &, core::int_t, bool) {output_stream << "<value><array><data>";}
            void end_array_(const core::value &, bool) {output_stream << "</data></array></value>";}

            void begin_object_(const core::value &, core::int_t, bool) {output_stream << "<value><struct>";}
            void end_object_(const core::value &, bool)
            {
                if (current_container_size() > 0)
                    output_stream << "</member>";
                output_stream << "</struct></value>";
            }
        };

        class pretty_stream_writer : public impl::stream_writer_base
        {
            size_t indent_width;
            size_t current_indent;

            void output_padding(size_t padding)
            {
                while (padding > 0)
                {
                    char buffer[core::buffer_size];
                    size_t size = std::min(sizeof(buffer)-1, padding);

                    memset(buffer, ' ', size);
                    buffer[size] = 0;

                    output_stream.write(buffer, size);
                    padding -= size;
                }
            }

        public:
            pretty_stream_writer(std::ostream &output, size_t indent_width) : impl::stream_writer_base(output), indent_width(indent_width) {}

        protected:
            void begin_() {current_indent = 0; output_stream << std::setprecision(CPPDATALIB_REAL_DIG);}

            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                {
                    current_indent -= indent_width;
                    output_stream << '\n', output_padding(current_indent);
                    output_stream << "</member>\n", output_padding(current_indent);
                }

                if (!v.is_string())
                    throw core::error("XML RPC - cannot write non-string key");

                output_stream << "<member>";
                current_indent += indent_width;
                output_stream << '\n', output_padding(current_indent);
            }
            void begin_item_(const core::value &)
            {
                if (current_container_size() > 0 || current_container() == core::object)
                    output_stream << '\n', output_padding(current_indent);
            }

            void null_(const core::value &) {throw core::error("XML RPC - 'null' value not allowed in output");}
            void bool_(const core::value &v)
            {
                output_stream << "<value>\n"; output_padding(current_indent + indent_width);
                output_stream << "<boolean>\n"; output_padding(current_indent + indent_width * 2);
                output_stream << v.as_int() << '\n'; output_padding(current_indent + indent_width);
                output_stream << "</boolean>\n"; output_padding(current_indent);
                output_stream << "</value>";
            }
            void integer_(const core::value &v)
            {
                output_stream << "<value>\n"; output_padding(current_indent + indent_width);
                output_stream << "<int>\n"; output_padding(current_indent + indent_width * 2);
                output_stream << v.get_int() << '\n'; output_padding(current_indent + indent_width);
                output_stream << "</int>\n"; output_padding(current_indent);
                output_stream << "</value>";
            }
            void uinteger_(const core::value &v)
            {
                output_stream << "<value>\n"; output_padding(current_indent + indent_width);
                output_stream << "<int>\n"; output_padding(current_indent + indent_width * 2);
                output_stream << v.get_uint() << '\n'; output_padding(current_indent + indent_width);
                output_stream << "</int>\n"; output_padding(current_indent);
                output_stream << "</value>";
            }
            void real_(const core::value &v)
            {
                output_stream << "<value>\n"; output_padding(current_indent + indent_width);
                output_stream << "<double>\n"; output_padding(current_indent + indent_width * 2);
                output_stream << v.get_real() << '\n'; output_padding(current_indent + indent_width);
                output_stream << "</double>\n"; output_padding(current_indent);
                output_stream << "</value>";
            }
            void begin_string_(const core::value &, core::int_t, bool is_key)
            {
                if (is_key)
                    output_stream << "<name>";
                else
                {
                    current_indent += indent_width;

                    output_stream << "<value>\n", output_padding(current_indent);
                    output_stream << "<string>";
                }
            }
            void string_data_(const core::value &v, bool)
            {
                if (current_container_size() == 0)
                    output_stream << '\n', output_padding(current_indent + indent_width);

                write_string(output_stream, v.get_string());
            }
            void end_string_(const core::value &, bool is_key)
            {
                if (current_container_size() > 0)
                    output_stream << '\n', output_padding(current_indent);

                if (is_key)
                    output_stream << "</name>";
                else
                {
                    current_indent -= indent_width;

                    output_stream << "</string>\n"; output_padding(current_indent);
                    output_stream << "</value>";
                }
            }

            void begin_array_(const core::value &, core::int_t, bool)
            {
                output_stream << "<value>\n", output_padding(current_indent + indent_width);
                output_stream << "<array>\n", output_padding(current_indent + indent_width * 2);
                output_stream << "<data>", output_padding(current_indent + indent_width * 3);
                current_indent += indent_width * 3;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width * 3;

                output_stream << '\n', output_padding(current_indent + indent_width * 2);
                output_stream << "</data>\n", output_padding(current_indent + indent_width);
                output_stream << "</array>\n", output_padding(current_indent);
                output_stream << "</value>";
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                output_stream << "<value>\n", output_padding(current_indent + indent_width);
                output_stream << "<struct>\n", output_padding(current_indent + indent_width * 2);
                current_indent += indent_width * 2;
            }
            void end_object_(const core::value &, bool)
            {
                if (current_container_size() > 0)
                {
                    current_indent -= indent_width;
                    output_stream << '\n', output_padding(current_indent);
                    output_stream << "</member>";
                }

                current_indent -= indent_width * 2;
                output_stream << '\n', output_padding(current_indent + indent_width);
                output_stream << "</struct>\n", output_padding(current_indent);
                output_stream << "</value>";
            }
        };

        inline std::string to_xml_rpc(const core::value &v)
        {
            std::ostringstream stream;
            stream_writer writer(stream);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_XML_RPC_H

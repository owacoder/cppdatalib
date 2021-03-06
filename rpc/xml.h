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

#include "../core/core.h"

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

        protected:
            void begin_() {stream().precision(CPPDATALIB_REAL_DIG);}

            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    stream() << "</member>";

                if (!v.is_string())
                    throw core::error("XML RPC - cannot write non-string key");

                stream() << "<member>";
            }

            void null_(const core::value &) {throw core::error("XML RPC - 'null' value not allowed in output");}
            void bool_(const core::value &v) {stream() << "<value><boolean>" << v.as_int() << "</boolean></value>";}
            void integer_(const core::value &v) {stream() << "<value><int>" << v.get_int_unchecked() << "</int></value>";}
            void uinteger_(const core::value &v) {stream() << "<value><int>" << v.get_uint_unchecked() << "</int></value>";}
            void real_(const core::value &v) {stream() << "<value><double>" << v.get_real_unchecked() << "</double></value>";}
            void begin_string_(const core::value &, core::int_t, bool is_key)
            {
                if (is_key)
                    stream() << "<name>";
                else
                    stream() << "<value><string>";
            }
            void string_data_(const core::value &v, bool) {write_string(stream(), v.get_string_unchecked());}
            void end_string_(const core::value &, bool is_key)
            {
                if (is_key)
                    stream() << "</name>";
                else
                    stream() << "</string></value>";
            }

            void begin_array_(const core::value &, core::int_t, bool) {stream() << "<value><array><data>";}
            void end_array_(const core::value &, bool) {stream() << "</data></array></value>";}

            void begin_object_(const core::value &, core::int_t, bool) {stream() << "<value><struct>";}
            void end_object_(const core::value &, bool)
            {
                if (current_container_size() > 0)
                    stream() << "</member>";
                stream() << "</struct></value>";
            }
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
            {}

        protected:
            void begin_() {current_indent = 0; stream().precision(CPPDATALIB_REAL_DIG);}

            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                {
                    current_indent -= indent_width;
                    stream() << '\n', output_padding(current_indent);
                    stream() << "</member>\n", output_padding(current_indent);
                }

                if (!v.is_string())
                    throw core::error("XML RPC - cannot write non-string key");

                stream() << "<member>";
                current_indent += indent_width;
                stream() << '\n', output_padding(current_indent);
            }
            void begin_item_(const core::value &)
            {
                if (current_container_size() > 0 || current_container() == core::object)
                    stream() << '\n', output_padding(current_indent);
            }

            void null_(const core::value &) {throw core::error("XML RPC - 'null' value not allowed in output");}
            void bool_(const core::value &v)
            {
                stream() << "<value>\n"; output_padding(current_indent + indent_width);
                stream() << "<boolean>\n"; output_padding(current_indent + indent_width * 2);
                stream() << v.as_int() << '\n'; output_padding(current_indent + indent_width);
                stream() << "</boolean>\n"; output_padding(current_indent);
                stream() << "</value>";
            }
            void integer_(const core::value &v)
            {
                stream() << "<value>\n"; output_padding(current_indent + indent_width);
                stream() << "<int>\n"; output_padding(current_indent + indent_width * 2);
                stream() << v.get_int_unchecked() << '\n'; output_padding(current_indent + indent_width);
                stream() << "</int>\n"; output_padding(current_indent);
                stream() << "</value>";
            }
            void uinteger_(const core::value &v)
            {
                stream() << "<value>\n"; output_padding(current_indent + indent_width);
                stream() << "<int>\n"; output_padding(current_indent + indent_width * 2);
                stream() << v.get_uint_unchecked() << '\n'; output_padding(current_indent + indent_width);
                stream() << "</int>\n"; output_padding(current_indent);
                stream() << "</value>";
            }
            void real_(const core::value &v)
            {
                stream() << "<value>\n"; output_padding(current_indent + indent_width);
                stream() << "<double>\n"; output_padding(current_indent + indent_width * 2);
                stream() << v.get_real_unchecked() << '\n'; output_padding(current_indent + indent_width);
                stream() << "</double>\n"; output_padding(current_indent);
                stream() << "</value>";
            }
            void begin_string_(const core::value &, core::int_t, bool is_key)
            {
                if (is_key)
                    stream() << "<name>";
                else
                {
                    current_indent += indent_width;

                    stream() << "<value>\n", output_padding(current_indent);
                    stream() << "<string>";
                }
            }
            void string_data_(const core::value &v, bool)
            {
                if (current_container_size() == 0)
                    stream() << '\n', output_padding(current_indent + indent_width);

                write_string(stream(), v.get_string_unchecked());
            }
            void end_string_(const core::value &, bool is_key)
            {
                if (current_container_size() > 0)
                    stream() << '\n', output_padding(current_indent);

                if (is_key)
                    stream() << "</name>";
                else
                {
                    current_indent -= indent_width;

                    stream() << "</string>\n"; output_padding(current_indent);
                    stream() << "</value>";
                }
            }

            void begin_array_(const core::value &, core::int_t, bool)
            {
                stream() << "<value>\n", output_padding(current_indent + indent_width);
                stream() << "<array>\n", output_padding(current_indent + indent_width * 2);
                stream() << "<data>", output_padding(current_indent + indent_width * 3);
                current_indent += indent_width * 3;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width * 3;

                stream() << '\n', output_padding(current_indent + indent_width * 2);
                stream() << "</data>\n", output_padding(current_indent + indent_width);
                stream() << "</array>\n", output_padding(current_indent);
                stream() << "</value>";
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                stream() << "<value>\n", output_padding(current_indent + indent_width);
                stream() << "<struct>\n", output_padding(current_indent + indent_width * 2);
                current_indent += indent_width * 2;
            }
            void end_object_(const core::value &, bool)
            {
                if (current_container_size() > 0)
                {
                    current_indent -= indent_width;
                    stream() << '\n', output_padding(current_indent);
                    stream() << "</member>";
                }

                current_indent -= indent_width * 2;
                stream() << '\n', output_padding(current_indent + indent_width);
                stream() << "</struct>\n", output_padding(current_indent);
                stream() << "</value>";
            }
        };

        inline std::string to_xml_rpc(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer writer(stream);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_XML_RPC_H
